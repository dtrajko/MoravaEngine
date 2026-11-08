# Builds a race track's models from its data file, in Blender without its window (docs/games/Racing_Plan.html, Phase 1):
#
#   blender --background --factory-startup --python Tools/TrackBuilder/build_track.py -- Projects/Racing/Tracks/TestTrack.track.json
#
# The data file (a .track.json, see README.md) is the source: a closed list of control points with a position, a road width,
# a bank angle and flags. This script makes from it, beside the project's other files (paths in the data, relative to it):
#   <Models>/Track_<Name>_Road.glb       the driven surface: the road, the curbs, the start line (a grippy collider)
#   <Models>/Track_<Name>_Walls.glb      the side walls (a slippery collider: the car scrapes along them)
#   <Models>/Track_<Name>_Structure.glb  the road's sides and underside and the supports (drawn only, no collider)
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


# ---------------------------------------------------------------------------------------------------------------- vectors
def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def mul(a, s): return (a[0] * s, a[1] * s, a[2] * s)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def length(a): return math.sqrt(dot(a, a))
def normalize(a):
    l = length(a)
    return mul(a, 1.0 / l) if l > 1e-9 else (0.0, 0.0, 0.0)
def lerp(a, b, t): return a + (b - a) * t
def smoothstep(t): return t * t * (3.0 - 2.0 * t)

WORLD_UP = (0.0, 1.0, 0.0)
CURB_TURN_WINDOW = 20.0  # meters before and after a sample over which its turn's direction is measured


def to_blender(p):
    # engine (x, y up, z) -> Blender (x, y = -z, z up): a rotation, so faces keep their winding
    return (p[0], -p[2], p[1])


# ------------------------------------------------------------------------------------------------------------- the data
DEFAULTS = {
    "Width": 16.0,          # meters, curb to curb
    "Bank": 0.0,            # degrees
    "Walls": True,
    "Curbs": False,
    "Supports": True,
}
SETTINGS = {
    "SampleSpacing": 1.0,   # meters between two cross sections
    "ChunkLength": 100.0,   # meters of track per chunk (culled one by one)
    "CurbWidth": 1.5,
    "CurbStripe": 3.0,      # meters per red or white stripe
    "WallHeight": 1.0,
    "WallThickness": 0.5,
    "SlabThickness": 0.8,
    "SupportSpacing": 24.0,
    "SupportSize": 1.4,     # meters, a square pillar
    "GroundHeight": 0.0,    # the engine's y of the ground below the track: supports go down to here (and a meter more)
    "StartLineWidth": 2.0,  # meters along the track, at distance 0 (the first point)
    "CenterlineSpacing": 2.0,
}


def load_track(path):
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    settings = dict(SETTINGS)
    settings.update(data.get("Settings", {}))
    defaults = dict(DEFAULTS)
    defaults.update(data.get("Defaults", {}))
    points = []
    for p in data["Points"]:
        point = dict(defaults)
        point.update(p)
        point["Position"] = tuple(float(v) for v in p["Position"])
        points.append(point)
    if len(points) < 4:
        raise ValueError("a track needs at least 4 points")
    return data, settings, points


# ------------------------------------------------------------------------------------------------- the spline and samples
def catmull_rom(p0, p1, p2, p3, t, alpha=0.5):
    # Centripetal Catmull-Rom (alpha 0.5): no loops or cusps where points are unevenly spaced
    def tj(ti, a, b):
        return ti + max(length(sub(b, a)), 1e-6) ** alpha
    t0 = 0.0
    t1 = tj(t0, p0, p1)
    t2 = tj(t1, p1, p2)
    t3 = tj(t2, p2, p3)
    u = lerp(t1, t2, t)
    def blend(a, b, ta, tb):
        return add(mul(a, (tb - u) / (tb - ta)), mul(b, (u - ta) / (tb - ta)))
    a1 = blend(p0, p1, t0, t1)
    a2 = blend(p1, p2, t1, t2)
    a3 = blend(p2, p3, t2, t3)
    b1 = blend(a1, a2, t0, t2)
    b2 = blend(a2, a3, t1, t3)
    return blend(b1, b2, t1, t2)


def sample_track(points, spacing):
    # A dense polyline first (segment, t, position), then evenly spaced samples along its length
    n = len(points)
    dense = []
    steps = 64
    for i in range(n):
        p0 = points[(i - 1) % n]["Position"]
        p1 = points[i]["Position"]
        p2 = points[(i + 1) % n]["Position"]
        p3 = points[(i + 2) % n]["Position"]
        for k in range(steps):
            t = k / steps
            dense.append((i, t, catmull_rom(p0, p1, p2, p3, t)))
    distances = [0.0]
    for k in range(1, len(dense) + 1):
        distances.append(distances[-1] + length(sub(dense[k % len(dense)][2], dense[k - 1][2])))
    total = distances[-1]
    count = max(8, int(round(total / spacing)))
    step = total / count

    samples = []
    k = 0
    for j in range(count):
        s = j * step
        while distances[k + 1] < s:
            k += 1
        f = (s - distances[k]) / max(distances[k + 1] - distances[k], 1e-9)
        a, b = dense[k], dense[(k + 1) % len(dense)]
        position = add(a[2], mul(sub(b[2], a[2]), f))
        segment, t = a[0], a[1] + (1.0 / steps) * f
        pa, pb = points[segment], points[(segment + 1) % n]
        e = smoothstep(min(max(t, 0.0), 1.0))
        samples.append({
            "s": s,
            "Position": position,
            "Width": lerp(pa["Width"], pb["Width"], e),
            "Bank": lerp(pa["Bank"], pb["Bank"], e),
            # flags hold for the segment from a point to the next
            "Walls": bool(pa["Walls"]),
            "Curbs": bool(pa["Curbs"]),
            "Supports": bool(pa["Supports"]),
        })

    # Frames: tangent from the neighbors, right from the world's up, both turned around the tangent by the bank
    for j, sample in enumerate(samples):
        prev = samples[(j - 1) % count]["Position"]
        nxt = samples[(j + 1) % count]["Position"]
        tangent = normalize(sub(nxt, prev))
        right = normalize(cross(tangent, WORLD_UP))
        up = cross(right, tangent)
        bank = math.radians(sample["Bank"])
        c, s = math.cos(bank), math.sin(bank)
        sample["Tangent"] = tangent
        sample["Right"] = sub(mul(right, c), mul(up, s))   # positive bank: the right edge goes down
        sample["Up"] = add(mul(up, c), mul(right, s))

    # The inner side of the turn at each sample (-1: left, 1: right), where its curb goes: the way the track turns over
    # CurbTurnWindow meters around it (a wide window: a curb doesn't jump sides in a nearly straight stretch)
    window = max(1, int(round(CURB_TURN_WINDOW / spacing)))
    for j, sample in enumerate(samples):
        before = samples[(j - window) % count]["Tangent"]
        after = samples[(j + window) % count]["Tangent"]
        turn = cross(before, after)[1]  # positive: turning left (seen from above)
        sample["Inner"] = -1.0 if turn > 0.0 else 1.0
    return samples, total


# ----------------------------------------------------------------------------------------------------------- the meshes
class MeshBuilder:
    """Vertices and faces of one chunk of one part; a face is turned to face its wanted side."""

    def __init__(self):
        self.vertices = []
        self.faces = []
        self.uvs = []  # per face corner, in face order

    def quad(self, a, b, c, d, uv, facing):
        # a, b, c, d: engine-space corners in order around the quad; facing: the direction its front should face
        normal = cross(sub(b, a), sub(c, a))
        if dot(normal, facing) < 0.0:
            a, b, c, d = d, c, b, a
            uv = (uv[3], uv[2], uv[1], uv[0])
        base = len(self.vertices)
        self.vertices.extend([a, b, c, d])
        self.faces.append((base, base + 1, base + 2, base + 3))
        self.uvs.extend(uv)

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


def build_parts(samples, settings, total):
    chunk_length = settings["ChunkLength"]
    chunks = max(1, int(math.ceil(total / chunk_length - 1e-6)))
    parts = {}

    def part(name, chunk):
        key = (name, chunk)
        if key not in parts:
            parts[key] = MeshBuilder()
        return parts[key]

    curb_w = settings["CurbWidth"]
    wall_h = settings["WallHeight"]
    wall_t = settings["WallThickness"]
    slab = settings["SlabThickness"]
    stripe = settings["CurbStripe"]
    count = len(samples)
    for j in range(count):
        s0 = samples[j]
        s1 = samples[(j + 1) % count]
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
        a = profile_point(s0, -(hw - left_curb), 0.0)
        b = profile_point(s0, hw - right_curb, 0.0)
        c = profile_point(s1, hw1 - right_curb, 0.0)
        d = profile_point(s1, -(hw1 - left_curb), 0.0)
        v1 = s1["s"] if s1["s"] > s0["s"] else s0["s"] + length(sub(s1["Position"], s0["Position"]))
        road.quad(a, b, c, d, ((-(hw - left_curb), s0["s"]), (hw - right_curb, s0["s"]), (hw1 - right_curb, v1), (-(hw1 - left_curb), v1)),
                  add(s0["Up"], s1["Up"]))

        # The curb: red and white stripes, flush with the road, on the inner side of the turn only
        if curbs:
            color = "Curb_Red" if int(s0["s"] / stripe) % 2 == 0 else "Curb_White"
            for side in (inner,):
                a = profile_point(s0, side * hw, 0.0)
                b = profile_point(s0, side * (hw - curb_w), 0.0)
                c = profile_point(s1, side * (hw1 - curb_w), 0.0)
                d = profile_point(s1, side * hw1, 0.0)
                part(color, chunk).quad(a, b, c, d, ((0, s0["s"]), (curb_w, s0["s"]), (curb_w, v1), (0, v1)), add(s0["Up"], s1["Up"]))

        # Walls (inner face, top, outer face down to the underside) or the bare edge of the slab
        outer = wall_t if walls else 0.0
        for side in (-1.0, 1.0):
            def q(builder, r0, u0, r1, u1, rr0, rr1, facing_r, facing_u, uv):
                # r0/r1 at s0, rr0/rr1 at s1 (the width changes): lateral offsets measured from the center
                a = profile_point(s0, r0, u0)
                b = profile_point(s0, r1, u1)
                c = profile_point(s1, rr1, u1)
                d = profile_point(s1, rr0, u0)
                facing = add(mul(add(s0["Right"], s1["Right"]), facing_r), mul(add(s0["Up"], s1["Up"]), facing_u))
                builder.quad(a, b, c, d, ((uv[0], s0["s"]), (uv[1], s0["s"]), (uv[1], v1), (uv[0], v1)), facing)
            if walls:
                w = part("Wall", chunk)
                q(w, side * hw, 0.0, side * hw, wall_h, side * hw1, side * hw1, -side, 0.0, (0.0, wall_h))
                q(w, side * hw, wall_h, side * (hw + wall_t), wall_h, side * hw1, side * (hw1 + wall_t), 0.0, 1.0, (0.0, wall_t))
                q(w, side * (hw + wall_t), wall_h, side * (hw + wall_t), -slab, side * (hw1 + wall_t), side * (hw1 + wall_t), side, 0.0, (0.0, wall_h + slab))
            else:
                q(part("Structure", chunk), side * hw, 0.0, side * hw, -slab, side * hw1, side * hw1, side, 0.0, (0.0, slab))
        # The underside
        a = profile_point(s0, -(hw + outer), -slab)
        b = profile_point(s0, hw + outer, -slab)
        c = profile_point(s1, hw1 + outer, -slab)
        d = profile_point(s1, -(hw1 + outer), -slab)
        part("Structure", chunk).quad(a, b, c, d, ((0, s0["s"]), (2 * (hw + outer), s0["s"]), (2 * (hw1 + outer), v1), (0, v1)),
                                      mul(add(s0["Up"], s1["Up"]), -1.0))

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

    # The start line: a checkered strip across the road at distance 0, just above it
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
}

# Which model file each part goes into (the colliders differ: see the top of this file)
MODEL_OF_PART = {
    "Road": "Road", "Curb_Red": "Road", "Curb_White": "Road", "Line_White": "Road", "Line_Black": "Road",
    "Wall": "Walls",
    "Structure": "Structure", "Support": "Structure",
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
    # Flat faces: each quad has its own corners (1 m long, so a bank or a curve changes little from one to the next)
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
    parts, chunks = build_parts(samples, settings, total)

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
        triangles_of_model[model] = triangles_of_model.get(model, 0) + 2 * len(builder.faces)

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
