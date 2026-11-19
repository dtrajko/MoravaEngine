# Dresses the soldiers' base body in a uniform (the shooter's plan, Phase 1.3), in Blender without its window. The look
# follows the user's reference (2026-11-18: combat pants in a desert three-color camouflage, a belt, ankle boots):
# - The clothes are shaped from the body's own mesh: its vertices smoothed (no muscles under cloth) and pushed out by the
#   clothes' thickness and fit, the trousers' details raised (the knee pads with their ribs, the calves' pockets), the hems worn over the boots, the feet reshaped into boots (a toe cap, a shaft, a sole's lip). The
#   vertices keep their UVs and bone weights: every clip plays on the dressed body as on the bare one.
# - The belt with its buckle and loops: meshes of their own, plain materials, weighted as the body under them.
# - The colors, seams and panels are painted into copies of the body's maps where those shapes are (the same regions).
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/CharacterPoses/make_uniform.py -- [--scheme Player] [--preview <folder>]
#
# Reads  Projects/Outpost/Assets/Models/QuaterniusCharacters/Superhero_Male_FullBody.gltf (and its .bin and maps)
# Writes Projects/Outpost/Assets/Models/QuaterniusCharacters/Soldier_<scheme>.gltf (.bin)
#        and T_Soldier_<scheme>_Color.png, _Normal.png, _Roughness.png beside it
#        and Soldier_<scheme>_Arms.gltf (.bin): the first person's arms (the view model, E2), the body cut to the arms
#
# How the paint finds its places: every triangle of the bare body is drawn into the maps by its UVs, so each texel knows
# where on the body it is (the rest pose, a T: the arms along x) and how much the head's bones move it; the clothes' regions
# are tests of that place, the same tests that shape the mesh. The camouflage is a 3D noise of the place, so it runs on
# across the UVs' seams. --preview renders the dressed body in the rest pose and in the poses where it bends most (the fast
# walk, the sprint, the crouch, the rifle's aim), to check the cloth doesn't cut into itself. Each scheme is a set of colors:
# the guards (Phase 5) wear the same uniform in theirs.
#
# Everything is generated: edits by hand are lost on the next run.

import json
import math
import os
import sys

import bpy
import numpy as np

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
MODELS = os.path.join(ROOT, "Projects", "Outpost", "Assets", "Models")
FOLDER = os.path.join(MODELS, "QuaterniusCharacters")
SOURCE = os.path.join(FOLDER, "Superhero_Male_FullBody.gltf")
BODY_MESH = "SuperHero_Male"
BODY_MATERIAL = "MI_Superhero_Male"
POSE_FILES = [os.path.join(MODELS, "QuaterniusAnimations", "UAL1_Standard.glb"),
              os.path.join(MODELS, "SoldierAnimations", "Soldier_Locomotion.glb"),
              os.path.join(MODELS, "SoldierAnimations", "Soldier_Rifles.glb")]

# The colors (sRGB 0-255). The camouflage: a desert three-color (a pinkish sand, large tan blobs, thin wavy red-brown
# streaks); the panels: the trousers' plain stretch parts (the crotch, the back under the belt, the knees) and the hems.
# The shirt is plain, its sleeves too: the camouflage is below the waist only (the user, 2026-11-18)
SCHEMES = {
    "Player": {
        "camo": {"base": (198, 176, 152), "blob": (178, 156, 118), "streak": (126, 76, 62)},
        "panel": (178, 156, 120), "shirt": (104, 98, 72), "belt": (98, 98, 72), "buckle": (72, 72, 68),
        "loops": (178, 156, 120), "boots": (154, 118, 82), "soles": (88, 72, 54), "laces": (104, 80, 56),
        "gloves": (100, 90, 70), "balaclava": (64, 64, 50),
    },
}

# The parts, in the rest pose's meters (x to the left, -y forward, z up, the ground at z 0)
BOOT_TOP = 0.135       # the boots up to here (ankle boots); the trousers' hems over them from here on
SOLE_TOP = 0.030       # the sole
WAIST = (1.065, 1.110) # the belt between these heights (the hips' top, above the buttocks: their back is out furthest at
                       # 1.00 m, the waist narrowest at 1.17); the trousers below, the shirt above
GLOVE_FROM = 0.685     # the gloves from this far out along the arm (|x|; the wrist bone is at 0.706)
CUFF = 0.018           # the glove's cuff, a little darker, this wide
HEAD_FROM = 0.5        # the balaclava where the head's and neck's bones move the skin at least this much
EYES = (0.0, 1.700)    # the eye opening's middle (x, z), in front of the face
EYES_HALF = (0.066, 0.026)  # its half width and half height (a rounded slot over both eyes and the brows)
COLLAR = (1.532, 0.020, 0.035)  # the shirt's neckline, where the balaclava ends (it covers the whole neck, the user
                                # 2026-11-19): its height at the front and back, how much higher at the sides (the
                                # shoulders' slope), the neck's axis (y)
EARS = (0.071, 1.625, 1.730)  # the ears: out past this |x|, between these heights (behind the face); pressed flat under it
RIBS = 64              # the knit's ribs round the head
CAMO_SCALE = 4.5       # camouflage blobs per meter (about)
LEG_X, LEG_Y = 0.114, 0.036  # each leg's axis (the thigh and calf bones), for the trousers' details

# The trousers' details: regions on the legs, each a set of limits that are positive inside (meters), so the distance to
# the nearest edge is their smallest. lx: out from the leg's axis (sideways), ly: back from it (front: negative)
KNEE_RIBS = (0.49, 0.535, 0.58)  # the knee pads' grooves (heights)
RAISE = {"knee": 0.009, "calf": 0.005}  # how far each stands out of the cloth
RAMP = {"knee": 0.012, "calf": 0.006}   # over how much of its edge it rises
GROOVE = (0.005, 0.006)  # the knee pads' grooves: how deep, how wide
FLAPS = {"calf": 0.028}  # the zip or the flap's line, this far under the pocket's top
SEAM = 0.005           # a seam's width
FLY = (0.92, 1.06)     # the trousers' fly, a seam up the front's middle

# How much of the skin's maps is kept under each part: its light and shade (the creases), and its normal map's relief
SHADE = 0.25
RELIEF = {"camo": 0.10, "panel": 0.10, "shirt": 0.12, "belt": 0.1, "boots": 0.05, "soles": 0.05, "laces": 0.05,
          "gloves": 0.4, "balaclava": 0.25}
ROUGHNESS = {"camo": 0.9, "panel": 0.85, "shirt": 0.9, "belt": 0.8, "boots": 0.75, "soles": 0.85, "laces": 0.85,
             "gloves": 0.7, "balaclava": 0.95}

# The clothes' shape (tailor). First the cloth is smoothed (Taubin: no shrinking), then pushed out along the surface by
# its thickness and fit (meters). Little where the body bends (between the legs, the armpits), or the cloth cuts into
# itself in a stride or a crouch
SMOOTH_CLOTH = 10      # steps for the shirt
SMOOTH_TROUSERS = 16   # for the trousers (a regular fit: the legs' muscles go)
# Then the hollows are filled from outside (the surface moved only outwards, towards its neighbors): cloth spans the
# buttocks' cleft, the crotch, the spine's and the belly's grooves instead of following them. Not near the armpits (it
# would web the arm to the chest)
FILL_STEPS = 40
OFFSET = {"shirt": 0.013, "sleeve": 0.016, "trousers": 0.012, "crotch": 0.006, "armpit": 0.004, "glove": 0.002, "head": 0.004}
# The balaclava: a knit hood over the head, smoothed (no ears, no skull's bumps under it) and over the hollows (the nape,
# behind the ears), the ears first pressed flat (smoothed alone, the head round them fixed); the eyes' slot stays the skin
SMOOTH_HEAD = 14
EAR_STEPS = 60
COLLAR_BAND = (0.010, 0.0025, 0.0005, 0.003)  # the shirt's collar, a narrow band over the balaclava's bottom (a ring
                                              # mesh, the shirt's color; the user found a taller one odd, 2026-11-19): its
                                              # height, thickness, gap to what is under it, how far under the neckline it starts
LEG_SLIM = (0.92, 0.20, 0.30, 0.80, 0.95)  # the legs drawn in toward their axis (the base body's thighs are bulky; the user
                                     # 2026-11-19): by this factor between the second and third heights, from none at
                                     # the first, back to none at the fourth
ARM_SLIM = (0.92, 0.21, 0.30, 0.62, GLOVE_FROM)  # the sleeves too (along |x|: from the shoulders to the wrists)
THIGH_EXTRA = (0.004, 0.45, 0.75)  # the trousers looser over the thighs: this much more, from the first height to the second
HEM = (0.018, 0.05)    # the trousers over the boots: out this much at the hem, back to their fit this far up
HEM_CUFF = (-0.015, 0.024, 0.004, 0.002)  # the hem's cuff (a ring mesh, the panels' color): from and to (around the boots'
                                          # top), its thickness and its gap to what is under it

# The boots: meshes of their own, each the convex hull of the foot and the ankle under BOOT_TOP (a toe box and a flat sole:
# no toes, no arch), remeshed evenly (so it bends at the ball of the foot as the foot does), rounded and a little bigger; the
# foot inside is removed. Weighted as the foot under them, with three plain materials: the upper, the sole, the laces
BOOT_VOXEL = 0.009     # the remesh's size (meters)
BOOT_ROUND = (0.5, 8)  # the smoothing that rounds the hull's ridges (factor, steps)
BOOT_OUT = 0.006       # pushed out by this past the hull
SOLE_LIP = 0.010       # the sole sticks out sideways by this at the ground, less up to SOLE_TOP

# The thighs' cargo pockets: bellows pockets with a flap, meshes of their own on the thighs' outsides (the trousers' map:
# the camouflage runs over them), weighted as the cloth under them
POCKET_HEIGHTS = (0.565, 0.795)  # their bottom and top
POCKET_ANGLES = (-62.0, 38.0)    # round the leg from its outside (degrees, front: negative): their back and front edges
POCKET_DEPTH = 0.020   # how far the bellows stand out of the cloth at the middle
POCKET_ROUND = 0.014   # over how much of the edges they rise
POCKET_SINK = 0.002    # their edges sunk into the cloth (no gap shows)
FLAP = (0.062, 0.006, 0.006)  # the flap: its height (down from the top), how far past the pocket's sides, its thickness
POCKET_GRID = (14, 20) # steps across and up

# The belt, its buckle and its loops
BELT = (WAIST[0] + 0.004, WAIST[1] - 0.004)  # its bottom and top
BELT_THICKNESS = 0.006
BELT_GAP = 0.0008      # off the cloth: about 3 mm on average with the cloth's dips (half of before, the user
                       # 2026-11-19)
BUCKLE_SIZE = (0.062, 0.010, 0.050)  # width, thickness, height
LOOPS = (-150, -100, -50, 50, 100, 150)  # degrees round the waist from the front (the buckle's)
LOOP_SIZE = (0.014, 0.003, 0.012)  # width, thickness, how far past the belt's edges

# The first person's arms (write_arms): what the bones whose names have these parts (both sides) move at least this much
ARM_PARTS = ("upperarm", "lowerarm", "hand", "thumb", "index", "middle", "ring", "pinky")
ARMS_FROM = 0.5

PADDING = 8            # texels each UV island is grown by (so mipmaps don't bring the skin back at the seams)


def load_pixels(path):
    image = bpy.data.images.load(path)
    image.colorspace_settings.name = "Non-Color"  # the file's values as they are
    width, height = image.size
    pixels = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    return pixels.reshape(height, width, 4)


def save_pixels(pixels, path):
    height, width = pixels.shape[:2]
    image = bpy.data.images.new(os.path.basename(path), width, height, alpha=True)
    image.colorspace_settings.name = "Non-Color"
    image.pixels.foreach_set(np.clip(pixels, 0.0, 1.0).astype(np.float32).ravel())
    image.filepath_raw = path
    image.file_format = "PNG"
    image.save()
    print(f"Wrote {os.path.relpath(path, ROOT)}")


def bone_weights(mesh_object, names):
    """Per vertex: the sum of the weights of the bones named."""
    indices = {group.index for group in mesh_object.vertex_groups if group.name in names}
    weights = np.zeros(len(mesh_object.data.vertices), dtype=np.float32)
    for vertex in mesh_object.data.vertices:
        weights[vertex.index] = sum(g.weight for g in vertex.groups if g.group in indices)
    return weights


def rasterize(mesh_object, size):
    """Each texel the body covers: its place in the rest pose and its head weight. Returns (covered, place, head)."""
    mesh = mesh_object.data
    mesh.calc_loop_triangles()
    positions = np.array([tuple(v.co) for v in mesh.vertices], dtype=np.float32)
    head = bone_weights(mesh_object, {"Head", "neck_01"})
    uv = mesh.uv_layers[0].data

    covered = np.zeros((size, size), dtype=bool)
    place = np.zeros((size, size, 3), dtype=np.float32)
    head_weight = np.zeros((size, size), dtype=np.float32)
    for triangle in mesh.loop_triangles:
        corners = np.array([tuple(uv[loop].uv) for loop in triangle.loops], dtype=np.float64) * size
        x0, y0 = np.floor(corners.min(axis=0) - 1).astype(int)
        x1, y1 = np.ceil(corners.max(axis=0) + 1).astype(int)
        x0, y0, x1, y1 = max(x0, 0), max(y0, 0), min(x1, size), min(y1, size)
        if x1 <= x0 or y1 <= y0:
            continue
        xs, ys = np.meshgrid(np.arange(x0, x1) + 0.5, np.arange(y0, y1) + 0.5)
        (ax, ay), (bx, by), (cx, cy) = corners
        area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay)
        if abs(area) < 1e-12:
            continue
        w1 = ((xs - ax) * (cy - ay) - (cx - ax) * (ys - ay)) / area
        w2 = ((bx - ax) * (ys - ay) - (xs - ax) * (by - ay)) / area
        w0 = 1.0 - w1 - w2
        inside = (w0 >= -1e-4) & (w1 >= -1e-4) & (w2 >= -1e-4)
        if not inside.any():
            continue
        v = triangle.vertices
        p = w0[..., None] * positions[v[0]] + w1[..., None] * positions[v[1]] + w2[..., None] * positions[v[2]]
        h = w0 * head[v[0]] + w1 * head[v[1]] + w2 * head[v[2]]
        rows, cols = ys[inside].astype(int), xs[inside].astype(int)
        covered[rows, cols] = True
        place[rows, cols] = p[inside]
        head_weight[rows, cols] = h[inside]
    return covered, place, head_weight


def value_noise(points, seed):
    """Smooth 3D noise in 0-1 at points (n x 3), one lattice cell per unit."""
    rng = np.random.default_rng(seed)
    lattice = rng.random((32, 32, 32)).astype(np.float32)
    cell = np.floor(points).astype(int)
    f = points - cell
    f = f * f * (3.0 - 2.0 * f)
    result = np.zeros(len(points), dtype=np.float32)
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                value = lattice[(cell[:, 0] + dx) % 32, (cell[:, 1] + dy) % 32, (cell[:, 2] + dz) % 32]
                weight = (f[:, 0] if dx else 1 - f[:, 0]) * (f[:, 1] if dy else 1 - f[:, 1]) * (f[:, 2] if dz else 1 - f[:, 2])
                result += value * weight
    return result


def fractal(points, seed):
    return 0.65 * value_noise(points, seed) + 0.35 * value_noise(points * 2.3 + 17.0, seed + 1)


def soft(value, width):
    """0 below -width, 1 above +width, smooth between (soft edges, no stairs)."""
    t = np.clip(value / (2.0 * width) + 0.5, 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def camouflage(points, colors):
    """The desert three-color: the sand base, large tan blobs, and thin wavy red-brown streaks in some places."""
    p = points * CAMO_SCALE
    base, blob, streak = (np.array(colors[k], dtype=np.float32) for k in ("base", "blob", "streak"))
    out = np.tile(base, (len(points), 1))
    out += (blob - out) * soft(fractal(p, 11) - 0.52, 0.006)[:, None]
    stretched = p * np.array([0.9, 0.9, 2.2], dtype=np.float32)    # the streaks run across (sideways), wavy
    line = 0.024 - np.abs(fractal(stretched * 1.3 + 5.0, 23) - 0.5)  # positive near the noise's middle: a thin band
    shown = soft(fractal(p * 0.7 + 9.0, 37) - 0.5, 0.01)           # only in some places (broken streaks)
    out += (streak - out) * (soft(line, 0.004) * shown)[:, None]
    return out / 255.0


def grow(pixels, covered, steps):
    """Grows the painted texels out over their neighbors outside every island, step by step (the islands' padding)."""
    filled = covered.copy()
    for _ in range(steps):
        total = np.zeros_like(pixels)
        count = np.zeros(filled.shape, dtype=np.float32)
        for axis, shift in ((0, 1), (0, -1), (1, 1), (1, -1)):
            total += np.roll(pixels * filled[..., None], shift, axis=axis)
            count += np.roll(filled, shift, axis=axis)
        new = ~filled & (count > 0)
        pixels[new] = total[new] / count[new][:, None]
        filled |= new
    return pixels


def kinds_of(x, y, z, head):
    """Which piece of clothing each place of the rest pose is in (arrays)."""
    ax = np.abs(x)
    on_arm = (ax > 0.2) & (z > 1.3)
    kind = np.full(x.shape, "shirt", dtype=object)
    kind[(ax > 0.13) & (z > 1.30) & (z < 1.45) & (np.abs(y) < 0.06) & ~on_arm] = "armpit"
    kind[(z <= WAIST[1]) & ~on_arm] = "trousers"
    kind[(z <= 0.90) & (ax < 0.055)] = "crotch"
    kind[z < BOOT_TOP] = "boot"
    kind[on_arm] = "sleeve"
    kind[on_arm & (ax > GLOVE_FROM)] = "glove"
    # The balaclava, down to the collar (round the neck there: not the shoulders; the face, higher, all of it)
    kind[(z >= neckline(x, y)) & ((np.hypot(x, y - COLLAR[2]) < 0.14) | (z > 1.6)) & ~on_arm] = "head"
    return kind


def neckline(x, y):
    """The shirt's neckline's height round the neck (a smooth line: higher at the sides)."""
    return COLLAR[0] + COLLAR[1] * np.sin(np.arctan2(x, y - COLLAR[2])) ** 2


def in_eye_slot(x, y, z, grow=0.0):
    """The eyes' slot in the balaclava (its skin shows), grown by this much (meters)."""
    ex = np.abs(x - EYES[0]) / (EYES_HALF[0] + grow)
    ez = (z - EYES[1]) / (EYES_HALF[1] + grow)
    return (y < -0.03) & (z > 1.6 - grow) & ((ex ** 4 + ez ** 4) < 1.0)


def details(x, y, z):
    """The trousers' details: per region, how far inside it each place is (meters to its nearest edge; negative: outside)."""
    ax = np.abs(x)
    lx, ly = ax - LEG_X, y - LEG_Y
    on_leg = (z >= BOOT_TOP) & (z < 0.95) & (ax > 0.03)
    regions = {
        "knee": ([z - 0.44, 0.62 - z, -0.012 - ly, 0.08 - np.abs(lx)], on_leg),
        "calf": ([z - 0.26, 0.37 - z, lx - 0.025, ly + 0.055, 0.035 - ly], on_leg),
        "gusset": ([0.06 - ax, z - 0.74, 0.88 - z, 0.07 - np.abs(y - 0.025)], z < 0.95),
        "yoke": ([y - 0.07, z - 1.00, WAIST[0] - z, 0.2 - ax], z <= WAIST[0]),
    }
    return {name: np.where(mask, np.min(np.stack(limits), axis=0), -1.0) for name, (limits, mask) in regions.items()}


def tailor(body):
    """The clothes' shape: the body's vertices smoothed and pushed out (see OFFSET, HEM, RAISE). The glTF's vertices are
    split along the UVs' seams: they are welded by place for this, so both sides of a seam move alike (no cracks), and the
    normals are given per place too (no hard edges along the seams)."""
    mesh = body.data
    count = len(mesh.vertices)
    positions = np.empty(count * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", positions)
    positions = positions.reshape(-1, 3)
    head = bone_weights(body, {"Head", "neck_01"})

    # One place per position (the seams' copies share it)
    keys = np.round(positions * 1e5).astype(np.int64)
    _, first, place_of = np.unique(keys, axis=0, return_index=True, return_inverse=True)
    place_of = place_of.ravel()
    rest = positions[first].copy()
    x, y, z = rest[:, 0], rest[:, 1], rest[:, 2]
    kinds = kinds_of(x, y, z, head[first])
    regions = details(x, y, z)

    # The places' neighbors, from the mesh's edges
    edges = np.empty(len(mesh.edges) * 2, dtype=np.int64)
    mesh.edges.foreach_get("vertices", edges)
    edges = place_of[edges.reshape(-1, 2)]
    edges = edges[edges[:, 0] != edges[:, 1]]
    edges = np.unique(np.sort(edges, axis=1), axis=0)
    degree = np.bincount(edges.ravel(), minlength=len(rest)).astype(np.float64)
    degree[degree == 0] = 1.0

    def smooth(points, movable, steps):
        """Taubin smoothing (a step in, a slightly larger one out: the bumps go, the size stays) of the movable places."""
        for _ in range(steps):
            for factor in (0.5, -0.53):
                total = np.zeros_like(points)
                np.add.at(total, edges[:, 0], points[edges[:, 1]])
                np.add.at(total, edges[:, 1], points[edges[:, 0]])
                change = total / degree[:, None] - points
                points[movable] += factor * change[movable]
        return points

    def place_normals(points):
        """Area-weighted normals of the places, from the mesh's triangles (all copies of a place together)."""
        mesh.calc_loop_triangles()
        triangles = np.empty(len(mesh.loop_triangles) * 3, dtype=np.int64)
        mesh.loop_triangles.foreach_get("vertices", triangles)
        triangles = place_of[triangles.reshape(-1, 3)]
        a, b, c = points[triangles[:, 0]], points[triangles[:, 1]], points[triangles[:, 2]]
        face = np.cross(b - a, c - a)
        normals = np.zeros_like(points)
        for corner in range(3):
            np.add.at(normals, triangles[:, corner], face)
        return normals / np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)

    places = rest.copy()
    trousers = np.isin(kinds, ["trousers", "crotch"])
    places = smooth(places, np.isin(kinds, ["shirt", "sleeve", "armpit"]), SMOOTH_CLOTH)
    places = smooth(places, trousers, SMOOTH_TROUSERS)
    # The legs and the arms drawn in toward their axis (their middle at each height, or each step along the arm), less
    # toward the hips, the hems, the shoulders and the wrists
    def slim(limb, along, across, slimming):
        factor, low, start, end, high = slimming
        for band in np.arange(low, high, 0.02):
            here = limb & (along >= band) & (along < band + 0.02)
            if not here.any():
                continue
            h = along[here]
            amount = np.clip(np.minimum((h - low) / (start - low), (high - h) / (high - end)), 0.0, 1.0)
            scale = 1.0 - (1.0 - factor) * amount
            for axis in across:
                middle = 0.5 * (places[here, axis].min() + places[here, axis].max())
                places[here, axis] = middle + (places[here, axis] - middle) * scale

    sleeves = kinds == "sleeve"
    for side in (1.0, -1.0):
        slim(trousers & (x * side > 0.0), z, (0, 1), LEG_SLIM)
        slim(sleeves & (x * side > 0.0), np.abs(x), (1, 2), ARM_SLIM)
    boot = kinds == "boot"
    # The balaclava: the ears pressed flat first (smoothed alone: they fold back onto the head round them), then the hood
    # smoothed, the eyes' slot and its rim kept
    hood = kinds == "head"
    slot = in_eye_slot(x, y, z, 0.012)
    ears = hood & (np.abs(x) > EARS[0]) & (z > EARS[1]) & (z < EARS[2]) & (y > -0.02)
    for _ in range(EAR_STEPS):
        total = np.zeros_like(places)
        np.add.at(total, edges[:, 0], places[edges[:, 1]])
        np.add.at(total, edges[:, 1], places[edges[:, 0]])
        places[ears] += 0.5 * (total / degree[:, None] - places)[ears]
    places = smooth(places, hood & ~slot, SMOOTH_HEAD)

    # The hollows filled from outside
    near_armpit = (np.abs(x) > 0.11) & (z > 1.22) & (z < 1.5)
    fillable = (np.isin(kinds, ["shirt", "trousers", "crotch"]) & ~near_armpit) | (hood & ~slot)
    for _ in range(FILL_STEPS):
        normals = place_normals(places)
        total = np.zeros_like(places)
        np.add.at(total, edges[:, 0], places[edges[:, 1]])
        np.add.at(total, edges[:, 1], places[edges[:, 0]])
        along = np.sum((total / degree[:, None] - places) * normals, axis=1)
        places += normals * (0.8 * np.where(fillable & (along > 0.0), along, 0.0))[:, None]

    normals = place_normals(places)
    offset = np.array([OFFSET.get(kind, 0.0) for kind in kinds])
    offset[in_eye_slot(x, y, z)] = 0.0  # the slot's skin: the knit's thickness is its rim
    offset[trousers] += THIGH_EXTRA[0] * np.clip((z[trousers] - THIGH_EXTRA[1]) / (THIGH_EXTRA[2] - THIGH_EXTRA[1]), 0.0, 1.0)
    # The hems over the boots: wider at once, back to the fit higher up (the cuff, a ring of its own, covers the join)
    hem = trousers & (z < BOOT_TOP + HEM[1])
    t = (z[hem] - BOOT_TOP) / HEM[1]
    offset[hem] = np.maximum(offset[hem], OFFSET["trousers"] + (HEM[0] - OFFSET["trousers"]) * (1.0 - t) ** 2)
    # The details: raised over their edge's ramp, the knee pads' grooves pressed in
    for name, inside in regions.items():
        if name in RAISE:
            offset += np.where(trousers, RAISE[name] * np.clip(inside / RAMP[name], 0.0, 1.0), 0.0)
    groove = (regions["knee"] > 0) & (np.min(np.abs(z[:, None] - np.array(KNEE_RIBS)[None, :]), axis=1) < GROOVE[1])
    offset[groove & trousers] -= GROOVE[0]
    places += normals * offset[:, None]

    mesh.vertices.foreach_set("co", places[place_of].ravel().astype(np.float32))
    mesh.update()
    normals = place_normals(places)  # the import's normals are stale now: per place, as custom normals
    mesh.normals_split_custom_set_from_vertices([tuple(n) for n in normals[place_of]])
    moved = np.linalg.norm(places - rest, axis=1)
    print(f"Tailored: {len(places)} places ({count} vertices), moved up to {moved.max() * 100:.1f} cm; "
          f"{int(trousers.sum())} trousers, {int(boot.sum())} boots")


def add_belt(body, scheme_name, scheme):
    """The belt, its buckle and its loops around the tailored waist: meshes of their own (plain materials), parented to the
    armature and weighted as the nearest body vertex."""
    from mathutils import kdtree
    mesh = body.data
    points = np.array([tuple(v.co) for v in mesh.vertices])
    bins = 96

    def radii(ring, center_x, center_y, gap):
        """How far the ring of points reaches out from its middle, per step round it (no dents), plus a gap."""
        angle = np.arctan2(ring[:, 1] - center_y, ring[:, 0] - center_x)
        radius = np.hypot(ring[:, 0] - center_x, ring[:, 1] - center_y)
        index = ((angle + np.pi) / (2 * np.pi) * bins).astype(int) % bins
        outer = np.full(bins, np.nan)
        for i in range(bins):
            if (index == i).any():
                outer[i] = radius[index == i].max()
        known = ~np.isnan(outer)
        outer = np.interp(np.arange(bins), np.arange(bins)[known], outer[known], period=bins)
        outer = np.maximum(outer, np.convolve(np.r_[outer[-2:], outer, outer[:2]], np.ones(5) / 5, "valid"))
        return outer + gap

    def tube(outer, center_x, center_y, low, high, thickness, verts, faces):
        """A band round a middle: per step four corners (outside bottom and top, inside top and bottom), joined in quads."""
        start = len(verts)
        low, high = np.broadcast_to(low, (bins,)), np.broadcast_to(high, (bins,))  # heights: one, or one per step
        for i in range(bins):
            a = (i + 0.5) / bins * 2 * np.pi - np.pi
            d = np.array([np.cos(a), np.sin(a)])
            for r, h in ((outer[i] + thickness, low[i]), (outer[i] + thickness, high[i]), (outer[i], high[i]),
                         (outer[i], low[i])):
                verts.append((center_x + d[0] * r, center_y + d[1] * r, h))
        for i in range(bins):
            j = (i + 1) % bins
            for k in range(4):
                faces.append((start + i * 4 + k, start + j * 4 + k, start + j * 4 + (k + 1) % 4, start + i * 4 + (k + 1) % 4))

    band = (points[:, 2] > BELT[0]) & (points[:, 2] < BELT[1]) & (np.abs(points[:, 0]) < 0.3)
    ring = points[band]
    center_y = 0.5 * (ring[:, 1].min() + ring[:, 1].max())
    outer = radii(ring, 0.0, center_y, BELT_GAP)
    under = np.hypot(ring[:, 0], ring[:, 1] - center_y)
    index = ((np.arctan2(ring[:, 1] - center_y, ring[:, 0]) + np.pi) / (2 * np.pi) * bins).astype(int) % bins
    print(f"Belt: off the cloth by {np.mean(outer[index] - under) * 1000:.1f} mm on average")

    tree = kdtree.KDTree(len(points))
    for i, p in enumerate(points):
        tree.insert(p, i)
    tree.balance()
    groups = {g.index: g.name for g in body.vertex_groups}
    armature = body.find_armature()

    def make(name, verts, faces, color, metallic, roughness, smooth):
        data = bpy.data.meshes.new(name)
        data.from_pydata([tuple(v) for v in verts], [], faces)
        data.update()
        plain_uvs(data)
        item = bpy.data.objects.new(name, data)
        bpy.context.scene.collection.objects.link(item)
        item.parent = armature
        item.modifiers.new("Armature", "ARMATURE").object = armature
        for v in data.vertices:  # the nearest body vertex's weights
            _, nearest, _ = tree.find(v.co)
            for g in (g for g in mesh.vertices[nearest].groups if g.weight > 0.001):  # no (nearly) empty weights: Assimp warns
                group = item.vertex_groups.get(groups[g.group]) or item.vertex_groups.new(name=groups[g.group])
                group.add([v.index], g.weight, "REPLACE")
        data.materials.append(plain_material(f"MI_Soldier_{scheme_name}_{name}", color, metallic, roughness))
        for polygon in data.polygons:
            polygon.use_smooth = smooth
        return item

    def at(degrees):
        """The belt's outside at an angle round the waist from the front: its place (x, y) and direction."""
        a = math.radians(-90.0 + degrees)
        i = int(((a + np.pi) / (2 * np.pi)) * bins) % bins
        r = outer[i] + BELT_THICKNESS
        return np.array([math.cos(a), math.sin(a)]), r

    verts, faces = [], []
    tube(outer, 0.0, center_y, BELT[0], BELT[1], BELT_THICKNESS, verts, faces)
    make("Belt", verts, faces, scheme["belt"], 0.0, 0.8, True)

    # The hems' cuffs round the boots' tops: they cover the join of the trousers and the boots and stand over the boots
    verts, faces = [], []
    low, high = BOOT_TOP + HEM_CUFF[0], BOOT_TOP + HEM_CUFF[1]
    for side in (1.0, -1.0):
        leg = points[(points[:, 2] > low - 0.005) & (points[:, 2] < high + 0.005) & (points[:, 0] * side > 0.0)]
        cx, cy = 0.5 * (leg[:, 0].min() + leg[:, 0].max()), 0.5 * (leg[:, 1].min() + leg[:, 1].max())
        tube(radii(leg, cx, cy, HEM_CUFF[3]), cx, cy, low, high, HEM_CUFF[2], verts, faces)
    make("Hems", verts, faces, scheme["panel"], 0.0, 0.85, True)

    # The shirt's collar round the neck's base, over the balaclava's bottom (tucked in under it)
    verts, faces = [], []
    line = neckline(points[:, 0], points[:, 1])
    near = ((points[:, 2] > line - COLLAR_BAND[3]) & (points[:, 2] < line - COLLAR_BAND[3] + COLLAR_BAND[0])
            & (np.hypot(points[:, 0], points[:, 1] - COLLAR[2]) < 0.14))
    angles = (np.arange(bins) + 0.5) / bins * 2 * np.pi - np.pi
    low = COLLAR[0] + COLLAR[1] * np.cos(angles) ** 2 - COLLAR_BAND[3]  # the tube's angle runs from +x: the sides at 0, pi
    collar = radii(points[near], 0.0, COLLAR[2], 0.0)  # only the cloth it covers (not the shoulders' slope under it)
    for _ in range(2):  # a smooth round edge
        collar = np.convolve(np.r_[collar[-3:], collar, collar[:3]], np.ones(7) / 7, "valid")
    tube(collar + COLLAR_BAND[2], 0.0, COLLAR[2], low, low + COLLAR_BAND[0], COLLAR_BAND[1], verts, faces)
    make("Collar", verts, faces, scheme["shirt"], 0.0, 0.9, True)

    box = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]

    def block(degrees, width, depth, low, high, inset):
        """A box on the belt's outside at an angle: width along the belt, depth out from it."""
        d, r = at(degrees)
        side = np.array([-d[1], d[0]])
        corners = []
        for s in (-1, 1):
            for out in (0, 1):
                for h in (low, high):
                    p = d * (r - inset + out * depth) + side * s * width / 2
                    corners.append((p[0], center_y + p[1], h))
        return corners

    # The buckle at the front, the loops round the waist (over the belt, from the trousers)
    zc = 0.5 * (BELT[0] + BELT[1])
    w, d, h = BUCKLE_SIZE
    make("Buckle", block(0, w, d, zc - h / 2, zc + h / 2, 0.001), box, scheme["buckle"], 0.9, 0.35, False)
    verts, faces = [], []
    for degrees in LOOPS:
        faces += [tuple(len(verts) + i for i in face) for face in box]
        verts += block(degrees, LOOP_SIZE[0], LOOP_SIZE[1], BELT[0] - LOOP_SIZE[2], BELT[1] + LOOP_SIZE[2] * 0.4, BELT_THICKNESS + 0.001)
    make("Loops", verts, faces, scheme["loops"], 0.0, 0.85, False)
    print(f"Belt: {bins} steps round the waist, {outer.min() * 100:.1f} to {outer.max() * 100:.1f} cm from its middle, "
          f"{len(LOOPS)} loops")


def add_pockets(body):
    """The thighs' cargo pockets (POCKET_*, FLAP): on each leg a patch over the tailored cloth, round the leg's middle at
    each height, standing out by a bellows' profile, and its flap over the top: a slab a little wider, its lower edge
    folded in onto the pocket, its sides and top down to the cloth. Their UVs are the cloth's under them (the nearest
    triangle's, interpolated: the thighs' outsides have no UV seams), so the camouflage runs on over them; their weights
    are the nearest body vertex's."""
    from mathutils import Vector, bvhtree, geometry, kdtree
    mesh = body.data
    points = np.array([tuple(v.co) for v in mesh.vertices])
    mesh.calc_loop_triangles()
    triangles = [tuple(t.vertices) for t in mesh.loop_triangles]
    uv_data = mesh.uv_layers.active.data
    triangle_uvs = [[Vector((*uv_data[l].uv, 0.0)) for l in t.loops] for t in mesh.loop_triangles]
    bvh = bvhtree.BVHTree.FromPolygons([tuple(p) for p in points], triangles)
    tree = kdtree.KDTree(len(points))
    for i, p in enumerate(points):
        tree.insert(p, i)
    tree.balance()

    columns, rows = POCKET_GRID
    heights = np.linspace(POCKET_HEIGHTS[0], POCKET_HEIGHTS[1], rows + 1)
    height = heights[-1] - heights[0]
    back, front = math.radians(POCKET_ANGLES[0]), math.radians(POCKET_ANGLES[1])
    verts, faces = [], []

    def grid(rows_of_points, flip):
        start = len(verts)
        width = len(rows_of_points[0])
        for row in rows_of_points:
            verts.extend(row[::-1] if flip else row)  # the faces keep facing out on the mirrored leg
        for j in range(len(rows_of_points) - 1):
            for i in range(width - 1):
                a = start + j * width + i
                faces.append((a, a + width, a + width + 1, a + 1))  # facing out of the leg

    for side in (1.0, -1.0):
        outside = 0.0 if side > 0 else math.pi
        turn = -side  # the angles' front (negative) toward -y on both legs

        def cloth(z):
            """The cloth round the leg at a height: the thigh bone's place there (straight up at LEG_X, LEG_Y) and a
            function from an angle to how far out the cloth is (a ray out to it)."""
            middle = np.array([side * LEG_X, LEG_Y])

            def reach(angle):
                direction = Vector((math.cos(angle), math.sin(angle), 0.0))
                hit, _, _, distance = bvh.ray_cast(Vector((middle[0], middle[1], z)), direction, 0.4)
                if hit is None:
                    raise RuntimeError(f"no cloth round the thigh at {z:.3f} m, {math.degrees(angle):.0f} degrees")
                return distance
            return middle, reach

        def at(middle, angle, radius, z):
            return (middle[0] + math.cos(angle) * radius, middle[1] + math.sin(angle) * radius, z)

        def angle_of(u):
            return outside + turn * (back + (front - back) * u)

        # The pocket: per height and step across, the cloth's reach plus the bellows (smoothly up from its edges)
        pocket, bulged = [], {}
        for j, z in enumerate(heights):
            middle, reach = cloth(z)
            row = []
            for i in range(columns + 1):
                u = i / columns
                radius = reach(angle_of(u))
                arc = radius * (front - back)
                edge = min(u * arc, (1.0 - u) * arc, z - heights[0], heights[-1] - z)
                t = min(max(edge / POCKET_ROUND, 0.0), 1.0)
                r = radius - POCKET_SINK + POCKET_DEPTH * t * t * (3.0 - 2.0 * t)
                bulged[j, i] = (middle, radius, r)
                row.append(at(middle, angle_of(u), r, z))
            pocket.append(row)
        grid(pocket, side < 0)

        # The flap: from its lower edge (folded in onto the pocket) up over its top; each row a step out at each side
        # (FLAP[1] past the pocket), across over the pocket by FLAP[2], and back in to the cloth
        first = next(j for j, z in enumerate(heights) if z >= heights[-1] - FLAP[0] - 1e-6)
        slab = []
        for j in [first] + list(range(first, rows + 1)) + [rows]:
            row = []
            index = len(slab)
            fold, top = index == 0, index == rows - first + 2
            z = heights[j] - (0.004 if fold else 0.0) + (0.004 if top else 0.0)
            ends = []
            for i in range(columns + 1):
                middle, radius, r = bulged[j, i]
                spread = FLAP[1] / radius if i in (0, columns) else 0.0
                angle = angle_of(i / columns) + turn * (-spread if i == 0 else spread) * (1.0 if not fold else 0.0)
                if top:
                    radius_here = radius - POCKET_SINK
                elif fold:
                    radius_here = r - 0.003
                else:
                    radius_here = r + FLAP[2]
                row.append(at(middle, angle, radius_here, z))
                if i in (0, columns):
                    ends.append(at(middle, angle, radius - POCKET_SINK, z))
            slab.append([ends[0]] + row + [ends[1]])
        grid(slab, side < 0)

    data = bpy.data.meshes.new("Pockets")
    data.from_pydata([tuple(v) for v in verts], [], faces)
    data.update()
    layer = data.uv_layers.new(name="UVMap")
    for loop in data.loops:  # the cloth's UVs under each corner
        location, _, index, _ = bvh.find_nearest(Vector(verts[loop.vertex_index]))
        a, b, c = (Vector(points[k]) for k in triangles[index])
        uv = geometry.barycentric_transform(location, a, b, c, *triangle_uvs[index])
        layer.data[loop.index].uv = (uv.x, uv.y)
    item = bpy.data.objects.new("Pockets", data)
    bpy.context.scene.collection.objects.link(item)
    armature = body.find_armature()
    item.parent = armature
    item.modifiers.new("Armature", "ARMATURE").object = armature
    groups = {g.index: g.name for g in body.vertex_groups}
    for v in data.vertices:  # the nearest body vertex's weights
        _, nearest, _ = tree.find(v.co)
        for g in (g for g in mesh.vertices[nearest].groups if g.weight > 0.001):  # no (nearly) empty weights: Assimp warns
            group = item.vertex_groups.get(groups[g.group]) or item.vertex_groups.new(name=groups[g.group])
            group.add([v.index], g.weight, "REPLACE")
    data.materials.append(body.material_slots[0].material)
    for polygon in data.polygons:
        polygon.use_smooth = True
    print(f"Pockets: {len(faces)} faces for both thighs")


def plain_uvs(data):
    """UVs for a mesh of plain materials (none are read: the importer's tangents need some), projected from the side."""
    layer = data.uv_layers.new(name="UVMap")
    for loop in data.loops:
        co = data.vertices[loop.vertex_index].co
        layer.data[loop.index].uv = (co.x * 4.0 + co.y * 4.0, co.z * 4.0)


def plain_material(name, color, metallic, roughness):
    """A material of one color (sRGB 0-255), no maps."""
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    shader = material.node_tree.nodes.get("Principled BSDF")
    linear = [((c / 255.0 + 0.055) / 1.055) ** 2.4 for c in color]
    shader.inputs["Base Color"].default_value = (*linear, 1.0)
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    return material


def add_boots(body, scheme_name, scheme):
    """The boots (see BOOT_VOXEL): built round each foot, weighted as it, and the bare foot inside removed."""
    import bmesh
    from mathutils import kdtree
    mesh = body.data
    armature = body.find_armature()
    points = np.array([tuple(v.co) for v in mesh.vertices])
    groups = {g.index: g.name for g in body.vertex_groups}
    near = np.flatnonzero(points[:, 2] < BOOT_TOP + 0.03)
    tree = kdtree.KDTree(len(near))
    for i in near:
        tree.insert(points[i], int(i))
    tree.balance()
    materials = [plain_material(f"MI_Soldier_{scheme_name}_Boots", scheme["boots"], 0.0, 0.75),
                 plain_material(f"MI_Soldier_{scheme_name}_Soles", scheme["soles"], 0.0, 0.85),
                 plain_material(f"MI_Soldier_{scheme_name}_Laces", scheme["laces"], 0.0, 0.85)]
    triangles = 0
    for side, name in ((1.0, "Boot_L"), (-1.0, "Boot_R")):
        foot = np.unique(np.round(points[(points[:, 2] < BOOT_TOP + 0.004) & (points[:, 0] * side > 0.0)], 5), axis=0)
        cx, cy = 0.5 * (foot[:, 0].min() + foot[:, 0].max()), 0.5 * (foot[:, 1].min() + foot[:, 1].max())
        # The hull, then remeshed, rounded and pushed out (the modifiers, applied by evaluating them)
        hull = bmesh.new()
        for p in foot:
            hull.verts.new(tuple(p))
        result = bmesh.ops.convex_hull(hull, input=hull.verts[:])
        loose = {v for v in result["geom_interior"] + result["geom_unused"] if isinstance(v, bmesh.types.BMVert)}
        bmesh.ops.delete(hull, geom=list(loose), context="VERTS")
        data = bpy.data.meshes.new(name + "_Hull")
        hull.to_mesh(data)
        hull.free()
        shape = bpy.data.objects.new(name + "_Hull", data)
        bpy.context.scene.collection.objects.link(shape)
        remesh = shape.modifiers.new("Remesh", "REMESH")
        remesh.mode, remesh.voxel_size = "VOXEL", BOOT_VOXEL
        rounding = shape.modifiers.new("Round", "SMOOTH")
        rounding.factor, rounding.iterations = BOOT_ROUND
        out = shape.modifiers.new("Out", "DISPLACE")
        out.mid_level, out.strength = 0.0, BOOT_OUT
        evaluated = shape.evaluated_get(bpy.context.evaluated_depsgraph_get())
        boot_mesh = bpy.data.meshes.new_from_object(evaluated)
        bpy.data.objects.remove(shape, do_unlink=True)
        bpy.data.meshes.remove(data)

        # Its top open under the hem's cuff, the foot on the ground, the sole's lip
        shaped = bmesh.new()
        shaped.from_mesh(boot_mesh)
        bmesh.ops.delete(shaped, geom=[v for v in shaped.verts if v.co.z > BOOT_TOP + 0.004], context="VERTS")
        for v in shaped.verts:
            v.co.z = max(v.co.z, 0.0)
            if v.co.z < SOLE_TOP:
                d = np.array([v.co.x - cx, v.co.y - cy])
                d /= max(np.linalg.norm(d), 1e-6)
                lip = SOLE_LIP * (1.0 - v.co.z / SOLE_TOP) ** 0.5
                v.co.x += d[0] * lip
                v.co.y += d[1] * lip
        shaped.to_mesh(boot_mesh)
        shaped.free()
        for layer in list(boot_mesh.uv_layers):
            boot_mesh.uv_layers.remove(layer)
        plain_uvs(boot_mesh)
        for material in materials:
            boot_mesh.materials.append(material)
        for polygon in boot_mesh.polygons:
            c, n = polygon.center, polygon.normal
            laces = n.z > 0.35 and c.z > 0.05 and -0.09 < c.y < 0.0 and abs(c.x - cx) < 0.022
            polygon.material_index = 1 if c.z < SOLE_TOP else (2 if laces else 0)
            polygon.use_smooth = True

        item = bpy.data.objects.new(name, boot_mesh)
        bpy.context.scene.collection.objects.link(item)
        item.parent = armature
        item.modifiers.new("Armature", "ARMATURE").object = armature
        for v in boot_mesh.vertices:  # the nearest foot vertex's weights
            _, nearest, _ = tree.find(v.co)
            for g in (g for g in mesh.vertices[nearest].groups if g.weight > 0.001):  # no (nearly) empty weights: Assimp warns
                group = item.vertex_groups.get(groups[g.group]) or item.vertex_groups.new(name=groups[g.group])
                group.add([v.index], g.weight, "REPLACE")
        triangles += sum(len(p.vertices) - 2 for p in boot_mesh.polygons)

    # The bare feet inside go
    feet = bmesh.new()
    feet.from_mesh(mesh)
    hidden = [f for f in feet.faces if all(v.co.z < BOOT_TOP - 0.012 for v in f.verts)]
    bmesh.ops.delete(feet, geom=hidden, context="FACES_ONLY")
    bmesh.ops.delete(feet, geom=[v for v in feet.verts if not v.link_faces], context="VERTS")
    feet.to_mesh(mesh)
    feet.free()
    print(f"Boots: {triangles} triangles for both, {len(hidden)} faces of the feet removed")


def paint(color, normal, rough, covered, place, head, scheme):
    """The uniform's colors, roughness and flattened normals into the maps (in place)."""
    size = color.shape[0]
    x, y, z = place[..., 0], place[..., 1], place[..., 2]
    ax = np.abs(x)
    lx, ly = ax - LEG_X, y - LEG_Y
    kind = kinds_of(x, y, z, head)
    regions = details(x, y, z)

    # The parts: the trousers' camouflage and panels, the shirt, the boots (soles, laces), the gloves, the balaclava
    part = np.full((size, size), "", dtype=object)
    trousers = covered & np.isin(kind, ["trousers", "crotch"])
    part[covered & np.isin(kind, ["shirt", "sleeve", "armpit"])] = "shirt"
    part[trousers] = "camo"  # below the waist only (the user): the shirt's sleeves are the shirt's
    for name in ("knee", "gusset", "yoke"):
        part[trousers & (regions[name] > 0)] = "panel"
    part[covered & (z >= WAIST[0]) & (z <= WAIST[1]) & (kind == "trousers")] = "belt"  # under the belt's mesh
    boots = covered & (kind == "boot")
    part[boots] = "boots"
    laces = boots & (np.abs(lx) < 0.018) & (z > 0.05) & (y > -0.085) & (y < 0.0) & ((z <= 0.085) | (ly < -0.03))
    rows = np.abs(((z + np.abs(lx) * 0.6) / 0.010) % 1.0 - 0.5) < 0.18  # crossed laces
    part[laces & rows] = "laces"
    part[boots & (z < SOLE_TOP)] = "soles"
    part[covered & (kind == "glove")] = "gloves"
    hood = covered & (kind == "head")
    part[hood] = "balaclava"
    part[covered & in_eye_slot(x, y, z)] = ""  # the eyes' slot: the skin stays

    # The skin's light and shade, relative to its average, softened: the creases stay under the cloth
    brightness = color[..., :3].mean(axis=2)
    shade = np.clip(brightness / max(float(brightness[covered].mean()), 1e-6), 0.5, 1.4)
    shade = 1.0 + SHADE * (shade - 1.0)

    # The seams: the details' edges, the pockets' zips and flaps, the knee pads' grooves, the fly, the hems' fold, the
    # boots' midsole line
    seams = np.ones((size, size), dtype=np.float32)
    for name, inside in regions.items():
        seams[trousers & (inside > 0) & (inside < SEAM)] = 0.62
    for name, below in FLAPS.items():
        top = {"calf": 0.37}[name]
        seams[trousers & (regions[name] > 0) & (np.abs(z - (top - below)) < SEAM * 0.6)] = 0.5
    ribs = np.min(np.abs(z[..., None] - np.array(KNEE_RIBS)), axis=-1)
    seams[trousers & (regions["knee"] > 0) & (ribs < GROOVE[1] * 0.6)] = 0.7
    seams[trousers & (ax < SEAM * 0.6) & (y < 0.0) & (z > FLY[0]) & (z < FLY[1])] = 0.62
    seams[boots & (np.abs(z - SOLE_TOP) < 0.003)] = 1.25
    # The balaclava's knit: ribs up and down round the head, a seam up the back's middle
    knit = 0.93 + 0.07 * np.cos(np.arctan2(x, y - 0.017) * RIBS)
    seams[hood] *= knit[hood]
    seams[hood & (ax < SEAM * 0.5) & (y > 0.0)] = 0.72
    shade = shade * seams

    flat = np.array([0.5, 0.5, 1.0], dtype=np.float32)
    weave = 0.96 + 0.08 * value_noise(place[covered] * 180.0, 5)  # the cloth's fine grain
    grain = np.ones((size, size), dtype=np.float32)
    grain[covered] = weave
    source_normal = normal.copy()
    for name in ("camo", "panel", "shirt", "belt", "boots", "soles", "laces", "gloves", "balaclava"):
        mask = part == name
        if not mask.any():
            continue
        if name == "camo":
            base = camouflage(place[mask], scheme["camo"])
        else:
            base = np.tile(np.array(scheme[name], dtype=np.float32) / 255.0, (int(mask.sum()), 1))
        if name == "gloves":
            base[(ax[mask] - GLOVE_FROM) < CUFF] *= 0.8
        color[mask, :3] = base * (shade[mask] * grain[mask])[:, None]
        rough[mask, 0] = rough[mask, 1] = ROUGHNESS[name]  # G (glTF's packing), R too (read when not packed)
        rough[mask, 2] = 0.0  # no metal
        keep = RELIEF.get(name, 0.3)
        n = source_normal[mask, :3] * 2.0 - 1.0
        n = keep * n + (1.0 - keep) * (flat * 2.0 - 1.0)
        n /= np.linalg.norm(n, axis=1, keepdims=True)
        normal[mask, :3] = n * 0.5 + 0.5

    # Grow every island out a little over the background (the original background stays further out)
    dressed = part != ""
    ring = ~covered & (grow(dressed[..., None].astype(np.float32).repeat(4, axis=2), covered, PADDING)[..., 0] > 0.5)
    for pixels in (color, normal, rough):
        grown = grow(pixels.copy(), covered, PADDING)
        pixels[ring] = grown[ring]
    counts = {name: int((part == name).sum()) for name in ("camo", "panel", "shirt", "boots", "laces", "soles", "gloves", "balaclava")}
    print(f"Painted texels {counts}, skin left {int((covered & (part == '')).sum())} (the eyes' slot)")


def export_with_materials(path, source_gltf):
    """Exports the scene's character as glTF (.gltf and .bin) without images, then gives each material the definition the
    source glTF has for it (its maps); materials it doesn't have (the belt's) keep their plain values."""
    for item in list(bpy.data.objects):
        if item.type == "MESH" and item.parent is None:
            bpy.data.objects.remove(item, do_unlink=True)  # the importer's bone shape
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLTF_SEPARATE", export_animations=False, export_skins=True,
                              export_yup=True, export_image_format="NONE")
    with open(path, "r", encoding="utf-8") as file:
        out = json.load(file)
    sources = {m["name"]: m for m in source_gltf["materials"]}
    out["images"], out["textures"] = [], []
    out["samplers"] = source_gltf.get("samplers", [{}])
    image_map = {}

    def texture_index(source_index):
        texture = source_gltf["textures"][source_index]
        image = texture["source"]
        if image not in image_map:
            image_map[image] = len(out["images"])
            out["images"].append(source_gltf["images"][image])
        out["textures"].append({"sampler": texture.get("sampler", 0), "source": image_map[image]})
        return len(out["textures"]) - 1

    for i, material in enumerate(out.get("materials", [])):
        # By its name, or without the suffix Blender gives a copy (a material the importer split: "MI_Soldier_Player.001")
        name = material["name"]
        source = sources.get(name) or sources.get(name.rsplit(".", 1)[0] if name[-4:-3] == "." and name[-3:].isdigit() else name)
        if source is None:
            for key in ("normalTexture", "occlusionTexture", "emissiveTexture"):
                material.pop(key, None)
            continue
        copy = json.loads(json.dumps(source))
        pbr = copy.get("pbrMetallicRoughness", {})
        for holder in [pbr.get("baseColorTexture"), pbr.get("metallicRoughnessTexture"), copy.get("normalTexture"),
                       copy.get("occlusionTexture"), copy.get("emissiveTexture")]:
            if holder is not None:
                holder["index"] = texture_index(holder["index"])
        out["materials"][i] = copy
    for key in ("images", "textures"):
        if not out[key]:
            out.pop(key)
    with open(path, "w", encoding="utf-8", newline="\n") as file:
        json.dump(out, file, indent=1)
    triangles = sum(len(p.vertices) - 2 for o in bpy.data.objects if o.type == "MESH" for p in o.data.polygons)
    print(f"Wrote {os.path.relpath(path, ROOT)} ({triangles} triangles)")
    return out


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    scheme_name = argv[argv.index("--scheme") + 1] if "--scheme" in argv else "Player"
    preview = argv[argv.index("--preview") + 1] if "--preview" in argv else None
    scheme = SCHEMES[scheme_name]

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=SOURCE)
    body = bpy.data.objects[BODY_MESH]
    if not body.matrix_world.is_identity:
        raise RuntimeError("the body's mesh is expected at the armature's origin, unturned")

    with open(SOURCE, "r", encoding="utf-8") as file:
        gltf = json.load(file)
    material = next(m for m in gltf["materials"] if m["name"] == BODY_MATERIAL)
    pbr = material["pbrMetallicRoughness"]
    uri = lambda texture: gltf["images"][gltf["textures"][texture["index"]]["source"]]["uri"]
    color = load_pixels(os.path.join(FOLDER, uri(pbr["baseColorTexture"])))
    normal = load_pixels(os.path.join(FOLDER, uri(material["normalTexture"])))
    rough = load_pixels(os.path.join(FOLDER, uri(pbr["metallicRoughnessTexture"])))
    if not (color.shape == normal.shape == rough.shape):
        raise RuntimeError("the body's maps differ in size")

    # The paint's places from the bare body, then the clothes' shape (the same regions), then the belt
    covered, place, head = rasterize(body, color.shape[0])
    tailor(body)
    add_belt(body, scheme_name, scheme)
    add_boots(body, scheme_name, scheme)
    add_pockets(body)
    paint(color, normal, rough, covered, place, head, scheme)

    names = {kind: f"T_Soldier_{scheme_name}_{kind}.png" for kind in ("Color", "Normal", "Roughness")}
    save_pixels(color, os.path.join(FOLDER, names["Color"]))
    save_pixels(normal, os.path.join(FOLDER, names["Normal"]))
    save_pixels(rough, os.path.join(FOLDER, names["Roughness"]))

    # The materials as the glTF gives them, the body's maps ours
    for texture, kind in ((pbr["baseColorTexture"], "Color"), (material["normalTexture"], "Normal"),
                          (pbr["metallicRoughnessTexture"], "Roughness")):
        gltf["images"].append({"mimeType": "image/png", "name": names[kind][:-4], "uri": names[kind]})
        gltf["textures"].append({"sampler": gltf["textures"][texture["index"]]["sampler"], "source": len(gltf["images"]) - 1})
        texture["index"] = len(gltf["textures"]) - 1
    material["name"] = f"MI_Soldier_{scheme_name}"
    body.material_slots[0].material.name = material["name"]
    output = os.path.join(FOLDER, f"Soldier_{scheme_name}.gltf")
    soldier = export_with_materials(output, gltf)

    write_arms(output, soldier, scheme_name)
    if preview:
        render_preview(output, preview)


def write_arms(body_path, body_gltf, scheme_name):
    """The first person's arms (the view model, Shooter E2): the dressed body cut down to what the arms' bones move, on
    the same rig (the same bones and rest pose: the body's clips play on it) and with the same maps."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=body_path)
    body = bpy.data.objects[BODY_MESH]
    for item in list(bpy.data.objects):
        if item.type == "MESH" and item != body:
            bpy.data.objects.remove(item, do_unlink=True)  # the eyes, the brows, the belt, the importer's bone shape
    arm_bones = {bone.name for bone in body.find_armature().data.bones
                 if any(part in bone.name for part in ARM_PARTS) and bone.name.endswith(("_l", "_r"))}
    weights = bone_weights(body, arm_bones)
    import bmesh
    mesh = bmesh.new()
    mesh.from_mesh(body.data)
    mesh.verts.ensure_lookup_table()
    bmesh.ops.delete(mesh, geom=[v for v in mesh.verts if weights[v.index] < ARMS_FROM], context="VERTS")
    mesh.to_mesh(body.data)
    mesh.free()
    export_with_materials(os.path.join(FOLDER, f"Soldier_{scheme_name}_Arms.gltf"), body_gltf)


def find_action(name):
    """An imported clip by its name (the importer may add a suffix)."""
    for action in bpy.data.actions:
        if action.name == name or action.name.startswith(name + "_") or action.name.startswith(name + "."):
            return action
    return None


def render_preview(path, folder):
    """The dressed body in the rest pose and in the poses where it bends most, from the front and the side, and close
    ups of the boots, the knees, the belt and the balaclava: one sheet (Workbench, the maps' colors)."""
    os.makedirs(folder, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=path)
    ours = set(bpy.data.objects)
    armature = next(o for o in ours if o.type == "ARMATURE")
    for item in ours:
        if item.type == "MESH" and item.parent is None:
            item.hide_render = True  # the importer's bone shape
    for file in POSE_FILES:
        bpy.ops.import_scene.gltf(filepath=file)
    for item in bpy.data.objects:
        if item not in ours:
            item.hide_render = True
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    shading = scene.display.shading
    shading.light = "STUDIO"
    shading.color_type = "TEXTURE"
    shading.show_specular_highlight = False
    shading.background_type = "VIEWPORT"
    shading.background_color = (0.42, 0.45, 0.5)
    scene.render.resolution_x, scene.render.resolution_y = 520, 520
    camera_data = bpy.data.cameras.new("Preview")
    camera_data.type = "ORTHO"
    camera = bpy.data.objects.new("Preview", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    if armature.animation_data is None:
        armature.animation_data_create()

    def pose(clip, at):
        data = armature.animation_data
        if clip is None:
            data.action = None
            for bone in armature.pose.bones:
                bone.matrix_basis.identity()
            scene.frame_set(0)
            return
        action = find_action(clip)
        if action is None:
            raise RuntimeError(f"no clip {clip}")
        data.action = action
        if getattr(action, "slots", None) and len(action.slots):
            data.action_slot = action.slots[0]
        start, end = action.frame_range
        scene.frame_set(int(round(start + at * (end - start))))

    front = (math.radians(90), 0, 0)
    side = (math.radians(90), 0, -math.pi / 2)
    back = (math.radians(90), 0, math.pi)
    three = (math.radians(80), 0, -math.radians(35))
    behind = (math.radians(90), 0, -math.radians(145))
    level = (math.radians(90), 0, -math.radians(35))
    shots = [
        ("rest", None, 0, (0, -4, 0.62), front, 1.35), ("rest", None, 0, (0, 4, 0.62), back, 1.35),
        ("rest", None, 0, (-4, 0, 0.62), side, 1.35),
        ("walk", "Walk_Fast_Loop", 0.25, (-4, 0, 0.62), side, 1.35), ("sprint", "Sprint_Loop", 0.3, (-4, 0, 0.62), side, 1.35),
        ("crouch", "Crouch_Idle_Loop", 0.0, (-4, 0, 0.5), side, 1.2), ("crouch", "Crouch_Idle_Loop", 0.0, (0, -4, 0.5), front, 1.2),
        ("aim", "Rifle_Aim_Neutral", 0.0, (-4, 0, 0.62), side, 1.35),
        ("boots", None, 0, (-2.3, -3.3, 0.12), three, 0.45), ("knees", None, 0, (-2.3, -3.3, 0.6), three, 0.5),
        ("belt", None, 0, (0, -4, 1.05), front, 0.55), ("belt back", None, 0, (0, 4, 1.0), back, 0.6),
        ("head", None, 0, (0, -4, 1.62), front, 0.4), ("head back", None, 0, (0, 4, 1.62), back, 0.4),
        ("head side", None, 0, (-4, 0, 1.62), side, 0.4), ("head behind", None, 0, (-2.3, 3.3, 1.62), behind, 0.4),
        ("head front", None, 0, (-2.3, -3.3, 1.62), level, 0.4),
        ("head aim", "Rifle_Aim_Neutral", 0.0, (-2.3, 3.3, 1.55), behind, 0.5),
    ]
    tiles = []
    for index, (name, clip, at, location, rotation, scale) in enumerate(shots):
        pose(clip, at)
        camera.location, camera.rotation_euler = location, rotation
        camera_data.ortho_scale = scale
        scene.render.filepath = os.path.join(folder, f"{index:02d}_{name}.png")
        bpy.ops.render.render(write_still=True)
        image = bpy.data.images.load(scene.render.filepath)
        pixels = np.empty(image.size[0] * image.size[1] * 4, dtype=np.float32)
        image.pixels.foreach_get(pixels)
        tiles.append(pixels.reshape(image.size[1], image.size[0], 4))
    columns = 6
    rows = [np.concatenate(tiles[r * columns:(r + 1) * columns], axis=1) for r in range(len(tiles) // columns)]
    sheet = np.concatenate(rows[::-1], axis=0)  # Blender's rows run bottom up: the first row on top
    image = bpy.data.images.new("Sheet", sheet.shape[1], sheet.shape[0])
    image.pixels.foreach_set(sheet.ravel())
    image.filepath_raw = os.path.join(folder, "sheet.png")
    image.file_format = "PNG"
    image.save()
    print(f"Wrote the preview to {folder} (sheet.png)")


if __name__ == "__main__":
    main()
