# Builds a level's greybox models from a data file, in Blender without its window (the shooter's plan,
# docs/games/Shooter_Plan.html, Phase 0.3: the level builder, version 1).
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/LevelBuilder/build_level.py -- Projects/Outpost/Levels/Greybox.level.json
#
# The data file is the source: change it and run the script again (Tools/LevelBuilder/README.md describes it). It
# writes, for a level <Level>:
#   <ModelsDirectory>/<Level>/Level_<Level>_<Building>_F<n>.gltf   a building's floor n: its slab, its walls, its stairs up
#   <ModelsDirectory>/<Level>/Level_<Level>_<Building>_Roof.gltf   the roof (a model of its own: hidden to look inside)
#   <ModelsDirectory>/<Level>/Level_<Level>_Ground.gltf            the ground around the buildings
#   <ModelsDirectory>/<Level>/Level_<Level>_<Catwalk>.gltf         a catwalk between buildings (its deck, steps, railings)
#   <ModelsDirectory>/Textures/Greybox_<Material>.png              the greybox materials' grid (1 m), shared by levels
# and puts them in the scene named by the data (Scene): one entity per model under an entity named Level_<Level>, each
# with a TriangleMesh collider (the greybox's surfaces are its colliders). The scene's other entities are kept; the
# level's entities (IDs from LEVEL_IDS) are replaced on each run.

import json
import math
import os
import re
import sys

import bpy
import numpy as np

LEVEL_IDS = 60000          # the level's entities in the scene: 60000 and up (one level per scene in version 1)
LEVEL_IDS_END = 69999
DOOR_IDS = LEVEL_IDS + 500  # the doors' leaves and their Nav Areas (after the models')

DEFAULTS = {
    "StoryHeight": 3.5,    # from one floor's top to the next
    "FloorThickness": 0.3,
    "WallThickness": 0.2,
    "DoorWidth": 1.0,
    "DoorHeight": 2.2,
    "WindowWidth": 1.6,
    "WindowSill": 1.0,     # the window's bottom above its floor
    "WindowHeight": 1.2,
    "StairWidth": 1.2,
    "StairRiser": 0.18,    # at most: a stair's steps are its rise divided evenly
    "StairTread": 0.28,
    "GroundThickness": 0.5,
    "Parapet": 0.0,        # a low wall around the roof, this high (0: none); its gaps are the roof's Openings
    "CatwalkWidth": 1.2,
    "CatwalkThickness": 0.15,
    "RailingHeight": 1.0,  # a catwalk's railings above its deck and steps (0: none)
    "RailingThickness": 0.05,
    "DoorLeafThickness": 0.05,  # a door's leaf (an opening with a Leaf), in the wall's middle
    "DoorScript": "",      # the script class of a door's leaves (Outpost.Door), "" none: the leaves only collide
}

# The greybox materials: a base color (sRGB) under a grid of 1 m lines; the floor's lines are darker every 5 m
MATERIALS = {
    "Ground": ((0.42, 0.45, 0.38), 0.95),
    "Floor": ((0.55, 0.55, 0.55), 0.85),
    "Wall": ((0.78, 0.75, 0.70), 0.9),
    "WallOutside": ((0.62, 0.64, 0.68), 0.9),
    "Stairs": ((0.85, 0.52, 0.22), 0.8),
    "Roof": ((0.36, 0.37, 0.40), 0.9),
    "Catwalk": ((0.30, 0.42, 0.55), 0.6),
    "Door": ((0.46, 0.33, 0.22), 0.7),
    "Handle": ((0.12, 0.12, 0.13), 0.4),
}

DIRECTIONS = {  # the engine's: Y up, North is -Z
    "North": (0.0, -1.0),
    "South": (0.0, 1.0),
    "East": (1.0, 0.0),
    "West": (-1.0, 0.0),
}


# ---------------------------------------------------------------------------------------------------------------------
# Geometry: boxes and rectangles in the engine's space (meters, Y up)

class MeshBuilder:
    """Boxes for one model, by material; each face gets UVs in meters, projected along its axis (the grid lines up
    across faces and boxes)"""

    def __init__(self):
        self.parts = {}  # material -> (vertices, faces, uvs)

    def box(self, material, lo, hi):
        """An axis-aligned box from its low corner to its high corner, (x, y, z) each"""
        if any(hi[i] - lo[i] < 1e-4 for i in range(3)):
            return
        vertices, faces, uvs = self.parts.setdefault(material, ([], [], []))
        x0, y0, z0 = lo
        x1, y1, z1 = hi
        # Each face: four corners counterclockwise seen from outside, and the axes its UVs come from
        quads = [
            ((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (0, 1)),   # +Z
            ((x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (0, 1)),   # -Z
            ((x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (2, 1)),   # +X
            ((x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0), (2, 1)),   # -X
            ((x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0), (0, 2)),   # +Y
            ((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1), (0, 2)),   # -Y
        ]
        for a, b, c, d, (u, v) in quads:
            start = len(vertices)
            for p in (a, b, c, d):
                vertices.append(p)
                uvs.append((p[u], p[v]))
            faces.append((start, start + 1, start + 2, start + 3))

    def triangles(self):
        return sum(2 * len(faces) for _, faces, _ in self.parts.values())

    def empty(self):
        return not self.parts


def subtract_rects(rect, holes):
    """A rectangle (x0, z0, x1, z1) without the holes, as rectangles: cut along every hole's edges, keep the cells
    outside the holes, join the cells of a row and then equal rows"""
    x0, z0, x1, z1 = rect
    xs = sorted({x0, x1} | {h[i] for h in holes for i in (0, 2) if x0 < h[i] < x1})
    zs = sorted({z0, z1} | {h[i] for h in holes for i in (1, 3) if z0 < h[i] < z1})

    def inside(cx, cz):
        return any(h[0] <= cx <= h[2] and h[1] <= cz <= h[3] for h in holes)

    rows = []
    for j in range(len(zs) - 1):
        spans = []
        for i in range(len(xs) - 1):
            if inside((xs[i] + xs[i + 1]) / 2, (zs[j] + zs[j + 1]) / 2):
                continue
            if spans and spans[-1][1] == xs[i]:
                spans[-1][1] = xs[i + 1]
            else:
                spans.append([xs[i], xs[i + 1]])
        rows.append((zs[j], zs[j + 1], spans))
    result = []
    open_spans = {}  # (xa, xb) -> z start: a span carried down equal rows
    for za, zb, spans in rows:
        keys = {(a, b) for a, b in spans}
        for key in list(open_spans):
            if key not in keys:
                result.append((key[0], open_spans.pop(key), key[1], za))
        for key in keys:
            open_spans.setdefault(key, za)
    for key, start in open_spans.items():
        result.append((key[0], start, key[1], zs[-1]))
    return result


def wall(builder, material, start, end, axis, thickness, bottom, top, openings, settings):
    """A straight wall along x (axis 0) or z (axis 2) between start and end (its center line, (x, y, z)), with its
    openings: each {"Offset": from the wall's west or north end (the smaller coordinate) to the opening's middle,
    "Kind": Door or Window, optional Width, Height, Sill}"""
    a = min(start[axis], end[axis])
    b = max(start[axis], end[axis])
    other = 2 - axis
    middle = start[other]
    cuts = []
    for o in openings:
        kind = o.get("Kind", "Door")
        width = o.get("Width", settings["DoorWidth"] if kind == "Door" else settings["WindowWidth"])
        sill = 0.0 if kind == "Door" else o.get("Sill", settings["WindowSill"])
        height = o.get("Height", settings["DoorHeight"] if kind == "Door" else settings["WindowHeight"])
        center = a + o["Offset"]
        lo, hi = center - width / 2, center + width / 2
        if lo < a - 1e-6 or hi > b + 1e-6:
            raise SystemExit(f"build_level: an opening at {o['Offset']} m doesn't fit its wall ({b - a:.2f} m long)")
        cuts.append((lo, hi, bottom + sill, min(top, bottom + sill + height)))
    cuts.sort()

    def piece(u0, u1, y0, y1):
        lo = [0.0, y0, 0.0]
        hi = [0.0, y1, 0.0]
        lo[axis], hi[axis] = u0, u1
        lo[other], hi[other] = middle - thickness / 2, middle + thickness / 2
        builder.box(material, lo, hi)

    u = a
    for lo, hi, y0, y1 in cuts:
        if lo < u - 1e-6:
            raise SystemExit("build_level: two openings in one wall overlap")
        piece(u, lo, bottom, top)          # the wall up to the opening
        piece(lo, hi, bottom, y0)          # under it (a window's sill)
        piece(lo, hi, y1, top)             # over it
        u = hi
    piece(u, b, bottom, top)


def stairs(builder, material, at, direction, width, bottom, rise, settings):
    """Straight stairs: at is the middle of the first step's front edge (x, z); they climb rise along direction.
    Returns the rectangle they cover (x0, z0, x1, z1): the floor above gets a hole there"""
    steps = max(1, math.ceil(rise / settings["StairRiser"] - 1e-6))
    riser = rise / steps
    tread = settings["StairTread"]
    run = steps * tread
    dx, dz = DIRECTIONS[direction]
    side = (-dz, dx)

    def rect(u0, u1):
        # From u0 to u1 along the climb, the stairs' width across
        corners = [(at[0] + dx * u + side[0] * s, at[1] + dz * u + side[1] * s)
                   for u in (u0, u1) for s in (-width / 2, width / 2)]
        xs = [c[0] for c in corners]
        zs = [c[1] for c in corners]
        return min(xs), min(zs), max(xs), max(zs)

    # Each step is a slab from its front to the top of the stairs, stacked: a solid staircase without overlaps
    for i in range(steps):
        x0, z0, x1, z1 = rect(i * tread, run)
        builder.box(material, (x0, bottom + i * riser, z0), (x1, bottom + (i + 1) * riser, z1))
    return rect(0.0, run), steps, riser, run


def catwalk(builder, name, c, s):
    """A catwalk: a deck along x or z from From to To ([x, y, z] each: the deck's top at that end, on its middle line),
    Width wide, with railings along both sides. Ends at different heights: flat at the higher one, and steps down at the
    lower end (each at most StairRiser), as long as they need. Returns the report's line"""
    a, b = c["From"], c["To"]
    width = c.get("Width", s["CatwalkWidth"])
    deck = s["CatwalkThickness"]
    railing = s["RailingHeight"]
    rail = s["RailingThickness"]
    if abs(a[0] - b[0]) > 1e-6 and abs(a[2] - b[2]) > 1e-6:
        raise SystemExit(f"build_level: the catwalk {name} from {a} to {b} isn't along x or z")
    axis = 0 if abs(a[2] - b[2]) < 1e-6 else 2
    other = 2 - axis
    high, low = (a, b) if a[1] >= b[1] else (b, a)
    length = abs(b[axis] - a[axis])
    sign = 1.0 if low[axis] > high[axis] else -1.0  # from the high end towards the low end along the axis

    def span(u0, u1):
        # From u0 to u1 along the catwalk (measured from the high end), its width across: (lo, hi) corners without y
        p0 = high[axis] + sign * u0
        p1 = high[axis] + sign * u1
        lo, hi = [0.0, 0.0, 0.0], [0.0, 0.0, 0.0]
        lo[axis], hi[axis] = min(p0, p1), max(p0, p1)
        lo[other], hi[other] = high[other] - width / 2, high[other] + width / 2
        return lo, hi

    rise = high[1] - low[1]
    run, steps = 0.0, 0
    if rise > 1e-6:
        # The steps climb from the low end towards the high one
        direction = {(0, 1.0): "West", (0, -1.0): "East", (2, 1.0): "North", (2, -1.0): "South"}[(axis, sign)]
        _, steps, _, run = stairs(builder, "Catwalk", (low[0], low[2]), direction, width, low[1], rise, s)
        if run > length + 1e-6:
            raise SystemExit(f"build_level: the catwalk {name} is {length:.2f} m long, its steps need {run:.2f} m")
    if length - run > 1e-6:
        lo, hi = span(0.0, length - run)
        builder.box("Catwalk", (lo[0], high[1] - deck, lo[2]), (hi[0], high[1], hi[2]))
    if railing > 0.0:
        for side in (-1.0, 1.0):
            edge = high[other] + side * (width / 2 - rail / 2)
            for u0, u1, bottom in ((0.0, length - run, high[1] - deck), (length - run, length, low[1])):
                if u1 - u0 < 1e-6:
                    continue
                lo, hi = span(u0, u1)
                lo[other], hi[other] = edge - rail / 2, edge + rail / 2
                builder.box("Catwalk", (lo[0], bottom, lo[2]), (hi[0], high[1] + railing, hi[2]))
    return (f"{name}: {length:.2f} m from {a} to {b}, {width} m wide" +
            (f", {steps} steps down {rise:.2f} m over {run:.2f} m" if steps else ""))


def outer_walls(model, building, floor, bottom, top, s):
    """The four outer walls of a building's floor (or its roof's parapet), in the footprint's edge, from bottom to top,
    with the openings of that floor: North and South run its whole width, East and West between them"""
    mx, mz = building["Min"]
    width, depth = building["Size"]
    t = s["WallThickness"]
    openings = {side: [] for side in WALL_SIDES}
    for o in building.get("Openings", []):
        if o.get("Floor", 0) == floor:
            openings[o["Wall"]].append(o)
    outside = "WallOutside"
    wall(model, outside, (mx, 0, mz + t / 2), (mx + width, 0, mz + t / 2), 0, t, bottom, top, openings["North"], s)
    wall(model, outside, (mx, 0, mz + depth - t / 2), (mx + width, 0, mz + depth - t / 2), 0, t, bottom, top,
         openings["South"], s)
    # East and West: their openings' offsets count from the footprint's north edge, as the walls' own start
    east = [dict(o, Offset=o["Offset"] - t) for o in openings["East"]]
    west = [dict(o, Offset=o["Offset"] - t) for o in openings["West"]]
    wall(model, outside, (mx + width - t / 2, 0, mz + t), (mx + width - t / 2, 0, mz + depth - t), 2, t, bottom, top, east, s)
    wall(model, outside, (mx + t / 2, 0, mz + t), (mx + t / 2, 0, mz + depth - t), 2, t, bottom, top, west, s)


# ---------------------------------------------------------------------------------------------------------------------
# Doors: an opening of Kind Door with a Leaf gets one (Hinged) or two (Double) leaves on hinges, models of their own,
# and a Nav Area (Door) in the doorway, closed while the door is locked

LEAF_GAP = 0.01  # between a leaf and its frame, the floor and the other leaf


def leaf_model(width, height, s):
    """A leaf from its hinge (the model's origin) along +X, the floor at y 0: its slab and a handle on each side near
    its free end"""
    builder = MeshBuilder()
    half = s["DoorLeafThickness"] / 2
    builder.box("Door", (LEAF_GAP, LEAF_GAP, -half), (width - LEAF_GAP, height - LEAF_GAP, half))
    for side in (-1, 1):
        z0, z1 = sorted((side * half, side * (half + 0.035)))
        builder.box("Handle", (width - 0.16, 0.98, z0), (width - 0.04, 1.02, z1))
    return builder


def outer_doors(building, floor, bottom, s):
    """The doors of a building's floor: (name, leaves, area, locked); a leaf is (its hinge, its yaw in degrees: the
    leaf's +X along the wall, its width, its height), the area (its middle, yaw, half extents)"""
    mx, mz = building["Min"]
    width, depth = building["Size"]
    t = s["WallThickness"]
    doors = []
    for o in building.get("Openings", []):
        if o.get("Floor", 0) != floor or o.get("Kind") != "Door" or not o.get("Leaf"):
            continue
        side = o["Wall"]
        w = o.get("Width", s["DoorWidth"])
        h = o.get("Height", s["DoorHeight"])
        if side in ("North", "South"):
            middle = (mx + o["Offset"], mz + t / 2 if side == "North" else mz + depth - t / 2)
            along, yaw = (1.0, 0.0), 0.0       # the hinge at the west end, the leaf east of it
        else:
            middle = (mx + width - t / 2 if side == "East" else mx + t / 2, mz + o["Offset"])
            along, yaw = (0.0, 1.0), -90.0     # the hinge at the north end, the leaf south of it
        low = (middle[0] - along[0] * w / 2, bottom, middle[1] - along[1] * w / 2)
        high = (middle[0] + along[0] * w / 2, bottom, middle[1] + along[1] * w / 2)
        if o["Leaf"] == "Hinged":
            leaves = [(low, yaw, w, h)]
        elif o["Leaf"] == "Double":
            leaves = [(low, yaw, w / 2, h), (high, yaw + 180.0, w / 2, h)]
        else:
            raise SystemExit(f"build_level: {building['Name']}: a door's Leaf is Hinged or Double, not {o['Leaf']}")
        name = f"Door_{building['Name']}_{side}" + (f"_F{floor}" if floor else "")
        area = ((middle[0], bottom + 1.0, middle[1]), 0.0 if side in ("North", "South") else 90.0, (w / 2, 1.0, 0.45))
        doors.append((name, leaves, area, bool(o.get("Locked", False))))
    return doors


# ---------------------------------------------------------------------------------------------------------------------
# The level

WALL_SIDES = ("North", "South", "East", "West")


def build_level(data):
    settings = dict(DEFAULTS)
    settings.update(data.get("Settings", {}))
    models = {}       # model name -> MeshBuilder
    leaves = {}       # the doors' leaf models, by name
    doors = []        # (name, leaves: (hinge, yaw, width, height, model name), area, locked)
    report = []

    footprints = []
    for building in data.get("Buildings", []):
        name = building["Name"]
        s = dict(settings)
        s.update(building.get("Settings", {}))
        mx, mz = building["Min"]
        width, depth = building["Size"]
        floors = building.get("Floors", 1)
        story = s["StoryHeight"]
        slab = s["FloorThickness"]
        t = s["WallThickness"]
        base = building.get("Base", 0.0)  # the ground floor's top above the level's origin
        footprints.append((mx, mz, mx + width, mz + depth))

        def local(p):
            return (mx + p[0], mz + p[1])

        # Stairs first: the floors above get their holes
        holes = {f: [] for f in range(floors + 1)}
        for h in building.get("Holes", []):
            x0, z0 = local(h["Min"])
            holes[h["Floor"]].append((x0, z0, x0 + h["Size"][0], z0 + h["Size"][1]))
        stair_parts = []
        for st in building.get("Stairs", []):
            f = st["From"]
            if not 0 <= f < floors:
                raise SystemExit(f"build_level: {name}'s stairs from floor {f}: the building has floors 0 to {floors - 1}")
            builder = models.setdefault(f"{name}_F{f}", MeshBuilder())
            covered, steps, riser, run = stairs(builder, "Stairs", local(st["At"]), st.get("Direction", "North"),
                                                st.get("Width", s["StairWidth"]), base + f * story, story, s)
            holes[f + 1].append(covered)
            stair_parts.append(f"floor {f} to {f + 1}: {steps} steps of {riser * 100:.1f} cm, {run:.2f} m long")

        for f in range(floors + 1):
            top = base + f * story
            model = models.setdefault(f"{name}_Roof" if f == floors else f"{name}_F{f}", MeshBuilder())
            material = "Roof" if f == floors else "Floor"
            for x0, z0, x1, z1 in subtract_rects((mx, mz, mx + width, mz + depth), holes[f]):
                model.box(material, (x0, top - slab, z0), (x1, top, z1))
            if f == floors:
                if s["Parapet"] > 0.0:
                    outer_walls(model, building, f, top, top + s["Parapet"], s)
                break

            # The outer walls, in the footprint's edge
            bottom, wall_top = top, top + story - slab
            outer_walls(model, building, f, bottom, wall_top, s)
            for door_name, door_leaves, area, locked in outer_doors(building, f, bottom, s):
                named = []
                for hinge, yaw, w, h in door_leaves:
                    model_name = f"DoorLeaf_{round(w * 100)}x{round(h * 100)}"
                    if model_name not in leaves:
                        leaves[model_name] = leaf_model(w, h, s)
                    named.append((hinge, yaw, w, h, model_name))
                doors.append((door_name, named, area, locked))
                report.append(f"{door_name}: {len(named)} leaf(s) of {named[0][2]:.2f} x {named[0][3]:.2f} m" +
                              (", locked" if locked else ""))

            # The rooms' walls
            for w in building.get("Walls", []):
                if w.get("Floor", 0) != f:
                    continue
                a, b = local(w["From"]), local(w["To"])
                if abs(a[0] - b[0]) > 1e-6 and abs(a[1] - b[1]) > 1e-6:
                    raise SystemExit(f"build_level: {name}: a wall from {w['From']} to {w['To']} isn't along x or z")
                axis = 0 if abs(a[1] - b[1]) < 1e-6 else 2
                wall(model, "Wall", (a[0], 0.0, a[1]), (b[0], 0.0, b[1]), axis, w.get("Thickness", t), bottom, wall_top,
                     w.get("Openings", []), s)

        report.append(f"{name}: {width} x {depth} m, {floors} floor(s) of {story} m" +
                      ("; stairs " + "; ".join(stair_parts) if stair_parts else "") +
                      (f"; a parapet of {s['Parapet']} m" if s["Parapet"] > 0.0 else ""))

    for i, c in enumerate(data.get("Catwalks", [])):
        name = c.get("Name", f"Catwalk{i + 1}")
        s = dict(settings)
        s.update(c.get("Settings", {}))
        report.append(catwalk(models.setdefault(name, MeshBuilder()), name, c, s))

    ground = data.get("Ground")
    if ground:
        gx, gz = ground["Min"]
        gw, gd = ground["Size"]
        thickness = ground.get("Thickness", settings["GroundThickness"])
        model = models.setdefault("Ground", MeshBuilder())
        for x0, z0, x1, z1 in subtract_rects((gx, gz, gx + gw, gz + gd), footprints):
            model.box("Ground", (x0, -thickness, z0), (x1, 0.0, z1))
    return models, leaves, doors, report


# ---------------------------------------------------------------------------------------------------------------------
# Blender: textures, materials, objects, files

def to_blender(p):
    # engine (x, y up, z) -> Blender (x, y = -z, z up): a rotation, so faces keep their winding
    return (p[0], -p[2], p[1])


def grid_texture(path, color, size=256):
    """The material's base color with a line along two edges: tiled once per meter, a grid of 1 m"""
    image = np.empty((size, size, 4), dtype=np.float32)
    image[:, :, :3] = color
    image[:, :, 3] = 1.0
    line = max(2, size // 64)
    shade = np.array(color) * 0.72
    image[:line, :, :3] = shade
    image[:, :line, :3] = shade
    img = bpy.data.images.new(os.path.basename(path), size, size, alpha=True)
    img.colorspace_settings.name = "Non-Color"  # the values as they are (sRGB in the file)
    img.pixels.foreach_set(image.ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    return img


def make_material(name, texture_dir):
    color, roughness = MATERIALS[name]
    mat = bpy.data.materials.new(f"Greybox_{name}")
    if mat.node_tree is None:  # Blender 5 gives a new material its nodes
        mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = 0.0
    mat.use_backface_culling = True  # one-sided in the glTF: the boxes are closed
    image = grid_texture(os.path.join(texture_dir, f"Greybox_{name}.png"), color)
    node = mat.node_tree.nodes.new("ShaderNodeTexImage")
    node.image = image
    mat.node_tree.links.new(node.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def make_object(name, builder, materials):
    vertices, faces, uvs, face_materials = [], [], [], []
    slots = sorted(builder.parts)
    for slot, material in enumerate(slots):
        v, f, uv = builder.parts[material]
        offset = len(vertices)
        vertices.extend(to_blender(p) for p in v)
        faces.extend(tuple(i + offset for i in face) for face in f)
        uvs.extend(uv)
        face_materials.extend([slot] * len(f))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    for material in slots:
        mesh.materials.append(materials[material])
    mesh.polygons.foreach_set("material_index", face_materials)
    layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        layer.data[loop.index].uv = uvs[loop.vertex_index]
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def export_model(path, obj, texture_dir):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    # .gltf with its .bin and the textures beside it in their own folder: the engine reads textures from files, not
    # embedded in a .glb; every model of every level names the same few grid textures
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLTF_SEPARATE", use_selection=True, export_yup=True,
                              export_apply=True, export_materials="EXPORT",
                              export_texture_dir=os.path.relpath(texture_dir, os.path.dirname(path)))


# ---------------------------------------------------------------------------------------------------------------------
# The scene

def door_entities(project_dir, doors, leaf_files, settings):
    """A door's leaves (a model on its hinge, a Box collider, a Kinematic body: the navmesh's bake leaves them out; the
    door script, its fields the Area, the other leaf, Locked) and its Nav Area (Door: closed while locked)"""
    entities = []
    next_id = DOOR_IDS
    script = settings["DoorScript"]
    half = settings["DoorLeafThickness"] / 2
    for name, leaves, (middle, area_yaw, extents), locked in doors:
        area_id = next_id + len(leaves)
        ids = [next_id + i for i in range(len(leaves))]
        for i, (hinge, yaw, w, h, model) in enumerate(leaves):
            relative = os.path.relpath(leaf_files[model], project_dir).replace("\\", "/")
            fields = f"        Area: {{Entity: {area_id}}}\n        Width: {{Float: {w:g}}}\n"
            if len(leaves) > 1:
                fields += f"        Partner: {{Entity: {ids[1 - i]}}}\n"
            if locked:
                fields += "        Locked: {Bool: true}\n"
            behavior = (f"    Script:\n      Class: {script}\n      Fields:\n{fields}") if script else ""
            leaf_name = name if len(leaves) == 1 else f"{name}_{'AB'[i]}"
            entities.append(f"""  - ID: {ids[i]}
    Name: {leaf_name}
    Parent: {LEVEL_IDS}
    Transform:
      Translation: [{hinge[0]:g}, {hinge[1]:g}, {hinge[2]:g}]
      Rotation: [0, {yaw:g}, 0]
      Scale: [1, 1, 1]
    Model:
      File: {relative}
    Collider:
      Shape: Box
      Offset: [{w / 2:g}, {h / 2:g}, 0]
      HalfExtents: [{w / 2 - LEAF_GAP:g}, {h / 2 - LEAF_GAP:g}, {half:g}]
      Radius: 0.5
      HalfHeight: 0.5
      Friction: 0.5
      Restitution: 0
      IsTrigger: false
    RigidBody:
      Type: Kinematic
      Mass: 1
      LinearDamping: 0.05
      AngularDamping: 0.05
      GravityFactor: 1
      LockRotation: [false, false, false]
      ContinuousCollision: false
{behavior}""")
        entities.append(f"""  - ID: {area_id}
    Name: {name} Area
    Parent: {LEVEL_IDS}
    Transform:
      Translation: [{middle[0]:g}, {middle[1]:g}, {middle[2]:g}]
      Rotation: [0, {area_yaw:g}, 0]
      Scale: [1, 1, 1]
    NavArea:
      Type: Door
      HalfExtents: [{extents[0]:g}, {extents[1]:g}, {extents[2]:g}]
      Cost: 1
      Open: {"false" if locked else "true"}
""")
        next_id = area_id + 1
    if next_id > LEVEL_IDS_END:
        raise SystemExit("build_level: too many doors for the level's IDs")
    return entities


def update_scene(scene_path, project_dir, level, files, extra=()):
    """Replaces the level's entities in the scene (IDs LEVEL_IDS to LEVEL_IDS_END) by an entity Level_<Level> with a
    model per file, each with a TriangleMesh collider, and the extra entities (the doors); the scene's other entities
    stay as they are. The level's entities go where the old ones were; an old one that has every line the new one would
    (the editor's save adds some: a model's FileID, a collider's unused sizes) stays as it is, so a run that changes
    nothing changes nothing in the scene. The file keeps its line ends"""
    with open(scene_path, "rb") as f:
        raw = f.read().decode("utf-8")
    newline = "\r\n" if "\r\n" in raw else "\n"
    text = raw.replace("\r\n", "\n")
    head, sep, body = text.partition("Entities:\n")
    if not sep:
        raise SystemExit(f"build_level: {scene_path} has no Entities")
    blocks = re.split(r"(?m)^(?=  - ID: )", body)

    def is_level(b):
        return b.startswith("  - ID: ") and LEVEL_IDS <= int(b.split()[2]) <= LEVEL_IDS_END

    old = {int(b.split()[2]): b for b in blocks if is_level(b)}
    entities = [f"""  - ID: {LEVEL_IDS}
    Name: Level_{level}
    Parent: 0
    Transform:
      Translation: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
"""]
    for i, path in enumerate(files):
        name = os.path.splitext(os.path.basename(path))[0]
        relative = os.path.relpath(path, project_dir).replace("\\", "/")
        entities.append(f"""  - ID: {LEVEL_IDS + 1 + i}
    Name: {name}
    Parent: {LEVEL_IDS}
    Transform:
      Translation: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    Model:
      File: {relative}
    Collider:
      Shape: TriangleMesh
      Friction: 0.6
      Restitution: 0
      IsTrigger: false
""")

    def keep_old(new):
        before = old.get(int(new.split()[2]))
        if before is None:
            return new
        lines = before.rstrip("\n").split("\n")
        return before if all(line in lines for line in new.rstrip("\n").split("\n")) else new

    # Each in the old one's place (a model's mesh parts, the editor's, follow it), the new ones after the last
    level = {int(e.split()[2]): keep_old(e) for e in entities + list(extra)}
    last = max((i for i, b in enumerate(blocks) if is_level(b)), default=len(blocks) - 1)
    out = []
    for i, b in enumerate(blocks):
        if is_level(b):
            out.append(level.pop(int(b.split()[2]), ""))
        elif b:
            out.append(b)
        if i == last:
            out.extend(level.pop(key) for key in sorted(level))
    out.extend(level[key] for key in sorted(level))
    text = head + sep + "".join(out)
    with open(scene_path, "wb") as f:
        f.write(text.replace("\n", newline).encode("utf-8"))


# ---------------------------------------------------------------------------------------------------------------------

def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if not args:
        raise SystemExit("build_level.py: give the level's data file after --")
    data_path = os.path.abspath(args[0])
    with open(data_path, encoding="utf-8") as f:
        data = json.load(f)
    level = data["Name"]
    data_dir = os.path.dirname(data_path)
    project_dir = os.path.normpath(os.path.join(data_dir, data.get("ProjectDirectory", "..")))
    models_root = os.path.normpath(os.path.join(data_dir, data.get("ModelsDirectory", "../Assets/Models/Levels")))
    models_dir = os.path.join(models_root, level)
    texture_dir = os.path.join(models_root, "Textures")
    os.makedirs(models_dir, exist_ok=True)
    os.makedirs(texture_dir, exist_ok=True)

    models, leaves, doors, report = build_level(data)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    materials = {name: make_material(name, texture_dir) for name in MATERIALS}

    # The files of an earlier run go first: a building or floor removed from the data leaves nothing behind
    for old in os.listdir(models_dir):
        if old.startswith(f"Level_{level}_") and old.endswith((".gltf", ".bin")):
            os.remove(os.path.join(models_dir, old))
    written = []
    for name, builder in sorted(models.items()):
        if builder.empty():
            continue
        obj = make_object(f"Level_{level}_{name}", builder, materials)
        path = os.path.join(models_dir, f"Level_{level}_{name}.gltf")
        export_model(path, obj, texture_dir)
        written.append(path)
        print(f"build_level: {os.path.basename(path)}: {builder.triangles()} triangles")
        bpy.data.objects.remove(obj)
    leaf_files = {}
    for name, builder in sorted(leaves.items()):
        obj = make_object(f"Level_{level}_{name}", builder, materials)
        path = os.path.join(models_dir, f"Level_{level}_{name}.gltf")
        export_model(path, obj, texture_dir)
        leaf_files[name] = path
        print(f"build_level: {os.path.basename(path)}: {builder.triangles()} triangles (a door's leaf)")
        bpy.data.objects.remove(obj)

    for line in report:
        print(f"build_level: {line}")
    scene = data.get("Scene")
    if scene:
        scene_path = os.path.normpath(os.path.join(data_dir, scene))
        settings = dict(DEFAULTS)
        settings.update(data.get("Settings", {}))
        update_scene(scene_path, project_dir, level, written, door_entities(project_dir, doors, leaf_files, settings))
        print(f"build_level: {len(written)} models and {len(doors)} doors in {scene_path}")


main()
