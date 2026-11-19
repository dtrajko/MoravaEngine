# Makes the soldiers' rifle clips (the shooter's plan, Phase 0.2: the rifle in third person) on the animation library's
# rig, in Blender without its window. The rifle is placed against the shoulder, the hands are put on its grip and
# handguard by IK, and every frame is baked into plain keyframes: the engine plays the clips like the library's own.
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/CharacterPoses/make_rifle_poses.py -- [--preview <folder>]
#
# Reads  Projects/Outpost/Assets/Models/QuaterniusAnimations/UAL1_Standard.glb (the rig, its idle and pistol clips)
#        Projects/Outpost/Assets/Models/QuaterniusGuns/AssaultRifle_2.fbx, SniperRifle_3.fbx (where the hands go)
# Writes Projects/Outpost/Assets/Models/SoldierAnimations/Soldier_Rifles.glb (the mannequin and the clips below)
#
# Clips (Rifle_ for the assault rifle, Sniper_ for the sniper rifle):
#   _Idle_Loop        low ready: the muzzle down, on the library's Idle_Loop (breathing)
#   _Aim_Up, _Aim_Neutral, _Aim_Down   single poses for the aim offset (E5): 45 degrees up, level, 45 degrees down
#   _Shoot            the kick of one shot (Sniper_Shoot is followed by Sniper_Bolt)
#   _Reload           assault rifle: the magazine out, a new one from the belt, in; sniper: the bolt open, two rounds in
#   Sniper_Bolt       the bolt worked after a shot
#
# The hands close on the gun (the user, 2026-11-19: the left fingers hung open in the air, the right hand hung down under
# the grip): each finger is curled from open along the library's fist (its three joints together) until its tip
# reaches what it holds, a glove's thickness out. The left hand's round the handguard (its cross-section measured on the
# gun's mesh); the right hand lies along the gun, level, its palm on the grip, its fingers and thumb round the grip and
# its index finger on the trigger. Every joint of a finger stops at the surface, not only its tip.
# The right hand is turned out about its index knuckle (level still) just as far as keeps the wrist out of the chest: the
# dressed player's body (Soldier_Player.gltf, the uniform's bulk) posed as the clip poses it, the forearm's thickness and
# the sleeve's clear. The left hand points along its forearm (the wrist straight), its palm turned toward the handguard
# by at most LEFT_TWIST about the forearm, so the wrist isn't wrung.
#
# Everything is generated: edits by hand in Blender are lost on the next run. --preview renders each clip's middle
# frame from the side and the front, the rifle in the hands (Workbench), to check the poses.

import bpy
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
    },
}

# The rig: Blender's armature space has z up, the character looking along -y, its right at -x
FORWARD = Vector((0.0, -1.0, 0.0))
LEFT = Vector((1.0, 0.0, 0.0))
UP = Vector((0.0, 0.0, 1.0))

SPINE = ["spine_01", "spine_02", "spine_03"]

GLOVE = 0.006         # meters out from the handguard the fingertips stop (the glove, the finger's own thickness)
CURL_MAX = 1.6        # how far past the library's fist a finger may close (its fist: 1)
CURL_STEP = 0.05
LEFT_FINGERS = ("index", "middle", "ring", "pinky", "thumb")
BODY = os.path.join(ASSETS, "QuaterniusCharacters", "Soldier_Player.gltf")  # the dressed player: the chest the wrist keeps out of
CHEST_CLEARANCE = 0.035 # meters the right wrist's joint stays out of the chest (the gloved forearm's half thickness)
WRIST_TURN_MAX = 50.0   # degrees the right hand may turn out at most
LEFT_TWIST = 35.0       # degrees the left wrist may turn about the forearm, toward the handguard; as much again the forearm
                        # rolls about its own length (a rig without twist bones shows a whole turn at the wrist)
# Each elbow's target from the middle of its arm: this far out to the side and down (meters). The right one mostly down:
# the upper arm in against the chest, not flared out (the user, 2026-11-19); while the right hand works the bolt or
# reloads, as the left one
ELBOW_TARGET = {"r": (0.3, 0.8), "l": (0.6, 0.6)}
ELBOW_INSIDE = {"r": 0.03, "l": 0.0}  # how far (m) an elbow may come inside its shoulder's line: the right one tucked in
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
    return preview


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
    bpy.ops.import_scene.fbx(filepath=os.path.join(GUNS, WEAPONS[weapon]["File"]))
    mesh = next(o for o in set(bpy.data.objects) - before if o.type == "MESH")
    frame = bpy.data.objects.new(f"{weapon}Frame", None)  # the gun in meters: x to the muzzle, y left, z up
    bpy.context.scene.collection.objects.link(frame)
    mesh.parent = frame
    mesh.matrix_parent_inverse = Matrix.Identity(4)
    mesh.location = (0.0, 0.0, 0.0)
    mesh.rotation_euler = (0.0, 0.0, 0.0)
    mesh.scale = (GUN_SCALE,) * 3
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

def right_hand_on_grip(w, turn=0.0):
    # Level along the gun (the user: parallel to it): the hand's x out to the gun's right (the palm, -x, on the grip's
    # right side), turned about it so the line from the wrist to the index knuckle runs to the muzzle; the knuckle
    # TRIGGER_REACH behind and above the trigger, so the index finger lies forward onto it. The hand's origin is the wrist
    x = Vector((0.0, -1.0, 0.0))
    y = Vector((1.0, 0.0, 0.0))
    z = x.cross(y)
    level = Matrix.Rotation(-math.atan2(KNUCKLE.z, KNUCKLE.y), 3, x)  # the knuckle's rise over the bone's line taken out
    # turn: degrees about the gun's up through the knuckle, the wrist out to the gun's right (away from the chest)
    rotation = Matrix.Rotation(math.radians(turn), 3, "Z") @ level @ frame_from_axes(x, y, z, Vector()).to_3x3()
    back, up, out = TRIGGER_REACH
    knuckle = w["Trigger"] + Vector((-back, -out, up))
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

def handguard(mesh, w):
    """The handguard's cross-section at the support point, in the gun's frame: its middle (y, z) and half sizes (y, z),
    from the mesh's vertices within 3 cm of the support along the gun, above the support point (not the magazine) and
    below the barrel's top"""
    support = w["Support"]
    points = [v.co * GUN_SCALE for v in mesh.data.vertices]
    near = [q for q in points if abs(q.x - support.x) < 0.03 and support.z - 0.01 < q.z < support.z + 0.07]
    if not near:
        raise SystemExit(f"no handguard at the support point {support}")
    ys, zs = [q.y for q in near], [q.z for q in near]
    middle = Vector((0.0, (min(ys) + max(ys)) * 0.5, (min(zs) + max(zs)) * 0.5))
    half = Vector((0.0, (max(ys) - min(ys)) * 0.5 + GLOVE, (max(zs) - min(zs)) * 0.5 + GLOVE))
    print(f"  the handguard at the support: {half.y * 200 - GLOVE * 200:.1f} cm wide, {half.z * 200 - GLOVE * 200:.1f} cm high")
    return middle, half


def inside_guard(point_in_gun, guard):
    """Below 1: inside the handguard's cross-section (an ellipse, the glove's thickness out)"""
    middle, half = guard
    dy, dz = (point_in_gun.y - middle.y) / half.y, (point_in_gun.z - middle.z) / half.z
    return dy * dy + dz * dz


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


def wrap_left_fingers(rig, gun, guard, fist):
    """Each finger of the left hand closed from open along the fist (its three joints together, a fraction of their fist
    turn) until its tip reaches the handguard's surface (a binary search on the fraction); a finger that can't reach it
    closes as far as brings its tip nearest. Returns the farthest a tip stayed from the surface (meters) and by which"""
    armature = rig.armature
    to_gun = gun.inverted()
    worst = (0.0, "")
    middle, half = guard
    for finger in LEFT_FINGERS:
        names = [f"{finger}_{j:02d}_l" for j in (1, 2, 3)]

        def curl(fraction):
            for name in names:
                axis, angle = fist[name]
                armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * fraction).to_matrix().to_4x4()
            rig.update()
            return inside_guard(to_gun @ armature.pose.bones[names[-1]].tail, guard)

        if curl(0.0) <= 1.0:
            continue  # open, it already lies on the handguard
        steps = [i * CURL_STEP for i in range(int(CURL_MAX / CURL_STEP) + 1)]
        values = [curl(t) for t in steps]
        touching = next((i for i, v in enumerate(values) if v <= 1.0), None)
        if touching is None:
            best = min(range(len(steps)), key=lambda i: values[i])
            curl(steps[best])
            gap = (math.sqrt(values[best]) - 1.0) * min(half.y, half.z)
            if gap > worst[0]:
                worst = (gap, finger)
            continue
        low, high = steps[touching - 1], steps[touching]
        for _ in range(8):
            t = (low + high) * 0.5
            if curl(t) <= 1.0:
                high = t
            else:
                low = t
        curl(high)
    return worst


def close_finger(rig, fist, names, test, nearest=False, nearest_joint=-1):
    """A finger (its three joints' names) closed from open along the fist: until test is at most 1 at its two outer
    joints or its tip (it lies on what it holds, none of it inside; the knuckle at the palm isn't tested; a binary search
    on the fraction), or, nearest, as far as makes test of its tip (or of the end of joint nearest_joint) smallest.
    Returns test's value then"""
    armature = rig.armature

    def curl(fraction):
        for name in names:
            axis, angle = fist[name]
            armature.pose.bones[name].matrix_basis = Quaternion(axis, angle * fraction).to_matrix().to_4x4()
        rig.update()
        if nearest:
            return test(armature.pose.bones[names[nearest_joint]].tail)
        return min(test(armature.pose.bones[name].tail) for name in names[1:])

    steps = [i * CURL_STEP for i in range(int(CURL_MAX / CURL_STEP) + 1)]
    values = [curl(t) for t in steps]
    touching = None if nearest else next((i for i, v in enumerate(values) if v <= 1.0), None)
    if touching is None or touching == 0:
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
        d = v.co * GUN_SCALE - w["Grip"]
        along = d.dot(up)
        flat = d - up * along
        if -0.05 < along < 0.0 and flat.length < 0.03:
            width = max(width, abs(flat.dot(across)))
            front = max(front, abs(flat.dot(depth)))
    return width, front, depth


def grip_right_fingers(rig, gun, w, fist):
    """The right hand's fingers and thumb round the grip (a cylinder along its slant, a glove's thickness out), its index
    finger closed as far as brings its tip nearest the trigger. Returns how far the index finger's tip stays from it"""
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
        a, b = flat.y / (width + GRIP_GLOVE), flat.dot(depth) / (front + GRIP_GLOVE)
        return a * a + b * b

    for finger in ("middle", "ring", "pinky", "thumb"):
        close_finger(rig, fist, [f"{finger}_{j:02d}_r" for j in (1, 2, 3)], on_grip)
    # The index finger's pad on the trigger: its last joint (the end of its middle bone) nearest it
    return close_finger(rig, fist, [f"index_{j:02d}_r" for j in (1, 2, 3)], lambda tip: (to_gun @ tip - w["Trigger"]).length,
                        nearest=True, nearest_joint=1)


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


def straight_left_hand(rig, gun, w, rest_relation):
    """The left hand under the support, pointing along its forearm (the wrist straight), turned about the forearm toward
    having its palm up at the handguard by at most twice LEFT_TWIST (half of it the forearm's roll: roll_forearm); the
    palm's middle at the support. Returns the hand (armature space) and how far it is turned about the forearm (degrees)"""
    lower = rig.bone("lowerarm_l").to_3x3().normalized()
    neutral = lower @ rest_relation           # the hand as the forearm carries it, unbent and untwisted
    along = neutral.col[1].normalized()
    palm = neutral.col[0].normalized()        # the left palm: the hand's +x
    up = gun.col[2].to_3d()
    wanted = (up - along * up.dot(along)).normalized()
    angle = palm.angle(wanted, 0.0)
    if palm.cross(wanted).dot(along) < 0.0:
        angle = -angle
    turn = max(-math.radians(2.0 * LEFT_TWIST), min(math.radians(2.0 * LEFT_TWIST), angle))
    rotation = Matrix.Rotation(turn, 3, along) @ neutral
    hand = rotation.to_4x4()
    support = gun @ w["Support"]
    hand.translation = support - rotation.col[1] * 0.07 - rotation.col[0] * 0.035
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


def gun_at_shoulder(w, rig):
    """The gun's frame in armature space, pointing level and forward: the sight line at the right eye (its height and
    side), the butt in the shoulder pocket (how far forward)"""
    frame = frame_from_axes(FORWARD, LEFT, UP, Vector())
    by_eye = right_eye(rig) - frame.to_3x3() @ w["Eye"]
    by_butt = shoulder_pocket(rig) - frame.to_3x3() @ w["Butt"]
    frame.translation = Vector((by_eye.x, by_butt.y, by_eye.z))
    if rig.report:
        print(f"  the butt is {(by_eye - by_butt) * 100} cm from the shoulder pocket (x, z count)")
    return frame


# ---------------------------------------------------------------------------------------------------------------------
# A clip: a base pose from the library (legs, spine, breathing), the extra bends, the gun and the hands per frame

class Clip:
    def __init__(self, name, base, frames=None, aim_pitch=0.0, low_ready=False):
        self.name = name
        self.base = base             # the library's clip the body comes from
        self.frames = frames         # None: as many as the base
        self.aim_pitch = aim_pitch   # degrees up: the spine bends, the gun follows the chest
        self.low_ready = low_ready
        self.gun_motion = []         # [(time, Matrix)]: the gun's motion in its own frame (kick, a tilt for the reload)
        self.left = None             # [(time, Matrix)] the left hand in the gun's frame, None: on the support
        self.right = None            # the same for the right hand, None: on the grip
        self.left_world = {}         # the left hand's trips to the belt pouch: {"from", "pouch", "to"[, "again"]} times
        self.left_after = None       # [(time, Matrix)] the left hand in the gun's frame after the first trip
        self.weapon = None
        self.left_free = False        # the left hand leaves the gun (the belt pouch): its elbow rules don't hold
        self.below_hand_margin = 0.0  # how far (m) an elbow may be above its hand: the gun aimed down lowers the hands


def make_clips(weapon):
    w = WEAPONS[weapon]
    clips = []

    clips.append(Clip(f"{weapon}_Idle_Loop", "Idle_Loop", low_ready=True))
    for name, pitch in (("Up", 45.0), ("Neutral", 0.0), ("Down", -45.0)):
        clips.append(Clip(f"{weapon}_Aim_{name}", "Pistol_Aim_Neutral", aim_pitch=pitch))
        clips[-1].below_hand_margin = 0.06 if pitch < 0.0 else 0.0

    shoot = Clip(f"{weapon}_Shoot", "Pistol_Aim_Neutral", frames=int(FPS * (0.35 if weapon == "Rifle" else 0.6)))
    kick = 0.035 if weapon == "Rifle" else 0.06
    rise = 4.0 if weapon == "Rifle" else 8.0
    shoot.gun_motion = [(0.0, Matrix.Identity(4)),
                        (0.04, Matrix.Translation((-kick, 0, 0)) @ rot("Y", -rise)),
                        (0.3 if weapon == "Rifle" else 0.5, Matrix.Identity(4))]
    clips.append(shoot)

    support = left_hand_under(w["Support"])
    grip = right_hand_on_grip(w)
    tilt = Matrix.Translation((0.0, 0.05, -0.04)) @ rot("X", -25.0) @ rot("Y", 12.0)  # rolled to show the magazine side
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
        cycle.gun_motion = [(0.0, Matrix.Identity(4)), (0.15, rot("X", -10.0)), (0.95, rot("X", -10.0)),
                            (1.1, Matrix.Identity(4))]
        cycle.right = [(0.0, grip), (0.2, knob), (0.35, up), (0.5, back), (0.65, up), (0.8, knob), (1.1, grip)]
        clips.append(cycle)

        reload = Clip("Sniper_Reload", "Pistol_Aim_Neutral", frames=int(FPS * 2.6))
        reload.left_free = True
        reload.gun_motion = [(0.0, Matrix.Identity(4)), (0.3, tilt), (2.3, tilt), (2.6, Matrix.Identity(4))]
        reload.right = [(0.0, grip), (0.25, knob), (0.4, up), (0.55, back), (1.95, back), (2.1, up), (2.25, knob),
                        (2.6, grip)]
        port = left_hand_on(w["Magazine"])
        reload.left = [(0.0, support), (0.6, support)]
        reload.left_world = {"from": 0.6, "pouch": 0.85, "to": 1.1, "again": (1.3, 1.55, 1.8)}
        reload.left_after = [(1.1, port), (1.25, port), (1.8, port), (1.95, port), (2.3, support)]
        clips.append(reload)
    return clips


# ---------------------------------------------------------------------------------------------------------------------
# Baking

def belt_pouch(rig):
    pelvis = rig.bone("pelvis").translation
    return pelvis + LEFT * 0.14 + FORWARD * 0.08 + UP * 0.05


def pouch_hand(rig):
    hand = frame_from_axes(UP * -1.0, FORWARD, LEFT * -1.0, belt_pouch(rig))  # fingers forward, palm to the body
    return hand


def left_hand_at(clip, time, gun, rig):
    """The left hand in armature space"""
    support = gun @ left_hand_under(WEAPONS[clip.weapon]["Support"])
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


def twist_chest(pose):
    turn = Matrix.Rotation(math.radians(CHEST_TWIST / 2.0), 4, "Y")
    pose["spine_02"] = pose["spine_02"] @ turn
    pose["spine_03"] = pose["spine_03"] @ turn
    pose["neck_01"] = pose["neck_01"] @ Matrix.Rotation(math.radians(-CHEST_TWIST), 4, "Y")


def bake_clip(rig, clip, base_frames, finger_pose, gun_frame):
    armature = rig.armature
    w = WEAPONS[clip.weapon]
    count = clip.frames or len(base_frames)
    head_bend = Matrix.Rotation(math.radians(20.0), 4, "X") @ Matrix.Rotation(math.radians(8.0), 4, "Z")  # down onto the stock, to the right

    # The gun's place on the chest, from the first frame's pose
    def body(f):
        pose = dict(base_frames[min(f, len(base_frames) - 1)])
        pose.update(finger_pose)
        if not clip.low_ready:
            bend = Matrix.Rotation(math.radians(-clip.aim_pitch * 1.13 / 3.0), 4, "X")  # +x leans forward; 1.13: the bones' axes are twisted, so the gun turned 40 of 45 degrees
            for name in SPINE:
                pose[name] = pose[name] @ bend
            twist_chest(pose)
            pose["Head"] = pose["Head"] @ head_bend
        return pose

    rig.set_pose(body(0))
    rig.update()
    if clip.low_ready:
        at_shoulder = gun_at_shoulder(w, rig)
        pivot = at_shoulder @ Matrix.Translation(w["Butt"])
        low = pivot @ rot("Y", 30.0) @ rot("Z", 25.0) @ Matrix.Translation(-w["Butt"])  # muzzle down and to the left
        placed = low @ Matrix.Translation((0.0, 0.0, 0.02))
        rig.set_pose(base_frames[0])
        rig.update()
    else:
        # Level at the neutral aim, then turned with the chest's bend
        neutral_pose = dict(base_frames[0])
        neutral_pose.update(finger_pose)
        twist_chest(neutral_pose)
        neutral_pose["Head"] = neutral_pose["Head"] @ head_bend
        rig.set_pose(neutral_pose)
        rig.update()
        placed = gun_at_shoulder(w, rig)
        chest_neutral = rig.bone("spine_03")
        rig.set_pose(body(0))
        rig.update()
        placed = rig.bone("spine_03") @ chest_neutral.inverted() @ placed
    on_chest = rig.bone("spine_03").inverted() @ placed

    action = bpy.data.actions.new(clip.name)
    poses = []
    lowest = {"r": math.inf, "l": math.inf}
    worst_below = {"r": math.inf, "l": math.inf}
    worst_outside = {"r": math.inf, "l": math.inf}
    for f in range(count):
        time = f / FPS
        rig.set_pose(body(f))
        rig.update()
        motion = keyed(clip.gun_motion, time) if clip.gun_motion else Matrix.Identity(4)
        gun = rig.bone("spine_03") @ on_chest @ motion
        right = gun @ (keyed(clip.right, time) if clip.right else right_hand_on_grip(w, clip.wrist_turn))
        left = left_hand_at(clip, time, gun, rig)

        def place(side, hand):
            rig.hand_target[side].matrix_world = armature.matrix_world @ hand
            # The elbow's target out to the side and down from the middle of its arm (the shoulder and the hand)
            shoulder = rig.bone(f"upperarm_{side}").translation
            outward = LEFT * -1.0 if side == "r" else LEFT
            middle = shoulder.lerp(hand.translation, 0.5)
            out, down = ELBOW_TARGET[side] if not (side == "r" and clip.right) else ELBOW_TARGET["l"]  # off the grip: out
            rig.pole[side].matrix_world = Matrix.Translation(armature.matrix_world @ (middle + outward * out - UP * down))

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
        if clip.left is None:
            # The left hand straight along its forearm (three rounds: the forearm moves a little with the hand)
            for _ in range(3):
                left, twist = straight_left_hand(rig, gun, w, rig.left_rest)
                place("l", left)
                rig.update()
            if f == 0:
                print(f"  {clip.name}: the left hand turned {twist:.1f} degrees about its forearm (half of it the forearm's roll)")
        if clip.left is None:
            # The left hand under the handguard: its fingers closed round it
            for name in FINGERS:
                if name.endswith("_l"):
                    armature.pose.bones[name].matrix_basis = Matrix.Identity(4)
            gap, finger = wrap_left_fingers(rig, gun, clip.guard, clip.fist)
            if f == 0 and gap > 0.0:
                print(f"  {clip.name}: the left {finger} stays {gap * 100:.1f} cm from the handguard at the closest")
        if clip.right is None:
            # The right hand on the grip: its fingers round it, the index finger on the trigger
            trigger = grip_right_fingers(rig, gun, w, clip.fist)
            if f == 0:
                print(f"  {clip.name}: the right index finger's last joint {trigger * 100:.1f} cm from the trigger")
        local = rig.local_pose()
        if clip.left is None:
            local = roll_forearm(local, armature, twist * 0.5)
        poses.append((local, gun.copy()))
        for side in "rl":
            drop = elbow_drop(rig, side)
            lowest[side] = min(lowest[side], drop)
            below, outside = elbow_place(rig, side)
            worst_below[side] = min(worst_below[side], below)
            worst_outside[side] = min(worst_outside[side], outside)
            if not (side == "l" and clip.left_free) and (below < -clip.below_hand_margin or outside < -ELBOW_INSIDE[side]):
                raise SystemExit(f"{clip.name}, frame {f}: the {'right' if side == 'r' else 'left'} elbow is "
                                 f"{below * 100:.1f} cm below its hand and {outside * 100:.1f} cm outside its shoulder "
                                 f"(both wanted above 0): check the elbow targets and the chest's twist")
            if drop < ELBOW_MIN_DROP:
                raise SystemExit(f"{clip.name}, frame {f}: the {'right' if side == 'r' else 'left'} elbow is "
                                 f"{drop * 100:.1f} cm below the line from the shoulder to the hand (above it): check the arm's "
                                 f"IK pole")
        if f == count // 2:
            miss = {s: (rig.bone(f"hand_{s}").translation - rig.hand_target[s].matrix_world.translation).length
                    for s in "rl"}
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
            print(f"  {clip.name}: gun pitch {pitch:.1f}, yaw {yaw:.1f} (deg); hands off target r {miss['r'] * 100:.1f} cm,"
                  f" l {miss['l'] * 100:.1f} cm; elbows below the shoulder-hand line r {elbow_drop(rig, 'r') * 100:.1f} cm,"
                  f" l {elbow_drop(rig, 'l') * 100:.1f} cm")

    print(f"  {clip.name}: the elbows at least r {lowest['r'] * 100:.1f} cm, l {lowest['l'] * 100:.1f} cm below the "
          f"shoulder-hand line in every frame; below the hand at least r {worst_below['r'] * 100:.1f}, "
          f"l {worst_below['l'] * 100:.1f} cm; outside the shoulder at least r {worst_outside['r'] * 100:.1f}, "
          f"l {worst_outside['l'] * 100:.1f} cm")
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
    }
    for view, (location, rotation) in views.items():
        camera.location = location
        camera.rotation_euler = rotation
        camera.data.ortho_scale = 1.6
        scene.render.filepath = os.path.join(folder, f"{clip.name}_{view}.png")
        bpy.ops.render.render(write_still=True)
    # Close-ups of the hands on the gun: from the right and from the left, level with them
    hands = (rig.bone("hand_r").translation + rig.bone("hand_l").translation) * 0.5
    for view, (offset, rotation) in {"hands_right": ((-1.5, 0.0, 0.0), (math.radians(90), 0, math.radians(-90))),
                                     "hands_left": ((1.5, 0.0, 0.0), (math.radians(90), 0, math.radians(90)))}.items():
        camera.location = rig.armature.matrix_world @ (hands + Vector(offset))
        camera.rotation_euler = rotation
        camera.data.ortho_scale = 0.55
        scene.render.filepath = os.path.join(folder, f"{clip.name}_{view}.png")
        bpy.ops.render.render(write_still=True)
    camera.data.ortho_scale = 1.6
    for c in [c for pb in rig.armature.pose.bones for c in pb.constraints]:
        c.mute = False
    mannequin.hide_render = False


# ---------------------------------------------------------------------------------------------------------------------

def main():
    preview = args()
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
    # Where the right index finger's knuckle is on the hand (its bone's space), for the hand on the grip
    rig.set_pose(bases["Idle_Loop"][0])
    rig.update()
    KNUCKLE.xyz = rig.bone("hand_r").inverted() @ armature.pose.bones["index_01_r"].head
    print(f"The right index knuckle in the hand's space: {tuple(round(v * 100, 1) for v in KNUCKLE)} cm")
    for weapon in WEAPONS:
        frame, mesh = load_gun(weapon)
        guard = handguard(mesh, WEAPONS[weapon])
        WEAPONS[weapon]["GripSection"] = grip_section(mesh, WEAPONS[weapon])
        width, front, _ = WEAPONS[weapon]["GripSection"]
        print(f"  the pistol grip: {width * 200:.1f} cm wide, {front * 200:.1f} cm deep")
        for clip in make_clips(weapon):
            clip.weapon = weapon
            clip.guard = guard
            clip.fist = fist
            clip.wrist_turn = 0.0
            action, poses = bake_clip(rig, clip, bases[clip.base], finger_pose, frame)
            write_action(armature, action, poses)
            made.append((clip, action, poses))
            if preview:
                render_preview(preview, rig, clip, poses, frame)
            print(f"{clip.name}: {len(poses)} frames")
        bpy.data.objects.remove(mesh)
        bpy.data.objects.remove(frame)

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

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=OUTPUT, export_format="GLB", export_animations=True,
                              export_animation_mode="ACTIONS", export_force_sampling=True, export_skins=True,
                              export_yup=True)
    print(f"Wrote {OUTPUT}: {len(made)} clips")


main()
