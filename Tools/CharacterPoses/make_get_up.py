# Makes a soldier's death on its back and its get-up (the shooter's plan, Phase 2.4; the user: the training dummy stood
# up with its right leg swinging through the floor). The dummy fell with the library's Death01 and stood up by a
# cross-fade to Idle: each bone turned on its own the shortest way, the legs through the ground. The animation library
# has no get-up, so this is our own, on the library's rig, in Blender without its window:
#
#   Death_Back: Death01, the left arm settling into a relaxed one on the floor once the body is down (the user,
#   2026-11-22: Death01 leaves it bent a right angle at the shoulder's height); the arm by IK, its hand as the forearm
#   carries it standing, the arm turned at the shoulder so the hand lies on the floor
#
#   Get_Up, from where Death_Back ends to where Idle_Loop starts (the first phase remade 2026-11-22, the user: the elbows
#   lifted first, "extremely awkward"):
#   lying -> a quick lift of the head, the shoulders and the back onto both elbows and forearms on the floor, the knees
#   starting up -> the left hand, then the right, back behind the hips: sitting on the hands, the knees up, the feet flat
#   -> forward over the feet into a deep squat, the hands on the knees -> standing (Idle_Loop's first frame)
#
# The hips follow a smooth path through the key poses; the feet and the hands follow their own (from the knees drawn up
# on, the feet stay where Idle_Loop stands them; leaving the floor the hands go with the shoulders, to the knees, then
# to Idle_Loop's hang); the legs and the arms are placed by two-bone IK onto them, so no limb turns through the floor,
# each arm bone's roll Idle_Loop's from the arm's bend plane (the elbow a hinge); the spine, the neck and the head turn
# between the key poses' (the chest's lean set in the world). Every frame the skinned mannequin is checked against the
# floor (Death_Back against Death01's own frames): the run fails if any part of it is more than FLOOR_TOLERANCE below
# it; and the supports: on the elbows the forearms lie on the floor, sitting the hands, the hips down until sitting.
# The engine plays both clips without In Place (the hips move from where Death01 left them to where Idle_Loop stands).
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/CharacterPoses/make_get_up.py -- [--preview <folder>]
#
# Reads  Projects/Outpost/Assets/Models/QuaterniusAnimations/UAL1_Standard.glb (the rig, Death01, Idle_Loop)
# Writes Projects/Outpost/Assets/Models/SoldierAnimations/Soldier_GetUp.glb (the mannequin, Get_Up and Death_Back)
#
# Everything is generated: edits by hand in Blender are lost on the next run. --preview renders every third frame from
# the side, the front, three quarters and above (Workbench).

import bpy
import math
import os
import sys
from mathutils import Matrix, Quaternion, Vector

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
ASSETS = os.path.join(ROOT, "Projects", "Outpost", "Assets", "Models")
LIBRARY = os.path.join(ASSETS, "QuaterniusAnimations", "UAL1_Standard.glb")
OUTPUT = os.path.join(ASSETS, "SoldierAnimations", "Soldier_GetUp.glb")

FPS = 30
CLIP = "Get_Up"
DEATH_END = 72          # Death01's last frame (it lies still from about frame 45)
FLOOR_TOLERANCE = 0.005 # meters any part of the mesh may be lower than the floor, or than Death01's lying pose itself
                        # sinks into it (its back: 3.2 cm), whichever is lower

# The key poses' times (seconds) and how the body is in them. The rig's armature space: x to the character's left, -y
# forward, z up, the ground at z 0; Death01 leaves the hips 0.46 m behind where it stood, lying with the head behind.
# The first phase (the user, 2026-11-22: the elbows lifted first, "extremely awkward"): a quick lift of the head, the
# shoulders and the back onto both elbows and forearms on the floor (T_ELBOWS), then from the elbows onto the hands
# behind the hips, one then the other (sitting, T_SIT); the knees drawn up meanwhile (the feet flat by T_FEET). The
# second phase as before: forward into a squat, standing
T_ELBOWS, T_FEET, T_SIT, T_SQUAT, T_STAND = 0.4, 0.85, 1.15, 1.8, 2.55
ELBOW_FLOOR = 0.065        # meters the elbow's joint is over the floor, on it (its skin then 0 to 1 cm over it)
FOREARM_FLOOR = 0.055      # the wrist's, the forearm flat on the floor ahead
ELBOW_OUT = 0.05           # meters the elbow is out from under the shoulder
ELBOW_PELVIS_UP = 0.0      # degrees the pelvis turns up with the chest (8 lifted the buttocks 6.5 cm)
ELBOW_HEAD_FORWARD = 25.0  # degrees the head is bent forward from the chest (the chin toward it)
LEFT_HAND_FIRST = (0.55, 0.85)   # the left hand leaves the floor ahead, is behind the hips (seconds)
HAND_SLIDE_CLEAR = 0.03          # meters the wrist is over its floor height while a hand slides along the floor (at 0.01
                                 # the left forearm went 0.4 cm too far into it, at 0.02 0.1)
RIGHT_HAND_THEN = (0.7, 1.0)     # the right one after it

# The death (the user, 2026-11-22: the left arm in Death01's end, its elbow bent a right angle at the shoulder's height,
# looked awkward): Death_Back is Death01 with the left arm eased into a relaxed lying one once the body is down
DEATH_CLIP = "Death_Back"
DEATH_ARM_FROM, DEATH_ARM_TO = 40, 60   # Death01's frames over which the left arm settles (it lies still from about 45)
RELAXED_ARM_OUT = 45.0                  # degrees the arm lies out from the body (toward the hips from straight out)
RELAXED_REACH = 0.96                    # of the arm's length from the shoulder to the wrist (the elbow a little bent)
RELAXED_WRIST_FLOOR = 0.05              # meters the wrist's joint is over the floor (the hand palm up on it)

SIT_HIPS = Vector((0.01, 0.50, 0.075))  # the pelvis's joint sitting on the floor (the buttocks on it: at 0.13 they were 6.5 cm up)
SIT_PELVIS_BACK = 45.0                  # degrees the pelvis is tilted back from standing, sitting
SIT_CHEST_BACK = 36.0                   # degrees the chest leans back from upright (the weight on the hand behind)
SIT_HEAD_BACK = 5.0
SIT_HAND = Vector((0.27, 0.80, 0.050))  # the wrists on the floor behind the hips, sitting (the palms flat; x: the left's)
SQUAT_HIPS = Vector((0.01, 0.13, 0.43))  # the pelvis's joint over the feet, low
SQUAT_PELVIS_FORWARD = 30.0
SQUAT_CHEST_FORWARD = 42.0               # the chest well forward over the knees (the weight over the feet)
SQUAT_HEAD_FORWARD = 15.0
KNEE_HAND_ABOVE = 0.07                   # the wrists this far above the knee they rest on, squatting

STRETCH = 0.995

LEGS = ("thigh", "calf", "foot", "ball")
ARMS = ("upperarm", "lowerarm", "hand")
SPINE = ["spine_01", "spine_02", "spine_03"]
HEAD = ["neck_01", "Head"]


def args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    return os.path.abspath(argv[argv.index("--preview") + 1]) if "--preview" in argv else None


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def eased(t):
    """Half linear, half smoothstep: through a key pose the motion slows, it doesn't stop"""
    return 0.5 * t + 0.5 * smooth(t)


def update():
    bpy.context.view_layer.update()


def load_library():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = FPS
    bpy.ops.import_scene.gltf(filepath=LIBRARY)
    armature = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    for o in list(bpy.data.objects):
        if o != armature and o.name != "Mannequin":
            bpy.data.objects.remove(o)
    return armature


def sample(armature, action_name, frame):
    armature.animation_data.action = bpy.data.actions[action_name]
    bpy.context.scene.frame_set(frame)
    pose = {pb.name: pb.matrix_basis.copy() for pb in armature.pose.bones}
    armature.animation_data.action = None
    return pose


def set_pose(armature, basis):
    for name, m in basis.items():
        armature.pose.bones[name].matrix_basis = m
    update()


def basis_of(armature):
    return {pb.name: pb.matrix_basis.copy() for pb in armature.pose.bones}


def world(armature, name):
    """A bone's matrix in armature space"""
    return armature.pose.bones[name].matrix.copy()


def set_world(armature, name, location, rotation):
    armature.pose.bones[name].matrix = Matrix.Translation(location) @ rotation.to_matrix().to_4x4()
    update()


def aim_bone(armature, name, direction):
    """Turns a bone (the least turn) so it points along a direction, in armature space; its head stays"""
    pb = armature.pose.bones[name]
    m = pb.matrix.copy()
    turn = m.col[1].to_3d().normalized().rotation_difference(direction.normalized())
    pb.matrix = Matrix.Translation(m.translation) @ (turn @ m.to_quaternion()).to_matrix().to_4x4()
    update()


def turn_world(armature, name, turn):
    """Turns a bone (and what hangs from it) by a turn in armature space round its head"""
    pb = armature.pose.bones[name]
    m = pb.matrix.copy()
    pb.matrix = Matrix.Translation(m.translation) @ (turn @ m.to_quaternion()).to_matrix().to_4x4()
    update()


def tilt(degrees):
    """Forward (positive) about the character's left-right axis: the up axis toward -y"""
    return Quaternion(Vector((1.0, 0.0, 0.0)), math.radians(degrees))


def two_bone(root, target, upper, lower, hint, stretch=STRETCH):
    """The middle joint (knee, elbow) for a root and an end, bending toward a hint direction"""
    to_end = target - root
    d = min(to_end.length, (upper + lower) * stretch)
    direction = to_end.normalized()
    bend = hint - direction * hint.dot(direction)
    if bend.length < 1e-6:
        bend = Vector((0.0, 0.0, 1.0)) - direction * direction.z
    bend.normalize()
    a = (upper * upper - lower * lower + d * d) / (2.0 * d)
    h = math.sqrt(max(0.0, upper * upper - a * a))
    return root + direction * a + bend * h, to_end.length > (upper + lower) * stretch


def joint_hint(root, middle, end):
    """Which way a middle joint bends now: from the line root-end toward the joint"""
    direction = (end - root).normalized()
    out = (middle - root) - direction * (middle - root).dot(direction)
    return out.normalized() if out.length > 1e-6 else Vector((0.0, 0.0, 1.0))


# ---------------------------------------------------------------------------------------------------------------------
# Tracks: a value through time, keyed; positions on a Catmull-Rom curve (still at the first and last key), turns slerped
# with eased in-betweens

class Track:
    def __init__(self, keys):
        self.keys = sorted(keys, key=lambda k: k[0])

    def segment(self, t):
        keys = self.keys
        if t <= keys[0][0]:
            return 0, 0.0
        for i in range(len(keys) - 1):
            if t <= keys[i + 1][0]:
                return i, (t - keys[i][0]) / (keys[i + 1][0] - keys[i][0])
        return len(keys) - 2, 1.0


class PointTrack(Track):
    def at(self, t):
        keys = self.keys
        if len(keys) == 1:
            return keys[0][1].copy()
        i, u = self.segment(t)
        (t0, p0), (t1, p1) = keys[i], keys[i + 1]
        # Tangents: zero at the ends, else Catmull-Rom (by time)
        m0 = Vector() if i == 0 else (p1 - keys[i - 1][1]) / (t1 - keys[i - 1][0]) * (t1 - t0)
        m1 = Vector() if i + 2 >= len(keys) else (keys[i + 2][1] - p0) / (keys[i + 2][0] - t0) * (t1 - t0)
        u2, u3 = u * u, u * u * u
        return (2 * u3 - 3 * u2 + 1) * p0 + (u3 - 2 * u2 + u) * m0 + (-2 * u3 + 3 * u2) * p1 + (u3 - u2) * m1


class TurnTrack(Track):
    def at(self, t):
        keys = self.keys
        if len(keys) == 1:
            return keys[0][1].copy()
        i, u = self.segment(t)
        q0, q1 = keys[i][1], keys[i + 1][1]
        if q0.dot(q1) < 0.0:
            q1 = -q1
        return q0.slerp(q1, eased(u))


class VectorTrack(Track):
    """Directions (the joints' bend), lerped and normalized"""
    def at(self, t):
        keys = self.keys
        if len(keys) == 1:
            return keys[0][1].copy()
        i, u = self.segment(t)
        v = keys[i][1].lerp(keys[i + 1][1], eased(u))
        return v.normalized() if v.length > 1e-6 else keys[i + 1][1].copy()


class LocalTrack(Track):
    """A bone's matrix_basis: its place lerped and its turn slerped"""
    def at(self, t):
        keys = self.keys
        if len(keys) == 1:
            return keys[0][1].copy()
        i, u = self.segment(t)
        l0, r0, s0 = keys[i][1].decompose()
        l1, r1, s1 = keys[i + 1][1].decompose()
        if r0.dot(r1) < 0.0:
            r1 = -r1
        w = eased(u)
        return Matrix.LocRotScale(l0.lerp(l1, w), r0.slerp(r1, w), s0.lerp(s1, w))


# ---------------------------------------------------------------------------------------------------------------------

def lean_chain(armature, names, reference_rotation, top):
    """Turns a chain of bones (each by an equal share, in armature space) until its top bone has a rotation"""
    current = world(armature, top).to_quaternion()
    turn = reference_rotation @ current.inverted()
    axis, angle = turn.to_axis_angle()
    if angle > math.pi:
        angle -= 2.0 * math.pi
    share = Quaternion(axis, angle / len(names))
    for name in names:
        turn_world(armature, name, share)


def palm_down(armature, side, direction, rest):
    """The hand flat, palm down, its fingers along a horizontal direction: the rest pose's hand (the T pose: palm down)
    turned about the vertical"""
    rest_rotation = rest[f"hand_{side}"]
    pointing = (rest_rotation @ Vector((0.0, 1.0, 0.0)))
    pointing.z = 0.0
    wanted = Vector((direction.x, direction.y, 0.0))
    return pointing.normalized().rotation_difference(wanted.normalized()) @ rest_rotation


def main():
    preview = args()
    armature = load_library()
    bones = armature.data.bones
    lengths = {name: (bones[child].head_local - bones[name].head_local).length
               for name, child in (("thigh_l", "calf_l"), ("calf_l", "foot_l"), ("upperarm_l", "lowerarm_l"), ("lowerarm_l", "hand_l"))}
    thigh, calf = lengths["thigh_l"], lengths["calf_l"]
    upperarm, lowerarm = lengths["upperarm_l"], lengths["lowerarm_l"]
    rest = {b.name: b.matrix_local.to_quaternion() for b in bones}

    standing = sample(armature, "Idle_Loop", 0)
    duration_frames = round(T_STAND * FPS)

    # What a pose is, in armature space
    def measure(pose):
        set_pose(armature, pose)
        m = {name: world(armature, name) for name in armature.pose.bones.keys()}
        return m

    S = measure(standing)

    # The elbows as Idle_Loop's (the user, 2026-11-22: not twisted): each arm bone's roll about its length is the one it
    # has standing, measured from the arm's bend plane (the bone's direction and the way the elbow bends from the line
    # shoulder-wrist), so the elbow stays a hinge and only the shoulder turns the arm. The wrist's turn from the
    # forearm (Idle_Loop's way) is reported
    def bend_frame(direction, bend):
        x = direction.normalized()
        z = (bend - x * bend.dot(x))
        z = z.normalized() if z.length > 1e-6 else Vector((0.0, 0.0, 1.0))
        y = z.cross(x)
        return Matrix((x, y, z)).transposed().to_quaternion()

    idle_roll, idle_wrist = {}, {}
    for side in "lr":
        sh, el, wr = (S[f"{b}_{side}"].translation for b in ARMS)
        bend = joint_hint(sh, el, wr)
        idle_roll[side] = (bend_frame(el - sh, bend).inverted() @ S[f"upperarm_{side}"].to_quaternion(),
                           bend_frame(wr - el, bend).inverted() @ S[f"lowerarm_{side}"].to_quaternion())
        idle_wrist[side] = S[f"lowerarm_{side}"].to_quaternion().inverted() @ S[f"hand_{side}"].to_quaternion()

    def place_arm_bone(name, head, rotation):
        pb = armature.pose.bones[name]
        pb.matrix = Matrix.Translation(head) @ rotation.to_matrix().to_4x4()
        update()

    def place_arm(side, shoulder, wrist, hint, hand_rotation=None):
        """The arm by two-bone IK onto a wrist, bending toward hint, its bones' rolls Idle's; the hand turned so (None:
        as the forearm carries it, Idle's wrist). Returns whether it was at full stretch"""
        elbow, over = two_bone(shoulder, wrist, upperarm, lowerarm, hint)
        bend = joint_hint(shoulder, elbow, wrist)
        upper_roll, lower_roll = idle_roll[side]
        place_arm_bone(f"upperarm_{side}", shoulder, bend_frame(elbow - shoulder, bend) @ upper_roll)
        elbow_now = world(armature, f"lowerarm_{side}").translation
        place_arm_bone(f"lowerarm_{side}", elbow_now, bend_frame(wrist - elbow_now, bend) @ lower_roll)
        if hand_rotation is None:
            hand_rotation = world(armature, f"lowerarm_{side}").to_quaternion() @ idle_wrist[side]
        set_world(armature, f"hand_{side}", world(armature, f"hand_{side}").translation, hand_rotation)
        return over

    # ---- The death: Death01, the left arm eased into a relaxed one lying on the floor, out toward the hips
    death = [sample(armature, "Death01", f) for f in range(DEATH_END + 1)]
    set_pose(armature, death[-1])
    shoulder = world(armature, "upperarm_l").translation
    out_angle = math.radians(RELAXED_ARM_OUT)
    lying_way = Vector((math.cos(out_angle), -math.sin(out_angle), 0.0))  # out (+x) and toward the feet (-y)
    wrist = shoulder + lying_way * (upperarm + lowerarm) * RELAXED_REACH
    wrist.z = RELAXED_WRIST_FLOOR
    # The hand as the forearm carries it standing (Idle's wrist: no twist; turned palm up the wrist was wrung 157 degrees),
    # the arm turned at the shoulder instead: of the elbow's ways round the shoulder-wrist line, the one that lays the
    # hand flattest (its skin's lowest highest: the thumb under the palm went 9 cm into the floor), the elbow not into the
    # floor; then the wrist lowered until the hand's skin just touches the floor
    hand_parts = {name for name in armature.pose.bones.keys() if name.endswith("_l") and
                  name.split("_")[0] in ("hand", "thumb", "index", "middle", "ring", "pinky")}
    line = (wrist - shoulder).normalized()
    side_way = line.cross(Vector((0.0, 0.0, 1.0))).normalized()
    up_way = side_way.cross(line).normalized()
    best = None
    for step in range(24):
        a = math.radians(15.0 * step)
        hint = up_way * math.cos(a) + side_way * math.sin(a)
        set_pose(armature, death[-1])
        place_arm("l", shoulder, wrist, hint)
        pose = basis_of(armature)
        hand_low, _ = lowest(armature, pose, hand_parts)
        arm_low, _ = lowest(armature, pose, {"upperarm_l", "lowerarm_l"})
        if arm_low > -FLOOR_TOLERANCE and (best is None or hand_low > best[0]):
            best = (hand_low, hint, 15 * step)
    if best is None:
        raise SystemExit(f"{DEATH_CLIP}: no way of the elbow keeps the arm out of the floor")
    wrist.z -= best[0] - 0.003
    set_pose(armature, death[-1])
    place_arm("l", shoulder, wrist, best[1])
    print(f"{DEATH_CLIP}: the left arm's elbow {best[2]} degrees round from up, the wrist {wrist.z * 100:.1f} cm over the floor")
    relaxed_wrist, relaxed_hint = wrist.copy(), best[1]
    relaxed_hand = world(armature, "hand_l").to_quaternion()
    # On the way there the arm is placed by IK (a blend of the bones' turns swung the hand 12 cm through the floor): the
    # wrist from Death01's along a path lifted a little off the floor, the elbow's way blended, the hand turned from
    # Death01's to the carried one
    for f in range(DEATH_ARM_FROM, DEATH_END + 1):
        w = smooth((f - DEATH_ARM_FROM) / (DEATH_ARM_TO - DEATH_ARM_FROM))
        set_pose(armature, death[f])
        sh, el, wr = (world(armature, f"{b}_l").translation for b in ARMS)
        start_hand = world(armature, "hand_l").to_quaternion()
        path = wr.lerp(relaxed_wrist, w) + Vector((0.0, 0.0, 0.10 * math.sin(math.pi * w)))
        hint = (joint_hint(sh, el, wr).lerp(relaxed_hint, w))
        hint = hint.normalized() if hint.length > 1e-6 else relaxed_hint
        if relaxed_hand.dot(start_hand) < 0.0:
            start_hand = -start_hand
        place_arm("l", sh, path, hint, start_hand.slerp(relaxed_hand, w))
        death[f] = basis_of(armature)
    lying = death[-1]
    L = measure(lying)
    for name in ("pelvis", "foot_l", "foot_r", "hand_l", "hand_r"):
        print(f"{name}: lying {tuple(round(v, 3) for v in L[name].translation)}, standing {tuple(round(v, 3) for v in S[name].translation)}")

    # ---- The key poses' upper bodies (the spine, neck and head locals), built on their hips
    def upper_key(hips, pelvis_rotation, chest_lean, head_lean):
        """The pose's spine and head: Idle_Loop's, on these hips, the chest and head leaning (forward +) in the world"""
        set_pose(armature, standing)
        set_world(armature, "pelvis", hips, pelvis_rotation)
        lean_chain(armature, SPINE, tilt(chest_lean) @ S["spine_03"].to_quaternion(), "spine_03")
        lean_chain(armature, HEAD, tilt(head_lean) @ S["Head"].to_quaternion(), "Head")
        return basis_of(armature)

    lying_hips = L["pelvis"].translation
    elbows_hips = lying_hips + Vector((0.0, -0.02, 0.01))
    elbows_pelvis = tilt(ELBOW_PELVIS_UP) @ L["pelvis"].to_quaternion()
    # Up onto the elbows: the chest as far up as keeps the elbows on the floor under the shoulders (the shoulders no
    # higher than the upper arm reaches down to the floor)
    chest_up = None
    for up in range(50, 14, -1):
        key = upper_key(elbows_hips, elbows_pelvis, -(90.0 - up), -(90.0 - up) + ELBOW_HEAD_FORWARD)
        highest = max(world(armature, f"upperarm_{side}").translation.z for side in "lr")
        if highest - ELBOW_FLOOR <= 0.92 * upperarm:
            chest_up, on_elbows = up, key
            break
    if chest_up is None:
        raise SystemExit(f"{CLIP}: no chest lean puts the elbows on the floor")
    print(f"{CLIP}: on the elbows, the chest {chest_up} degrees up off the floor")
    sit_pelvis = tilt(-SIT_PELVIS_BACK) @ S["pelvis"].to_quaternion()
    squat_pelvis = tilt(SQUAT_PELVIS_FORWARD) @ S["pelvis"].to_quaternion()
    sit = upper_key(SIT_HIPS, sit_pelvis, -SIT_CHEST_BACK, -SIT_HEAD_BACK)
    squat = upper_key(SQUAT_HIPS, squat_pelvis, SQUAT_CHEST_FORWARD, SQUAT_HEAD_FORWARD)

    # ---- The tracks
    pelvis_place = PointTrack([(0.0, lying_hips), (T_ELBOWS, elbows_hips),
                               (T_SIT, SIT_HIPS), (T_SQUAT, SQUAT_HIPS), (T_STAND, S["pelvis"].translation)])
    pelvis_turn = TurnTrack([(0.0, L["pelvis"].to_quaternion()), (T_ELBOWS, elbows_pelvis),
                             (T_SIT, sit_pelvis), (T_SQUAT, squat_pelvis), (T_STAND, S["pelvis"].to_quaternion())])
    upper_names = SPINE + HEAD + ["clavicle_l", "clavicle_r"]
    upper = {name: LocalTrack([(0.0, lying[name]), (T_ELBOWS, on_elbows[name]), (T_SIT, sit[name]), (T_SQUAT, squat[name]),
                               (T_STAND, standing[name])]) for name in upper_names}
    # The fingers: straight (the rest pose's) while the hands are on the floor and the knees, Idle_Loop's at the end; the
    # rest (the root) from the lying pose's to the standing's over the whole get-up
    fingers = [name for name in lying if name.split("_")[0] in ("thumb", "index", "middle", "ring", "pinky")]
    others = [name for name in lying if name not in upper_names and name != "pelvis" and name not in fingers
              and not any(name.startswith(part + "_") for part in LEGS + ARMS)]
    rest_tracks = {name: LocalTrack([(0.0, lying[name]), (T_STAND, standing[name])]) for name in others}
    # The rest pose's hand is palm down, but its thumb hangs 10 cm below the palm: flat on the floor it would go through
    # it. Its first bone is turned up into the palm's plane (the rest pose's horizontal), the others straight
    set_pose(armature, {name: Matrix.Identity(4) for name in lying})
    flat_thumb = {}
    for side in "lr":
        base = armature.pose.bones[f"thumb_01_{side}"]
        along = bones[f"thumb_04_leaf_{side}"].tail_local - base.head
        aim_bone(armature, f"thumb_01_{side}", Vector((along.x, along.y, 0.15 * abs(along.length))))
        flat_thumb[f"thumb_01_{side}"] = base.matrix_basis.copy()
    straight = Matrix.Identity(4)
    for name in fingers:
        flat = flat_thumb.get(name, straight)
        rest_tracks[name] = LocalTrack([(0.0, lying[name]), (0.12, flat), (T_SQUAT, flat), (T_STAND, standing[name])])

    # The feet: from where they lie (heels down, toes up) to where Idle_Loop stands them, flat, by the time the knees are up
    feet, knees = {}, {}
    for side in "lr":
        ankle_l, ankle_s = L[f"foot_{side}"].translation, S[f"foot_{side}"].translation
        feet[side] = (PointTrack([(0.0, ankle_l), (T_FEET, ankle_s), (T_STAND, ankle_s)]),
                      TurnTrack([(0.0, L[f"foot_{side}"].to_quaternion()), (T_FEET, S[f"foot_{side}"].to_quaternion()),
                                 (T_STAND, S[f"foot_{side}"].to_quaternion())]),
                      TurnTrack([(0.0, L[f"ball_{side}"].to_quaternion()), (T_FEET, S[f"ball_{side}"].to_quaternion()),
                                 (T_STAND, S[f"ball_{side}"].to_quaternion())]))
        out = 0.25 if side == "l" else -0.25
        knees[side] = VectorTrack([
            (0.0, joint_hint(L[f"thigh_{side}"].translation, L[f"calf_{side}"].translation, ankle_l)),
            (T_FEET, Vector((out, 0.0, 1.0)).normalized()),            # up, a little out
            (T_SIT, Vector((out, -0.6, 1.0)).normalized()),            # up and forward
            (T_SQUAT, Vector((out * 1.2, -1.0, 0.3)).normalized()),     # forward, out over the toes
            (T_STAND, joint_hint(S[f"thigh_{side}"].translation, S[f"calf_{side}"].translation, ankle_s))])

    def pose_upper(t):
        pose = {name: track.at(t) for name, track in rest_tracks.items()}
        pose.update({name: track.at(t) for name, track in upper.items()})
        set_pose(armature, pose)
        set_world(armature, "pelvis", pelvis_place.at(t), pelvis_turn.at(t))

    # The hands (the user, 2026-11-22: they always point forward; the elbows mustn't lift first): from where they lie they
    # slide along the floor, the elbows coming in under the shoulders as the chest lifts, the forearms flat ahead, the
    # palms down (T_ELBOWS); then the left hand, then the right, goes back behind the hips (palm down, the fingers
    # forward) and takes the weight as the chest comes up to sitting; from sitting on they go with the body: to the knees
    # by the squat, then Idle_Loop's hang
    pose_upper(T_ELBOWS)
    hands, elbows, supports = {}, {}, {}
    forward = Vector((0.0, -1.0, 0.0))
    for side in "lr":
        out = 1.0 if side == "l" else -1.0
        shoulder = world(armature, f"upperarm_{side}").translation
        down = shoulder.z - ELBOW_FLOOR
        behind = math.sqrt(max(0.0, upperarm * upperarm - ELBOW_OUT * ELBOW_OUT - down * down))
        elbow_on_floor = Vector((shoulder.x + out * ELBOW_OUT, shoulder.y + behind, ELBOW_FLOOR))
        ahead = Vector((-0.2 * out, -1.0, 0.0)).normalized()
        wrist_on_floor = elbow_on_floor + ahead * lowerarm
        wrist_on_floor.z = FOREARM_FLOOR
        supports[side] = (elbow_on_floor, wrist_on_floor)
        lying_wrist = L[f"hand_{side}"].translation
        # (sliding on the floor as it comes in, 3 cm to spare: the user, 2026-11-22, the hands were on the ground, went up
        # and came down again)
        sliding = lying_wrist.lerp(wrist_on_floor, 0.5)
        sliding.z = max(lying_wrist.z, wrist_on_floor.z) + HAND_SLIDE_CLEAR
        # A hand lying overhead (beyond the shoulder toward the head) comes down along the floor in an arc, out to the
        # side, the arm nearly straight, the hand flat and pointing out along it (straight across, it turned half round on
        # the floor and its fingers went 11 cm in); then the elbow bends in under the shoulder
        overhead = lying_wrist.y > shoulder.y + 0.05
        sweep_turn = None
        if overhead:
            sliding = shoulder + Vector((out, 0.0, 0.0)) * (upperarm + lowerarm) * 0.9
            sliding.z = FOREARM_FLOOR + HAND_SLIDE_CLEAR
            sweep_turn = palm_down(armature, side, Vector((out, 0.0, 0.0)), rest)
        sitting = Vector((out * SIT_HAND.x, SIT_HAND.y, SIT_HAND.z))
        leave, arrive = LEFT_HAND_FIRST if side == "l" else RIGHT_HAND_THEN
        over = wrist_on_floor.lerp(sitting, 0.5)  # (along the floor on its way back, not lifted)
        over.z = max(wrist_on_floor.z, sitting.z) + HAND_SLIDE_CLEAR
        place = PointTrack([(0.0, lying_wrist), (T_ELBOWS * 0.5, sliding), (T_ELBOWS, wrist_on_floor),
                            (leave, wrist_on_floor), ((leave + arrive) * 0.5, over), (arrive, sitting), (T_SIT, sitting)])
        down_toward_feet = palm_down(armature, side, forward, rest)
        # (palm down from T_ELBOWS on; before, eased there from the way the forearm carries it: turned at once while the
        # forearm still pointed back overhead, the wrist bent 176 degrees)
        keys = [(T_ELBOWS, down_toward_feet), (T_SIT, down_toward_feet), (T_SQUAT, down_toward_feet),
                (T_STAND, S[f"hand_{side}"].to_quaternion())]
        if sweep_turn is not None:
            keys.insert(0, (T_ELBOWS * 0.5, sweep_turn))
        turn = TurnTrack(keys)
        hands[side] = (place, turn, sweep_turn is not None)
        on_elbow = joint_hint(shoulder, elbow_on_floor, wrist_on_floor)
        elbows[side] = VectorTrack([
            (0.0, joint_hint(L[f"upperarm_{side}"].translation, L[f"lowerarm_{side}"].translation, L[f"hand_{side}"].translation)),
            # (swinging in, the elbow a little up: straight across, the forearm went through the floor)
            (T_ELBOWS * 0.5, (joint_hint(L[f"upperarm_{side}"].translation, L[f"lowerarm_{side}"].translation, L[f"hand_{side}"].translation)
                              + on_elbow + Vector((0.0, 0.0, 0.6))).normalized()),
            (T_ELBOWS, on_elbow), (leave, on_elbow),
            (arrive, Vector((out * 0.3, 1.0, 0.1)).normalized()),              # back, the hands behind, fingers forward
            (T_SIT, Vector((out * 0.3, 1.0, 0.1)).normalized()),
            (T_SQUAT, Vector((out, 0.3, -0.3)).normalized()),
            (T_STAND, joint_hint(S[f"upperarm_{side}"].translation, S[f"lowerarm_{side}"].translation, S[f"hand_{side}"].translation))])

    pose_upper(T_SIT)
    from_shoulder_sitting = {side: hands[side][0].at(T_SIT) - world(armature, f"upperarm_{side}").translation for side in "lr"}
    from_shoulder_standing = {side: S[f"hand_{side}"].translation - S[f"upperarm_{side}"].translation for side in "lr"}

    def hand_target(side, t, shoulder, knee):
        # The left hand on the left knee (the foot ahead); the right knee (the foot behind) is low and back, out of reach:
        # the right arm hangs loose, a little forward
        on_knee = knee + Vector((0.02, -0.02, KNEE_HAND_ABOVE)) if side == "l" else shoulder + Vector((-0.04, -0.12, -0.50))
        if t <= T_SIT:
            return hands[side][0].at(t)
        if t <= T_SQUAT:
            w = eased((t - T_SIT) / (T_SQUAT - T_SIT))
            return (shoulder + from_shoulder_sitting[side]).lerp(on_knee, w)
        w = eased((t - T_SQUAT) / (T_STAND - T_SQUAT))
        return on_knee.lerp(shoulder + from_shoulder_standing[side], w)

    wrist_turn = {"l": (0.0, 0), "r": (0.0, 0)}
    wrist_heights = {"l": [], "r": []}  # until sitting: the hands stay on the floor (the user: no lift in the middle)

    # ---- Every frame
    poses = []
    stretched = {"legs": 0, "arms": 0}
    for f in range(duration_frames + 1):
        t = f / FPS
        pose_upper(t)
        for side in "lr":
            place, turn, ball = feet[side]
            hip = world(armature, f"thigh_{side}").translation
            ankle = place.at(t)
            knee, over = two_bone(hip, ankle, thigh, calf, knees[side].at(t))
            stretched["legs"] += over
            aim_bone(armature, f"thigh_{side}", knee - hip)
            aim_bone(armature, f"calf_{side}", ankle - world(armature, f"calf_{side}").translation)
            set_world(armature, f"foot_{side}", world(armature, f"foot_{side}").translation, turn.at(t))
            set_world(armature, f"ball_{side}", world(armature, f"ball_{side}").translation, ball.at(t))
        for side in "lr":
            place, turn, sweeps = hands[side]
            shoulder = world(armature, f"upperarm_{side}").translation
            wrist = hand_target(side, t, shoulder, world(armature, f"calf_{side}").translation)
            hand = turn.at(t)
            if t < T_ELBOWS:
                # From the lying hand's turn to the forearm's carrying (Idle's wrist), then onto the palm down
                place_arm(side, shoulder, wrist, elbows[side].at(t))
                carried = world(armature, f"hand_{side}").to_quaternion()
                start = L[f"hand_{side}"].to_quaternion()
                if start.dot(carried) < 0.0:
                    start = -start
                carried = start.slerp(carried, smooth((t - 0.06) / (0.3 * T_ELBOWS)))  # (as it lay, until the wrist is up)
                if carried.dot(hand) < 0.0:
                    hand = -hand
                hand = carried.slerp(hand, smooth(t / (T_ELBOWS * (0.5 if sweeps else 1.0))))
            stretched["arms"] += place_arm(side, shoulder, wrist, elbows[side].at(t), hand)
            if t <= T_SIT + 1e-6:
                wrist_heights[side].append(world(armature, f"hand_{side}").translation.z)
            from_idle = (world(armature, f"lowerarm_{side}").to_quaternion() @ idle_wrist[side]).rotation_difference(
                world(armature, f"hand_{side}").to_quaternion())
            angle = math.degrees(from_idle.angle)
            angle = min(angle, 360.0 - angle)
            if 0 < f < duration_frames and angle > wrist_turn[side][0]:
                wrist_turn[side] = (angle, f)
        poses.append(basis_of(armature))

    # The ends are the clips': the first frame Death01's last, the last Idle_Loop's first (the IK's roll differences gone)
    poses[0] = dict(lying)
    poses[-1] = dict(standing)
    print(f"{CLIP}: {len(poses)} frames ({T_STAND:.2f} s); IK at full stretch: {stretched['legs']} leg and {stretched['arms']} arm frames")
    for side in "lr":
        heights = wrist_heights[side]
        print(f"{CLIP}: the {'left' if side == 'l' else 'right'} wrist's height until sitting (cm), every 3rd frame: " +
              " ".join(f"{h * 100:.0f}" for h in heights[::3]) + f"; at most {max(heights[1:]) * 100:.1f}")
    print(f"{CLIP}: the wrists turned from Idle_Loop's way at most: left {wrist_turn['l'][0]:.0f} degrees (frame "
          f"{wrist_turn['l'][1]}), right {wrist_turn['r'][0]:.0f} (frame {wrist_turn['r'][1]})")

    through = check_floor(armature, poses)
    # The death: no frame lower than Death01's own (its back sinks 3.2 cm into the floor lying)
    original = [sample(armature, "Death01", f) for f in range(DEATH_END + 1)]
    death_through = check_floor(armature, death, [lowest(armature, pose)[0] for pose in original], DEATH_CLIP)
    problems = [p for p in (through, death_through, check_supports(armature, poses, supports)) if p]
    death_action = bake(armature, DEATH_CLIP, death)
    action = bake(armature, CLIP, poses)
    if preview:
        render_preview(preview, armature, poses)
        render_preview(preview, armature, death[DEATH_ARM_FROM - 6:], DEATH_CLIP)
    if problems:
        raise SystemExit("; ".join(problems))
    for a in list(bpy.data.actions):
        if a not in (action, death_action):
            bpy.data.actions.remove(a)
    for pb in armature.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    mannequin = bpy.data.objects["Mannequin"]
    for group in mannequin.vertex_groups:
        empty = [v.index for v in mannequin.data.vertices for g in v.groups if g.group == group.index and g.weight <= 0.0]
        if empty:
            group.remove(empty)
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=OUTPUT, export_format="GLB", export_animations=True,
                              export_animation_mode="ACTIONS", export_force_sampling=True, export_skins=True,
                              export_yup=True)
    print(f"Wrote {OUTPUT}: {CLIP} ({len(poses)} frames), {DEATH_CLIP} ({len(death)} frames)")


def lowest(armature, pose, parts=None):
    """The skinned mannequin's lowest point in a pose (meters) and the bone it is weighted to most; parts: only the
    vertices weighted most to these bones"""
    mannequin = bpy.data.objects["Mannequin"]
    groups = {g.index: g.name for g in mannequin.vertex_groups}
    set_pose(armature, pose)
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = mannequin.evaluated_get(depsgraph)
    mesh = evaluated.to_mesh()
    matrix = mannequin.matrix_world
    low, name = math.inf, "?"
    for v, original in zip(mesh.vertices, mannequin.data.vertices):
        part = max(original.groups, key=lambda g: g.weight, default=None)
        bone = groups.get(part.group, "?") if part else "?"
        if parts is not None and bone not in parts:
            continue
        z = (matrix @ v.co).z
        if z < low:
            low, name = z, bone
    evaluated.to_mesh_clear()
    return low, name


def check_floor(armature, poses, reference=None, clip=CLIP):
    """The skinned mannequin's lowest point each frame: no part may go through the floor more than FLOOR_TOLERANCE (or
    than the lying pose itself sinks; reference: each frame's own limit, the clip it is made from)"""
    worst = (0.0, -1, "")
    lowest_each = [lowest(armature, pose) for pose in poses]
    for f, (low, name) in enumerate(lowest_each):
        floor = min(0.0, reference[f] if reference else lowest_each[0][0]) - FLOOR_TOLERANCE
        if low - floor < worst[0]:
            worst = (low - floor, f, name)
    print(f"{clip}: lowest point by frame (cm): " + " ".join(f"{f}:{low * 100:.1f}({name})" for f, (low, name) in enumerate(lowest_each) if f % 3 == 0))
    if worst[0] < 0.0:
        return f"{clip}: frame {worst[1]} goes {-worst[0] * 100:.1f} cm further through the floor than allowed ({worst[2]})"
    print(f"{clip}: no frame through the floor (the closest {worst[0] * 100:.1f} cm within the limit, frame {worst[1]})")
    return None


SUPPORT_ABOVE = 0.012  # meters the skin of what bears the weight may be above the floor at the most (it lies on it)


def check_supports(armature, poses, supports):
    """The get-up's supports lie on the floor: on the elbows the forearms (their skin's lowest at most SUPPORT_ABOVE over
    it), sitting the hands; the hips down until sitting"""
    problems = []
    f_elbows, f_sit = round(T_ELBOWS * FPS), round(T_SIT * FPS)
    for side in "lr":
        for f, part in ((f_elbows, f"lowerarm_{side}"), (f_sit, f"hand_{side}")):
            low, _ = lowest(armature, poses[f], {part})
            print(f"{CLIP}: frame {f}: {part}'s skin {low * 100:.1f} cm over the floor at the lowest")
            if low > SUPPORT_ABOVE or low < -FLOOR_TOLERANCE:
                problems.append(f"{CLIP}: frame {f}: {part} doesn't lie on the floor ({low * 100:.1f} cm)")
    heights = [(lowest(armature, poses[f], {"pelvis", "thigh_l", "thigh_r"})[0], f) for f in range(0, f_sit + 1, 3)]
    hips = max(h for h, _ in heights)
    print(f"{CLIP}: the hips over the floor until sitting (cm): " + " ".join(f"{f}:{h * 100:.1f}" for h, f in heights))
    if hips > 0.03:
        problems.append(f"{CLIP}: the hips leave the floor before sitting ({hips * 100:.1f} cm)")
    return "; ".join(problems) or None


def bake(armature, clip, poses):
    action = bpy.data.actions.new(clip)
    armature.animation_data.action = action
    for f, pose in enumerate(poses):
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
    return action


def render_preview(folder, armature, poses, clip=CLIP):
    os.makedirs(folder, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.color_type = "MATERIAL"
    scene.render.resolution_x = 360
    scene.render.resolution_y = 360
    camera = bpy.data.objects.new("PreviewCamera", bpy.data.cameras.new("PreviewCamera"))
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 2.4
    scene.collection.objects.link(camera)
    scene.camera = camera
    bpy.ops.mesh.primitive_plane_add(size=6.0, location=(0.0, 0.0, -0.002))
    views = {"side": ((-4.0, 0.4, 0.7), (math.radians(90), 0, math.radians(-90))),
             "front": ((0.0, -4.0, 0.7), (math.radians(90), 0, 0)),
             "quarter": ((-3.0, -2.6, 1.6), (math.radians(68), 0, math.radians(-49))),
             "above": ((0.0, 0.7, 4.0), (0.0, 0.0, 0.0))}  # (straight down: the lying arms)
    for f, pose in enumerate(poses):
        if f % 3 and f != len(poses) - 1:
            continue
        set_pose(armature, pose)
        for view, (location, rotation) in views.items():
            camera.location = location
            camera.rotation_euler = rotation
            scene.render.filepath = os.path.join(folder, f"{clip}_{view}_{f:02d}.png")
            bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(bpy.data.objects["Plane"])
    bpy.data.objects.remove(camera)


main()
