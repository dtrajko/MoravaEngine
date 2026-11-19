# Makes the player's fast walk (the shooter's plan, Phase 1: the default pace of WASD, 3 m/s) on the animation library's
# rig, in Blender without its window, in four directions (Phase 2.1: holding a gun the body faces the look, so it walks
# forward, back and to each side; PlayerBody blends them by the move's direction). Forward is described below; back is
# forward played backwards (a step back is a step forward reversed in time); to the sides a step-together at SIDE_SPEED
# (the user, 2026-11-19: people don't cross their legs to walk sideways): the leading foot steps out, the trailing one
# closes in until the feet nearly touch, flat (no heel strike), the legs never crossing (the bake fails if they come
# closer across than SIDE_CLOSED). The library's walks are slow (0.9 m/s) and its jog leaves the ground, so this is
# our own: the legs are placed by two-bone IK on feet that roll heel to toe on a ground moving at the walk's speed (no
# sliding at the speed it is made for), the hips a little lower (longer steps, a soldier's bent knees), one foot always
# down; the upper body is the library's Walk_Loop on the same timing, upright (the head over the hips), the arms relaxed
# and swinging a little more, the hands open. Every frame is baked into plain keyframes: the engine plays it like the library's own clips.
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/CharacterPoses/make_fast_walk.py -- [--preview <folder>]
#
# Reads  Projects/Outpost/Assets/Models/QuaterniusAnimations/UAL1_Standard.glb (the rig, Walk_Loop)
# Writes Projects/Outpost/Assets/Models/SoldierAnimations/Soldier_Locomotion.glb (the mannequin and Walk_Fast_Loop,
#        Walk_Fast_Back_Loop, Walk_Fast_Left_Loop, Walk_Fast_Right_Loop: the left foot landing at the start of each, so
#        they blend in step; the side walks' cycle is shorter, the engine blends the lengths)
#
# Everything is generated: edits by hand in Blender are lost on the next run. --preview renders the clip's frames from
# the side and one from the front (Workbench), to check the gait.

import bpy
import math
import os
import sys
from mathutils import Matrix, Quaternion, Vector

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
ASSETS = os.path.join(ROOT, "Projects", "Outpost", "Assets", "Models")
LIBRARY = os.path.join(ASSETS, "QuaterniusAnimations", "UAL1_Standard.glb")
OUTPUT = os.path.join(ASSETS, "SoldierAnimations", "Soldier_Locomotion.glb")

FPS = 30
CLIP = "Walk_Fast_Loop"

# The gait. The rig's armature space: x to the character's left, -y forward, z up, the ground at z 0
SPEED = 3.0            # meters per second the feet roll at (the motor's walk; PlayerBody's WalkFastClipSpeed)
STEP = 1.00            # meters from one foot's strike to the other's: a cycle of two steps is 2 STEP / SPEED seconds
STANCE = 0.53          # of the cycle each foot is down (just over half: one foot always down, the strike close in)
HEEL_STRIKE = 14.0     # degrees the toes are up as the heel lands
TOE_OFF = 45.0         # degrees the heel is up as the toes leave
HEEL_DOWN = 0.10       # of the cycle the foot rolls down from the heel
HEEL_RISE = 0.34       # of the cycle the heel starts to rise (the stance ends at STANCE)
LIFT = 0.05            # meters the ankle clears its flat height in the middle of the swing (low: a high knee is a march)
CENTER = 0.07          # the stance's middle, meters behind the hips (behind: the back leg, up on its toes, reaches further)
STRETCH = 0.999        # of the leg's length the IK may straighten to (the front leg straight at the strike)
BOB = 0.08             # meters the hips rise at most from the long steps' low to over the standing leg
KNEES = 0.0            # meters the hips stay under the library's walk at their highest (0: as high)
ELBOW = 12.0           # degrees the elbows are bent from the rig's straight rest pose (relaxed, the arms hanging long)
ELBOW_SWING = 10.0     # degrees more as the arm swings forward (a relaxed arm folds a little in front, opens behind)
ARM_SWING = 1.15       # the library's arm swing from the shoulders, scaled
FINGERS = 0.35         # how far the fingers curl, from the rest pose's flat hand (0) to Idle_Loop's fist (1)
# The spine upright: it is turned (forward or back) until the head is over the hips, on average over the cycle; the
# hands are open and relaxed (Idle_Loop's fingers: the walk's are fists)
HIP_SWING = 2.2        # the library's pelvis turn and sway, scaled (the hips turn with the long steps)
# The shoulders steady (the user: a swagger, cocky): the chest keeps its own turn, not the hips' wider one (the spine
# turns back what the hips turned more), then its turn and tilt in the world are scaled down, and the collarbones' too
CHEST_STEADY = 0.4     # the chest's turn and tilt in the world round the cycle, scaled (the library's walk: 20 and 5 degrees)
CLAVICLE_SWING = 0.3   # the library's collarbone motion (the shoulders' rise and fall, forward and back), scaled

# To the side the legs never cross (the user, 2026-11-19): the leading leg steps out, the trailing one closes in until
# the feet nearly touch, and steps out again from there: a step-together. Its cycle covers one step's opening (the
# closing step gives nothing back), so it is slower than the walk: SIDE_SPEED, not 3 m/s (faster, people hop)
SIDE_SPEED = 1.15      # meters per second to the side (the motor's SideSpeed; PlayerBody's MoveSideClipSpeed)
SIDE_OPEN = 0.728      # meters a cycle covers (the user: 0.46 was too short and quick, 0.92 too long; 60 % more: 19 frames): the ankles from SIDE_CLOSED apart to SIDE_CLOSED + SIDE_OPEN
SIDE_CLOSED = 0.16     # meters between the ankles when the trailing foot has closed in (the feet' sides nearly touching)
SIDE_TOE_OFF = 8.0     # degrees the heel comes up as a foot leaves the ground (a side step lifts it nearly flat)

# The directions: each one's move in the rig's armature space (x the character's left, -y forward), its speed and the
# meters a cycle covers (the engine's blend keeps clips of other lengths in step), the stance's middle behind the hips
# along the move, the heel strike's and the toe-off's angles, and where each foot's stance is centered across (None: the
# library's walk's width; to the side the two centers SIDE_CLOSED + SIDE_OPEN / 2 apart, so the ankles open to
# SIDE_CLOSED + SIDE_OPEN as the leading foot lands and close to SIDE_CLOSED as the trailing one does)
SIDE_X = (SIDE_CLOSED + SIDE_OPEN * 0.5) * 0.5
DIRECTIONS = {
    "Walk_Fast_Loop": {"move": (0.0, -1.0), "speed": SPEED, "cycle": 2.0 * STEP, "center": CENTER,
                       "heel_strike": HEEL_STRIKE, "toe_off": TOE_OFF, "x": None, "hip_swing": HIP_SWING},
    "Walk_Fast_Left_Loop": {"move": (1.0, 0.0), "speed": SIDE_SPEED, "cycle": SIDE_OPEN, "center": 0.0,
                            "heel_strike": 0.0, "toe_off": SIDE_TOE_OFF, "x": {"l": SIDE_X, "r": -SIDE_X},
                            "hip_swing": 0.5},
    "Walk_Fast_Right_Loop": {"move": (-1.0, 0.0), "speed": SIDE_SPEED, "cycle": SIDE_OPEN, "center": 0.0,
                             "heel_strike": 0.0, "toe_off": SIDE_TOE_OFF, "x": {"l": SIDE_X, "r": -SIDE_X},
                             "hip_swing": 0.5},
}
BACK = "Walk_Fast_Back_Loop"  # Walk_Fast_Loop played backwards

ARM_BONES = ["upperarm_l", "upperarm_r"]
CLAVICLES = ["clavicle_l", "clavicle_r"]
SPINE = ["spine_01", "spine_02", "spine_03"]


def args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    preview = None
    if "--preview" in argv:
        preview = os.path.abspath(argv[argv.index("--preview") + 1])
    return preview


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def minimum_jerk(t):
    t = max(0.0, min(1.0, t))
    return t * t * t * (10.0 + t * (-15.0 + 6.0 * t))


def pitch(degrees):
    """Toes up (positive) about the character's left-right axis: the toes point along -y, so a turn about -x"""
    return Matrix.Rotation(math.radians(-degrees), 4, "X")


def scaled(q, k):
    """q's turn, k times as far"""
    axis, angle = q.to_axis_angle()
    if angle > math.pi:
        angle -= 2.0 * math.pi
    return Quaternion(axis, angle * k)


def mean_rotation(quaternions):
    total = [0.0, 0.0, 0.0, 0.0]
    first = quaternions[0]
    for q in quaternions:
        sign = 1.0 if first.dot(q) >= 0.0 else -1.0
        for i, value in enumerate((q.w, q.x, q.y, q.z)):
            total[i] += sign * value
    return Quaternion(total).normalized()


# ---------------------------------------------------------------------------------------------------------------------
# The rig

def load_library():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = FPS
    bpy.ops.import_scene.gltf(filepath=LIBRARY)
    armature = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    for o in list(bpy.data.objects):
        if o != armature and o.name != "Mannequin":
            bpy.data.objects.remove(o)
    return armature


def update():
    bpy.context.view_layer.update()


def sample(armature, action_name, time):
    """Every bone's matrix_basis in a library clip at a time in frames (between frames: Blender's own interpolation)"""
    armature.animation_data.action = bpy.data.actions[action_name]
    whole = math.floor(time)
    bpy.context.scene.frame_set(int(whole), subframe=time - whole)
    pose = {pb.name: pb.matrix_basis.copy() for pb in armature.pose.bones}
    armature.animation_data.action = None
    return pose


def set_pose(armature, basis):
    for name, m in basis.items():
        armature.pose.bones[name].matrix_basis = m


def local_pose(armature):
    result = {}
    for pb in armature.pose.bones:
        rest = pb.bone.matrix_local
        if pb.parent:
            rest = pb.parent.bone.matrix_local.inverted() @ rest
            pose = pb.parent.matrix.inverted() @ pb.matrix
        else:
            pose = pb.matrix
        result[pb.name] = rest.inverted() @ pose
    return result


def aim_bone(armature, name, direction):
    """Turns a bone (the least turn) so it points along a direction, in armature space; its head stays"""
    pb = armature.pose.bones[name]
    m = pb.matrix.copy()
    current = m.col[1].to_3d().normalized()
    turn = current.rotation_difference(direction.normalized())
    rotation = turn @ m.to_quaternion()
    pb.matrix = Matrix.Translation(m.translation) @ rotation.to_matrix().to_4x4()
    update()


def place_bone(armature, name, rotation):
    """A bone's rotation in armature space; its head stays where its parent puts it"""
    pb = armature.pose.bones[name]
    pb.matrix = Matrix.Translation(pb.matrix.translation) @ rotation.to_matrix().to_4x4()
    update()


# ---------------------------------------------------------------------------------------------------------------------
# The feet: where each ankle is and how its foot is turned, from the leg's phase (0: the heel lands)

class Foot:
    def __init__(self, armature, side, flat_frame):
        self.side = side
        flat = sample(armature, "Walk_Loop", flat_frame)
        set_pose(armature, flat)
        update()
        foot = armature.pose.bones[f"foot_{side}"].matrix
        ball = armature.pose.bones[f"ball_{side}"].matrix
        self.x = foot.translation.x
        self.height = foot.translation.z                                  # the ankle over the ground, the foot flat
        self.rotation = foot.to_quaternion()                              # the foot flat (its toe-out kept)
        self.ball_rotation = ball.to_quaternion()
        self.ball = ball.translation - foot.translation                   # the ball's joint from the ankle, flat
        self.heel = Vector((0.0, 0.055, -self.height))                    # the heel's ground point from the ankle

    def at(self, phase, cycle_seconds, direction):
        """(ankle, foot rotation, ball rotation or None to follow the foot, what it stands on: "heel", "flat", "ball" or
        None), walking in a direction of DIRECTIONS"""
        ground_speed = direction["cycle"] / cycle_seconds
        stance_length = ground_speed * STANCE * cycle_seconds
        move = Vector((direction["move"][0], direction["move"][1], 0.0))
        heel_strike, toe_off = direction["heel_strike"], direction["toe_off"]
        phase %= 1.0
        if phase < STANCE:
            # On the ground, moving back with it (against the move): rolls from the heel, flat, then up onto the ball
            x = self.x if direction["x"] is None else direction["x"][self.side]
            base = Vector((x, 0.0, self.height)) - move * direction["center"]
            flat = base + move * (stance_length * 0.5 - ground_speed * phase * cycle_seconds)
            if phase < HEEL_DOWN and heel_strike > 0.0:
                angle = heel_strike * (1.0 - smooth(phase / HEEL_DOWN))
                pivot = flat + self.heel
                ankle = pivot + (pitch(angle).to_3x3() @ (flat - pivot))
                return ankle, pitch(angle).to_quaternion() @ self.rotation, None, "heel"
            if phase < HEEL_RISE:
                return flat, self.rotation, None, "flat"
            t = (phase - HEEL_RISE) / (STANCE - HEEL_RISE)
            angle = -toe_off * smooth(t) ** 1.3
            pivot = flat + self.ball
            ankle = pivot + (pitch(angle).to_3x3() @ (flat - pivot))
            return ankle, pitch(angle).to_quaternion() @ self.rotation, self.ball_rotation, "ball"
        # In the air: from where the toes left to where the heel lands, the toes coming up on the way
        t = (phase - STANCE) / (1.0 - STANCE)
        leave, leave_rotation, _, _ = self.at(STANCE - 1e-6, cycle_seconds, direction)
        land, land_rotation, _, _ = self.at(0.0, cycle_seconds, direction)
        s = minimum_jerk(t)
        ankle = leave.lerp(land, s)
        # Height: down from the toe-off's quickly, a low clearance over the ground through the middle (a walk keeps the
        # swinging foot low; a high knee is a march), up to the heel strike's at the end
        ankle.z = (self.height + LIFT * math.sin(math.pi * t)
                   + (leave.z - self.height) * (1.0 - smooth(t / 0.45))
                   + (land.z - self.height) * smooth((t - 0.6) / 0.4))
        angle_leave, angle_land = -toe_off, heel_strike
        # Quickly out of the toe-off, level in the middle, the toes up for the strike at the end
        if t < 0.35:
            angle = angle_leave * (1.0 - smooth(t / 0.35))
        elif t < 0.7:
            angle = 0.0
        else:
            angle = angle_land * smooth((t - 0.7) / 0.3)
        return ankle, pitch(angle).to_quaternion() @ self.rotation, None, None


def knee_place(hip, ankle, thigh, calf, side):
    """The knee for a hip and an ankle (two-bone IK), bending forward and a little out"""
    to_ankle = ankle - hip
    d = to_ankle.length
    reach = (thigh + calf) * STRETCH
    stretched = d > reach
    if stretched:
        d = reach
    direction = to_ankle.normalized()
    forward = Vector((0.08 if side == "l" else -0.08, -1.0, 0.0))
    bend = (forward - direction * forward.dot(direction)).normalized()
    a = (thigh * thigh - calf * calf + d * d) / (2.0 * d)
    h = math.sqrt(max(0.0, thigh * thigh - a * a))
    return hip + direction * a + bend * h, stretched


# ---------------------------------------------------------------------------------------------------------------------

def elbow_axis(armature, side):
    """The lowerarm's own axis that bends the elbow (the hand comes forward), found by trying each"""
    set_pose(armature, sample(armature, "Walk_Loop", 0.0))
    update()
    pb = armature.pose.bones[f"lowerarm_{side}"]
    hand = armature.pose.bones[f"hand_{side}"]
    base = pb.matrix_basis.copy()
    best = None
    for axis in ("X", "Z"):
        for sign in (1.0, -1.0):
            pb.matrix_basis = base @ Matrix.Rotation(math.radians(30.0 * sign), 4, axis)
            update()
            forward = -hand.matrix.translation.y
            if best is None or forward > best[0]:
                best = (forward, axis, sign)
    pb.matrix_basis = base
    update()
    return best[1], best[2]


def resample(poses, count):
    """A looped cycle of poses (bone name to matrix_basis) at another number of frames: each bone's place lerped and its
    turn slerped between the two nearest frames"""
    result = []
    for f in range(count):
        at = f * len(poses) / count
        a, t = int(at), at - int(at)
        p0, p1 = poses[a % len(poses)], poses[(a + 1) % len(poses)]
        pose = {}
        for name in p0:
            l0, r0, s0 = p0[name].decompose()
            l1, r1, s1 = p1[name].decompose()
            pose[name] = Matrix.LocRotScale(l0.lerp(l1, t), r0.slerp(r1, t), s0.lerp(s1, t))
        result.append(pose)
    return result


def build_direction(armature, clip, direction, bodies, sources, feet, offsets, frames, cycle_seconds, speed, thigh_length,
                    calf_length):
    """One direction's frames: the upper body's (bodies), the hips' height as the legs reach, the legs by IK onto the
    feet's paths; each frame's local pose"""
    # The hips' height, frame by frame: as high as both legs reach (within STRETCH), never above the library's walk
    # less KNEES (a soldier's bent knees), smoothed round the cycle (never above what reaches): the hips rise over
    # the standing leg and come down as the steps are long, as a walk's do, without a run's bounce
    pelvis_z = -math.inf  # the library's hips at their highest (over the standing leg)
    for f in range(frames):
        set_pose(armature, sources[f])
        update()
        pelvis_z = max(pelvis_z, armature.pose.bones["pelvis"].matrix.translation.z)
    reach = (thigh_length + calf_length) * STRETCH
    allowed = []
    for f in range(frames):
        set_pose(armature, bodies[f])
        update()
        phase = f / frames
        pelvis_now = armature.pose.bones["pelvis"].matrix.translation.z
        highest = pelvis_z - KNEES
        for side in "lr":
            ankle = feet[side].at(phase + offsets[side], cycle_seconds, direction)[0]
            hip = armature.pose.bones[f"thigh_{side}"].matrix.translation
            horizontal = Vector((ankle.x - hip.x, ankle.y - hip.y)).length
            vertical = math.sqrt(max(0.0, reach * reach - horizontal * horizontal))
            highest = min(highest, ankle.z + vertical - (hip.z - pelvis_now))
        allowed.append(highest)
    ceiling = min(allowed) + BOB  # the rise over the standing leg held to a walk's (more is a run's bounce)
    allowed = [min(a, ceiling) for a in allowed]
    heights = list(allowed)
    for _ in range(3):
        heights = [min(allowed[i], (heights[i - 1] + 2.0 * heights[i] + heights[(i + 1) % frames]) * 0.25)
                   for i in range(frames)]
    print(f"{clip}: {frames} frames ({cycle_seconds:.3f} s a cycle, {2.0 / cycle_seconds:.2f} steps a second), "
          f"{direction['cycle']:.2f} m a cycle, {speed:.2f} m/s; hips {pelvis_z - max(heights):.3f} to {pelvis_z - min(heights):.3f} m "
          f"lower than the library's walk (a bob of {(max(heights) - min(heights)) * 100:.1f} cm)")

    poses = []
    contacts = {"l": [], "r": []}
    stretched_frames = 0
    closest = math.inf  # the ankles' nearest approach
    across = math.inf   # the left ankle's least distance to the left of the right one (the legs never cross)
    for f in range(frames):
        phase = f / frames
        pose = dict(bodies[f])
        set_pose(armature, pose)
        update()
        # The pelvis's height in armature space: its matrix_basis moves it in the bone's own axes, so set the matrix
        pb = armature.pose.bones["pelvis"]
        m = pb.matrix.copy()
        m.translation.z = heights[f]
        pb.matrix = m
        update()
        ankles = {}
        for side in "lr":
            ankle, foot_rotation, ball_rotation, down = feet[side].at(phase + offsets[side], cycle_seconds, direction)
            ankles[side] = ankle
            hip = armature.pose.bones[f"thigh_{side}"].matrix.translation.copy()
            knee, stretched = knee_place(hip, ankle, thigh_length, calf_length, side)
            stretched_frames += 1 if stretched else 0
            aim_bone(armature, f"thigh_{side}", knee - hip)
            knee_now = armature.pose.bones[f"calf_{side}"].matrix.translation.copy()
            aim_bone(armature, f"calf_{side}", ankle - knee_now)
            place_bone(armature, f"foot_{side}", foot_rotation)
            # The toes flat on the ground while the heel is up; else they follow the foot as in the flat pose
            foot_flat_to_ball = feet[side].rotation.inverted() @ feet[side].ball_rotation
            place_bone(armature, f"ball_{side}", ball_rotation if ball_rotation is not None else foot_rotation @ foot_flat_to_ball)
            if down:
                foot = armature.pose.bones[f"foot_{side}"].matrix
                heel = foot.translation + foot.to_quaternion() @ (feet[side].rotation.inverted() @ feet[side].heel)
                ball = armature.pose.bones[f"ball_{side}"].matrix.translation
                contacts[side].append((f, down, heel.copy(), ball.copy()))
        closest = min(closest, (ankles["l"] - ankles["r"]).length)
        across = min(across, ankles["l"].x - ankles["r"].x)
        poses.append(local_pose(armature))
    if direction["x"] is not None and across < SIDE_CLOSED - 0.02:
        raise SystemExit(f"{clip}: the ankles came within {across * 100:.0f} cm across (wanted {SIDE_CLOSED * 100:.0f}): "
                         f"the legs mustn't cross")

    # The feet on the ground move against the move at the walk's speed: the point each stands on (the heel's ground point
    # rolling down and flat, the ball's joint on the toes), frame to frame
    ground = -Vector((direction["move"][0], direction["move"][1], 0.0)) * (direction["cycle"] / cycle_seconds)
    worst = 0.0
    for side in "lr":
        points = contacts[side]
        for (f0, _, h0, b0), (f1, kind, h1, b1) in zip(points, points[1:]):
            if f1 != f0 + 1:
                continue
            moved = (b1 - b0) if kind == "ball" else (h1 - h0)
            worst = max(worst, (moved * FPS - ground).length)
    print(f"{clip}: the feet on the ground off the walk's speed by at most {worst:.2f} m/s; "
          f"{stretched_frames} leg frames at the IK's full stretch; the ankles {closest * 100:.0f} cm apart at the closest, "
          f"the left one at least {across * 100:.0f} cm to the left of the right")
    return poses


def bake(armature, clip, poses):
    """Location and rotation of every bone, frame by frame, the loop closed by the first frame again"""
    action = bpy.data.actions.new(clip)
    armature.animation_data.action = action
    for f, pose in enumerate(poses + [poses[0]]):
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


def main():
    preview = args()
    armature = load_library()
    bones = armature.data.bones
    thigh_length = (bones["calf_l"].head_local - bones["thigh_l"].head_local).length
    calf_length = (bones["foot_l"].head_local - bones["calf_l"].head_local).length

    # Walk_Loop: 40 frames a cycle, the left heel landing at frame 0, the right at 20; each foot flat at its stance's middle
    source_frames = 40
    feet = {"l": Foot(armature, "l", 5.0), "r": Foot(armature, "r", 25.0)}
    offsets = {"l": 0.0, "r": 0.5}

    cycle_seconds = 2.0 * STEP / SPEED
    frames = max(8, round(cycle_seconds * FPS))
    cycle_seconds = frames / FPS
    speed = 2.0 * STEP / cycle_seconds

    elbows = {s: elbow_axis(armature, s) for s in "lr"}
    sources = [sample(armature, "Walk_Loop", source_frames * f / frames) for f in range(frames)]
    idle = sample(armature, "Idle_Loop", 0.0)
    fingers = {name: Matrix.Translation(idle[name].translation) @ Quaternion().slerp(idle[name].to_quaternion(), FINGERS).to_matrix().to_4x4()
               for name in idle if any(name.startswith(finger) for finger in ("thumb_", "index_", "middle_", "ring_", "pinky_"))}
    means = {name: mean_rotation([p[name].to_quaternion() for p in sources]) for name in ARM_BONES + CLAVICLES + SPINE + ["pelvis"]}
    pelvis_to_spine = (bones["pelvis"].matrix_local.inverted() @ bones["spine_01"].matrix_local).to_quaternion()
    pelvis_mean = sum((p["pelvis"].translation for p in sources), Vector()) / frames

    # How far forward each arm is in the library's walk, -1 (furthest back) to 1 (furthest forward): its elbow's y
    elbow_y = {s: [] for s in "lr"}
    for f in range(frames):
        set_pose(armature, sources[f])
        update()
        for side in "lr":
            elbow_y[side].append(-armature.pose.bones[f"lowerarm_{side}"].matrix.translation.y)

    def swing_forward(side, f):
        values = elbow_y[side]
        low, high = min(values), max(values)
        return (values[f] - low) / (high - low) * 2.0 - 1.0 if high > low else 0.0

    def upper_body(f, lean_degrees, hip_swing=HIP_SWING):
        pose = dict(sources[f])
        pose.update(fingers)
        def swung(name, k):
            q = pose[name].to_quaternion()
            return Matrix.Translation(pose[name].translation) @ (means[name] @ scaled(means[name].inverted() @ q, k)).to_matrix().to_4x4()

        for name in ARM_BONES:
            pose[name] = swung(name, ARM_SWING)
        for name in CLAVICLES:
            pose[name] = swung(name, CLAVICLE_SWING)
        original = pose["pelvis"].to_quaternion()
        rotation = means["pelvis"] @ scaled(means["pelvis"].inverted() @ original, hip_swing)
        location = pelvis_mean + (pose["pelvis"].translation - pelvis_mean) * hip_swing
        pose["pelvis"] = Matrix.Translation(location) @ rotation.to_matrix().to_4x4()
        # The spine turns back what the hips turned more than the library's: the chest as on the library's hips
        back = pelvis_to_spine.inverted() @ (rotation.inverted() @ original) @ pelvis_to_spine
        pose["spine_01"] = Matrix.Translation(pose["spine_01"].translation) @ (back @ pose["spine_01"].to_quaternion()).to_matrix().to_4x4()
        lean = Matrix.Rotation(math.radians(lean_degrees / len(SPINE)), 4, "X")  # +x leans forward (the rifle script's finding)
        for name in SPINE:
            pose[name] = pose[name] @ lean
        for side in "lr":
            axis, sign = elbows[side]
            forward = swing_forward(side, f)
            bend = ELBOW + ELBOW_SWING * max(0.0, forward)
            pose[f"lowerarm_{side}"] = Matrix.Rotation(math.radians(bend * sign), 4, axis)
        return pose

    # The spine's lean that puts the head over the hips (armature space, -y forward), averaged over the cycle: the head
    # as far ahead of the hip joints as in the rig's rest pose, standing upright
    def ahead(head, thigh_l, thigh_r):
        return (thigh_l.y + thigh_r.y) * 0.5 - head.y

    upright = ahead(bones["Head"].head_local, bones["thigh_l"].head_local, bones["thigh_r"].head_local)

    def head_ahead(lean_degrees):
        total = 0.0
        for f in range(frames):
            set_pose(armature, upper_body(f, lean_degrees))
            update()
            b = armature.pose.bones
            total += ahead(b["Head"].matrix.translation, b["thigh_l"].matrix.translation, b["thigh_r"].matrix.translation)
        return total / frames - upright

    low, high = -15.0, 15.0
    for _ in range(30):  # the head ahead grows with the lean: halve the range round where it is 0
        middle = (low + high) * 0.5
        if head_ahead(middle) > 0.0:
            high = middle
        else:
            low = middle
    lean = (low + high) * 0.5
    print(f"{CLIP}: the spine turned {lean:+.1f} degrees (forward +) to put the head over the hips "
          f"(the library's walk: {head_ahead(0.0) * 100:+.1f} cm further ahead than standing)")

    # The chest steadied in the world: its turn and tilt round the cycle (the spine's top, as the hips and the spine put
    # it) kept to CHEST_STEADY of themselves, the difference given to the spine's lowest bone; the arms hang from it
    def steady_bodies(hip_swing):
        bodies = [upper_body(f, lean, hip_swing) for f in range(frames)]
        chest = []
        for f in range(frames):
            set_pose(armature, bodies[f])
            update()
            chest.append(armature.pose.bones["spine_03"].matrix.to_quaternion())
        chest_mean = mean_rotation(chest)
        for f in range(frames):
            set_pose(armature, bodies[f])
            update()
            wanted = chest_mean @ scaled(chest_mean.inverted() @ chest[f], CHEST_STEADY)
            spine = armature.pose.bones["spine_01"]
            m = spine.matrix.copy()
            spine.matrix = Matrix.Translation(m.translation) @ ((wanted @ chest[f].inverted()) @ m.to_quaternion()).to_matrix().to_4x4()
            update()
            bodies[f]["spine_01"] = spine.matrix_basis.copy()
        return bodies

    clips = {}
    for clip, direction in DIRECTIONS.items():
        # The direction's cycle: the meters it covers at its speed; the upper body's frames resampled to it (to the side
        # the hips turn less: a step-together doesn't swing them)
        bodies = steady_bodies(direction["hip_swing"])
        clip_frames = max(8, round(direction["cycle"] / direction["speed"] * FPS))
        clip_seconds = clip_frames / FPS
        clip_bodies = bodies if clip_frames == frames else resample(bodies, clip_frames)
        clip_sources = sources if clip_frames == frames else resample(sources, clip_frames)
        clips[clip] = build_direction(armature, clip, direction, clip_bodies, clip_sources, feet, offsets, clip_frames,
                                      clip_seconds, direction["cycle"] / clip_seconds, thigh_length, calf_length)
    # Back: forward reversed in time (its frame f is forward's frame -f: the left foot down at the start, as in the others)
    forward = clips["Walk_Fast_Loop"]
    clips[BACK] = [forward[(frames - f) % frames] for f in range(frames)]
    print(f"{BACK}: Walk_Fast_Loop played backwards, {frames} frames")
    frames = {clip: len(poses) for clip, poses in clips.items()}

    actions = []
    for clip, poses in clips.items():
        actions.append(bake(armature, clip, poses))
        if preview:
            render_preview(preview, armature, poses, clip)

    for a in list(bpy.data.actions):
        if a not in actions:
            bpy.data.actions.remove(a)
    for pb in armature.pose.bones:
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
    print(f"Wrote {OUTPUT}: " + ", ".join(f"{clip} ({count} frames)" for clip, count in frames.items()) + f"; {SPEED:.2f} m/s")


def render_preview(folder, armature, poses, clip):
    os.makedirs(folder, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.color_type = "MATERIAL"
    scene.render.resolution_x = 360
    scene.render.resolution_y = 560
    camera = bpy.data.objects.new("PreviewCamera", bpy.data.cameras.new("PreviewCamera"))
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 2.1
    scene.collection.objects.link(camera)
    scene.camera = camera
    # The ground, a thin line to see the feet against
    bpy.ops.mesh.primitive_plane_add(size=4.0, location=(0.0, 0.0, -0.002))
    views = {"side": ((-3.0, -0.1, 0.95), (math.radians(90), 0, math.radians(-90))),
             "front": ((0.0, -3.0, 0.95), (math.radians(90), 0, 0))}
    for f, pose in enumerate(poses):
        set_pose(armature, pose)
        update()
        for view, (location, rotation) in views.items():
            if view == "front" and f % 4:
                continue
            camera.location = location
            camera.rotation_euler = rotation
            scene.render.filepath = os.path.join(folder, f"{clip}_{view}_{f:02d}.png")
            bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(bpy.data.objects["Plane"])
    bpy.data.objects.remove(camera)


main()
