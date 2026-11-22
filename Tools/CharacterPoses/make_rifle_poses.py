# Makes the soldiers' weapon clips (the shooter's plan, Phase 0.2: the rifle in third person; Phase 2.4: the pistol and
# the knife) on the animation library's rig, in Blender without its window. The rifle is placed against the shoulder,
# the pistol at arm's length on the sight line, the knife in the right fist; the hands are put on the weapon by IK, and
# every frame is baked into plain keyframes: the engine plays the clips like the library's own.
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/CharacterPoses/make_rifle_poses.py -- [--set rifles|sidearms] [--preview <folder>] [--only <clip>]
#
# Reads  Projects/Outpost/Assets/Models/QuaterniusAnimations/UAL1_Standard.glb (the rig, its idle and pistol clips)
#        Projects/Outpost/Assets/Models/QuaterniusGuns/AssaultRifle_2.fbx, SniperRifle_3.fbx, Pistol_1.fbx and
#        Projects/Outpost/Assets/Models/OutpostWeapons/Knife.glb (Tools/Models/make_knife.py): where the hands go
# Writes Projects/Outpost/Assets/Models/SoldierAnimations/Soldier_Rifles.glb (--set rifles, the default: the rifle's and
#        the sniper rifle's clips) or Soldier_Sidearms.glb (--set sidearms: the pistol's and the knife's), the mannequin
#        and the clips below
#
# Clips (Rifle_ for the assault rifle, Sniper_ for the sniper rifle):
#   _Idle_Loop        low ready: the muzzle down, on the library's Idle_Loop (breathing)
#   _Aim_Up, _Aim_Neutral, _Aim_Down   single poses for the aim offset (E5): 45 degrees up, level, 45 degrees down
#   _Shoot            the kick of one shot (Sniper_Shoot is followed by Sniper_Bolt)
#   _Reload           assault rifle: the magazine out, a new one from the belt, in; sniper: the bolt open, two rounds in
#   Sniper_Bolt       the bolt worked after a shot
#   Pistol_Idle_Loop, Pistol_Aim_Up/_Neutral/_Down, Pistol_Shoot, Pistol_Reload: the pistol in the right hand alone
#                     at arm's length (PISTOL_ONE_HAND; the hand posed by hand: ONE_HAND_*), the left arm at the side;
#                     the reload: the magazine out, a new one from the belt pushed up into the grip by the left hand
#   Knife_Idle_Loop   the knife in the right fist, its blade forward and up, the left fist up in a guard
#   Knife_Slash       back to the right shoulder, a slash across to the left hip, back; the chest turns with it
#
# The hands close on the gun (the user, 2026-11-19: the left fingers hung open in the air, the right hand hung down under
# the grip): each finger is curled from open along the library's fist (its three joints together) until its tip
# reaches what it holds, a glove's thickness out. The left hand's round the handguard (its cross-section measured on the
# gun's mesh); the right hand lies along the gun, level, its palm on the grip, its fingers and thumb round the grip and
# its index finger on the trigger. Every joint of a finger stops at the surface, not only its tip.
# The right hand is turned out about its index knuckle (level still) just as far as keeps the wrist out of the chest: the
# dressed player's body (Soldier_Player.gltf, the uniform's bulk) posed as the clip poses it, the forearm's thickness and
# the sleeve's clear. The left hand points along its forearm (the wrist straight), its palm turned up under the handguard
# and toward the gun's right (LEFT_PALM_TILT), half the turn the forearm's roll, so the wrist isn't wrung; the thumb
# along the handguard's left side. Then the hand's skin is tested against the gun's mesh (the user, 2026-11-19: the left
# hand went through the rifle): a finger in it is held further out, a palm in it moves out.
# The face is kept off the gun (the user: a few centimeters between them): the gun forward and lower in the aim poses
# (GUN_FORWARD, EYE_ABOVE), every frame of the aiming clips checked against the dressed body's head (FACE_CLEARANCE).
#
# Everything is generated: edits by hand in Blender are lost on the next run. --preview renders each clip's middle
# frame from the side and the front, the rifle in the hands (Workbench), to check the poses.

import bpy
import copy
import math
import os
import sys
from mathutils import Matrix, Quaternion, Vector

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
ASSETS = os.path.join(ROOT, "Projects", "Outpost", "Assets", "Models")
LIBRARY = os.path.join(ASSETS, "QuaterniusAnimations", "UAL1_Standard.glb")
GUNS = os.path.join(ASSETS, "QuaterniusGuns")
OUTPUT = os.path.join(ASSETS, "SoldierAnimations", "Soldier_Rifles.glb")

FPS = 30

# The guns as Blender imports the FBX files (x to the muzzle, y to the gun's left, z up; 100 units to the file's meter),
# scaled to meters by GUN_SCALE: the engine's 0.0017 on the raw file. Points read off side renders of the models.
GUN_SCALE = 0.17


def gun_point(x, y, z):
    return Vector((x, y, z)) * GUN_SCALE


WEAPONS = {
    "Rifle": {
        "File": "AssaultRifle_2.fbx",
        "Butt": gun_point(-1.55, 0.0, 0.25),       # the middle of the butt plate
        "Eye": gun_point(-0.55, 0.0, 0.82),        # on the sight line, over the comb, where the eye looks along it
        "Grip": gun_point(0.08, 0.0, 0.05),        # inside the pistol grip, where the palm closes
        "GripUp": Vector((0.43, 0.0, 0.9)),        # the pistol grip's slant, up its length
        "Trigger": gun_point(0.38, 0.0, 0.22),     # the trigger, in its guard (read off the mesh's side silhouette)
        "Support": gun_point(1.75, 0.0, 0.40),      # under the handguard
        "Magazine": gun_point(1.2, 0.0, -0.15),    # the magazine's middle, for the reload
        "Bolt": None,
        # The sights (the user, 2026-11-22: the crosshair where the rifle's sights are): the rear notch's top in its middle
        # and the front post's tip, from the mesh; the eye aiming down the sights is on the line through them
        "RearSight": gun_point(0.004, 0.0, 0.828),   # (2 mm over the block's top: the model has no notch cut, and
                                                       # level with its top the post's tip hid behind it)
        "FrontSight": gun_point(3.68, 0.0, 0.86),
        "LowReady": (30.0, 25.0, -0.01),           # the low ready: degrees the muzzle down and left about the butt, meters up
                                                   # (2 cm up: the stock 4 mm in the jaw breathing)
    },
    "Sniper": {
        "File": "SniperRifle_3.fbx",
        "Butt": gun_point(-1.9, 0.0, 0.0),
        "Eye": gun_point(-0.45, 0.0, 0.68),        # behind the scope's eyepiece
        "Grip": gun_point(-0.05, 0.0, -0.08),      # the thumbhole stock's grip
        "GripUp": Vector((0.45, 0.0, 0.9)),
        "Trigger": gun_point(0.2, 0.0, 0.0),
        "Support": gun_point(1.4, 0.0, 0.08),      # under the forend
        "Magazine": gun_point(0.6, -0.12, 0.38),   # the open bolt's port, where the rounds go in (right side)
        "Bolt": gun_point(0.33, -0.25, 0.30),      # the bolt handle's knob, on the right
        "LowReady": (30.0, 15.0, -0.03),          # lower and less across the chest: at the rifle's its comb was 1 cm in the jaw
    },
}

for _rifle in WEAPONS.values():
    _rifle.update({"Scale": GUN_SCALE, "Hold": "Rifle"})

# The sidearms (Phase 2.4). The pistol's file is the rifles' pack's, but a chunky 31 cm long at their scale: 23 cm at
# PISTOL_SCALE. Points read off a side render with a grid (x to the muzzle, y left, z up, the file's units / 100)
PISTOL_SCALE = 0.125


def pistol_point(x, y, z):
    return Vector((x, y, z)) * PISTOL_SCALE


# The knife (Tools/Models/make_knife.py, in meters): its handle leans KNIFE_SLANT forward from up, the blade going on
# along that line. Held as a pistol grip is: the fist closes round the handle at the slant
KNIFE_SLANT = math.radians(17.5)
KNIFE_AXIS = Vector((math.sin(KNIFE_SLANT), 0.0, math.cos(KNIFE_SLANT)))   # up the handle, to the tip
KNIFE_FRONT = Vector((math.cos(KNIFE_SLANT), 0.0, -math.sin(KNIFE_SLANT)))  # the edge's side


def knife_point(along, forward=0.0):
    return KNIFE_AXIS * along + KNIFE_FRONT * forward


SIDEARMS = {
    "Pistol": {
        "File": "Pistol_1.fbx", "Scale": PISTOL_SCALE, "Hold": "Pistol",
        "Eye": pistol_point(-0.10, 0.0, 0.74),     # the rear sight's notch: the sight line's start
        "RearSight": pistol_point(-0.106, 0.0, 0.7385),  # the rear notch's top in its middle (from the mesh)
        "FrontSight": pistol_point(1.261, 0.0, 0.754),   # the front blade's tip
        "Grip": pistol_point(-0.03, 0.0, 0.0),     # inside the grip, where the palm closes
        "GripUp": Vector((0.17, 0.0, 0.985)),      # the grip's slant (about 10 degrees)
        "Trigger": pistol_point(0.42, 0.0, 0.20),
        "Magazine": pistol_point(-0.08, 0.0, -0.39),  # the magazine's base, under the grip
        "Support": None, "Bolt": None, "Butt": None,
    },
    "Knife": {
        "File": os.path.join("..", "OutpostWeapons", "Knife.glb"), "Scale": 1.0, "Hold": "Knife",
        "Eye": None,
        "Grip": knife_point(0.01),                 # on the handle where the fist closes (under the guard)
        "GripUp": KNIFE_AXIS.copy(),
        # No trigger: a point where a pistol's would be from its grip (5.6 cm ahead of the handle's line, the fist's top
        # just under the guard), so the hand is placed on the handle as on a pistol grip; the index finger closes round
        # the handle as the others do
        "Trigger": Vector((0.066, 0.0, 0.01)),  # (the user: the knife 2.5 cm up in the fist, its handle top showing, and 1 cm back into the palm)
        "Magazine": None, "Support": None, "Bolt": None, "Butt": None,
    },
}
WEAPONS.update(SIDEARMS)
SETS = {"rifles": (["Rifle", "Sniper"], "Soldier_Rifles.glb"), "sidearms": (["Pistol", "Knife"], "Soldier_Sidearms.glb")}

# The rig: Blender's armature space has z up, the character looking along -y, its right at -x
FORWARD = Vector((0.0, -1.0, 0.0))
LEFT = Vector((1.0, 0.0, 0.0))
UP = Vector((0.0, 0.0, 1.0))

SPINE = ["spine_01", "spine_02", "spine_03"]

LEFT_GLOVE = 0.015    # meters out from the handguard the left fingers' joints stop (the player's gloved finger's half
                      # thickness: its skin is that far round the bones)
CURL_MAX = 1.6        # how far past the library's fist a finger may close (its fist: 1)
CURL_STEP = 0.05
LEFT_FINGERS = ("index", "middle", "ring", "pinky", "thumb")
BODY = os.path.join(ASSETS, "QuaterniusCharacters", "Soldier_Player.gltf")  # the dressed player: the chest the wrist keeps out of
CHEST_CLEARANCE = 0.035 # meters the right wrist's joint stays out of the chest (the gloved forearm's half thickness)
WRIST_TURN_MAX = 50.0   # degrees the right hand may turn out at most
LEFT_TWIST = 60.0       # degrees the left wrist may turn about the forearm, toward the handguard; as much again the forearm
                        # rolls about its own length (a rig without twist bones shows a whole turn at the wrist). The
                        # rig's rest has the palm down: under the handguard it needs about 100 degrees
LEFT_PALM_TILT = 35.0   # degrees the left palm faces from straight up toward the gun's right: the handguard lies in the
                        # hand from its lower left, the thumb up its left side, the fingers round under and up the right
# Each elbow's target from the middle of its arm: this far out to the side and down (meters). The right one mostly down:
# the upper arm in against the chest, not flared out (the user, 2026-11-19); while the right hand works the bolt or
# reloads, as the left one
ELBOW_TARGET = {"r": (0.45, 0.65), "l": (0.6, 0.6)}
# At the low ready (the muzzle down) the left elbow more down than out: the palm under the handguard brings the hand
# low, and with (0.6, 0.6) the sniper's elbow came level with it while breathing
LOW_READY_ELBOW_L = (0.6, 0.8)
ELBOW_INSIDE = {"r": 0.03, "l": 0.0}  # how far (m) an elbow may come inside its shoulder's line: the right one tucked in
PISTOL_ELBOW_TARGET = {"r": (0.35, 0.85), "l": (0.4, 0.95)}  # the pistol's (and the knife's): the arms reach forward and
                                                            # up, the elbows hang lower
PISTOL_ELBOW_INSIDE = 0.20            # the pistol's (and the knife's): both arms reach in to the middle
# The right hand placed by its index finger's knuckle: this far behind the trigger, above it, and out to the gun's right
# (the finger's own thickness beside the grip), the hand turned so its line from the wrist to that knuckle is level. The
# knuckles at the grip's front top, so the fingers close round its front and the index finger bends to the trigger
TRIGGER_REACH = (0.035, 0.012, 0.022)
GRIP_GLOVE = 0.010    # the right fingers' margin on the grip (the player's fingers are thicker than the mannequin's)
KNUCKLE = Vector((0.0, 0.12, 0.03))  # the index knuckle in the right hand bone's space: measured on the rig (main)
FINGERS = [f"{finger}_{joint:02d}_{side}" for side in "lr" for finger in ("index", "middle", "pinky", "ring", "thumb")
           for joint in (1, 2, 3)]


# ---------------------------------------------------------------------------------------------------------------------
# Small helpers

def args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    preview = None
    if "--preview" in argv:
        preview = os.path.abspath(argv[argv.index("--preview") + 1])
    only = argv[argv.index("--only") + 1] if "--only" in argv else None  # one clip, for trying a change: no file written
    which = argv[argv.index("--set") + 1] if "--set" in argv else "rifles"
    if which not in SETS:
        raise SystemExit(f"--set {which}: one of {', '.join(SETS)}")
    return preview, only, which


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def rot(axis, degrees):
    return Matrix.Rotation(math.radians(degrees), 4, axis)


def frame_from_axes(x, y, z, position):
    m = Matrix.Identity(4)
    for i in range(3):
        m[i][0], m[i][1], m[i][2] = x[i], y[i], z[i]
    m.translation = position
    return m


def lerp_matrix(a, b, t):
    la, ra, _ = a.decompose()
    lb, rb, _ = b.decompose()
    return Matrix.LocRotScale(la.lerp(lb, t), ra.slerp(rb, t), None)


def keyed(keys, time):
    """keys: [(time, value)] sorted; the value at a time, eased between keys (matrices are blended, floats lerped)"""
    if time <= keys[0][0]:
        return keys[0][1]
    for (t0, v0), (t1, v1) in zip(keys, keys[1:]):
        if time <= t1:
            t = smooth((time - t0) / (t1 - t0)) if t1 > t0 else 1.0
            if isinstance(v0, Matrix):
                return lerp_matrix(v0, v1, t)
            return v0 + (v1 - v0) * t
    return keys[-1][1]


# ---------------------------------------------------------------------------------------------------------------------
# The scene: the library's rig and mannequin, a gun, the IK targets

def load_library():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = FPS
    bpy.ops.import_scene.gltf(filepath=LIBRARY)
    armature = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    for o in list(bpy.data.objects):
        if o != armature and o.name != "Mannequin":
            bpy.data.objects.remove(o)
    return armature


def sample_action(armature, name):
    """Every frame's local pose (matrix_basis) of every bone in one of the library's clips"""
    action = bpy.data.actions[name]
    armature.animation_data.action = action
    first, last = (int(round(f)) for f in action.frame_range)
    frames = []
    for f in range(first, last + 1):
        bpy.context.scene.frame_set(f)
        frames.append({pb.name: pb.matrix_basis.copy() for pb in armature.pose.bones})
    armature.animation_data.action = None
    return frames


def load_gun(weapon):
    before = set(bpy.data.objects)
    path = os.path.normpath(os.path.join(GUNS, WEAPONS[weapon]["File"]))
    if path.endswith(".glb"):
        bpy.ops.import_scene.gltf(filepath=path)
    else:
        bpy.ops.import_scene.fbx(filepath=path)
    mesh = next(o for o in set(bpy.data.objects) - before if o.type == "MESH")
    frame = bpy.data.objects.new(f"{weapon}Frame", None)  # the gun in meters: x to the muzzle, y left, z up
    bpy.context.scene.collection.objects.link(frame)
    mesh.parent = frame
    mesh.matrix_parent_inverse = Matrix.Identity(4)
    mesh.location = (0.0, 0.0, 0.0)
    mesh.rotation_euler = (0.0, 0.0, 0.0)
    mesh.scale = (WEAPONS[weapon]["Scale"],) * 3
    return frame, mesh


def add_empty(name):
    empty = bpy.data.objects.new(name, None)
    empty.empty_display_size = 0.05
    bpy.context.scene.collection.objects.link(empty)
    return empty


class Rig:
    def __init__(self, armature):
        self.armature = armature
        self.report = True
        self.hand_target = {s: add_empty(f"Hand_{s}") for s in "rl"}
        self.pole = {s: add_empty(f"Elbow_{s}") for s in "rl"}
        self.ik = {}  # the arms' IK constraints: aim_elbows sets their pole angles
        for side in "rl":
            ik = armature.pose.bones[f"lowerarm_{side}"].constraints.new("IK")
            ik.target = self.hand_target[side]
            ik.pole_target = self.pole[side]
            self.ik[side] = ik
            ik.chain_count = 2
            copy = armature.pose.bones[f"hand_{side}"].constraints.new("COPY_ROTATION")
            copy.target = self.hand_target[side]

    def bone(self, name):
        """A bone's current matrix in armature space (constraints applied)"""
        return self.armature.pose.bones[name].matrix.copy()

    def update(self):
        bpy.context.view_layer.update()

    def set_pose(self, basis):
        for name, m in basis.items():
            self.armature.pose.bones[name].matrix_basis = m

    def local_pose(self):
        """matrix_basis of every bone that gives the current, constrained pose without constraints"""
        result = {}
        for pb in self.armature.pose.bones:
            rest = pb.bone.matrix_local
            if pb.parent:
                rest = pb.parent.bone.matrix_local.inverted() @ rest
                pose = pb.parent.matrix.inverted() @ pb.matrix
            else:
                pose = pb.matrix
            result[pb.name] = rest.inverted() @ pose
        return result


# ---------------------------------------------------------------------------------------------------------------------
# Where the hands go on a gun, in the gun's frame (meters)

PISTOL_HAND_DOWN = 0.012   # meters the pistol's hand is lower than the rifle's rule puts it: the web under the slide
THUMB_ROOT = Vector((0.0, 0.03, -0.02))             # the right thumb's root in the right hand bone's space: measured (main)


def right_hand_on_grip(w, turn=0.0):
    # Level along the gun (the user: parallel to it): the hand's x out to the gun's right (the palm, -x, on the grip's
    # right side), turned about it so the line from the wrist to the index knuckle runs to the muzzle; the knuckle
    # TRIGGER_REACH behind and above the trigger, so the index finger lies forward onto it. The hand's origin is the wrist
    # The pistol (the user: the hand straight, its wrist not bent): the same, turned never, only moved where its skin was
    # in the grip (RightOut, RightPush), and a little lower
    x = Vector((0.0, -1.0, 0.0))
    y = Vector((1.0, 0.0, 0.0))
    z = x.cross(y)
    level = Matrix.Rotation(-math.atan2(KNUCKLE.z, KNUCKLE.y), 3, x)  # the knuckle's rise over the bone's line taken out
    # turn: degrees about the gun's up through the knuckle, the wrist out to the gun's right (away from the chest)
    rotation = Matrix.Rotation(math.radians(turn), 3, "Z") @ level @ frame_from_axes(x, y, z, Vector()).to_3x3()
    one_hand = w.get("Hold") == "Pistol" and PISTOL_ONE_HAND
    # (the one-handed pistol: the hand tilted in the palm's plane, about the gun's side axis through the knuckle)
    rotation = Matrix.Rotation(math.radians(ONE_HAND_PITCH if one_hand else 0.0), 3, "Y") @ rotation
    back, up, out = TRIGGER_REACH
    out += 0.0 if one_hand else w.get("RightOut", 0.0)  # (the pistol's right hand moved out when its skin was in the grip)
    if w.get("Hold") == "Pistol":
        up -= 0.0 if PISTOL_ONE_HAND else PISTOL_HAND_DOWN  # (one hand: from the rifle's height, the fit moves it)
    knuckle = w["Trigger"] + Vector((-back, -out, up)) + (ONE_HAND_PUSH if one_hand else w.get("RightPush", Vector()))
    hand = rotation.to_4x4()
    hand.translation = knuckle - rotation @ KNUCKLE
    return hand


def left_hand_under(point, roll=0.0):
    # Palm up under the handguard (the left hand's palm is +x), the knuckles across to the gun's right and forward
    x = Vector((0.0, 0.0, 1.0))
    y = Vector((0.45, -0.89, 0.0)).normalized()
    z = x.cross(y)
    hand = frame_from_axes(x, y, z, Vector())
    hand = Matrix.Rotation(math.radians(roll), 4, "X") @ hand
    wrist = point - hand.col[1].to_3d() * 0.07 - hand.col[0].to_3d() * 0.035
    hand.translation = wrist
    return hand


def left_hand_on(point):
    # Fingers around a part from the right: the magazine, the bolt's port (palm to the gun's left)
    x = Vector((0.0, 1.0, 0.0))
    y = Vector((0.3, 0.0, -0.95)).normalized()
    z = x.cross(y)
    hand = frame_from_axes(x, y, z, Vector())
    hand.translation = point - y * 0.07 - x * 0.03
    return hand


def right_hand_on_bolt(point):
    # Thumb and fingers around the bolt's knob from above and behind
    z = Vector((0.0, 0.0, 1.0))
    y = Vector((0.6, 0.0, -0.8)).normalized()
    z = (z - y * y.dot(z)).normalized()
    x = y.cross(z)
    hand = frame_from_axes(x, y, z, Vector())
    hand.translation = point - y * 0.075 + x * 0.02
    return hand


# ---------------------------------------------------------------------------------------------------------------------
# The support hand's fingers round the handguard

GUARD_REACH = 0.03  # meters along the gun either side of the support the handguard is measured over (under the palm)


def handguard(mesh, w):
    """The handguard's cross-section under the palm, in the gun's frame: its middle (y, z) and half sizes (y, z), a glove
    out; and its bottom, where the support point goes. Measured by rays at 1 cm steps within GUARD_REACH of the support
    along the gun (a scan of the vertices missed a forend's slope and a keel under it: their faces are large): up from
    below across its middle for the lowest surface, then in from either side every 5 mm up for its width, as high as the
    width keeps over half the widest (above: the barrel, the gas tube's top, a scope's mount)"""
    from mathutils import bvhtree
    tree = bvhtree.BVHTree.FromPolygons([v.co * w["Scale"] for v in mesh.data.vertices],
                                        [tuple(p.vertices) for p in mesh.data.polygons])
    support = w["Support"]
    steps = [support.x + i * 0.01 for i in range(-int(GUARD_REACH * 100), int(GUARD_REACH * 100) + 1)]
    bottom = math.inf
    for x in steps:
        for y in (-0.01, 0.0, 0.01):
            hit, _, _, _ = tree.ray_cast(Vector((x, y, support.z - 0.1)), Vector((0.0, 0.0, 1.0)), 0.2)
            if hit is not None:
                bottom = min(bottom, hit.z)
    if bottom == math.inf:
        raise SystemExit(f"no handguard over the support point {support}")
    left, right, top = -math.inf, math.inf, bottom
    widest = 0.0
    for k in range(1, 30):
        z = bottom + k * 0.005
        row_left, row_right = -math.inf, math.inf
        for x in steps:
            hit, _, _, _ = tree.ray_cast(Vector((x, 0.2, z)), Vector((0.0, -1.0, 0.0)), 0.4)
            if hit is not None:
                row_left = max(row_left, hit.y)
            hit, _, _, _ = tree.ray_cast(Vector((x, -0.2, z)), Vector((0.0, 1.0, 0.0)), 0.4)
            if hit is not None:
                row_right = min(row_right, hit.y)
        width = row_left - row_right
        if width < 0.5 * widest or width <= 0.0:
            break
        widest = max(widest, width)
        left, right, top = max(left, row_left), min(right, row_right), z
    middle = Vector((0.0, (left + right) * 0.5, (bottom + top) * 0.5))
    half = Vector((0.0, (left - right) * 0.5 + LEFT_GLOVE, (top - bottom) * 0.5 + LEFT_GLOVE))
    print(f"  the handguard under the palm: {(left - right) * 100:.1f} cm wide, {(top - bottom) * 100:.1f} cm high, its "
          f"bottom {bottom * 100:.1f} cm up (the support point given at {support.z * 100:.1f})")
    return middle, half, bottom


def inside_guard(point_in_gun, guard):
    """Below 1: inside the handguard's cross-section (a box with rounded corners, the glove's thickness out: an ellipse
    through the box's sides leaves its corners out, and the fingers went into them)"""
    middle, half = guard
    dy, dz = (point_in_gun.y - middle.y) / half.y, (point_in_gun.z - middle.z) / half.z
    return dy ** 4 + dz ** 4


def finger_fist(fist_pose):
    """Each finger joint's turn in the library's fist (Idle_Loop's hands), as an axis and an angle: the fingers close
    along it, the way the rig's hand is made to"""
    turns = {}
    for finger, side in ((f, s) for f in LEFT_FINGERS for s in "lr"):
        for j in (1, 2, 3):
            name = f"{finger}_{j:02d}_{side}"
            axis, angle = fist_pose[name].to_quaternion().to_axis_angle()
            if angle > math.pi:
                angle -= 2.0 * math.pi
            turns[name] = (axis, angle)
    return turns


def wrap_left_fingers(rig, gun, guard, fist, margins, limits=None):
    """Each finger of the left hand closed from open along the fist (its three joints together, a fraction of their fist
    turn) until its outer joints reach the handguard's surface (a binary search on the fraction); a finger that can't
    reach it closes as far as brings its tip nearest. margins: meters a finger stays further out (its skin was found in
    the gun); limits: the most a finger closes (a fraction of the fist: its skin was in the gun at the margins' most).
    Returns the farthest a tip stayed from the surface (meters) and by which"""
    limits = limits or {}
    to_gun = gun.inverted()
    worst = (0.0, "")
    middle, half = guard

    def out_by(extra):
        return middle, half + Vector((0.0, extra, extra))

    for finger in LEFT_FINGERS:
        names = [f"{finger}_{j:02d}_l" for j in (1, 2, 3)]
        finger_guard = out_by(margins.get(finger, 0.0))
        if finger == "thumb":
            continue  # place_thumb
        limit = limits.get(finger)
        value = close_finger(rig, fist, names, lambda tip: inside_guard(to_gun @ tip, finger_guard), limit=limit)
        if value > 1.0 and limit is None:
            gap = (value ** 0.25 - 1.0) * min(half.y, half.z)
            if gap > worst[0]:
                worst = (gap, finger)
    return worst


HAND_PRESS = 0.0015                 # meters the hand's skin may be in the gun: a glove pressed on it, not through it
PALM_OUT_MAX = 0.045                # meters the left wrist may go out under the support at most (from 0.035)
FINGER_MARGIN_MAX = 0.02            # meters a left finger may be held further out at most (the sniper's forend is an
                                    # open shell: its inside can't be told everywhere, and the margins ran away)
# The left thumb (the user, 2026-11-19: not twisted; stretched or relaxed, forward and a little up along the barrel):
# nearly straight, its first joint aimed with the least turn along the gun, THUMB_UP above it, turned out from the
# handguard's left side only as far as keeps it out of the handguard
THUMB_RELAX = 0.12     # of the library's fist the thumb's joints are curled (0: the rest pose's flat hand, stiff)
THUMB_UP = 10.0        # degrees above the gun's line the thumb points at the least (more if that brings it onto the side)
THUMB_UPS = range(10, 41, 3)    # the rises tried (degrees)
THUMB_OUTS = range(-24, 41, 2)  # the turns out to the gun's left tried (degrees; negative: in, toward the handguard)
THUMB_GLOVE = 0.010    # meters out from the handguard the thumb's joints stop (thinner than the fingers' LEFT_GLOVE: the
                       # user, 2026-11-19: the thumb touching the handguard; the skin check keeps it out of the gun)


def set_thumb(rig, fist, gun, aim, side="l"):
    """A thumb relaxed (THUMB_RELAX of the fist) and its first joint turned, the least turn, so the thumb from its root
    to its tip points along the gun, aim = (degrees up, degrees out to the gun's left)"""
    up_degrees, out = aim
    armature = rig.armature
    names = [f"thumb_{j:02d}_{side}" for j in (1, 2, 3)]
    for name in names:
        axis, angle = fist[name]
        armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * THUMB_RELAX).to_matrix().to_4x4()
    rig.update()
    along, left, up = (gun.col[i].to_3d().normalized() for i in range(3))
    wanted = (Matrix.Rotation(math.radians(out), 3, up) @ (Matrix.Rotation(-math.radians(up_degrees), 3, left) @ along))
    first = armature.pose.bones[names[0]]
    root = first.matrix.translation.copy()
    now = (armature.pose.bones[names[2]].tail - root).normalized()
    turn = now.rotation_difference(wanted.normalized())
    first.matrix = Matrix.Translation(root) @ (turn @ first.matrix.to_quaternion()).to_matrix().to_4x4()
    rig.update()


def place_thumb(rig, gun, w, guard, fist, margin):
    """The left thumb on the handguard's left side, toward the muzzle and a little up: of the rises and turns out tried,
    the one whose joints are all out of the handguard (THUMB_GLOVE and margin out) and the nearest to touching it (its
    closest joint's box value nearest 1), the least rise and turn breaking ties. Returns (degrees up, degrees out)"""
    to_gun = gun.inverted()
    middle, half = guard
    extra = THUMB_GLOVE - LEFT_GLOVE + margin
    box = (middle, half + Vector((0.0, extra, extra)))
    armature = rig.armature
    names = [f"thumb_{j:02d}_l" for j in (1, 2, 3)]
    best = (math.inf, (THUMB_UP, 0))
    for up_degrees in THUMB_UPS:
        for out in THUMB_OUTS:
            set_thumb(rig, fist, gun, (up_degrees, out))
            joints = [armature.pose.bones[n].tail for n in names] + [armature.pose.bones[n].head for n in names[1:]]
            values = [inside_guard(to_gun @ p, box) for p in joints]
            if min(values) < 1.0:
                continue
            score = (min(values) - 1.0) + 0.002 * (up_degrees - THUMB_UP) + 0.002 * abs(out)
            if score < best[0]:
                best = (score, (up_degrees, out))
    set_thumb(rig, fist, gun, best[1])
    return best[1]


CURLED = {}  # each finger's (its first joint's name) last fraction of the fist close_finger left it at


def close_finger(rig, fist, names, test, nearest=False, nearest_joint=-1, limit=None):
    """A finger (its three joints' names) closed from open along the fist: until test is at most 1 at its two outer
    joints or its tip (it lies on what it holds, none of it inside; the knuckle at the palm isn't tested; a binary search
    on the fraction), or, nearest, as far as makes test of its tip (or of the end of joint nearest_joint) smallest.
    limit: the most it closes (a fraction of the fist; None: CURL_MAX), where the skin test found it in the gun further
    closed. Returns test's value then (the fraction is kept in CURLED)"""
    armature = rig.armature

    def curl(fraction):
        CURLED[names[0]] = fraction
        for name in names:
            axis, angle = fist[name]
            armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * fraction).to_matrix().to_4x4()
        rig.update()
        if nearest:
            return test(armature.pose.bones[names[nearest_joint]].tail)
        return min(test(armature.pose.bones[name].tail) for name in names[1:])

    most = CURL_MAX if limit is None else min(CURL_MAX, limit)
    steps = [i * CURL_STEP for i in range(int(most / CURL_STEP) + 1)]
    if limit is not None and steps[-1] < most - 1e-6:
        steps.append(most)
    values = [curl(t) for t in steps]
    touching = None if nearest else next((i for i, v in enumerate(values) if v <= 1.0), None)
    if touching is None or touching == 0:
        if touching is None and not nearest and limit is not None:
            return curl(steps[-1])  # held open at the limit: as far as it may close
        best = min(range(len(steps)), key=lambda i: values[i]) if touching is None else 0
        return curl(steps[best])
    low, high = steps[touching - 1], steps[touching]
    for _ in range(8):
        t = (low + high) * 0.5
        if curl(t) <= 1.0:
            high = t
        else:
            low = t
    return curl(high)


def grip_section(mesh, w):
    """The pistol grip's cross-section round its slant's line: its half width across the gun and its half depth front to
    back (meters), from its mesh's vertices between 5 cm below the grip's middle and the middle (under the trigger
    guard and the receiver), within 3 cm of the line"""
    up = w["GripUp"].normalized()
    across = Vector((0.0, 1.0, 0.0))
    depth = up.cross(across).normalized()
    width = front = 0.0
    for v in mesh.data.vertices:
        d = v.co * w["Scale"] - w["Grip"]
        along = d.dot(up)
        flat = d - up * along
        if -0.05 < along < 0.0 and flat.length < 0.03:
            width = max(width, abs(flat.dot(across)))
            front = max(front, abs(flat.dot(depth)))
    return width, front, depth


def grip_test(gun, w, margin):
    """Below 1: inside the grip's cross-section round its slant's line (an ellipse, margin meters out), within its length"""
    to_gun = gun.inverted()
    up = w["GripUp"].normalized()
    width, front, depth = w["GripSection"]

    def on_grip(tip):
        # The grip's length: from 6 cm below its middle to 2 cm above (its top, under the receiver and the trigger
        # guard: a finger over it is not on the grip)
        v = to_gun @ tip - w["Grip"]
        along = v.dot(up)
        if not -0.06 < along < 0.02:
            return math.inf
        flat = v - up * along
        a, b = flat.y / (width + margin), flat.dot(depth) / (front + margin)
        return a * a + b * b
    return on_grip


def surface_test(tree, margin):
    """Below 1: a point within margin of the gun's real surface, or inside it (the pistol's: its grip isn't an ellipse,
    the trigger guard is in front of it)"""
    return lambda p: outside(tree, p) / margin


def finger_points(rig, names):
    """A finger's outer joints and its bones' middles (armature space)"""
    bones = [rig.armature.pose.bones[n] for n in names]
    return [bones[0].tail.copy()] + [b.tail.copy() for b in bones[1:]] + [(b.head + b.tail) * 0.5 for b in bones]


def close_finger_on(rig, fist, names, tree, margin, toward=None):
    """A finger closed from open along the fist as far as keeps all of it margin out of the gun's surface; toward: the
    curl that brings its last joint nearest that point instead, within the same limit (the index finger to the trigger)"""
    armature = rig.armature

    def curl(fraction):
        for name in names:
            axis, angle = fist[name]
            armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * fraction).to_matrix().to_4x4()
        rig.update()

    steps = [i * CURL_STEP for i in range(int(CURL_MAX / CURL_STEP) + 1)]
    allowed = []
    for s in steps:
        curl(s)
        if min(outside(tree, p) for p in finger_points(rig, names)) < margin:
            break
        allowed.append(s)
    if not allowed:
        curl(0.0)
        return math.inf
    if toward is None:
        curl(allowed[-1])
        return 0.0
    best = min(allowed, key=lambda s: (curl(s), (armature.pose.bones[names[1]].tail - toward).length)[1])
    curl(best)
    return (armature.pose.bones[names[1]].tail - toward).length


def set_one_hand_fingers(rig, fist, scale=None):
    """The one-handed pistol's right fingers and thumb as ONE_HAND_FINGERS has them (the index finger spread in the
    palm's plane at its root first)"""
    armature = rig.armature
    for finger in ONE_HAND_FINGERS:
        for j in (1, 2, 3):
            armature.pose.bones[f"{finger}_{j:02d}_r"].matrix_basis = Matrix.Identity(4)
    rig.update()
    root = armature.pose.bones["index_01_r"]
    normal = (root.matrix.to_3x3().inverted() @ rig.bone("hand_r").col[0].to_3d()).normalized()
    for finger, fractions in ONE_HAND_FINGERS.items():
        for j, fraction in zip((1, 2, 3), fractions):
            fraction *= (scale or {}).get(finger, 1.0)
            name = f"{finger}_{j:02d}_r"
            axis, angle = fist[name]
            q = Quaternion(axis, angle * fraction)
            if name == "index_01_r":
                q = Quaternion(normal, math.radians(ONE_HAND_INDEX_SPREAD)) @ q
            armature.pose.bones[name].matrix_basis = q.to_matrix().to_4x4()
    rig.update()


def close_rifle_thumb(rig, fist, tree):
    """The right thumb closed from open along the fist as far as keeps its bones a glove out of the gun and its skin
    pressed at most PALM_PRESS into it"""
    names = [f"thumb_{j:02d}_r" for j in (1, 2, 3)]
    armature = rig.armature

    def curl(fraction):
        for name in names:
            axis, angle = fist[name]
            armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * fraction).to_matrix().to_4x4()
        rig.update()

    best = 0.0
    for i in range(int(CURL_MAX / CURL_STEP) + 1):
        s = i * CURL_STEP
        curl(s)
        if min(outside(tree, p) for p in finger_points(rig, names)) < THUMB_GLOVE * 0.6:
            break
        if rig.chest is not None and rig.chest.thumb_depth(tree, "r") > PALM_PRESS:
            break
        best = s
    curl(best)
    return best


def grip_right_fingers(rig, gun, w, fist, margins=None, tree=None):
    """The right hand's fingers and thumb round the grip (a cylinder along its slant, a glove's thickness out, and a
    finger's margin found in the skin check), its index finger closed as far as brings its tip nearest the trigger. The
    pistol's thumb is placed apart (place_pistol_thumbs). Returns how far the index finger's tip stays from it"""
    to_gun = gun.inverted()
    margins = margins or {}
    if w["Hold"] == "Pistol" and tree is not None and not PISTOL_ONE_HAND:
        # On the pistol's own surface: each finger round the grip as far as keeps it a glove out; the index finger as near
        # the trigger as that allows
        for finger in ("middle", "ring", "pinky"):
            close_finger_on(rig, fist, [f"{finger}_{j:02d}_r" for j in (1, 2, 3)], tree, GRIP_GLOVE + margins.get(finger, 0.0))
        # The index finger straight along the frame over the trigger guard (the user's photograph: off the trigger until
        # the shot), a little curled, as far as keeps it out of the frame
        names = [f"index_{j:02d}_r" for j in (1, 2, 3)]
        for fraction in (0.15, 0.1, 0.05, 0.0):
            for name in names:
                axis, angle = fist[name]
                rig.armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * fraction).to_matrix().to_4x4()
            rig.update()
            if min(outside(tree, p) for p in finger_points(rig, names)) >= GRIP_GLOVE + margins.get("index", 0.0):
                break
        return (rig.armature.pose.bones[names[1]].tail - gun @ w["Trigger"]).length
    rifle_thumb = w["Hold"] == "Rifle" and tree is not None
    fingers = ("middle", "ring", "pinky") + (("thumb",) if w["Hold"] != "Pistol" and not rifle_thumb else ()) + \
        (("index",) if w["Hold"] == "Knife" else ())
    for finger in fingers:
        close_finger(rig, fist, [f"{finger}_{j:02d}_r" for j in (1, 2, 3)], grip_test(gun, w, GRIP_GLOVE + margins.get(finger, 0.0)))
    if rifle_thumb:
        # The rifle's thumb (the user's photograph of a hand on an AK's grip: the thumb round the grip's back to its far
        # side, none of it in the rifle): closed round the grip as far as keeps its skin out of the rifle's real surface
        close_rifle_thumb(rig, fist, tree)
    if w["Hold"] == "Knife":
        return 0.0  # (no trigger: the index finger round the handle too)
    # The index finger's pad on the trigger: its last joint (the end of its middle bone) nearest it
    return close_finger(rig, fist, [f"index_{j:02d}_r" for j in (1, 2, 3)], lambda tip: (to_gun @ tip - w["Trigger"]).length,
                        nearest=True, nearest_joint=1)


# The pistol's thumbs (the user, 2026-11-21, two photographs of a thumbs-forward grip): both point forward along the
# frame's left side, the right one high, just under the slide, the left one below it and ahead of it; they don't touch,
# and neither is in the gun. The left fingers close round the grip over the right ones (the grip's section, a right
# finger's thickness out)
PALM_PRESS = 0.0025          # meters a palm's (and a thumb's ball's) glove may press into the grip it holds
PISTOL_SLIDE_UNDER = 0.049   # meters up the gun's frame (from its origin) the right thumb lies at, under the slide
THUMB_APART = 0.019          # meters between the two thumbs' joints at the least (two gloved thumbs' half thicknesses)
LEFT_UNDER_RIGHT = 0.012     # meters the left thumb's joints stay under the right one's lowest at the least
LEFT_OVER_RIGHT_FINGERS = 0.024  # meters out from the grip the left fingers close (over the right fingers)
PISTOL_THUMB_UPS = range(-45, 31, 3)
PISTOL_THUMB_OUTS = range(-60, 61, 3)


def gun_tree(gun_mesh):
    """The gun's surface (its mesh where the clip holds it) for distances, in armature space"""
    from mathutils import bvhtree
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = gun_mesh.evaluated_get(depsgraph)
    mesh = evaluated.to_mesh()
    tree = bvhtree.BVHTree.FromPolygons([evaluated.matrix_world @ v.co for v in mesh.vertices],
                                        [tuple(p.vertices) for p in mesh.polygons])
    evaluated.to_mesh_clear()
    return tree


def thumb_points(rig, side):
    """A thumb's joints and the middles of its bones (armature space)"""
    bones = [rig.armature.pose.bones[f"thumb_{j:02d}_{side}"] for j in (1, 2, 3)]
    points = []
    for b in bones:
        points += [b.head.copy(), b.tail.copy(), (b.head + b.tail) * 0.5]
    return points[1:]  # (not the root, inside the hand)


def crosses_odd(tree, point):
    """A ray up from the point crosses the gun's surface an odd number of times (with the nearest face's side: inside;
    the gun is made of parts that overlap and aren't all closed, which fools either test alone)"""
    crossings, start = 0, point
    for _ in range(64):
        hit, _, _, _ = tree.ray_cast(start, Vector((0.0, 0.0, 1.0)))
        if hit is None:
            break
        crossings += 1
        start = hit + Vector((0.0, 0.0, 1e-5))
    return crossings % 2 == 1


def outside(tree, point):
    """How far a point is out of the gun's surface (negative: inside it)"""
    location, normal, _, distance = tree.find_nearest(point)
    return distance if (point - location).dot(normal) >= 0.0 else -distance


# The right thumb (the user: close to the pistol, not far, not in it): its first bone round the back of the grip to the
# frame's left side, the other two forward along it, a few degrees in toward it. Searched: where the first bone points
# (how far out to the left, how high, in the pistol's frame), the nearest the frame whose skin stays out of it
RIGHT_THUMB_BACK = -0.040
RIGHT_THUMB_OUTS = [0.016 + 0.002 * i for i in range(14)]     # meters to the frame's left the first bone points
RIGHT_THUMB_HEIGHTS = [0.030 + 0.003 * i for i in range(9)]   # meters up
RIGHT_THUMB_IN = 4.0                                          # degrees the rest of it turns in toward the frame


def turn_bone_to(rig, name, direction):
    """A pose bone turned (the least turn) so it points along a direction, in armature space; its head stays"""
    pb = rig.armature.pose.bones[name]
    m = pb.matrix.copy()
    turn = m.col[1].to_3d().normalized().rotation_difference(direction.normalized())
    pb.matrix = Matrix.Translation(m.translation) @ (turn @ m.to_quaternion()).to_matrix().to_4x4()
    rig.update()


def set_wrapped_thumb(rig, fist, gun, aim):
    """The right thumb round the back of the grip and forward along its left side: aim = (meters out to the left, meters
    up) of the point its first bone points at, RIGHT_THUMB_BACK along the gun"""
    out, up = aim
    armature = rig.armature
    names = [f"thumb_{j:02d}_r" for j in (1, 2, 3)]
    for name in names:
        axis, angle = fist[name]
        armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * THUMB_RELAX).to_matrix().to_4x4()
    rig.update()
    first = armature.pose.bones[names[0]]
    point = gun @ Vector((RIGHT_THUMB_BACK, out, up))
    turn_bone_to(rig, names[0], point - first.head)
    along, left, gun_up = (gun.col[i].to_3d().normalized() for i in range(3))
    forward = Matrix.Rotation(-math.radians(RIGHT_THUMB_IN), 3, gun_up) @ along  # (in toward the frame: to the gun's right)
    for name in names[1:]:
        turn_bone_to(rig, name, forward)


def place_pistol_thumbs(rig, fist, gun, tree, left_on):
    """The right thumb forward along the frame's left side just under the slide, out of the gun; then (left_on: the
    left hand round the right) the left thumb forward below and ahead of it, out of the gun and off the right thumb.
    Returns the two thumbs' aims (degrees up, out) and the report's numbers"""
    to_gun = gun.inverted()

    def first_clear(candidates, side):
        """Of the aims (score, aim) the best whose skin is out of the gun (the bones' distance is a guess at it)"""
        least = (math.inf, None)
        for _, aim in sorted(candidates)[:200]:
            set_thumb(rig, fist, gun, aim, side)
            depth = rig.chest.thumb_depth(tree, side) if rig.chest is not None else 0.0
            if depth <= HAND_PRESS:
                return aim
            least = min(least, (depth, aim))
        print(f"    the {'left' if side == 'l' else 'right'} thumb: the least its skin went into the gun {least[0] * 1000:.0f} mm "
              f"(aim {least[1]}), at {tuple(round(v * 100, 1) for v in to_gun @ rig.chest.deepest_point)} cm on the pistol")
        return None

    candidates = []
    for out in RIGHT_THUMB_OUTS:
        for up in RIGHT_THUMB_HEIGHTS:
            set_wrapped_thumb(rig, fist, gun, (out, up))
            points = thumb_points(rig, "r")
            gaps = [outside(tree, p) for p in points]
            if min(gaps) < THUMB_GLOVE * 0.6:
                continue
            height = sum((to_gun @ p).z for p in points[3:]) / len(points[3:])  # (its outer bones)
            # As close to the frame as its skin allows, its outer bones just under the slide
            candidates.append(((min(gaps) - THUMB_GLOVE * 0.6) + 0.5 * abs(height - PISTOL_SLIDE_UNDER), (out, up)))

    def first_clear_wrapped(found):
        least = (math.inf, None)
        for _, aim in sorted(found)[:120]:
            set_wrapped_thumb(rig, fist, gun, aim)
            depth = rig.chest.thumb_depth(tree, "r") if rig.chest is not None else 0.0
            if depth <= PALM_PRESS:  # (a gloved thumb lying on the frame, as the skin check allows)
                return aim
            least = min(least, (depth, aim))
        where = tuple(round(v * 100, 1) for v in to_gun @ rig.chest.deepest_point) if least[1] is not None else ()
        print(f"    the right thumb: the least its skin went into the gun {least[0] * 1000:.0f} mm (aim {least[1]}), at {where} cm on the pistol")
        return None

    right_aim = first_clear_wrapped(candidates)
    if right_aim is None:
        raise SystemExit(f"the pistol's right thumb: no aim keeps its skin out of the gun ({len(candidates)} keep its bones out)")
    set_wrapped_thumb(rig, fist, gun, right_aim)
    right = thumb_points(rig, "r")
    right_low = min((to_gun @ p).z for p in right)
    right_tip = (to_gun @ right[-2]).x  # (the third bone's tail: its tip)
    report = {"right": right_aim, "right gap": min(outside(tree, p) for p in right)}
    if not left_on:
        return right_aim, None, report
    candidates = []
    failures = {}
    for up in PISTOL_THUMB_UPS:
        for out in PISTOL_THUMB_OUTS:
            set_thumb(rig, fist, gun, (up, out), "l")
            points = thumb_points(rig, "l")
            gaps = [outside(tree, p) for p in points]
            apart = min((a - b).length for a in points for b in right)
            low = max((to_gun @ p).z for p in points)
            tip = (to_gun @ points[-2]).x
            fails = [name for name, bad in (("in the gun", min(gaps) < THUMB_GLOVE), ("on the right thumb", apart < THUMB_APART),
                                            ("not under it", low > right_low - LEFT_UNDER_RIGHT), ("not ahead", tip < right_tip)) if bad]
            for name in fails:
                failures[name] = failures.get(name, 0) + 1
            if fails:
                continue
            # (along the frame's side under the right thumb: its tip at that height too, not dipping under the frame)
            tip_z = (to_gun @ points[-2]).z
            score = (min(gaps) - THUMB_GLOVE) + 0.5 * (right_low - LEFT_UNDER_RIGHT - low) + \
                abs(tip_z - (right_low - LEFT_UNDER_RIGHT - 0.004)) + 0.0005 * abs(up)
            candidates.append((score, (up, out)))
    left_aim = first_clear(candidates, "l")
    if left_aim is None:
        raise SystemExit(f"the pistol's left thumb: no aim keeps it out of the gun, off the right thumb, under and ahead of it "
                         f"(the aims failing each: {failures}; the right thumb {right_aim}, its lowest {right_low * 100:.1f} cm up the gun, "
                         f"its tip {right_tip * 100:.1f} cm along; {len(candidates)} keep its bones clear, none its skin)")
    set_thumb(rig, fist, gun, left_aim, "l")
    left = thumb_points(rig, "l")
    report.update({"left": left_aim, "left gap": min(outside(tree, p) for p in left),
                   "apart": min((a - b).length for a in left for b in right)})
    for side, points in (("right", right), ("left", left)):
        tip = to_gun @ points[-2]
        root = to_gun @ points[0]
        print(f"    the {side} thumb on the pistol (cm: along, to its left, up): its first joint "
              f"{tuple(round(v * 100, 1) for v in root)}, its tip {tuple(round(v * 100, 1) for v in tip)}")
    return right_aim, left_aim, report


def close_left_round_grip(rig, fist, gun, w, margins):
    """The pistol's left fingers closed round the grip over the right ones; one below the grip (nothing to hold) closes
    into a relaxed fist"""
    for finger in ("index", "middle", "ring", "pinky"):
        names = [f"{finger}_{j:02d}_l" for j in (1, 2, 3)]
        value = close_finger(rig, fist, names, grip_test(gun, w, LEFT_OVER_RIGHT_FINGERS + margins.get(finger, 0.0)))
        if value == math.inf:
            for name in names:
                axis, angle = fist[name]
                rig.armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * 0.8).to_matrix().to_4x4()
            rig.update()


class Chest:
    """The dressed player's body on its own rig, posed as the rig's pose plays on it in the engine, for how far a point is
    out of its torso (the spine's, the collarbones' and the neck's skin). In the engine a clip's keys replace every node's
    local transform, so each of the body's bones is where the library's is; its Blender bone is turned from the
    library's by the two rests' difference (the importer orients each rig's bones by its own proportions, so their local
    rotations can't be copied; the files' node rests agree within a few degrees)"""

    def __init__(self, rig):
        before = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=BODY)
        new = set(bpy.data.objects) - before
        self.armature = next(o for o in new if o.type == "ARMATURE")
        self.mesh = next(o for o in new if o.type == "MESH" and o.name.startswith("SuperHero"))
        for o in new:
            if o not in (self.armature, self.mesh):
                bpy.data.objects.remove(o)
        self.rig = rig
        torso_groups = {g.index for g in self.mesh.vertex_groups if g.name in
                        ("spine_01", "spine_02", "spine_03", "clavicle_l", "clavicle_r", "neck_01")}
        self.torso = [sum(g.weight for g in v.groups if g.group in torso_groups) >= 0.5 for v in self.mesh.data.vertices]
        head_groups = {g.index for g in self.mesh.vertex_groups if g.name == "Head"}
        self.head = [sum(g.weight for g in v.groups if g.group in head_groups) >= 0.5 for v in self.mesh.data.vertices]
        # Each hand's skin, each vertex named by its finger ("palm": the hand bone's), for which part is in the gun
        self.hands = {}
        for side in "lr":
            hand_groups = {g.index: ("palm" if g.name == f"hand_{side}" else g.name.split("_")[0]) for g in self.mesh.vertex_groups
                           if g.name == f"hand_{side}" or (g.name.endswith(f"_{side}") and g.name.split("_")[0] in LEFT_FINGERS)}
            parts = []
            for v in self.mesh.data.vertices:
                weights = [(g.weight, hand_groups[g.group]) for g in v.groups if g.group in hand_groups]
                parts.append(max(weights)[1] if sum(w for w, _ in weights) >= 0.5 else None)
            self.hands[side] = parts
        self.left_hand = self.hands["l"]

        self.rest_body = {b.name: b.matrix_local.copy() for b in self.armature.data.bones}
        self.rest_rig = {b.name: b.matrix_local.copy() for b in rig.armature.data.bones}
        self.order = [b.name for b in self.armature.data.bones if b.name in self.rest_rig]  # parents first
        self.order.sort(key=lambda name: len(self.armature.data.bones[name].parent_recursive))

    def pose(self):
        """The body's bones where the library rig's are (armature space), each turned by the rests' difference"""
        to_body = self.armature.matrix_world.inverted() @ self.rig.armature.matrix_world
        wanted = {}
        for name in self.order:
            m = to_body @ self.rig.armature.pose.bones[name].matrix
            turn = m.to_3x3().normalized() @ self.rest_rig[name].to_3x3().inverted() @ self.rest_body[name].to_3x3()
            wanted[name] = Matrix.Translation(m.translation) @ turn.to_4x4()
        for name in self.order:
            bone = self.armature.data.bones[name]
            if bone.parent and bone.parent.name in wanted:
                parent = wanted[bone.parent.name]
                rest = bone.parent.matrix_local.inverted() @ bone.matrix_local
                self.armature.pose.bones[name].matrix_basis = rest.inverted() @ parent.inverted() @ wanted[name]
            else:
                self.armature.pose.bones[name].matrix_basis = bone.matrix_local.inverted() @ wanted[name]
        bpy.context.view_layer.update()

    def clearance(self, point):
        """How far a point (the rig's armature space) is out of the torso's skin (negative: inside)"""
        from mathutils import bvhtree
        self.pose()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = self.mesh.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        world = evaluated.matrix_world
        verts = [world @ v.co for v in mesh.vertices]
        polys = [tuple(poly.vertices) for poly in mesh.polygons if all(self.torso[i] for i in poly.vertices)]
        tree = bvhtree.BVHTree.FromPolygons(verts, polys)
        evaluated.to_mesh_clear()
        at = self.rig.armature.matrix_world @ point
        location, normal, _, distance = tree.find_nearest(at)
        return distance if (at - location).dot(normal) >= 0.0 else -distance

    def thumb_depth(self, tree, side):
        """How deep a thumb's skin is in the gun at the deepest (meters; 0: out of it), the gun's surface a BVH tree in
        armature space (gun_tree), the body posed as the rig is"""
        self.pose()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        body = self.mesh.evaluated_get(depsgraph)
        mesh = body.to_mesh()
        to_rig = self.rig.armature.matrix_world.inverted() @ body.matrix_world
        deepest = 0.0
        self.deepest_point = Vector()
        for i, v in enumerate(mesh.vertices):
            if self.hands[side][i] != "thumb":
                continue
            point = to_rig @ v.co
            location, normal, _, distance = tree.find_nearest(point)
            if (point - location).dot(normal) < 0.0 and distance > deepest and crosses_odd(tree, point):
                deepest = distance
                self.deepest_point = point.copy()
        body.to_mesh_clear()
        return deepest

    def against_gun(self, gun_mesh, side="l"):
        """The posed body against the gun (its mesh where the clip holds it): how far the face (the head's skin) is from
        the gun at the closest, and how deep the left hand's skin is in it at the deepest (meters; 0: nothing inside).
        Inside: behind the nearest face, and a ray from the point along +z crosses the gun's surface an odd number of
        times"""
        from mathutils import bvhtree
        self.pose()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = gun_mesh.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        gun_tree = bvhtree.BVHTree.FromPolygons([evaluated.matrix_world @ v.co for v in mesh.vertices],
                                                [tuple(p.vertices) for p in mesh.polygons])
        evaluated.to_mesh_clear()
        body = self.mesh.evaluated_get(depsgraph)
        mesh = body.to_mesh()
        verts = [body.matrix_world @ v.co for v in mesh.vertices]
        body.to_mesh_clear()

        def inside(point):
            # Both: behind the nearest face (its normal) and an odd count of crossings. The gun is made of parts that
            # overlap and aren't all closed, which fools either test alone
            location, normal, _, _ = gun_tree.find_nearest(point)
            if (point - location).dot(normal) >= 0.0:
                return False
            crossings, start = 0, point
            for _ in range(64):
                hit, _, _, _ = gun_tree.ray_cast(start, Vector((0.0, 0.0, 1.0)))
                if hit is None:
                    break
                crossings += 1
                start = hit + Vector((0.0, 0.0, 1e-5))
            return crossings % 2 == 1

        face = min(gun_tree.find_nearest(v)[3] for v, h in zip(verts, self.head) if h)
        in_face = [gun_tree.find_nearest(v)[3] for v, h in zip(verts, self.head) if h and inside(v)]
        face_inside = bool(in_face)
        if in_face:
            face = max(in_face)  # how deep at the deepest
        deepest = {}
        self.deepest_at = {}  # where each part is deepest, in the gun's frame (cm), for the report
        to_gun = gun_mesh.parent.matrix_world.inverted()
        for v, part in zip(verts, self.hands[side]):
            if part and inside(v):
                depth = gun_tree.find_nearest(v)[3]
                if depth > deepest.get(part, 0.0):
                    deepest[part] = depth
                    self.deepest_at[part] = tuple(round(c * 100, 1) for c in to_gun @ v)
        return (-face if face_inside else face), deepest


def straight_left_hand(rig, gun, w, rest_relation, palm_out=0.035):
    """The left hand under the support, pointing along its forearm (the wrist straight), turned about the forearm toward
    having its palm up at the handguard by at most twice LEFT_TWIST (half of it the forearm's roll: roll_forearm); the
    palm's middle at the support. Returns the hand (armature space) and how far it is turned about the forearm (degrees)"""
    lower = rig.bone("lowerarm_l").to_3x3().normalized()
    neutral = lower @ rest_relation           # the hand as the forearm carries it, unbent and untwisted
    along = neutral.col[1].normalized()
    palm = neutral.col[0].normalized()        # the left palm: the hand's +x
    up = Matrix.Rotation(math.radians(LEFT_PALM_TILT), 3, gun.col[0].to_3d()) @ gun.col[2].to_3d()  # tilted to the right
    wanted = (up - along * up.dot(along)).normalized()
    angle = palm.angle(wanted, 0.0)
    if palm.cross(wanted).dot(along) < 0.0:
        angle = -angle
    turn = max(-math.radians(2.0 * LEFT_TWIST), min(math.radians(2.0 * LEFT_TWIST), angle))
    rotation = Matrix.Rotation(turn, 3, along) @ neutral
    hand = rotation.to_4x4()
    support = gun @ w["Support"]
    hand.translation = support - rotation.col[1] * 0.07 - rotation.col[0] * palm_out
    return hand, math.degrees(turn)


def roll_forearm(pose, armature, degrees):
    """A local pose (matrix_basis) with the left forearm rolled about its own length by degrees and the hand turned back
    as much, so the hand stays where it is: half the hand's turn goes up the forearm instead of all at the wrist"""
    bones = armature.data.bones
    rest = bones["lowerarm_l"].matrix_local.inverted() @ bones["hand_l"].matrix_local  # the hand's rest in the forearm's
    roll = Matrix.Rotation(math.radians(degrees), 4, "Y")
    pose = dict(pose)
    pose["lowerarm_l"] = pose["lowerarm_l"] @ roll
    pose["hand_l"] = rest.inverted() @ roll.inverted() @ rest @ pose["hand_l"]
    return pose


# ---------------------------------------------------------------------------------------------------------------------
# The gun against the body

def shoulder_pocket(rig):
    # In front of the right shoulder joint, toward the chest
    shoulder = rig.bone("upperarm_r").translation
    return shoulder + FORWARD * 0.07 + LEFT * 0.07 + UP * 0.05


def right_eye(rig):
    # The head bone's y is up the head, its z forward, its x to the character's left
    head = rig.bone("Head")
    x, y, z = (head.col[i].to_3d() for i in range(3))
    return head.translation + y * 0.085 + z * 0.075 - x * 0.033


def gun_at_shoulder(w, rig, eye_above=0.0):
    """The gun's frame in armature space, pointing level and forward: the sight line at the right eye (its height and
    side; eye_above meters under it), the butt in the shoulder pocket (how far forward)"""
    frame = frame_from_axes(FORWARD, LEFT, UP, Vector())
    by_eye = right_eye(rig) - frame.to_3x3() @ (w["Eye"] + Vector((0.0, 0.0, eye_above)))
    by_butt = shoulder_pocket(rig) - frame.to_3x3() @ w["Butt"]
    frame.translation = Vector((by_eye.x, by_butt.y, by_eye.z))
    if rig.report:
        print(f"  the butt is {(by_eye - by_butt) * 100} cm from the shoulder pocket (x, z count)")
    return frame


# The pistol (Phase 2.4): both arms reach forward, nearly straight, the rear sight PISTOL_REACH in front of the right
# eye on its line (level, the eye's height and side), so the eye looks along the sights
PISTOL_BELOW_EYE = 0.12  # meters the sight line is below the right eye (the arms reach the pistol with the wrists nearly straight)
PISTOL_REACH = 0.45
PISTOL_CUP_FORWARD = Vector((0.04, 0.0, 0.008))  # meters (the gun's frame) the left hand is moved from the library's
                                                # place (behind the grip: its pistol is smaller)
# The pistol's shoulders rolled forward (an isosceles stance: the arms reach out from shoulders brought forward; with
# the relaxed standing pose's the right shoulder was 26 cm behind the eye and the arms couldn't reach a pistol held out
# with the wrists straight): each collarbone turned forward about the vertical
PISTOL_PROTRACT = 24.0
PISTOL_VIEW_SHOULDER_DROP = 35.0  # degrees the first person's right collarbone turns down (the _View clips)
PISTOL_VIEW_PROTRACT = 10.0       # and forward, more than PISTOL_PROTRACT (the arm at full stretch: room for the drop)
# One hand (the user, 2026-11-21: the two-handed grip's hands lay open on the grip, a finger up beside the slide; "you
# can make it hold and fire with single hand, the right one"): the right hand a fist round the grip as the rifle's and
# the knife's (the thumb round the grip's back, checked on the pistol's surface; the index finger on the trigger), the
# arm reaching straight out, the right shoulder a little forward; the left arm hangs at the side (the reload: it brings
# the magazine up from the belt and goes back down)
PISTOL_ONE_HAND = True
PISTOL_ONE_HAND_TWIST = 15.0    # degrees the chest turns (positive: the right shoulder forward; the head stays ahead)
PISTOL_LEFT_RELAX = 0.45        # of a fist the hanging left hand's fingers curl
PISTOL_ONE_HAND_RIGHT = 0.09    # meters right of the right eye the pistol is (toward the shoulder: the arm points ahead)
PISTOL_ONE_HAND_REACH = 0.52    # meters ahead of the eye (the arm nearly straight)
# The right hand posed by hand (the user, 2026-11-21: automatic fits never held this pistol's grip; the index finger's
# knuckle ended under the trigger guard's bar once the palm cleared the slide's overhang): where the hand is on the
# grip and how far each joint bends are set here, judged on the close-up renders (--preview: right_hand_*), the skin
# check reporting any part of the hand in the pistol
ONE_HAND_PUSH = Vector((0.004, -0.017, -0.015))  # meters the hand from the rifle rule's place (the gun's frame: along, left, up)
ONE_HAND_PITCH = -12.0                        # degrees the hand tilts in the palm's plane (about the gun's side axis)
ONE_HAND_FINGERS = {                         # each joint's turn, as a fraction of the library's fist's (root first)
    "middle": (1.05, 1.0, 0.8),
    "ring": (1.05, 1.0, 0.8),
    "pinky": (1.05, 1.0, 0.8),
    "index": (0.15, 1.1, 0.6),
    "thumb": (0.9, 0.6, 0.4),
}
ONE_HAND_PALM_PRESS = 0.0035   # meters the heel of the palm may press into the grip (where the curled little and ring
                                # fingers bend the skin; under the hand, unseen: moving the hand didn't change it)
ONE_HAND_INDEX_SPREAD = -20.0                  # degrees the index finger spreads in the palm's plane (toward the trigger)
PISTOL_ONE_HAND_STRETCH = 1.0   # of the arm's length the hand may be from the shoulder at the most (the level aim's)
PISTOL_TWIST = 0.0        # degrees the chest turns (a little: the stance is square to the target, the arms in a narrow
                           # triangle; the rifle's -30 is a bladed stance)
PISTOL_HEAD_DOWN = 4.0     # degrees the head bends down toward the sights (the rifle's 20 lay the cheek on the stock)
# The ready (sprinting, the low ready's place): the pistol in to the chest, the muzzle down ahead
PISTOL_READY = (0.33, -0.13, 15.0)  # meters forward of the chest bone and down, degrees the muzzle down


# The left hand on the pistol (the user's photographs of a thumbs-forward grip): its palm flat on the grip's left panel,
# over the right hand's fingertips, the wrist canted so the fingers point forward and down round the front of the grip,
# under the trigger guard (they close round it over the right fingers); the thumb forward along the frame
PISTOL_LEFT_CANT = 42.0          # degrees the left fingers point down from along the gun
PISTOL_LEFT_PALM = Vector((-0.012, 0.0, -0.033))  # the palm's middle from the grip's middle (meters: back, -, down)
PISTOL_LEFT_OVER = 0.016         # meters the palm lies out from the grip's left side (the right fingertips under it)


def pistol_left_hand(w):
    """The left hand in the pistol's frame (the hand's +x is its palm, +y its fingers; the origin the wrist)"""
    width = w["GripSection"][0]
    x = Vector((0.0, -1.0, 0.0))  # the palm toward the grip's left side
    cant = math.radians(PISTOL_LEFT_CANT)
    y = Vector((math.cos(cant), 0.0, -math.sin(cant)))
    hand = frame_from_axes(x, y, x.cross(y), Vector())
    palm = w["Grip"] + PISTOL_LEFT_PALM + Vector((0.0, width + PISTOL_LEFT_OVER + w.get("CupOut", 0.0), 0.0))
    hand.translation = palm - y * 0.07 - x * 0.03
    return hand


def protract(rig, w, clip):
    """The pistol's collarbones turned forward about the vertical (each toward the front), on the pose the rig has"""
    if w["Hold"] != "Pistol" or clip.low_ready:
        return
    for side, sign in ((("r", 1.0),) if PISTOL_ONE_HAND else (("r", 1.0), ("l", -1.0))):
        pb = rig.armature.pose.bones[f"clavicle_{side}"]
        m = pb.matrix.copy()
        extra = PISTOL_VIEW_PROTRACT if clip.shoulder_drop and side == "r" else 0.0
        turn = Quaternion(UP, math.radians((PISTOL_PROTRACT + extra) * sign))
        pb.matrix = Matrix.Translation(m.translation) @ (turn @ m.to_quaternion()).to_matrix().to_4x4()
        rig.update()
    if clip.shoulder_drop:
        # The right shoulder down (the first person's clips: the arm then rises to the pistol from lower in the view):
        # the collarbone turned about the forward axis, the way that lowers the shoulder
        pb = rig.armature.pose.bones["clavicle_r"]
        m = pb.matrix.copy()
        before = rig.bone("upperarm_r").translation.z
        for sign in (1.0, -1.0):
            turn = Quaternion(FORWARD, math.radians(clip.shoulder_drop * sign))
            pb.matrix = Matrix.Translation(m.translation) @ (turn @ m.to_quaternion()).to_matrix().to_4x4()
            rig.update()
            if rig.bone("upperarm_r").translation.z < before:
                break


def gun_in_front(w, rig):
    """The pistol's frame in armature space, level and forward: its rear sight on the right eye's line, PISTOL_REACH ahead"""
    frame = frame_from_axes(FORWARD, LEFT, UP, Vector())
    reach, right = (PISTOL_ONE_HAND_REACH, PISTOL_ONE_HAND_RIGHT) if PISTOL_ONE_HAND else (PISTOL_REACH, 0.0)
    frame.translation = right_eye(rig) + FORWARD * reach - LEFT * right - UP * PISTOL_BELOW_EYE - frame.to_3x3() @ w["Eye"]
    return frame


def pistol_ready(w, rig):
    """The pistol at the chest, its muzzle down ahead (the grip where the frame's origin isn't)"""
    forward, down, degrees = PISTOL_READY
    rotation = frame_from_axes(FORWARD, LEFT, UP, Vector()) @ rot("Y", degrees)
    grip_at = rig.bone("spine_03").translation + FORWARD * forward + UP * down + LEFT * -0.03
    return Matrix.Translation(grip_at - rotation.to_3x3() @ w["Grip"]) @ rotation


def knife_frame(position, forearm, blade):
    """The knife's frame in armature space, its handle's middle at position. Held as a pistol grip is, the frame's x is
    the forearm's way (the hand lies along it: right_hand_on_grip) and the blade goes up the fist, near square to it:
    forearm the forearm's way, blade the blade's (made square to the forearm; the handle's slant leans it forward)"""
    x = forearm.normalized()
    z = (blade - x * blade.dot(x)).normalized()
    m = frame_from_axes(x, z.cross(x), z, Vector())
    m.translation = position - m.to_3x3() @ knife_point(0.0)
    return m


def path_at(keys, time):
    """keys: [(time, Matrix)]: the place on a Catmull-Rom curve through the keys' (still at the first and last), the turn
    slerped, eased only into the first and out of the last (a slash doesn't stop halfway)"""
    if time <= keys[0][0]:
        return keys[0][1].copy()
    if time >= keys[-1][0]:
        return keys[-1][1].copy()
    i = max(k for k in range(len(keys) - 1) if keys[k][0] <= time)
    (t0, m0), (t1, m1) = keys[i], keys[i + 1]
    u = (time - t0) / (t1 - t0)
    p = [k[1].translation for k in keys]
    tangent0 = Vector() if i == 0 else (p[i + 1] - p[i - 1]) / (t1 - keys[i - 1][0]) * (t1 - t0)
    tangent1 = Vector() if i + 2 >= len(keys) else (p[i + 2] - p[i]) / (keys[i + 2][0] - t0) * (t1 - t0)
    u2, u3 = u * u, u * u * u
    place = (2 * u3 - 3 * u2 + 1) * p[i] + (u3 - 2 * u2 + u) * tangent0 + (-2 * u3 + 3 * u2) * p[i + 1] + (u3 - u2) * tangent1
    w = smooth(u) if i == 0 or i + 2 >= len(keys) else u
    q0, q1 = m0.to_quaternion(), m1.to_quaternion()
    if q0.dot(q1) < 0.0:
        q1 = -q1
    m = q0.slerp(q1, w).to_matrix().to_4x4()
    m.translation = place
    return m


# ---------------------------------------------------------------------------------------------------------------------
# A clip: a base pose from the library (legs, spine, breathing), the extra bends, the gun and the hands per frame

class Clip:
    def __init__(self, name, base, frames=None, aim_pitch=0.0, low_ready=False):
        self.name = name
        self.base = base             # the library's clip the body comes from
        self.frames = frames         # None: as many as the base
        self.aim_pitch = aim_pitch   # degrees up: the spine bends, the gun follows the chest
        self.low_ready = low_ready
        self.arms_down = 0.0         # degrees the arms and the gun swing down about the shoulders (the spine stays upright)
        self.gun_motion = []         # [(time, Matrix)]: the gun's motion in its own frame (kick, a tilt for the reload)
        self.left = None             # [(time, Matrix)] the left hand in the gun's frame, None: on the support
        self.right = None            # the same for the right hand, None: on the grip
        self.left_world = {}         # the left hand's trips to the belt pouch: {"from", "pouch", "to"[, "again"]} times
        self.left_after = None       # [(time, Matrix)] the left hand in the gun's frame after the first trip
        self.weapon = None
        self.left_free = False        # the left hand leaves the gun (the belt pouch): its elbow rules don't hold
        self.below_hand_margin = 0.0  # how far (m) an elbow may be above its hand: the gun aimed down lowers the hands
        self.twist = CHEST_TWIST      # degrees the chest turns aiming (the pistol's less)
        self.head_down = 20.0         # degrees the head bends down toward the gun aiming
        self.twist_keys = None        # [(time, degrees)]: the chest's turn through the clip (the knife's slash)
        self.knife_keys = None        # [(time, Matrix)]: the knife's frame (armature space, at the first frame's chest)
        self.left_chest = None        # the left hand off the weapon (a guard), armature space at the first frame's chest
        self.left_hang = False        # the left arm hanging at the side (the one-handed pistol): left_chest from the rig
        self.left_mag = None          # the one-handed reload: the left hand under the magazine (the gun's frame)
        self.left_trip = None         # [(time, "side" | "pouch" | "mag_low" | "mag")]: its trip
        self.check_elbows = True      # the elbows' rules (below the hand, outside the shoulder): not for a slash
        self.shoulder_drop = 0.0      # degrees the right collarbone turns down (the first person's pistol clips: _View)
        # The sniper's bolt and reload (the right hand leaves the grip for the bolt): the seconds the gun is held by the
        # left hand (on the forend) and by the right (on the grip); the game shows its model on that hand's bone then
        # (Sniper.weapon.json's LeftHand). Measured: where the gun is in each hand over those frames
        self.left_holds = None
        self.right_holds = None
        self.held_in = {"l": [], "r": []}
        self.held_mean = {}
        self.problem = None           # the first rule the clip broke (the run fails after its previews)


def pistol_clips(weapon):
    w = WEAPONS[weapon]
    # The ready (sprinting forward): the aiming stance, upright, with only the arms and the pistol swung down 35 degrees
    # about the shoulders, a low ready (the user, 2026-11-21: pitching the whole aim down hunched the run over; held in
    # to the chest the wrist bent so far that the palm's skin went into the grip)
    ready = Clip(f"{weapon}_Idle_Loop", "Pistol_Aim_Neutral")
    ready.arms_down = 35.0
    ready.below_hand_margin = 0.25
    clips = []
    for name, pitch in (("Neutral", 0.0), ("Up", 45.0), ("Down", -45.0)):
        clips.append(Clip(f"{weapon}_Aim_{name}", "Pistol_Aim_Neutral", aim_pitch=pitch))
        clips[-1].below_hand_margin = 0.25 if pitch < 0.0 else 0.0  # (aimed down the nearly straight arms point down: the elbows above the hands)
    # (the level aim first: it settles the hands on the pistol, which every pistol clip then holds the same way, as the
    #  engine does: the pistol on the hand's bone at one offset)
    clips.insert(1, ready)
    shoot = Clip(f"{weapon}_Shoot", "Pistol_Aim_Neutral", frames=int(FPS * 0.3))
    shoot.gun_motion = [(0.0, Matrix.Identity(4)), (0.035, Matrix.Translation((-0.025, 0, 0.006)) @ rot("Y", -9.0)),
                        (0.24, Matrix.Identity(4))]
    clips.append(shoot)
    # The reload: the pistol in toward the chest and up, rolled to show its left side; the left hand to the belt for a
    # new magazine, pushed up into the grip from below (its palm up under the base), then back round the right hand
    reload = Clip(f"{weapon}_Reload", "Pistol_Aim_Neutral", frames=int(FPS * 1.8))
    reload.left_free = True
    tilt = Matrix.Translation((-0.17, 0.03, 0.07)) @ rot("X", -18.0) @ rot("Y", -14.0)
    reload.gun_motion = [(0.0, Matrix.Identity(4)), (0.25, tilt), (1.5, tilt), (1.8, Matrix.Identity(4))]
    cup = support_hand(weapon)
    mag = left_hand_under(w["Magazine"] + Vector((0.0, 0.0, -0.012)))
    mag_low = Matrix.Translation((0.0, 0.0, -0.10)) @ mag
    reload.left = [(0.0, cup), (0.3, cup)]
    reload.left_world = {"from": 0.3, "pouch": 0.68, "to": 1.05}
    reload.left_after = [(1.05, mag_low), (1.2, mag), (1.3, mag), (1.62, cup)]
    clips.append(reload)
    for clip in clips:
        clip.twist = PISTOL_TWIST
        clip.head_down = PISTOL_HEAD_DOWN
        if PISTOL_ONE_HAND:
            clip.twist = PISTOL_ONE_HAND_TWIST
            clip.left_hang = True      # the left arm at the side (left_hang_frame), its hand relaxed
            clip.below_hand_margin = max(clip.below_hand_margin, 0.02)  # (the arm nearly straight: the elbow level with the hand)
            clip.left_free = True      # (its elbow's rules are a gun-holding arm's)
            clip.left = None
            clip.left_world = {}
            clip.left_after = None
    if PISTOL_ONE_HAND:
        # The reload's left hand: from the side down to the belt pouch, up under the grip with the magazine, pushed in,
        # back down to the side
        reload.left_mag = mag
        reload.left_trip = [(0.25, "side"), (0.6, "pouch"), (0.95, "mag_low"), (1.1, "mag"), (1.25, "mag"), (1.6, "side")]
    # The first person's (the view model's) aim and shot: the same, the right shoulder dropped and forward (the user,
    # 2026-11-22: down the sights the arm, cut off at the screen's edge, took its whole bottom right quarter). The pistol
    # is where the eye puts it, as in the others (the weapon file's AimEye and SightDirection from the _View aim); the arm
    # comes up to it from lower. Not the reload: pulled in to the chest, the elbow rose over the shoulder-hand line (the
    # first person crossfades into the body's reload)
    for clip in [c for c in clips if c.name in (f"{weapon}_Aim_Neutral", f"{weapon}_Shoot")]:
        view = copy.copy(clip)
        view.name = clip.name + "_View"
        view.shoulder_drop = PISTOL_VIEW_SHOULDER_DROP
        clips.append(view)
    return clips


# The knife (Phase 2.4): the handle's middle (armature space, the Idle pose's chest), the blade's way and the edge's
# The left fist low in front of the belly, in to the body (in first person the knife shows, the fist stays below the view)
KNIFE_GUARD_LEFT = (Vector((0.10, -0.22, 1.10)), Vector((0.0, -0.7, 0.7)))  # the left fist: where, the knuckles' way


def knife_clips(weapon):
    # The handle's middle, the forearm's way, the blade's (armature space: x left, -y forward, z up). The ready: the
    # forearm forward and down, the blade forward and up. The windup: the hand back by the right shoulder, the forearm up
    # and forward, the blade forward. The slash: the hand sweeps across the chest to the left, the forearm turning to
    # point left, the blade forward; the follow-through low at the left hip. Within the arm's reach as the chest turns
    idle = knife_frame(Vector((-0.15, -0.42, 1.22)), Vector((0.05, -0.95, -0.15)), Vector((0.05, -0.45, 0.90)))
    windup = knife_frame(Vector((-0.30, -0.24, 1.40)), Vector((0.10, -0.40, 0.90)), Vector((0.45, -0.85, 0.0)))
    middle = knife_frame(Vector((-0.05, -0.44, 1.25)), Vector((0.40, -0.90, -0.05)), Vector((0.30, -0.60, 0.75)))
    end = knife_frame(Vector((0.12, -0.36, 1.10)), Vector((0.75, -0.60, -0.25)), Vector((0.20, -0.65, 0.70)))
    hold = Clip(f"{weapon}_Idle_Loop", "Idle_Loop")
    hold.knife_keys = [(0.0, idle)]
    hold.below_hand_margin = 0.05  # (the hand at the hip, the forearm level: the elbow as high as the hand)
    slash = Clip(f"{weapon}_Slash", "Idle_Loop", frames=int(FPS * 0.55))
    slash.knife_keys = [(0.0, idle), (0.13, windup), (0.21, middle), (0.29, end), (0.55, idle)]
    # The chest turns with the arm: the right shoulder back in the windup, round to the left through the slash
    slash.twist_keys = [(0.0, 0.0), (0.13, -22.0), (0.21, 4.0), (0.29, 26.0), (0.55, 0.0)]
    slash.check_elbows = False
    clips = [hold, slash]
    for clip in clips:
        place, knuckles = KNIFE_GUARD_LEFT
        y = knuckles.normalized()
        x = (-LEFT - y * (-LEFT).dot(y)).normalized()  # the left palm (its +x) toward the body's middle
        clip.left_chest = frame_from_axes(x, y, x.cross(y), place)
        clip.twist = 0.0
        clip.head_down = 0.0
    return clips


def support_hand(weapon):
    """The left hand on the weapon in its frame: under the rifle's handguard, round the right hand on the pistol's grip"""
    w = WEAPONS[weapon]
    if w["Hold"] == "Pistol":
        return pistol_left_hand(w)
    return left_hand_under(w["Support"])


def make_clips(weapon):
    w = WEAPONS[weapon]
    if w["Hold"] == "Pistol":
        return pistol_clips(weapon)
    if w["Hold"] == "Knife":
        return knife_clips(weapon)
    clips = []

    clips.append(Clip(f"{weapon}_Idle_Loop", "Idle_Loop", low_ready=True))
    if weapon == "Sniper":
        # (its long forend low: the left arm reaches down along it, the elbow a little above the wrist)
        clips[-1].below_hand_margin = 0.05
    for name, pitch in (("Up", 45.0), ("Neutral", 0.0), ("Down", -45.0)):
        clips.append(Clip(f"{weapon}_Aim_{name}", "Pistol_Aim_Neutral", aim_pitch=pitch))
        # Aimed down the left arm reaches nearly straight down the gun: its elbow above the wrist (under the handguard,
        # the palm up), with the gun forward and lower for the face 10.4 cm (rifle) and 12.4 cm (sniper: its forend deeper)
        clips[-1].below_hand_margin = 0.13 if pitch < 0.0 else 0.0

    shoot = Clip(f"{weapon}_Shoot", "Pistol_Aim_Neutral", frames=int(FPS * (0.35 if weapon == "Rifle" else 0.6)))
    kick = 0.035 if weapon == "Rifle" else 0.06
    rise = 4.0 if weapon == "Rifle" else 8.0
    shoot.gun_motion = [(0.0, Matrix.Identity(4)),
                        (0.04, Matrix.Translation((-kick, 0, 0)) @ rot("Y", -rise)),
                        (0.3 if weapon == "Rifle" else 0.5, Matrix.Identity(4))]
    clips.append(shoot)

    support = left_hand_under(w["Support"])
    grip = right_hand_on_grip(w)
    # Rolled to show the magazine side, toward the middle of the body and down (3 cm in: 5 brought the gun within 3 cm of
    # the face; the sniper 6 cm down, not 4, for its scope's eyepiece, which the tilt lifts toward the eye)
    tilt = Matrix.Translation((0.0, 0.03, -0.04 if weapon == "Rifle" else -0.06)) @ rot("X", -25.0) @ rot("Y", 12.0)
    if weapon == "Rifle":
        reload = Clip("Rifle_Reload", "Pistol_Aim_Neutral", frames=int(FPS * 2.2))
        reload.left_free = True
        reload.gun_motion = [(0.0, Matrix.Identity(4)), (0.3, tilt), (1.9, tilt), (2.2, Matrix.Identity(4))]
        mag = left_hand_on(w["Magazine"])
        mag_out = Matrix.Translation((0.0, 0.0, -0.12)) @ mag
        reload.left = [(0.0, support), (0.35, mag), (0.55, mag_out)]
        # 0.55 to 1.35: down to the belt and back with a new magazine (in armature space), then in and back
        reload.left_world = {"from": 0.55, "pouch": 0.95, "to": 1.35}
        reload.left_after = [(1.35, mag_out), (1.55, mag), (1.7, mag), (2.05, support)]
        clips.append(reload)
    else:
        bolt = w["Bolt"]
        knob = right_hand_on_bolt(bolt)
        up = Matrix.Translation(bolt) @ rot("X", -60.0) @ Matrix.Translation(-bolt) @ knob  # the handle lifted
        back = Matrix.Translation((-0.09, 0.0, 0.0)) @ up                                  # and pulled back

        cycle = Clip("Sniper_Bolt", "Pistol_Aim_Neutral", frames=int(FPS * 1.1))
        # Not rolled toward the bolt (it was 6 degrees): the left hand, which holds the gun meanwhile (the game's model on
        # its bone), followed the roll and moved 12 mm on the forend. The right hand rests on the grip a moment before it
        # leaves and after it is back: the game switches the hand there
        # (both hands hold it a tenth of a second either side of the game's switches, 0.05 and 1.01 s: the animation's
        # frame and the fixed step's time differ by up to a frame)
        # (the switch at 0.11 s comes after the game's 0.08 s crossfade from the shot's clip, the gun settled; at 0.05 it
        # was inside it, 7 mm and 2 degrees off)
        cycle.right = [(0.0, grip), (0.15, grip), (0.26, knob), (0.38, up), (0.5, back), (0.62, up), (0.74, knob),
                       (0.88, grip), (1.1, grip)]
        cycle.left_holds = [(0.0, 1.1)]
        cycle.right_holds = [(0.0, 0.15), (0.88, 1.1)]
        clips.append(cycle)

        # The reload: one hand always holds the gun (the game's model is on a hand's bone). The right hand opens the bolt
        # while the left holds the forend, goes back to the grip while the left loads two rounds through the port, then
        # shuts the bolt while the left holds the forend again
        reload = Clip("Sniper_Reload", "Pistol_Aim_Neutral", frames=int(FPS * 2.6))
        reload.left_free = True
        reload.gun_motion = [(0.0, Matrix.Identity(4)), (0.3, tilt), (2.3, tilt), (2.6, Matrix.Identity(4))]
        # (both hands hold it a tenth of a second either side of the game's switches: 0.05, 0.65, 1.85 and 2.52 s)
        # (the right hand on the grip until 0.2 s: the game's 0.1 s crossfade into the clip ends before the switch)
        reload.right = [(0.0, grip), (0.2, grip), (0.28, knob), (0.36, up), (0.45, back), (0.55, grip), (1.95, grip),
                        (2.08, back), (2.2, up), (2.32, knob), (2.42, grip), (2.6, grip)]
        port = left_hand_on(w["Magazine"])
        reload.left = [(0.0, support), (0.75, support)]
        reload.left_world = {"from": 0.75, "pouch": 0.95, "to": 1.15, "again": (1.35, 1.52, 1.65)}
        reload.left_after = [(1.15, port), (1.3, port), (1.65, port), (1.68, port), (1.75, support)]
        reload.left_holds = [(0.0, 0.75), (1.75, 2.6)]
        reload.right_holds = [(0.0, 0.2), (0.55, 1.95), (2.42, 2.6)]
        clips.append(reload)
    return clips


# ---------------------------------------------------------------------------------------------------------------------
# Baking

def left_hang_frame(rig):
    """The left hand hanging at the side, a little forward and out of the thigh: the wrist an arm's length (a little
    bent) below the shoulder, the palm toward the thigh, the fingers down"""
    bones = rig.armature.data.bones
    arm = (bones["lowerarm_l"].head_local - bones["upperarm_l"].head_local).length + \
        (bones["hand_l"].head_local - bones["lowerarm_l"].head_local).length
    shoulder = rig.bone("upperarm_l").translation
    wrist = shoulder - UP * arm * 0.95 + FORWARD * 0.06 + LEFT * 0.05
    x = LEFT * -1.0                                   # the palm toward the body
    y = (UP * -1.0 + FORWARD * 0.12).normalized()     # the fingers down, a little forward
    y = (y - x * y.dot(x)).normalized()
    return frame_from_axes(x, y, x.cross(y), wrist)


def belt_pouch(rig):
    pelvis = rig.bone("pelvis").translation
    return pelvis + LEFT * 0.14 + FORWARD * 0.08 + UP * 0.05


def pouch_hand(rig):
    hand = frame_from_axes(UP * -1.0, FORWARD, LEFT * -1.0, belt_pouch(rig))  # fingers forward, palm to the body
    return hand


def left_hand_at(clip, time, gun, rig):
    """The left hand in armature space"""
    support = gun @ support_hand(clip.weapon)
    if clip.left is None:
        return support
    lw = clip.left_world
    if lw and time >= lw["from"]:
        if time <= lw["to"] or ("again" in lw and lw["to"] < time <= lw["again"][2]):
            # Down to the pouch and back up to the gun (a magazine, a round)
            a, p, b = (lw["from"], lw["pouch"], lw["to"])
            if time > lw["to"]:
                a, p, b = lw["again"]
            at_gun_from = gun @ keyed(clip.left, a) if a <= clip.left[-1][0] else gun @ keyed(clip.left_after, a)
            at_gun_to = gun @ keyed(clip.left_after, b)
            pouch = pouch_hand(rig)
            if time <= p:
                return lerp_matrix(at_gun_from, pouch, smooth((time - a) / (p - a)))
            return lerp_matrix(pouch, at_gun_to, smooth((time - p) / (b - p)))
        return gun @ keyed(clip.left_after, time)
    return gun @ keyed(clip.left, time)


# An arm holding a gun hangs from its elbow: the elbow is below the hand and outside the shoulder (checked in every
# frame, but for the left arm on its way to the belt pouch, and a few centimeters for the hands of the gun aimed down),
# and never above the line from the shoulder to the hand
ELBOW_MIN_DROP = 0.0  # never above (a hand down at the belt pouch hangs the arm nearly straight: about 1 cm)


def elbow_drop(rig, side):
    """How far the elbow is below the line from the shoulder to the hand (meters; negative: above it)"""
    shoulder = rig.bone(f"upperarm_{side}").translation
    elbow = rig.bone(f"lowerarm_{side}").translation
    hand = rig.bone(f"hand_{side}").translation
    arm = hand - shoulder
    t = max(0.0, min(1.0, (elbow - shoulder).dot(arm) / arm.length_squared))
    return shoulder.lerp(hand, t).z - elbow.z


def elbow_off_target(rig, side):
    """The angle (degrees) between where the elbow points and where its target is, around the shoulder-hand line"""
    shoulder = rig.bone(f"upperarm_{side}").translation
    arm = rig.bone(f"hand_{side}").translation - shoulder

    def across(point):
        v = point - shoulder
        return v - arm * (v.dot(arm) / arm.length_squared)

    elbow = across(rig.bone(f"lowerarm_{side}").translation)
    target = across(rig.pole[side].matrix_world.translation)
    return math.degrees(elbow.angle(target, math.pi))


def aim_elbows(rig):
    """Turns each IK pole angle until the elbow points at its target. The angle that does depends on the pose (the
    shoulder's turn): measured on a clip's first frame, kept for the clip"""
    for side in "rl":
        ik = rig.ik[side]

        def miss(degrees):
            ik.pole_angle = math.radians(degrees)
            rig.update()
            return elbow_off_target(rig, side)

        best = min(range(-180, 180, 10), key=miss)
        best = min((best + d for d in range(-10, 11)), key=miss)
        miss(best)


def elbow_place(rig, side):
    """How far the elbow is below the hand and outside the shoulder (meters; negative: above, inside)"""
    shoulder = rig.bone(f"upperarm_{side}").translation
    elbow = rig.bone(f"lowerarm_{side}").translation
    hand = rig.bone(f"hand_{side}").translation
    outward = LEFT * -1.0 if side == "r" else LEFT
    return hand.z - elbow.z, (elbow - shoulder).dot(outward)


# Aiming, the chest turns further sideways than the pistol stance's (a bladed stance): the left shoulder comes forward
# toward the handguard, so the left arm bends and its elbow can hang out and down. The head turns back to face the gun.
CHEST_TWIST = -30.0  # degrees (negative: the right shoulder further back)
# The face kept off the gun (the user, 2026-11-19: the chin and mouth lay on the receiver; a few centimeters between
# them): the gun forward of where the butt meets the shoulder pocket and lower than the sight line at the eye (meters).
# Measured on the dressed body's head: 3.2 cm at the closest in the level aim. A neck leant back, a head tilted less, the
# gun moved right didn't help: the gun follows the eye, and the chin is beside the receiver's top
GUN_FORWARD = 0.065
EYE_ABOVE = 0.055
FACE_CLEARANCE = 0.03       # less than this between the face and the gun in any frame of a clip aiming: the bake fails
FACE_LOW_READY_CLEARANCE = 0.01  # the rifles' low ready's (the muzzle down, the stock under the chin): off the face
FACE_KICK_CLEARANCE = 0.01  # the same while the gun kicks back (a shot: for two frames)


TWIST_BONES = SPINE  # the turn spread over the whole spine (on two bones it corkscrewed the chest)


def twist_chest(pose, twist=CHEST_TWIST):
    turn = Matrix.Rotation(math.radians(twist / len(TWIST_BONES)), 4, "Y")
    for name in TWIST_BONES:
        pose[name] = pose[name] @ turn
    pose["neck_01"] = pose["neck_01"] @ Matrix.Rotation(math.radians(-twist), 4, "Y")


IDLE = {}  # Idle_Loop's first frame (main): the relaxed standing pose


# The aim poses' back and shoulders (the user, 2026-11-20: unnaturally bent, not a person's back and shoulders): the
# base clip is the library's pistol stance, whose arms reach straight out, so its collarbones shrugged the shoulders up
# and forward and its spine rounded the back; with the chest twisted 30 degrees on the forward-facing hips the right
# shoulder made a hump on the back. The collarbones, the spine, the neck and the head are the relaxed standing pose's
# (Idle_Loop) instead, the chest's turn spread over the three spine bones (TWIST_BONES): a straight back, level
# shoulders, the head up; the right elbow a little out and forward (ELBOW_TARGET), not folded back along the side
POSTURE_BONES = ["clavicle_l", "clavicle_r"] + SPINE + ["neck_01", "Head"]


def aim_posture(pose):
    """The aim's back and shoulders: the pistol stance's collarbones, spine, neck and head replaced by the relaxed
    standing pose's"""
    for name in POSTURE_BONES:
        pose[name] = IDLE[name].copy()


def bake_clip(rig, clip, base_frames, finger_pose, gun_frame):
    armature = rig.armature
    w = WEAPONS[clip.weapon]
    count = clip.frames or len(base_frames)
    # The head down toward the gun (the rifle's stock: to the right too)
    head_bend = Matrix.Rotation(math.radians(clip.head_down), 4, "X") @ \
        Matrix.Rotation(math.radians(8.0 if w["Hold"] == "Rifle" else 0.0), 4, "Z")
    knife = clip.knife_keys is not None

    # The gun's place on the chest, from the first frame's pose
    def body(f):
        pose = dict(base_frames[min(f, len(base_frames) - 1)])
        pose.update(finger_pose)
        if clip.twist_keys:
            twist_chest(pose, keyed(clip.twist_keys, f / FPS))
        if not clip.low_ready and not knife:
            aim_posture(pose)
            bend = Matrix.Rotation(math.radians(-clip.aim_pitch * 1.13 / 3.0), 4, "X")  # +x leans forward; 1.13: the bones' axes are twisted, so the gun turned 40 of 45 degrees
            for name in SPINE:
                pose[name] = pose[name] @ bend
            twist_chest(pose, clip.twist)
            pose["Head"] = pose["Head"] @ head_bend
        return pose

    rig.set_pose(body(0))
    rig.update()
    protract(rig, w, clip)
    if knife:
        placed = clip.knife_keys[0][1]  # (the knife's frame comes from its keys each frame)
    elif clip.low_ready and w["Hold"] == "Pistol":
        placed = pistol_ready(w, rig)
    elif clip.low_ready:
        at_shoulder = gun_at_shoulder(w, rig)
        pivot = at_shoulder @ Matrix.Translation(w["Butt"])
        down, left, lift = w["LowReady"]  # muzzle down and to the left about the butt, then up
        low = pivot @ rot("Y", down) @ rot("Z", left) @ Matrix.Translation(-w["Butt"])
        placed = low @ Matrix.Translation((0.0, 0.0, lift))
        rig.set_pose(base_frames[0])
        rig.update()
    else:
        # Level at the neutral aim, then turned with the chest's bend
        neutral_pose = dict(base_frames[0])
        neutral_pose.update(finger_pose)
        aim_posture(neutral_pose)
        twist_chest(neutral_pose, clip.twist)
        neutral_pose["Head"] = neutral_pose["Head"] @ head_bend
        rig.set_pose(neutral_pose)
        rig.update()
        protract(rig, w, clip)
        if w["Hold"] == "Pistol":
            placed = gun_in_front(w, rig)
        else:
            placed = gun_at_shoulder(w, rig, EYE_ABOVE) @ Matrix.Translation((GUN_FORWARD, 0.0, 0.0))
        chest_neutral = rig.bone("spine_03")
        rig.set_pose(body(0))
        rig.update()
        protract(rig, w, clip)
        placed = rig.bone("spine_03") @ chest_neutral.inverted() @ placed
        if clip.arms_down:
            # Swung down about the line through the shoulders (the gun's muzzle lowered, its place nearer the belt)
            pivot = (rig.bone("upperarm_r").translation + rig.bone("upperarm_l").translation) * 0.5
            along = placed.col[0].to_3d().normalized()
            lateral = along.cross(UP).normalized()
            swing = Matrix.Rotation(math.radians(-clip.arms_down), 4, lateral)
            placed = Matrix.Translation(pivot) @ swing @ Matrix.Translation(-pivot) @ placed
        if w["Hold"] == "Pistol" and PISTOL_ONE_HAND:
            # Within the straight arm's reach (the spine's bend aiming up or down carries the pistol away from the
            # shoulder): moved toward the shoulder until the hand's place is an arm's length (as the level aim's) from it
            bones = rig.armature.data.bones
            arm = (bones["lowerarm_r"].head_local - bones["upperarm_r"].head_local).length + \
                (bones["hand_r"].head_local - bones["lowerarm_r"].head_local).length
            shoulder = rig.bone("upperarm_r").translation
            for _ in range(20):
                hand_at = (placed @ right_hand_on_grip(w, 0.0)).translation
                over = (hand_at - shoulder).length - arm * PISTOL_ONE_HAND_STRETCH
                if over <= 0.001:
                    break
                placed = Matrix.Translation((shoulder - hand_at).normalized() * over) @ placed
    on_chest = rig.bone("spine_03").inverted() @ placed
    chest_first = rig.bone("spine_03")
    if clip.left_hang:
        clip.left_chest = left_hang_frame(rig)

    # (the library has its own Pistol_ clips, sampled at the start: renamed out of the way, else Blender names ours .001)
    existing = bpy.data.actions.get(clip.name)
    if existing is not None:
        existing.name = clip.name + "_Library"
    action = bpy.data.actions.new(clip.name)
    poses = []
    lowest = {"r": math.inf, "l": math.inf}
    worst_below = {"r": math.inf, "l": math.inf}
    worst_outside = {"r": math.inf, "l": math.inf}
    nearest_face = (math.inf, 0)
    for f in range(count):
        time = f / FPS
        rig.set_pose(body(f))
        rig.update()
        protract(rig, w, clip)
        motion = keyed(clip.gun_motion, time) if clip.gun_motion else Matrix.Identity(4)
        # (the knife and the left guard go with the chest as it breathes and turns, from the first frame's)
        follow = rig.bone("spine_03") @ chest_first.inverted()
        if knife:
            gun = follow @ path_at(clip.knife_keys, time)
        else:
            gun = rig.bone("spine_03") @ on_chest @ motion
        right = gun @ (keyed(clip.right, time) if clip.right else right_hand_on_grip(w, clip.wrist_turn))
        left = follow @ clip.left_chest if clip.left_chest is not None else left_hand_at(clip, time, gun, rig)
        if clip.left_trip:
            places = {"side": left, "pouch": pouch_hand(rig), "mag": gun @ clip.left_mag,
                      "mag_low": gun @ (Matrix.Translation((0.0, 0.0, -0.10)) @ clip.left_mag)}
            keys = [(0.0, "side")] + clip.left_trip
            left = places[keys[-1][1]]
            for (t0, a), (t1, b) in zip(keys, keys[1:]):
                if t0 <= time <= t1:
                    left = lerp_matrix(places[a], places[b], smooth((time - t0) / (t1 - t0)) if t1 > t0 else 1.0)
                    break

        def place(side, hand):
            rig.hand_target[side].matrix_world = armature.matrix_world @ hand
            # The elbow's target out to the side and down from the middle of its arm (the shoulder and the hand)
            shoulder = rig.bone(f"upperarm_{side}").translation
            outward = LEFT * -1.0 if side == "r" else LEFT
            middle = shoulder.lerp(hand.translation, 0.5)
            targets = PISTOL_ELBOW_TARGET if w["Hold"] != "Rifle" else ELBOW_TARGET
            out, down = targets[side] if not (side == "r" and clip.right) else targets["l"]  # off the grip: out
            if side == "l" and clip.low_ready:
                out, down = LOW_READY_ELBOW_L
            target = middle + outward * out - UP * down
            if side == "l" and clip.left_hang:
                target = middle - FORWARD * 0.4 + outward * 0.08  # (hanging: the elbow back, a little out)
            locked = w.get("ElbowInHand", {}).get(side) if w["Hold"] == "Pistol" else None
            if w["Hold"] == "Pistol" and locked is None and side == "r":
                # The level aim: the forearm along the gun (the hand lies level along it: the wrist straight), the elbow
                # behind the wrist on the gun's line, a little out and down
                along = gun.col[0].to_3d().normalized()
                target = hand.translation - along * 0.27 + outward * 0.05 - UP * 0.10
                if PISTOL_ONE_HAND:
                    target = hand.translation - along * 0.27 + outward * 0.12 - UP * 0.04  # (the arm nearly straight)
            if locked is not None and (side == "r" and clip.right is None or side == "l" and clip.left is None):
                # The pistol's wrists locked as aiming level (a bent wrist's skin went into the grip): the elbow toward
                # where the level aim has it from its hand
                target = hand @ locked
            rig.pole[side].matrix_world = Matrix.Translation(armature.matrix_world @ target)

        place("r", right)
        place("l", left)
        if f == 0:
            aim_elbows(rig)
        rig.update()
        if f == 0 and clip.right is None and rig.chest:
            # The right hand turned out about its knuckle just as far as keeps the wrist out of the chest (kept for the clip)
            for step in range(int(WRIST_TURN_MAX / 2.5) + 1):
                clip.wrist_turn = step * 2.5
                right = gun @ right_hand_on_grip(w, clip.wrist_turn)
                place("r", right)
                rig.update()
                out = rig.chest.clearance(rig.bone("hand_r").translation)
                if out >= CHEST_CLEARANCE:
                    break
            print(f"  {clip.name}: the right hand turned out {clip.wrist_turn:.1f} degrees, the wrist {out * 100:.1f} cm out "
                  f"of the chest")
        def pistol_hands(f):
            """The pistol's thumbs (searched on the first frame, kept for the clip), and on the first frame the hands'
            skin out of the gun: a palm in it moves the left hand out, a finger in it closes further out"""
            gun_frame.matrix_world = armature.matrix_world @ gun
            rig.update()
            left_on = clip.left is None
            settled = w.get("Settled")
            if f == 0:
                shoulder = rig.bone("upperarm_r").translation
                miss = (rig.bone("hand_r").translation - rig.hand_target["r"].matrix_world.translation).length
                print(f"  {clip.name}: the right shoulder {(right_eye(rig) - shoulder).dot(FORWARD) * 100:.1f} cm behind the eye; "
                      f"the hand {(rig.hand_target['r'].matrix_world.translation - shoulder).length * 100:.1f} cm from it, "
                      f"{miss * 100:.1f} cm short")
            if settled is not None and clip.pistol_thumbs is None:
                # Held as the level aim settled it
                clip.pistol_thumbs = settled["thumbs"]
                clip.finger_margins.update(settled["left"])
                clip.right_margins.update(settled["right"])
                grip_right_fingers(rig, gun, w, clip.fist, clip.right_margins, gun_tree(gun_frame.children[0]))
                if left_on:
                    for name in FINGERS:
                        if name.endswith("_l"):
                            armature.pose.bones[name].matrix_basis = Matrix.Identity(4)
                    close_left_round_grip(rig, clip.fist, gun, w, clip.finger_margins)
            if clip.pistol_thumbs is None:
                right_aim, left_aim, report = place_pistol_thumbs(rig, clip.fist, gun, gun_tree(gun_frame.children[0]), left_on)
                clip.pistol_thumbs = (right_aim, left_aim)
                print(f"  {clip.name}: the right thumb round the grip to {right_aim[0] * 100:.1f} cm out, {right_aim[1] * 100:.1f} cm up, {report['right gap'] * 1000:.0f} mm "
                      f"out of the gun" + (f"; the left thumb {left_aim[0]} up, {left_aim[1]} out, {report['left gap'] * 1000:.0f} mm out "
                                           f"of the gun, {report['apart'] * 1000:.0f} mm from the right one" if left_aim else ""))
            else:
                right_aim, left_aim = clip.pistol_thumbs
                set_wrapped_thumb(rig, clip.fist, gun, right_aim)
                if left_aim is not None and left_on:
                    set_thumb(rig, clip.fist, gun, left_aim, "l")
            if f != 0 or not rig.chest:
                return
            for _ in range(16 if settled is None else 0):
                kept = clip.pistol_thumbs  # (if no aim fits after a move, the last is kept and the hand moves on)
                rig.update()
                inside = {}
                for side in ("lr" if left_on else "r"):
                    _, deepest = rig.chest.against_gun(gun_frame.children[0], side)
                    inside.update({(side, part): depth for part, depth in deepest.items() if depth > HAND_PRESS})
                if not inside:
                    break
                for (side, part), depth in inside.items():
                    if part in ("palm", "thumb") and depth <= PALM_PRESS:
                        continue  # (a gloved palm pressed on the grip)
                    if part in ("palm", "thumb") and side == "r":
                        # The right hand pushed out the shortest way: along the surface's normal nearest the deepest
                        # point (in the gun's frame), the thumbs searched again from there
                        rig.update()
                        rig.chest.against_gun(gun_frame.children[0], "r")
                        point = gun @ (Vector(rig.chest.deepest_at[part]) / 100.0)
                        _, normal, _, _ = gun_tree(gun_frame.children[0]).find_nearest(point)
                        way = (gun.to_3x3().inverted() @ normal).normalized()
                        w["RightPush"] = w.get("RightPush", Vector()) + way * (depth - PALM_PRESS + 0.001)
                        clip.pistol_thumbs = None
                        continue
                    if part in ("palm", "thumb"):
                        # (a thumb's skin in the gun is its root's, the ball of the hand: the hand moves out, the thumbs
                        #  are searched again from there)
                        key = "CupOut" if side == "l" else "RightOut"
                        w[key] = min(0.020, w.get(key, 0.0) + depth - PALM_PRESS + 0.001)
                        clip.pistol_thumbs = None
                    else:
                        margins = clip.finger_margins if side == "l" else clip.right_margins
                        if margins.get(part, 0.0) >= FINGER_MARGIN_MAX - 1e-6:
                            # (closing no further helps: its root is in the gun's side, the hand moves out)
                            key = "CupOut" if side == "l" else "RightOut"
                            w[key] = min(0.020, w.get(key, 0.0) + depth + 0.001)
                            clip.pistol_thumbs = None
                        margins[part] = min(FINGER_MARGIN_MAX, margins.get(part, 0.0) + depth + 0.001)
                place("r", gun @ right_hand_on_grip(w, clip.wrist_turn))
                if left_on:
                    place("l", gun @ support_hand(clip.weapon))
                rig.update()
                if left_on:
                    for name in FINGERS:
                        if name.endswith("_l"):
                            armature.pose.bones[name].matrix_basis = Matrix.Identity(4)
                    close_left_round_grip(rig, clip.fist, gun, w, clip.finger_margins)
                grip_right_fingers(rig, gun, w, clip.fist, clip.right_margins, gun_tree(gun_frame.children[0]))
                pistol_hands(1) if clip.pistol_thumbs is not None else None
                if clip.pistol_thumbs is None:
                    try:
                        right_aim, left_aim, _ = place_pistol_thumbs(rig, clip.fist, gun, gun_tree(gun_frame.children[0]), left_on)
                        clip.pistol_thumbs = (right_aim, left_aim)
                    except SystemExit as e:
                        print(f"    (kept the thumbs: {e})")
                        clip.pistol_thumbs = kept
                        set_wrapped_thumb(rig, clip.fist, gun, kept[0])
                        if kept[1] is not None and left_on:
                            set_thumb(rig, clip.fist, gun, kept[1], "l")
            rig.update()
            fore = rig.bone("lowerarm_r").to_quaternion()
            wrist = (fore.inverted() @ rig.bone("hand_r").to_quaternion())
            bend = math.degrees(rig.bone("lowerarm_r").col[1].to_3d().angle(rig.bone("hand_r").col[1].to_3d()))
            print(f"    the right wrist bent {bend:.1f} degrees (the hand's line from the forearm's)")
            print(f"    the right wrist's turn from the forearm: {math.degrees(wrist.angle):.1f} degrees, about "
                  f"{tuple(round(v, 2) for v in wrist.axis)}; the elbow at {tuple(round(v * 100, 1) for v in rig.bone('lowerarm_r').translation)} cm")
            report = []
            for side in ("lr" if left_on else "r"):
                _, deepest = rig.chest.against_gun(gun_frame.children[0], side)
                report += [f"{'left' if side == 'l' else 'right'} {part} {depth * 1000:.0f} mm" for part, depth in sorted(deepest.items())]
                for part, depth in sorted(deepest.items()):
                    if depth > HAND_PRESS:
                        print(f"    the {'left' if side == 'l' else 'right'} {part} deepest at {rig.chest.deepest_at[part]} cm on the pistol")
            print(f"  {clip.name}: the hands' skin in the pistol at the deepest: " + (", ".join(report) or "nothing") +
                  f"; the hands moved out: left {w.get('CupOut', 0.0) * 1000:.0f} mm, right {w.get('RightOut', 0.0) * 1000:.0f} mm "
                  f"and {tuple(round(v * 1000) for v in w.get('RightPush', Vector()))} mm; "
                  f"the thumbs' margin {THUMB_GLOVE * 1000:.0f} mm")
            if settled is None and left_on:
                w["Settled"] = {"thumbs": clip.pistol_thumbs, "left": dict(clip.finger_margins), "right": dict(clip.right_margins)}
                # Where the elbows are from the hands' targets, aiming level: the other clips' elbows go there
                w["ElbowInHand"] = {s: rig.hand_target[s].matrix_world.inverted() @ (armature.matrix_world @ rig.bone(f"lowerarm_{s}").translation)
                                    for s in "lr"}
            # The user (2026-11-21): meshes must not go into each other
            deep = [r for r in report if int(r.split()[-2]) > (PALM_PRESS if r.split()[1] in ("palm", "thumb") else HAND_PRESS) * 1000 + 0.5]
            if deep and settled is None:
                clip.problem = clip.problem or f"{clip.name}: the hands' skin still in the pistol: {', '.join(deep)}"
            elif deep:
                # (the hands hold the pistol as the level aim settled it, the same in every clip as on the engine's hand
                #  bone; the skin measured differently here is reported, the level aim's decides)
                print(f"  {clip.name}: NOTE the hands' skin measured in the pistol: {', '.join(deep)}")

        def hold_left():
            # The left hand straight along its forearm (three rounds: the forearm moves a little with the hand), under
            # the handguard, its fingers closed round it
            for _ in range(3):
                left, twist = straight_left_hand(rig, gun, w, rig.left_rest, clip.palm_out)
                place("l", left)
                rig.update()
            for name in FINGERS:
                if name.endswith("_l"):
                    armature.pose.bones[name].matrix_basis = Matrix.Identity(4)
            result = wrap_left_fingers(rig, gun, clip.guard, clip.fist, clip.finger_margins, clip.finger_limits)
            if clip.thumb is None:  # how far out: searched on the first frame, kept for the clip
                clip.thumb = place_thumb(rig, gun, w, clip.guard, clip.fist, clip.finger_margins.get("thumb", 0.0))
            else:
                set_thumb(rig, clip.fist, gun, clip.thumb)
            return twist, result

        twist = 0.0
        if clip.left_chest is not None:
            # A fist (Idle's), off the weapon; hanging at the side half closed
            for name in FINGERS:
                if name.endswith("_l"):
                    axis, angle = clip.fist[name]
                    angle *= PISTOL_LEFT_RELAX if clip.left_hang else 1.0
                    armature.pose.bones[name].matrix_basis = Quaternion(axis, angle).to_matrix().to_4x4()
            rig.update()
        elif clip.left is None and w["Hold"] == "Pistol":
            # Round the right hand: the fingers over the right ones (the thumbs below, with the right hand's)
            for name in FINGERS:
                if name.endswith("_l"):
                    armature.pose.bones[name].matrix_basis = Matrix.Identity(4)
            close_left_round_grip(rig, clip.fist, gun, w, clip.finger_margins)
        elif clip.left is None:
            twist, (gap, finger) = hold_left()
            if f == 0 and rig.chest:
                # The skin of the hand out of the gun: the bones' shapes above are a guess at it. A finger whose skin is
                # in the gun stays further out, a palm in it moves out along its normal (kept for the clip)
                gun_frame.matrix_world = armature.matrix_world @ gun
                for _ in range(6):
                    rig.update()
                    _, deepest = rig.chest.against_gun(gun_frame.children[0])
                    deepest = {part: depth for part, depth in deepest.items() if depth > HAND_PRESS}
                    if not deepest:
                        break
                    for part, depth in deepest.items():
                        if part == "palm":
                            clip.palm_out = min(PALM_OUT_MAX, clip.palm_out + depth + 0.001)
                        else:
                            clip.finger_margins[part] = min(FINGER_MARGIN_MAX,
                                                            clip.finger_margins.get(part, 0.0) + depth + 0.001)
                        if part == "thumb":
                            clip.thumb = None  # searched again, further out
                    twist, (gap, finger) = hold_left()
                # A finger still in at the most it may be held out (the sniper's forend is wider above its keel than
                # the box measured under the palm: its idle hold's index went 7 mm in): opened, a little at a time,
                # until its skin is out (kept for the clip)
                for _ in range(12):
                    stuck = [part for part in deepest if part in LEFT_FINGERS and part != "thumb"
                             and clip.finger_margins.get(part, 0.0) >= FINGER_MARGIN_MAX - 1e-6]
                    if not stuck:
                        break
                    for part in stuck:
                        now = clip.finger_limits.get(part, CURLED.get(f"{part}_01_l", 1.0))
                        clip.finger_limits[part] = max(0.0, now - 0.05)
                    twist, (gap, finger) = hold_left()
                    rig.update()
                    _, deepest = rig.chest.against_gun(gun_frame.children[0])
                    deepest = {part: depth for part, depth in deepest.items() if depth > HAND_PRESS}
                if clip.finger_limits:
                    print(f"  {clip.name}: left fingers opened to (of the fist): " +
                          ", ".join(f"{p} {m:.2f}" for p, m in sorted(clip.finger_limits.items())))
                margins = ", ".join(f"{p} {m * 1000:.0f}" for p, m in sorted(clip.finger_margins.items())) or "none"
                print(f"  {clip.name}: the left hand turned {twist:.1f} degrees about its forearm (half of it the forearm's "
                      f"roll), the palm {clip.palm_out * 100:.1f} cm under the support; fingers further out (mm): {margins}; the thumb "
                      f"{clip.thumb[0]} degrees up, {clip.thumb[1]} out; "
                      f"still in the gun: " + (", ".join(f"{p} {deepest[p] * 1000:.0f} mm at {rig.chest.deepest_at[p]} cm"
                                                         for p in sorted(deepest)) or "nothing"))
            if f == 0 and gap > 0.0:
                print(f"  {clip.name}: the left {finger} stays {gap * 100:.1f} cm from the handguard at the closest")
        if clip.right is None:
            # The right hand on the grip: its fingers round it, the index finger on the trigger
            tree = None
            if w["Hold"] in ("Pistol", "Rifle"):
                gun_frame.matrix_world = armature.matrix_world @ gun
                rig.update()
                tree = gun_tree(gun_frame.children[0])
            if w["Hold"] == "Pistol" and PISTOL_ONE_HAND:
                if f == 0 and rig.chest:
                    # Each finger (and the thumb) opened from the table in steps until its skin is out of the pistol (kept
                    # for the clip): the table sets the grip's shape, this its last millimeters
                    gun_frame.matrix_world = armature.matrix_world @ gun
                    clip.one_hand_scale = dict(w.get("OneHandScale", {}))
                    for _ in range(10 if "OneHandScale" not in w else 0):  # (the level aim's, first: every clip's fist)
                        set_one_hand_fingers(rig, clip.fist, clip.one_hand_scale)
                        _, deepest = rig.chest.against_gun(gun_frame.children[0], "r")
                        over = [p for p, d in deepest.items() if p in ONE_HAND_FINGERS
                                and d > (PALM_PRESS if p == "thumb" else HAND_PRESS) and clip.one_hand_scale.get(p, 1.0) > 0.6]
                        if not over:
                            break
                        for p in over:
                            clip.one_hand_scale[p] = clip.one_hand_scale.get(p, 1.0) - 0.04
                    w.setdefault("OneHandScale", dict(clip.one_hand_scale))
                    print(f"  {clip.name}: one hand: opened from the table: " +
                          (", ".join(f"{p} to {s:.2f}" for p, s in sorted(clip.one_hand_scale.items())) or "nothing"))
                set_one_hand_fingers(rig, clip.fist, getattr(clip, "one_hand_scale", None))
                trigger = (armature.pose.bones["index_02_r"].tail - gun @ w["Trigger"]).length
            else:
                trigger = grip_right_fingers(rig, gun, w, clip.fist, clip.right_margins, tree)
            if f == 0:
                print(f"  {clip.name}: the right index finger's last joint {trigger * 100:.1f} cm from the trigger")
            if w["Hold"] == "Pistol" and not PISTOL_ONE_HAND:
                pistol_hands(f)
            elif w["Hold"] == "Pistol" and f == 0 and rig.chest:
                gun_frame.matrix_world = armature.matrix_world @ gun
                rig.update()
                _, deepest = rig.chest.against_gun(gun_frame.children[0], "r")
                bend = math.degrees(rig.bone("lowerarm_r").col[1].to_3d().angle(rig.bone("hand_r").col[1].to_3d()))
                miss = (rig.bone("hand_r").translation - rig.hand_target["r"].matrix_world.translation).length
                deep = {part: depth for part, depth in deepest.items()
                        if depth > (ONE_HAND_PALM_PRESS if part == "palm" else PALM_PRESS if part == "thumb" else HAND_PRESS)}
                print(f"  {clip.name}: one hand: the wrist bent {bend:.1f} degrees, the hand {miss * 100:.1f} cm short; its "
                      f"skin in the pistol: " + (", ".join(f"{p} {d * 1000:.0f} mm at {rig.chest.deepest_at[p]} cm" for p, d in sorted(deepest.items())) or "nothing"))
                if deep and clip.low_ready is False and clip.name.endswith("Aim_Neutral"):
                    clip.problem = clip.problem or f"{clip.name}: the right hand's skin in the pistol: {deep}"
        if not knife and rig.chest and (not clip.low_ready or w["Hold"] == "Rifle"):
            # The face off the gun in every frame (the kick and the reload's tilt move the gun; the low ready breathes)
            gun_frame.matrix_world = armature.matrix_world @ gun
            rig.update()
            face, _ = rig.chest.against_gun(gun_frame.children[0])
            if face < nearest_face[0]:
                nearest_face = (face, f)
        local = rig.local_pose()
        if clip.left is None and twist:
            local = roll_forearm(local, armature, twist * 0.5)
        poses.append((local, gun.copy()))
        # The hand that holds the gun now (the sniper's bolt: the left one while the right works the bolt): where the
        # gun is in it, for the game's model on that hand's bone
        for side, windows in (("l", clip.left_holds), ("r", clip.right_holds)):
            if windows and any(a - 1e-6 <= time <= b + 1e-6 for a, b in windows):
                clip.held_in[side].append(rig.bone(f"hand_{side}").inverted() @ gun)
        for side in "rl":
            drop = elbow_drop(rig, side)
            lowest[side] = min(lowest[side], drop)
            below, outside = elbow_place(rig, side)
            worst_below[side] = min(worst_below[side], below)
            worst_outside[side] = min(worst_outside[side], outside)
            inside = ELBOW_INSIDE[side] if w["Hold"] == "Rifle" else PISTOL_ELBOW_INSIDE  # (a pistol's arms reach in to the middle)
            # (a rule broken fails the run after the clip's previews: the first one is kept)
            if clip.check_elbows and not (side == "l" and clip.left_free) and (below < -clip.below_hand_margin or outside < -inside):
                clip.problem = clip.problem or (f"{clip.name}, frame {f}: the {'right' if side == 'r' else 'left'} elbow is "
                                                f"{below * 100:.1f} cm below its hand and {outside * 100:.1f} cm outside its "
                                                f"shoulder (both wanted above 0): check the elbow targets and the chest's twist")
            if drop < ELBOW_MIN_DROP - (0.01 if w["Hold"] == "Pistol" else 0.0) and clip.check_elbows and not (side == "l" and clip.left_hang):  # (the pistol's arms nearly straight)
                clip.problem = clip.problem or (f"{clip.name}, frame {f}: the {'right' if side == 'r' else 'left'} elbow is "
                                                f"{drop * 100:.1f} cm below the line from the shoulder to the hand (above it): "
                                                f"check the arm's IK pole")
        if f == count // 2:
            miss = {s: (rig.bone(f"hand_{s}").translation - rig.hand_target[s].matrix_world.translation).length
                    for s in "rl"}
            shoulder = rig.bone("upperarm_r").translation
            wanted = rig.hand_target["r"].matrix_world.translation
            arm = (rig.armature.data.bones["lowerarm_r"].head_local - rig.armature.data.bones["upperarm_r"].head_local).length +                 (rig.armature.data.bones["hand_r"].head_local - rig.armature.data.bones["lowerarm_r"].head_local).length
            print(f"  {clip.name}: the right shoulder at {tuple(round(v * 100, 1) for v in shoulder)} cm, the hand wanted at "
                  f"{tuple(round(v * 100, 1) for v in wanted)} cm: {(wanted - shoulder).length * 100:.1f} cm apart, the arm "
                  f"{arm * 100:.1f} cm; the eye at {tuple(round(v * 100, 1) for v in right_eye(rig))} cm")
            direction = gun.col[0].to_3d()
            pitch = math.degrees(math.asin(max(-1.0, min(1.0, direction.z))))
            yaw = math.degrees(math.atan2(-direction.x, -direction.y))
            # Where the gun is in the model's space (glTF: y up, the character looking along +z), for a gun put
            # beside a still pose in a scene; and in the right hand's space, for a gun on the hand's bone (E3)
            to_gltf = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))
            in_model = to_gltf @ gun @ to_gltf.inverted()
            in_hand = rig.bone("hand_r").inverted() @ gun
            print(f"  {clip.name}: gun in the model at {tuple(round(v, 4) for v in in_model.translation)}, "
                  f"rotation (deg) {tuple(round(math.degrees(a), 2) for a in in_model.to_euler('YXZ'))}; in hand_r at "
                  f"{tuple(round(v, 4) for v in in_hand.translation)}, quaternion "
                  f"{tuple(round(v, 4) for v in in_hand.to_quaternion())}")
            # For the first person's view model (Player.cs): the right eye and the sight line's start in the model's
            # space (glTF: y up, the character looking along +z)
            eye_in_model = (to_gltf @ Matrix.Translation(right_eye(rig))).translation
            line = f"  {clip.name}: the right eye in the model at {tuple(round(v, 4) for v in eye_in_model)}"
            if w["Eye"] is not None:
                sight = (to_gltf @ gun @ Matrix.Translation(w["Eye"])).translation
                line += f", the sight line's start at {tuple(round(v, 4) for v in sight)}"
            print(line)
            if w.get("FrontSight") is not None and clip.name.endswith(("_Aim_Neutral", "_Aim_Neutral_View")):
                # The sight line in the model (the first person's view model): through the rear sight and the front one;
                # aiming down the sights the eye is on it at the right eye's depth, and the view looks along it
                # (the weapon file's AimEye and SightDirection)
                rear = (to_gltf @ gun @ Matrix.Translation(w["RearSight"])).translation
                front = (to_gltf @ gun @ Matrix.Translation(w["FrontSight"])).translation
                along = (front - rear).normalized()
                t = (eye_in_model.z - rear.z) / along.z if abs(along.z) > 1e-6 else 0.0
                on_line = rear + along * t
                pitch = math.degrees(math.asin(max(-1.0, min(1.0, along.y))))
                yaw = math.degrees(math.atan2(along.x, along.z))
                print(f"  {clip.name}: the sights in the model: rear {tuple(round(v, 4) for v in rear)}, front "
                      f"{tuple(round(v, 4) for v in front)}; the line {pitch:.2f} degrees up, {yaw:.2f} to the model's left "
                      f"from the gun's forward; AimEye {tuple(round(v, 4) for v in on_line)}, SightDirection "
                      f"{tuple(round(v, 5) for v in along)}")
            gun_frame.matrix_world = armature.matrix_world @ gun
            rig.update()
            face, deepest = rig.chest.against_gun(gun_frame.children[0])
            print(f"  {clip.name}: the face {face * 100:.1f} cm from the gun at the closest (negative: in it); the left "
                  f"hand in it at the deepest: " +
                  (", ".join(f"{part} {depth * 100:.1f} cm" for part, depth in sorted(deepest.items())) or "nothing"))
            print(f"  {clip.name}: gun pitch {pitch:.1f}, yaw {yaw:.1f} (deg); hands off target r {miss['r'] * 100:.1f} cm,"
                  f" l {miss['l'] * 100:.1f} cm; elbows below the shoulder-hand line r {elbow_drop(rig, 'r') * 100:.1f} cm,"
                  f" l {elbow_drop(rig, 'l') * 100:.1f} cm")

    for side in "lr":
        held = clip.held_in[side]
        if not held:
            continue
        mean_t = sum((m.translation for m in held), Vector()) / len(held)
        q0 = held[0].to_quaternion()
        total = [0.0, 0.0, 0.0, 0.0]
        for m in held:
            q = m.to_quaternion()
            if q.dot(q0) < 0.0:
                q.negate()
            total = [total[0] + q.w, total[1] + q.x, total[2] + q.y, total[3] + q.z]
        mean_q = Quaternion(total).normalized()
        moved = max((m.translation - mean_t).length for m in held)
        turned = max(math.degrees(m.to_quaternion().rotation_difference(mean_q).angle) for m in held)
        print(f"  {clip.name}: the gun in hand_{side} over {len(held)} frames it holds it: at "
              f"{tuple(round(v, 4) for v in mean_t)}, quaternion {tuple(round(v, 4) for v in mean_q)}; "
              f"at most {moved * 1000:.1f} mm and {turned:.1f} degrees from that")
        clip.held_mean[side] = (mean_t, mean_q, moved, turned)
    print(f"  {clip.name}: the elbows at least r {lowest['r'] * 100:.1f} cm, l {lowest['l'] * 100:.1f} cm below the "
          f"shoulder-hand line in every frame; below the hand at least r {worst_below['r'] * 100:.1f}, "
          f"l {worst_below['l'] * 100:.1f} cm; outside the shoulder at least r {worst_outside['r'] * 100:.1f}, "
          f"l {worst_outside['l'] * 100:.1f} cm")
    if nearest_face[0] < math.inf:
        print(f"  {clip.name}: the face at least {nearest_face[0] * 100:.2f} cm from the gun (frame {nearest_face[1]})")
        wanted = (FACE_KICK_CLEARANCE if clip.name.endswith("_Shoot") else
                  FACE_LOW_READY_CLEARANCE if clip.low_ready else FACE_CLEARANCE)
        if nearest_face[0] < wanted:
            clip.problem = clip.problem or (f"{clip.name}, frame {nearest_face[1]}: the face is {nearest_face[0] * 100:.1f} "
                                            f"cm from the gun (wanted {wanted * 100:.0f}): check GUN_FORWARD and EYE_ABOVE")
    return action, poses


def write_action(armature, action, poses):
    """The baked poses as keyframes (location and rotation of every bone), with the constraints off"""
    for c in [c for pb in armature.pose.bones for c in pb.constraints]:
        c.mute = True
    armature.animation_data.action = action
    for f, (pose, _) in enumerate(poses):
        for pb in armature.pose.bones:
            loc, quat, scale = pose[pb.name].decompose()
            pb.rotation_mode = "QUATERNION"
            pb.location = loc
            pb.rotation_quaternion = quat
            pb.scale = scale
            pb.keyframe_insert("location", frame=f)
            pb.keyframe_insert("rotation_quaternion", frame=f)
            pb.keyframe_insert("scale", frame=f)
    action.use_fake_user = True
    armature.animation_data.action = None
    for c in [c for pb in armature.pose.bones for c in pb.constraints]:
        c.mute = False


# ---------------------------------------------------------------------------------------------------------------------
# The preview renders

def render_preview(folder, rig, clip, poses, gun_mesh_frame):
    os.makedirs(folder, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.color_type = "MATERIAL"
    scene.render.resolution_x = 640
    scene.render.resolution_y = 720
    camera = scene.camera
    if camera is None:
        camera = bpy.data.objects.new("PreviewCamera", bpy.data.cameras.new("PreviewCamera"))
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = 1.6
        scene.collection.objects.link(camera)
        scene.camera = camera
    pose, gun = poses[len(poses) // 2]
    for c in [c for pb in rig.armature.pose.bones for c in pb.constraints]:
        c.mute = True
    rig.set_pose(pose)
    gun_mesh_frame.matrix_world = rig.armature.matrix_world @ gun
    rig.update()
    # The dressed player's body as the engine poses it, in place of the library's mannequin
    rig.chest.clearance(Vector())
    mannequin = bpy.data.objects["Mannequin"]
    mannequin.hide_render = True
    views = {
        "side": ((-2.5, -0.2, 1.25), (math.radians(90), 0, math.radians(-90))),
        "front": ((-0.05, -2.5, 1.25), (math.radians(90), 0, 0)),
        "top": ((-0.05, -0.2, 3.5), (0, 0, 0)),
        # The back and the shoulders: from behind, and from behind at the right (the third-person camera's side)
        "back": ((-0.05, 2.5, 1.25), (math.radians(90), 0, math.radians(180))),
        "back_right": ((-1.8, 1.8, 1.45), (math.radians(85), 0, math.radians(-135))),
    }
    for view, (location, rotation) in views.items():
        camera.location = location
        camera.rotation_euler = rotation
        camera.data.ortho_scale = 1.6
        scene.render.filepath = os.path.join(folder, f"{clip.name}_{view}.png")
        bpy.ops.render.render(write_still=True)
    # Close-ups of the hands on the gun: from the right and from the left, level with them
    hands = (rig.bone("hand_r").translation + rig.bone("hand_l").translation) * 0.5
    if getattr(clip, "left_hang", False):
        hands = rig.bone("hand_r").translation  # (one hand on the gun: on it)
    for view, (offset, rotation) in {"hands_right": ((-1.5, 0.0, 0.0), (math.radians(90), 0, math.radians(-90))),
                                     "hands_left": ((1.5, 0.0, 0.0), (math.radians(90), 0, math.radians(90)))}.items():
        camera.location = rig.armature.matrix_world @ (hands + Vector(offset))
        camera.rotation_euler = rotation
        camera.data.ortho_scale = 0.55
        scene.render.filepath = os.path.join(folder, f"{clip.name}_{view}.png")
        bpy.ops.render.render(write_still=True)
    # Close-ups of the left hand on the handguard (from below, from the muzzle's end, from the left, from above) and of the
    # face beside the stock (from above and from the left)
    left = rig.bone("hand_l").translation
    face = right_eye(rig)
    closeups = {"left_hand_below": (left, (0.0, 0.0, -1.0), (math.radians(180), 0, 0)),
                "left_hand_front": (left, (0.0, -1.0, 0.0), (math.radians(90), 0, 0)),
                "left_hand_left": (left, (1.0, 0.0, 0.0), (math.radians(90), 0, math.radians(90))),
                "left_hand_above": (left, (0.0, 0.0, 1.0), (0, 0, 0)),
                "face_above": (face, (0.0, 0.0, 1.0), (0, 0, 0)),
                "face_left": (face, (1.0, 0.0, 0.0), (math.radians(90), 0, math.radians(90)))}
    if getattr(clip, "left_hang", False):
        # One hand on the gun: the right hand's close-ups (from below, its back, the far side, above, the shooter's view)
        right = gun @ WEAPONS[clip.weapon]["Grip"]  # (the grip in the middle)
        closeups.update({"right_hand_below": (right, (0.0, 0.0, -1.0), (math.radians(180), 0, 0)),
                         "right_hand_right": (right, (-1.0, 0.0, 0.0), (math.radians(90), 0, math.radians(-90))),
                         "right_hand_left": (right, (1.0, 0.0, 0.0), (math.radians(90), 0, math.radians(90))),
                         "right_hand_above": (right, (0.0, 0.0, 1.0), (0, 0, 0)),
                         "right_hand_front": (right, (0.0, -1.0, 0.0), (math.radians(90), 0, 0))})
    for view, (at, direction, rotation) in closeups.items():
        camera.location = rig.armature.matrix_world @ (at + Vector(direction) * 1.0)
        camera.rotation_euler = rotation
        camera.data.ortho_scale = 0.3
        scene.render.filepath = os.path.join(folder, f"{clip.name}_{view}.png")
        bpy.ops.render.render(write_still=True)
    # Cross-sections through the left hand and the handguard, seen from the muzzle's end: cut 2 cm ahead of the support
    # point, at it and 2 cm behind it (the near clipping plane)
    support = gun @ WEAPONS[clip.weapon]["Support"] if WEAPONS[clip.weapon]["Support"] is not None else None
    for cut in ((-0.02, 0.0, 0.02) if support is not None else ()):
        at = support + gun.col[0].to_3d() * cut
        camera.location = rig.armature.matrix_world @ (at + Vector((0.0, -1.0, 0.0)))
        camera.rotation_euler = (math.radians(90), 0, 0)
        camera.data.ortho_scale = 0.2
        camera.data.clip_start = 1.0  # the camera 1 m in front of the cut (a level gun points along -y)
        scene.render.filepath = os.path.join(folder, f"{clip.name}_left_hand_section_{int(round(cut * 100)):+d}cm.png")
        bpy.ops.render.render(write_still=True)
    camera.data.clip_start = 0.1
    camera.data.ortho_scale = 1.6
    # A moving knife (the slash): every second frame from the front, and from above at the right
    if clip.knife_keys is not None and len(clip.knife_keys) > 1:
        for f in range(0, len(poses), 2):
            pose, gun = poses[f]
            rig.set_pose(pose)
            gun_mesh_frame.matrix_world = rig.armature.matrix_world @ gun
            rig.update()
            rig.chest.clearance(Vector())
            for view, (location, rotation) in {"seq_front": ((-0.05, -2.5, 1.25), (math.radians(90), 0, 0)),
                                               "seq_above": ((-1.2, -1.6, 2.6), (math.radians(40), 0, math.radians(-35)))}.items():
                camera.location = location
                camera.rotation_euler = rotation
                scene.render.filepath = os.path.join(folder, f"{clip.name}_{view}_{f:02d}.png")
                bpy.ops.render.render(write_still=True)
    for c in [c for pb in rig.armature.pose.bones for c in pb.constraints]:
        c.mute = False
    mannequin.hide_render = False


# ---------------------------------------------------------------------------------------------------------------------

def main():
    preview, only, which = args()
    names, output = SETS[which]
    armature = load_library()
    rig = Rig(armature)

    base_names = {"Idle_Loop", "Pistol_Aim_Neutral"}
    bases = {name: sample_action(armature, name) for name in base_names}
    aim = bases["Pistol_Aim_Neutral"][0]
    finger_pose = {name: aim[name] for name in FINGERS}  # the pistol's grip: both hands closed around a handle

    made = []
    fist = finger_fist(bases["Idle_Loop"][0])
    bones = armature.data.bones
    rig.left_rest = (bones["lowerarm_l"].matrix_local.to_3x3().inverted() @ bones["hand_l"].matrix_local.to_3x3()).normalized()
    rig.chest = Chest(rig)
    IDLE.update(bases["Idle_Loop"][0])
    # Where the right index finger's knuckle is on the hand (its bone's space), for the hand on the grip
    rig.set_pose(bases["Idle_Loop"][0])
    rig.update()
    KNUCKLE.xyz = rig.bone("hand_r").inverted() @ armature.pose.bones["index_01_r"].head
    THUMB_ROOT.xyz = rig.bone("hand_r").inverted() @ armature.pose.bones["thumb_01_r"].head
    print(f"The right index knuckle in the hand's space: {tuple(round(v * 100, 1) for v in KNUCKLE)} cm, the thumb's root "
          f"{tuple(round(v * 100, 1) for v in THUMB_ROOT)} cm")
    # The pistol's left hand round the right: where the library's pistol stance has it from the right hand
    # (the arms' IK off: it would pull the hands to its targets)
    constraints = [c for pb in armature.pose.bones for c in pb.constraints]
    for c in constraints:
        c.mute = True
    rig.set_pose(bases["Pistol_Aim_Neutral"][0])
    rig.update()
    WEAPONS["Pistol"]["Cup"] = rig.bone("hand_r").inverted() @ rig.bone("hand_l")
    for c in constraints:
        c.mute = False
    print(f"The pistol's left hand from the right: {tuple(round(v * 100, 1) for v in WEAPONS['Pistol']['Cup'].translation)} cm")
    for weapon in names:
        frame, mesh = load_gun(weapon)
        guard = None
        if WEAPONS[weapon]["Support"] is not None:
            middle, half, bottom = handguard(mesh, WEAPONS[weapon])
            guard = (middle, half)
            WEAPONS[weapon]["Support"].z = bottom  # the palm under the handguard's lowest surface
        WEAPONS[weapon]["GripSection"] = grip_section(mesh, WEAPONS[weapon])
        width, front, _ = WEAPONS[weapon]["GripSection"]
        print(f"  the pistol grip: {width * 200:.1f} cm wide, {front * 200:.1f} cm deep")
        for clip in make_clips(weapon):
            if only and clip.name != only:
                continue
            clip.weapon = weapon
            clip.guard = guard
            clip.fist = fist
            clip.wrist_turn = 0.0
            clip.palm_out = 0.035      # the left hand's origin (the wrist) under the support, along the palm's normal
            clip.finger_margins = {}   # left fingers held further out from the handguard (meters)
            clip.finger_limits = {}    # left fingers held open: the most they close (a fraction of the fist)
            clip.thumb = None          # the left thumb's degrees out from along the handguard: place_thumb
            clip.pistol_thumbs = None  # the pistol's thumbs' aims (right, left): place_pistol_thumbs
            clip.right_margins = {}    # right fingers held further out from the grip (meters)
            action, poses = bake_clip(rig, clip, bases[clip.base], finger_pose, frame)
            write_action(armature, action, poses)
            made.append((clip, action, poses))
            if preview:
                render_preview(preview, rig, clip, poses, frame)
            print(f"{clip.name}: {len(poses)} frames")
            if clip.problem:
                raise SystemExit(clip.problem)
        bpy.data.objects.remove(mesh)
        bpy.data.objects.remove(frame)
    if only:
        print(f"--only {only}: nothing written")
        return

    # Only the new clips go into the file
    for action in list(bpy.data.actions):
        if action not in [a for _, a, _ in made]:
            bpy.data.actions.remove(action)
    for o in list(bpy.data.objects):
        if o.type == "EMPTY":
            bpy.data.objects.remove(o)
    bpy.data.objects.remove(rig.chest.mesh)  # the dressed body the right wrist kept out of: not in the clips' file
    bpy.data.objects.remove(rig.chest.armature)
    for pb in armature.pose.bones:
        for c in list(pb.constraints):
            pb.constraints.remove(c)
        pb.matrix_basis = Matrix.Identity(4)

    # The mannequin's vertex groups name vertices with a weight of 0, which the exporter keeps and assimp warns about
    mannequin = bpy.data.objects["Mannequin"]
    for group in mannequin.vertex_groups:
        empty = [v.index for v in mannequin.data.vertices for g in v.groups if g.group == group.index and g.weight <= 0.0]
        if empty:
            group.remove(empty)

    path = os.path.join(os.path.dirname(OUTPUT), output)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    # The keys are at frame numbers; the export makes them seconds by the scene's frame rate. Importing the guns (FBX) sets
    # it to theirs (24): every clip played 25 % slow until 2026-11-22 (the rifle's 2.2 s reload took 2.7 s)
    bpy.context.scene.render.fps = FPS
    bpy.context.scene.render.fps_base = 1.0
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", export_animations=True,
                              export_animation_mode="ACTIONS", export_force_sampling=True, export_skins=True,
                              export_yup=True)
    print(f"Wrote {path}: {len(made)} clips")


main()
