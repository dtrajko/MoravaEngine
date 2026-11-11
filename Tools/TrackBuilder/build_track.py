# Builds a race track's models from its data file, in Blender without its window (docs/games/Racing_Plan.html, Phase 1):
#
#   blender --background --factory-startup --python Tools/TrackBuilder/build_track.py -- Projects/Racing/Tracks/TestTrack.track.json
#
# The data file (a .track.json, see README.md) is the source: a closed list of control points with a position, a road width,
# a bank angle and flags. This script makes from it, beside the project's other files (paths in the data, relative to it):
#   <Models>/Track_<Name>_Road.glb       the driven surface: the road, the curbs (a grippy collider)
#   <Models>/Track_<Name>_Walls.glb      the side walls (a slippery collider: the car scrapes along them)
#   <Models>/Track_<Name>_Structure.glb  the road's sides and underside, the supports, the start line (drawn only, no
#                                        collider: the line lies just above the road, and as a collider it was a step the
#                                        car hopped over)
#   <Assets>/Tracks/<Name>.centerline.json  the center line every few meters, the start: for scripts (Assets.ReadText:
#                                        the test driver, the race), in the Assets folder so a built game has it
# Each model is cut into chunks along the track (ChunkLength meters), so the engine culls it chunk by chunk.
#
# Coordinates in the data and the outputs are the engine's: meters, Y up, a car looks along -Z. Blender's are Z up; the
# vertices are turned into Blender's space here and the glTF exporter turns them back (+Y up).
#
# The cross section, at each sample along the track (R: right, U: up, both turned by the bank; the road's top is U = 0):
#
#        wall                                              wall
#      +--+                                                +--+
#      |  |curb|            road                   |curb|  |  |
#      |  +----+-----------------------------------+----+  |  |   U = 0
#      +-----------------------------------------------------+   U = -SlabThickness (underside)
#                                    | |  support, every SupportSpacing meters, down into the ground
#
# Bank: degrees around the direction of travel; positive raises the left edge (for a right turn), negative the right edge.
# Curbs: where a point has them, on the inner side of the turn only (the side it turns towards), the road wall to wall on
# the outer side.

import bpy
import json
import math
import os
import sys


# The data and the spline are shared with preview_track.py (plain Python): track_spline.py, beside this file
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from track_spline import (add, sub, mul, dot, cross, length, normalize, WORLD_UP, load_track, sample_track,  # noqa: E402
                          simplify_samples, road_twist)


def to_blender(p):
    # engine (x, y up, z) -> Blender (x, y = -z, z up): a rotation, so faces keep their winding
    return (p[0], -p[2], p[1])


# ----------------------------------------------------------------------------------------------------------- the meshes
class MeshBuilder:
    """Vertices and faces of one chunk of one part; a face is turned to face its wanted side."""

    def __init__(self):
        self.vertices = []
        self.faces = []
        self.uvs = []  # per face corner, in face order
        self.shared = {}  # (surface, position) -> vertex: the quads of one surface along the track share their corners

    def vertex(self, p, surface):
        if surface is None:
            self.vertices.append(p)
            return len(self.vertices) - 1
        key = (surface, round(p[0], 4), round(p[1], 4), round(p[2], 4))
        index = self.shared.get(key)
        if index is None:
            index = self.shared[key] = len(self.vertices)
            self.vertices.append(p)
        return index

    def quad(self, a, b, c, d, uv, facing, surface=None):
        # a, b, c, d: engine-space corners in order around the quad; facing: the direction its front should face.
        # surface: a name for one surface along the track (the road, a wall's inner face...): its quads share corners, so
        # it is shaded smooth along the track; a corner between two surfaces stays sharp (each has its own vertices there)
        normal = cross(sub(b, a), sub(c, a))
        if dot(normal, facing) < 0.0:
            a, b, c, d = d, c, b, a
            uv = (uv[3], uv[2], uv[1], uv[0])
        self.faces.append(tuple(self.vertex(p, surface) for p in (a, b, c, d)))
        self.uvs.extend(uv)

    def triangle(self, a, b, c, uv, facing, surface=None):
        # As quad, for one triangle: the road's strips are joined by triangles of their own (build_parts)
        normal = cross(sub(b, a), sub(c, a))
        if dot(normal, facing) < 0.0:
            a, c = c, a
            uv = (uv[2], uv[1], uv[0])
        self.faces.append(tuple(self.vertex(p, surface) for p in (a, b, c)))
        self.uvs.extend(uv)

    def triangles(self):
        return sum(len(face) - 2 for face in self.faces)

    def empty(self):
        return not self.faces


def profile_point(sample, r, u):
    return add(sample["Position"], add(mul(sample["Right"], r), mul(sample["Up"], u)))


def box(builder, center_bottom, center_top, right, forward, half):
    # A pillar between two points (vertical), its sides along right and forward (horizontal)
    r = mul(right, half)
    f = mul(forward, half)
    height = length(sub(center_top, center_bottom))
    corners = [(-1, -1), (1, -1), (1, 1), (-1, 1)]
    bottom = [add(center_bottom, add(mul(r, x), mul(f, z))) for x, z in corners]
    top = [add(center_top, add(mul(r, x), mul(f, z))) for x, z in corners]
    for i in range(4):
        j = (i + 1) % 4
        side = normalize(sub(add(bottom[i], bottom[j]), mul(center_bottom, 2.0)))
        builder.quad(bottom[i], bottom[j], top[j], top[i], ((0, 0), (half * 2, 0), (half * 2, height), (0, height)), side)


def cuboid(builder, center, ax, ay, az):
    # A closed box: center and its three half axes (each face faces out from the center)
    for axis, u, v in ((ax, ay, az), (ay, az, ax), (az, ax, ay)):
        for sign in (-1.0, 1.0):
            n = mul(axis, sign)
            c = add(center, n)
            corners = [add(c, add(mul(u, x), mul(v, y))) for x, y in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            lu, lv = 2.0 * length(u), 2.0 * length(v)
            builder.quad(corners[0], corners[1], corners[2], corners[3], ((0, 0), (lu, 0), (lu, lv), (0, lv)), n)


def build_parts(samples, settings, total, mesh_samples=None):
    # mesh_samples: the samples the road, curbs and walls are made of (simplify_samples: fewer where the track is
    # straight); the supports and the start keep every sample's spacing
    chunk_length = settings["ChunkLength"]
    chunks = max(1, int(math.ceil(total / chunk_length - 1e-6)))
    parts = {}

    def part(name, chunk):
        key = (name, chunk)
        if key not in parts:
            parts[key] = MeshBuilder()
        return parts[key]

    curb_w = settings["CurbWidth"]
    wall_t = settings["WallThickness"]
    slab = settings["SlabThickness"]
    stripe = settings["CurbStripe"]
    along = samples if mesh_samples is None else mesh_samples
    count = len(along)

    # The road's strips across its width: where the bank changes, a quad wall to wall is twisted and folds along its
    # diagonal by about twist / length (road_twist), up to 5.6 degrees on the circuit; n strips fold n times less. Each
    # cross section is cut into as many as the quads on either side of it need (one where the bank holds: the straights
    # and the middle of a turn stay a quad wide), and two cross sections with different counts are joined by triangles
    # between their corners, so no strip ends in the middle of another (no T-junction: no crack to see through)
    max_fold = math.radians(settings["MeshMaxFold"])
    need = []
    for j in range(count):
        s0, s1 = along[j], along[(j + 1) % count]
        span = (s1["s"] if s1["s"] > s0["s"] else total) - s0["s"]
        n = max(1, int(math.ceil(abs(road_twist(s0, s1)) / (span * max_fold) - 1e-9)))
        # A power of two: neighboring cross sections mostly have the same count, and where they don't, every corner of
        # the coarser one is a corner of the finer one (counts like 10 and 12 make skewed triangles: folds of 1 degree)
        need.append(min(32, 1 << (n - 1).bit_length()))
    strips = [max(need[j - 1], need[j]) for j in range(count)]
    print("build_track: the road in 1 to %d strips across (MeshMaxFold %.2f degrees)" % (max(strips), settings["MeshMaxFold"]))
    for j in range(count):
        s0 = along[j]
        s1 = along[(j + 1) % count]
        chunk = min(int(s0["s"] / chunk_length), chunks - 1)
        hw = 0.5 * s0["Width"]
        hw1 = 0.5 * s1["Width"]  # the width changes smoothly: each side of a quad uses its own sample's
        curbs = s0["Curbs"]
        walls = s0["Walls"]

        # The road (its top), up to the curb on the inner side of a turn when there are curbs, else from wall to wall
        road = part("Road", chunk)
        inner = s0["Inner"]
        left_curb = curb_w if curbs and inner < 0.0 else 0.0
        right_curb = curb_w if curbs and inner > 0.0 else 0.0
        v1 = s1["s"] if s1["s"] > s0["s"] else total  # the last quad closes the loop at the first sample
        facing = add(s0["Up"], s1["Up"])
        n0, n1 = strips[j], strips[(j + 1) % count]
        row0 = [-(hw - left_curb) + (2.0 * hw - left_curb - right_curb) * i / n0 for i in range(n0 + 1)]    # across, at s0
        row1 = [-(hw1 - left_curb) + (2.0 * hw1 - left_curb - right_curb) * i / n1 for i in range(n1 + 1)]  # at s1
        if n0 == 1 and n1 == 1:
            road.quad(profile_point(s0, row0[0], 0.0), profile_point(s0, row0[1], 0.0), profile_point(s1, row1[1], 0.0),
                      profile_point(s1, row1[0], 0.0), ((row0[0], s0["s"]), (row0[1], s0["s"]), (row1[1], v1), (row1[0], v1)), facing, "road")
        else:
            # Zip the two rows of corners together, left to right: each triangle takes the next corner of the row that
            # is behind (by its place across the road, 0 to 1), so the diagonals all lean the same way
            i0 = i1 = 0
            while i0 < n0 or i1 < n1:
                p0, p1 = (s0, row0[i0], s0["s"]), (s1, row1[i1], v1)
                if i1 == n1 or (i0 < n0 and (i0 + 1) / n0 <= (i1 + 1) / n1):
                    i0 += 1
                    p2 = (s0, row0[i0], s0["s"])
                else:
                    i1 += 1
                    p2 = (s1, row1[i1], v1)
                road.triangle(*(profile_point(p[0], p[1], 0.0) for p in (p0, p2, p1)), tuple((p[1], p[2]) for p in (p0, p2, p1)), facing, "road")

        # The curb: red and white stripes, flush with the road, on the inner side of the turn only
        if curbs:
            color = "Curb_Red" if int(s0["s"] / stripe) % 2 == 0 else "Curb_White"
            for side in (inner,):
                a = profile_point(s0, side * hw, 0.0)
                b = profile_point(s0, side * (hw - curb_w), 0.0)
                c = profile_point(s1, side * (hw1 - curb_w), 0.0)
                d = profile_point(s1, side * hw1, 0.0)
                part(color, chunk).quad(a, b, c, d, ((0, s0["s"]), (curb_w, s0["s"]), (curb_w, v1), (0, v1)), add(s0["Up"], s1["Up"]),
                                        ("curb", side))

        # Walls (inner face, top, outer face down to the underside) or the bare edge of the slab
        outer = wall_t if walls else 0.0
        for side in (-1.0, 1.0):
            def q(builder, r0, u0, r1, u1, rr0, rr1, facing_r, facing_u, uv, uu0=None, uu1=None, surface=None):
                # r0/r1 at s0, rr0/rr1 at s1 (the width changes): lateral offsets measured from the center; u0/u1 the
                # heights at s0, uu0/uu1 at s1 (the same unless given: a wall's height changes)
                uu0 = u0 if uu0 is None else uu0
                uu1 = u1 if uu1 is None else uu1
                a = profile_point(s0, r0, u0)
                b = profile_point(s0, r1, u1)
                c = profile_point(s1, rr1, uu1)
                d = profile_point(s1, rr0, uu0)
                facing = add(mul(add(s0["Right"], s1["Right"]), facing_r), mul(add(s0["Up"], s1["Up"]), facing_u))
                builder.quad(a, b, c, d, ((uv[0], s0["s"]), (uv[1], s0["s"]), (uv[1], v1), (uv[0], v1)), facing, surface)
            if walls:
                w = part("Wall", chunk)
                key = "WallLeft" if side < 0.0 else "WallRight"
                h0, h1 = s0[key], s1[key]
                q(w, side * hw, 0.0, side * hw, h0, side * hw1, side * hw1, -side, 0.0, (0.0, h0), uu1=h1, surface=("inner", side))
                q(w, side * hw, h0, side * (hw + wall_t), h0, side * hw1, side * (hw1 + wall_t), 0.0, 1.0, (0.0, wall_t), h1, h1,
                  surface=("top", side))
                q(w, side * (hw + wall_t), h0, side * (hw + wall_t), -slab, side * (hw1 + wall_t), side * (hw1 + wall_t), side, 0.0,
                  (0.0, h0 + slab), h1, -slab, surface=("outer", side))
            else:
                q(part("Structure", chunk), side * hw, 0.0, side * hw, -slab, side * hw1, side * hw1, side, 0.0, (0.0, slab),
                  surface=("edge", side))
        # The underside
        a = profile_point(s0, -(hw + outer), -slab)
        b = profile_point(s0, hw + outer, -slab)
        c = profile_point(s1, hw1 + outer, -slab)
        d = profile_point(s1, -(hw1 + outer), -slab)
        part("Structure", chunk).quad(a, b, c, d, ((0, s0["s"]), (2 * (hw + outer), s0["s"]), (2 * (hw1 + outer), v1), (0, v1)),
                                      mul(add(s0["Up"], s1["Up"]), -1.0), "under")

    # Supports: square pillars under the road's center, every SupportSpacing meters, where the road is high enough
    ground = settings["GroundHeight"]
    spacing = settings["SupportSpacing"]
    half = 0.5 * settings["SupportSize"]
    next_support = spacing * 0.5
    for sample in samples:
        if sample["s"] < next_support:
            continue
        next_support += spacing
        if not sample["Supports"]:
            continue
        bottom_of_road = profile_point(sample, 0.0, -slab)
        if bottom_of_road[1] - ground < 1.5:
            continue
        forward = normalize((sample["Tangent"][0], 0.0, sample["Tangent"][2]))
        right = normalize(cross(forward, WORLD_UP))
        top = add(bottom_of_road, (0.0, 0.3, 0.0))  # into the slab, so a banked road never shows a gap
        chunk = min(int(sample["s"] / chunk_length), chunks - 1)
        box(part("Support", chunk), (top[0], ground - 1.0, top[2]), top, right, forward, half)

    # The start line: a checkered strip across the road at distance 0, just above it (drawn only: the road under it is
    # what the car drives on, so the line is no step)
    line_w = settings["StartLineWidth"]
    first = samples[0]
    hw = 0.5 * first["Width"]
    squares_across = 16
    rows = 2
    size_across = 2.0 * hw / squares_across
    lift = 0.02
    along = first["Tangent"]
    for row in range(rows):
        for k in range(squares_across):
            r0 = -hw + k * size_across
            r1 = r0 + size_across
            f0 = -0.5 * line_w + row * line_w / rows
            f1 = f0 + line_w / rows
            corners = [add(profile_point(first, r, lift), mul(along, f)) for r, f in ((r0, f0), (r1, f0), (r1, f1), (r0, f1))]
            name = "Line_White" if (row + k) % 2 == 0 else "Line_Black"
            part(name, 0).quad(corners[0], corners[1], corners[2], corners[3], ((0, 0), (1, 0), (1, 1), (0, 1)), first["Up"])

    # The start and finish gantry: a pillar beyond each wall, from the ground (or the underside) up past the beam, and a
    # beam across the road above the start line (drawn only: the car can't reach it from the road)
    if settings["Gantry"]:
        gantry = part("Gantry", 0)
        clear = settings["GantryHeight"]
        beam = 1.2                                  # meters: the beam's height and depth
        pillar = 0.6                                # half a pillar's side
        level = normalize((along[0], 0.0, along[2]))
        across = normalize(cross(level, WORLD_UP))
        reach = hw + settings["WallThickness"] + pillar + 0.3
        top_of_beam = clear + beam
        for side in (-1.0, 1.0):
            base = profile_point(first, side * reach, 0.0)
            bottom_y = ground - 1.0 if first["Supports"] else base[1] - slab
            top = (base[0], first["Position"][1] + top_of_beam + 0.4, base[2])
            mid = ((base[0] + top[0]) * 0.5, (bottom_y + top[1]) * 0.5, (base[2] + top[2]) * 0.5)
            cuboid(gantry, mid, mul(across, pillar), (0.0, 0.5 * (top[1] - bottom_y), 0.0), mul(level, pillar))
        center = add(first["Position"], (0.0, clear + 0.5 * beam, 0.0))
        cuboid(gantry, center, mul(across, reach + pillar), (0.0, 0.5 * beam, 0.0), mul(level, 0.5 * beam))
    return parts, chunks


# ------------------------------------------------------------------------------------------------------------ Blender
MATERIALS = {
    # name: (base color, roughness, metallic): stand-ins until the project's .mmat files are assigned in the scene
    "Road": ((0.12, 0.12, 0.13), 0.85, 0.0),
    "Curb_Red": ((0.75, 0.08, 0.06), 0.6, 0.0),
    "Curb_White": ((0.9, 0.9, 0.9), 0.6, 0.0),
    "Wall": ((0.85, 0.86, 0.9), 0.5, 0.0),
    "Structure": ((0.55, 0.55, 0.57), 0.9, 0.0),
    "Support": ((0.55, 0.55, 0.57), 0.9, 0.0),
    "Line_White": ((0.95, 0.95, 0.95), 0.6, 0.0),
    "Line_Black": ((0.03, 0.03, 0.03), 0.6, 0.0),
    "Gantry": ((0.08, 0.09, 0.1), 0.4, 0.6),
}

# Which model file each part goes into (the colliders differ: see the top of this file)
MODEL_OF_PART = {
    "Road": "Road", "Curb_Red": "Road", "Curb_White": "Road",
    "Wall": "Walls",
    "Structure": "Structure", "Support": "Structure", "Line_White": "Structure", "Line_Black": "Structure",
    "Gantry": "Structure",
}


def clear_blender():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def material(name):
    mat = bpy.data.materials.get(name)
    if mat is None:
        color, roughness, metallic = MATERIALS[name]
        mat = bpy.data.materials.new(name)
        if mat.node_tree is None:  # Blender 5 gives a new material its nodes (use_nodes goes away in 6)
            mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        bsdf.inputs["Base Color"].default_value = (color[0], color[1], color[2], 1.0)
        bsdf.inputs["Roughness"].default_value = roughness
        bsdf.inputs["Metallic"].default_value = metallic
    return mat


def make_object(name, builder, collection):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([to_blender(v) for v in builder.vertices], [], builder.faces)
    mesh.update()
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for i, loop in enumerate(mesh.loops):
        uv_layer.data[i].uv = builder.uvs[loop.index]
    # Smooth along the track where a surface's quads share corners (MeshBuilder.quad's surface); sharp where two surfaces
    # meet or a quad has corners of its own (supports, the gantry: boxes)
    mesh.shade_smooth()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    return obj


def export_model(path, objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_apply=True, export_materials="EXPORT")


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if not args:
        print("build_track.py: give the track's data file after --")
        sys.exit(1)
    track_path = os.path.abspath(args[0])
    data, settings, points = load_track(track_path)
    name = data["Name"]
    track_dir = os.path.dirname(track_path)
    models_dir = os.path.normpath(os.path.join(track_dir, data.get("ModelsDirectory", "../Assets/Models")))
    os.makedirs(models_dir, exist_ok=True)

    samples, total = sample_track(points, settings["SampleSpacing"])
    mesh_samples = [samples[k] for k in simplify_samples(samples, total, settings)]
    print("build_track: %d cross sections in the models, of %d samples (MeshTolerance %.3f m)" % (
        len(mesh_samples), len(samples), settings["MeshTolerance"]))
    parts, chunks = build_parts(samples, settings, total, mesh_samples)

    clear_blender()
    collection = bpy.context.scene.collection
    objects_of_model = {}
    triangles_of_model = {}
    for (part_name, chunk), builder in sorted(parts.items(), key=lambda item: (item[0][0], item[0][1])):
        if builder.empty():
            continue
        obj = make_object("%s_%02d" % (part_name, chunk), builder, collection)
        obj.data.materials.append(material(part_name))
        model = MODEL_OF_PART[part_name]
        objects_of_model.setdefault(model, []).append(obj)
        triangles_of_model[model] = triangles_of_model.get(model, 0) + builder.triangles()

    written = []
    for model, objects in sorted(objects_of_model.items()):
        path = os.path.join(models_dir, "Track_%s_%s.glb" % (name, model))
        export_model(path, objects)
        written.append(path)
        print("build_track: %s: %d meshes, %d triangles" % (os.path.basename(path), len(objects), triangles_of_model[model]))

    # The center line, for scripts: every CenterlineSpacing meters (the samples are SampleSpacing apart)
    every = max(1, int(round(settings["CenterlineSpacing"] / settings["SampleSpacing"])))
    def r3(v): return [round(x, 3) for x in v]
    centerline = {
        "Name": name,
        "Length": round(total, 2),
        "Closed": True,
        "Points": [{"s": round(sm["s"], 2), "Position": r3(sm["Position"]), "Right": r3(sm["Right"]), "Up": r3(sm["Up"]),
                    "Width": round(sm["Width"], 2)} for sm in samples[::every]],
    }
    start_distance = float(data.get("StartDistance", 12.0))  # the car starts this many meters before the start line
    k = int(round((total - start_distance) / settings["SampleSpacing"])) % len(samples)
    start = samples[k]
    forward = start["Tangent"]
    centerline["Start"] = {
        "Position": r3(start["Position"]),
        "Forward": r3(forward),
        # the engine's rotation around Y in degrees (0 looks along -Z, positive turns to the left)
        "Yaw": round(math.degrees(math.atan2(-forward[0], -forward[2])), 2),
    }
    # In the project's Assets folder: a built game's pack holds it, and scripts read it there (Assets.ReadText)
    centerline_dir = os.path.normpath(os.path.join(track_dir, data.get("CenterlineDirectory", "../Assets/Tracks")))
    os.makedirs(centerline_dir, exist_ok=True)
    centerline_path = os.path.join(centerline_dir, "%s.centerline.json" % name)
    with open(centerline_path, "w", encoding="utf-8", newline="\n") as f:
        # One point per line: readable, and a change of the track shows as changed lines. Start before Points
        f.write("{\n")
        for key in ("Name", "Length", "Closed", "Start"):
            f.write(' "%s": %s,\n' % (key, json.dumps(centerline[key])))
        f.write(' "Points": [\n')
        f.write(",\n".join("  " + json.dumps(point) for point in centerline["Points"]))
        f.write("\n ]\n}\n")
    written.append(centerline_path)

    lowest = min(sm["Position"][1] for sm in samples)
    highest = max(sm["Position"][1] for sm in samples)
    print("build_track: '%s': %.0f m, %d samples, %d chunks, %.1f m from the lowest to the highest point" %
          (name, total, len(samples), chunks, highest - lowest))
    print("build_track: start at %s, yaw %.1f" % (centerline["Start"]["Position"], centerline["Start"]["Yaw"]))
    for path in written:
        print("build_track: wrote %s" % path)


main()
