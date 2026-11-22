# Makes a soldier's get-up from lying on the back (the shooter's plan, Phase 2.4; the user: the training dummy stood up
# with its right leg swinging through the floor). The dummy fell with the library's Death01 and stood up by a cross-fade
# to Idle: each bone turned on its own the shortest way, the legs through the ground. The animation library has no
# get-up, so this is our own, on the library's rig, in Blender without its window, from where Death01 ends to where
# Idle_Loop starts:
#
#   lying (Death01's last frame) -> the knees drawn up, the feet flat, the arms down beside the body -> sitting up, the
#   hands on the floor behind -> forward over the feet into a deep squat, the hands on the knees -> standing
#   (Idle_Loop's first frame)
#
# The hips follow a smooth path through the key poses; the feet and the hands follow their own (from the knees drawn up
# on, the feet stay where Idle_Loop stands them; leaving the floor the hands go with the shoulders, to the knees, then
# to Idle_Loop's hang); the legs and the arms are placed by two-bone IK onto them, so no limb
# turns through the floor; the spine, the neck and the head turn between the key poses' (the chest's lean set in the
# world). Every frame the skinned mannequin is checked against the floor: the run fails if any part of it is more than
# FLOOR_TOLERANCE below it. The engine plays the clip without In Place (the hips move from where Death01 left them to
# where Idle_Loop stands), so Death01 must play without it too (Dummy.manim).
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/CharacterPoses/make_get_up.py -- [--preview <folder>]
#
# Reads  Projects/Outpost/Assets/Models/QuaterniusAnimations/UAL1_Standard.glb (the rig, Death01, Idle_Loop)
# Writes Projects/Outpost/Assets/Models/SoldierAnimations/Soldier_GetUp.glb (the mannequin and Get_Up)
#
# Everything is generated: edits by hand in Blender are lost on the next run. --preview renders every third frame from
# the side, the front and three quarters (Workbench).

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
# forward, z up, the ground at z 0; Death01 leaves the hips 0.46 m behind where it stood, lying with the head behind
T_KNEES, T_SIT, T_SQUAT, T_STAND = 0.55, 1.15, 1.8, 2.55

SIT_HIPS = Vector((0.01, 0.50, 0.13))   # the pelvis's joint sitting on the floor (the buttocks under it)
SIT_PELVIS_BACK = 45.0                  # degrees the pelvis is tilted back from standing, sitting
SIT_CHEST_BACK = 36.0                   # degrees the chest leans back from upright (the weight on the hand behind)
SIT_HEAD_BACK = 5.0
SIT_HAND = Vector((0.27, 0.80, 0.068))  # the wrists on the floor behind the hips, sitting (the palms flat; x: the left's)
SQUAT_HIPS = Vector((0.01, 0.13, 0.43))  # the pelvis's joint over the feet, low
SQUAT_PELVIS_FORWARD = 30.0
SQUAT_CHEST_FORWARD = 42.0               # the chest well forward over the knees (the weight over the feet)
SQUAT_HEAD_FORWARD = 15.0
KNEE_HAND_ABOVE = 0.07                   # the wrists this far above the knee they rest on, squatting
HANDS_BESIDE = 0.34                      # meters out from the middle the hands lie beside the hips, lying with the knees up

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

    lying = sample(armature, "Death01", DEATH_END)
    standing = sample(armature, "Idle_Loop", 0)
    duration_frames = round(T_STAND * FPS)

    # What the two ends are, in armature space
    def measure(pose):
        set_pose(armature, pose)
        m = {name: world(armature, name) for name in armature.pose.bones.keys()}
        return m

    L = measure(lying)
    S = measure(standing)
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

    sit_pelvis = tilt(-SIT_PELVIS_BACK) @ S["pelvis"].to_quaternion()
    squat_pelvis = tilt(SQUAT_PELVIS_FORWARD) @ S["pelvis"].to_quaternion()
    sit = upper_key(SIT_HIPS, sit_pelvis, -SIT_CHEST_BACK, -SIT_HEAD_BACK)
    squat = upper_key(SQUAT_HIPS, squat_pelvis, SQUAT_CHEST_FORWARD, SQUAT_HEAD_FORWARD)

    # ---- The tracks
    lying_hips = L["pelvis"].translation
    pelvis_place = PointTrack([(0.0, lying_hips), (T_KNEES, lying_hips + Vector((0.0, -0.01, 0.01))),
                               (T_SIT, SIT_HIPS), (T_SQUAT, SQUAT_HIPS), (T_STAND, S["pelvis"].translation)])
    pelvis_turn = TurnTrack([(0.0, L["pelvis"].to_quaternion()), (T_KNEES, tilt(4.0) @ L["pelvis"].to_quaternion()),
                             (T_SIT, sit_pelvis), (T_SQUAT, squat_pelvis), (T_STAND, S["pelvis"].to_quaternion())])
    upper_names = SPINE + HEAD + ["clavicle_l", "clavicle_r"]
    upper = {name: LocalTrack([(0.0, lying[name]), (T_KNEES, lying[name]), (T_SIT, sit[name]), (T_SQUAT, squat[name]),
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
        feet[side] = (PointTrack([(0.0, ankle_l), (T_KNEES, ankle_s), (T_STAND, ankle_s)]),
                      TurnTrack([(0.0, L[f"foot_{side}"].to_quaternion()), (T_KNEES, S[f"foot_{side}"].to_quaternion()),
                                 (T_STAND, S[f"foot_{side}"].to_quaternion())]),
                      TurnTrack([(0.0, L[f"ball_{side}"].to_quaternion()), (T_KNEES, S[f"ball_{side}"].to_quaternion()),
                                 (T_STAND, S[f"ball_{side}"].to_quaternion())]))
        out = 0.25 if side == "l" else -0.25
        knees[side] = VectorTrack([
            (0.0, joint_hint(L[f"thigh_{side}"].translation, L[f"calf_{side}"].translation, ankle_l)),
            (T_KNEES, Vector((out, 0.0, 1.0)).normalized()),           # up, a little out
            (T_SIT, Vector((out, -0.6, 1.0)).normalized()),            # up and forward
            (T_SQUAT, Vector((out * 1.2, -1.0, 0.3)).normalized()),     # forward, out over the toes
            (T_STAND, joint_hint(S[f"thigh_{side}"].translation, S[f"calf_{side}"].translation, ankle_s))])

    # The hands: overhead on the floor -> swept out to the sides at full reach -> beside the hips (palms down) -> sitting:
    # the right on the floor behind, the left on the left knee -> squatting: both on the knees -> Idle_Loop's
    # Where the knee is at a time, from the hips' and the foot's tracks (as the legs' IK will put it): the thigh's head
    # from the pelvis's turns with the pelvis's turn from its rest
    rest_offset = {side: (bones[f"thigh_{side}"].head_local - bones["pelvis"].head_local) for side in "lr"}
    pelvis_rest = rest["pelvis"]

    def hip_at(side, t):
        turn = pelvis_turn.at(t) @ pelvis_rest.inverted()
        return pelvis_place.at(t) + turn @ rest_offset[side]

    def knee_place(side, t):
        knee, _ = two_bone(hip_at(side, t), feet[side][0].at(t), thigh, calf, knees[side].at(t))
        return knee

    hands, elbows = {}, {}
    forward = Vector((0.0, -1.0, 0.0))
    for side in "lr":
        out = 1.0 if side == "l" else -1.0
        shoulder = L[f"upperarm_{side}"].translation
        # (swept a little off the floor: the hand turns palm down on the way, its fingers clear of it; the arm bent, the
        # elbow out, so the hand points forward along the forearm, not out at full reach)
        swept = Vector((out * (abs(shoulder.x) + 0.30), shoulder.y - 0.22, 0.20))
        beside = Vector((out * HANDS_BESIDE, lying_hips.y + 0.05, 0.055))
        sitting = Vector((out * SIT_HAND.x, SIT_HAND.y, SIT_HAND.z))
        # (first up off the floor as it lies, then turned on the way out); from sitting on, hand_target() below
        lifted = L[f"hand_{side}"].translation + Vector((0.0, -0.03, 0.12))
        place = PointTrack([(0.0, L[f"hand_{side}"].translation), (0.08, lifted), (0.22, swept), (T_KNEES, beside),
                            (T_SIT, sitting)])
        # The hands always point forward (the user, 2026-11-22: never backwards): palm down, the fingers toward the feet,
        # on the floor beside the hips, behind them sitting, on the knees squatting
        down_toward_feet = palm_down(armature, side, forward, rest)
        turn = TurnTrack([(0.0, L[f"hand_{side}"].to_quaternion()), (0.08, L[f"hand_{side}"].to_quaternion()),
                          (0.22, palm_down(armature, side, Vector((0.35 * out, -1.0, 0.0)), rest)),
                          (T_KNEES, down_toward_feet), (T_SIT, down_toward_feet), (T_SQUAT, down_toward_feet),
                          (T_STAND, S[f"hand_{side}"].to_quaternion())])
        hands[side] = (place, turn)
        elbows[side] = VectorTrack([
            (0.0, joint_hint(L[f"upperarm_{side}"].translation, L[f"lowerarm_{side}"].translation, L[f"hand_{side}"].translation)),
            (0.22, Vector((0.0, 0.3, 1.0)).normalized()),                       # the swept arm's elbow up, a little back
            (T_KNEES, Vector((out * 0.4, 0.2, 1.0)).normalized()),
            (T_SIT, Vector((out * 0.3, 1.0, 0.1)).normalized()),                # back, the hands behind, fingers forward
            (T_SQUAT, Vector((out, 0.3, -0.3)).normalized()),
            (T_STAND, joint_hint(S[f"upperarm_{side}"].translation, S[f"lowerarm_{side}"].translation, S[f"hand_{side}"].translation))])

    # From sitting on, the hands go with the body: off the floor they keep their place from the shoulders (as sitting) and
    # come to the knees by the squat, then to their place from the shoulders standing (Idle_Loop's hang)
    def pose_upper(t):
        pose = {name: track.at(t) for name, track in rest_tracks.items()}
        pose.update({name: track.at(t) for name, track in upper.items()})
        set_pose(armature, pose)
        set_world(armature, "pelvis", pelvis_place.at(t), pelvis_turn.at(t))

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

    wrist_turn = {"l": (0.0, 0), "r": (0.0, 0)}

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
            place, turn = hands[side]
            shoulder = world(armature, f"upperarm_{side}").translation
            wrist = hand_target(side, t, shoulder, world(armature, f"calf_{side}").translation)
            elbow, over = two_bone(shoulder, wrist, upperarm, lowerarm, elbows[side].at(t))
            stretched["arms"] += over
            bend = joint_hint(shoulder, elbow, wrist)
            upper_roll, lower_roll = idle_roll[side]
            place_arm_bone(f"upperarm_{side}", shoulder, bend_frame(elbow - shoulder, bend) @ upper_roll)
            elbow_now = world(armature, f"lowerarm_{side}").translation
            place_arm_bone(f"lowerarm_{side}", elbow_now, bend_frame(wrist - elbow_now, bend) @ lower_roll)
            set_world(armature, f"hand_{side}", world(armature, f"hand_{side}").translation, turn.at(t))
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
    print(f"{CLIP}: the wrists turned from Idle_Loop's way at most: left {wrist_turn['l'][0]:.0f} degrees (frame "
          f"{wrist_turn['l'][1]}), right {wrist_turn['r'][0]:.0f} (frame {wrist_turn['r'][1]})")

    through = check_floor(armature, poses)
    action = bake(armature, CLIP, poses)
    if preview:
        render_preview(preview, armature, poses)
    if through:
        raise SystemExit(through)
    for a in list(bpy.data.actions):
        if a != action:
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
    print(f"Wrote {OUTPUT}: {CLIP} ({len(poses)} frames)")


def check_floor(armature, poses):
    """The skinned mannequin's lowest point each frame: no part may go through the floor (more than Death01's lying pose
    itself sinks, FLOOR_TOLERANCE)"""
    mannequin = bpy.data.objects["Mannequin"]
    groups = {g.index: g.name for g in mannequin.vertex_groups}
    worst = (0.0, -1, "")
    lowest_each = []
    for f, pose in enumerate(poses):
        set_pose(armature, pose)
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = mannequin.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        matrix = mannequin.matrix_world
        low, index = math.inf, -1
        for v in mesh.vertices:
            z = (matrix @ v.co).z
            if z < low:
                low, index = z, v.index
        evaluated.to_mesh_clear()
        part = max(mannequin.data.vertices[index].groups, key=lambda g: g.weight, default=None)
        name = groups.get(part.group, "?") if part else "?"
        lowest_each.append((low, name))
        if low < worst[0]:
            worst = (low, f, name)
    print("Lowest point by frame (cm): " + " ".join(f"{f}:{low * 100:.1f}({name})" for f, (low, name) in enumerate(lowest_each) if f % 3 == 0))
    limit = min(0.0, lowest_each[0][0]) - FLOOR_TOLERANCE
    print(f"{CLIP}: the lowest point {worst[0] * 100:.1f} cm (frame {worst[1]}, {worst[2]}); the lying pose's "
          f"{lowest_each[0][0] * 100:.1f} cm, the limit {limit * 100:.1f} cm")
    if worst[0] < limit:
        return f"{CLIP}: frame {worst[1]} goes {-worst[0] * 100:.1f} cm through the floor ({worst[2]})"
    return None


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


def render_preview(folder, armature, poses):
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
             "quarter": ((-3.0, -2.6, 1.6), (math.radians(68), 0, math.radians(-49)))}
    for f, pose in enumerate(poses):
        if f % 3 and f != len(poses) - 1:
            continue
        set_pose(armature, pose)
        for view, (location, rotation) in views.items():
            camera.location = location
            camera.rotation_euler = rotation
            scene.render.filepath = os.path.join(folder, f"{CLIP}_{view}_{f:02d}.png")
            bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(bpy.data.objects["Plane"])
    bpy.data.objects.remove(camera)


main()
