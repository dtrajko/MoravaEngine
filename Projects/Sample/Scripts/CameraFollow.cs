using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>Where CameraFollow looks from (SwitchCamera goes to the next one).</summary>
    public enum CameraView
    {
        /// <summary>Above the playground: the offset and rotation the camera starts with.</summary>
        Above,
        /// <summary>Behind the ball, looking past it: Orbit turns the view around the ball.</summary>
        Behind,
        /// <summary>The ball's eyes: just in front of it, looking where Orbit turns.</summary>
        FirstPerson,
    }

    /// <summary>
    /// A camera that follows its target, from one of three views (SwitchCamera: C, the gamepad's Y): above the playground
    /// (the offset and rotation it starts with, catching up smoothly), behind the target, or from its front. Behind and
    /// in front, the mouse (Look, with MouseLook: the game captures the cursor, Escape gives it back) and Orbit (Q, E, R,
    /// F, the right stick) turn the view; the target's controller moves it where the camera looks. Behind, Zoom (the
    /// mouse wheel, the D-pad's up and down) brings it closer or farther. With FollowMovement, the view also turns by itself towards where the
    /// target moves (not while the mouse or Orbit turns it, and not when it moves back towards the camera). Behind, the
    /// camera comes closer at once instead of going through a wall, glides back out once the wall is passed, and rises
    /// when it is pushed close, so the target stays in view. A switch glides there in SwitchSeconds. ControlForward is the
    /// way the target's controller steers by: the view's, but without the view's own turns towards the movement until Move
    /// is let go or pushed a new way (a stick never pushed exactly straight would otherwise turn the view, and so the way,
    /// further and further).
    /// </summary>
    public class CameraFollow : Entity
    {
        /// <summary>The entity followed.</summary>
        public Entity? Target;

        /// <summary>The view the game starts with.</summary>
        public CameraView View = CameraView.Above;

        /// <summary>How fast it catches up above the playground: higher is tighter (0: it stays exactly at the offset).</summary>
        public float Smoothing = 6.0f;

        /// <summary>Meters from the target, behind it.</summary>
        public float BehindDistance = 4.0f;

        /// <summary>Meters above the target's origin the view turns around, behind it (a ball: its center; a character: its feet).</summary>
        public float BehindHeight = 0.5f;

        /// <summary>Degrees the view looks down, behind the target, when it switches there.</summary>
        public float BehindPitch = 20.0f;

        /// <summary>
        /// The first-person view's height over the target's origin (a character's: its eyes over its feet), in meters, and
        /// how far ahead of it the camera is, along the view (so its own head is out of sight). Below 0: a ball's view,
        /// just in front of its surface and a little above its center (its radius: its scale).
        /// </summary>
        public float EyeHeight = -1.0f;
        public float EyeForward = 0.2f;

        /// <summary>The mouse turns the view (the game captures the cursor while it has the input).</summary>
        public bool MouseLook = true;

        /// <summary>Times the mouse's turn (the input map's Look: 0.1 degrees per pixel).</summary>
        public float MouseSensitivity = 1.0f;

        /// <summary>Meters closer (the wheel turned away from you, the D-pad's up) or farther per notch of Zoom (a press, on a gamepad), behind the target.</summary>
        public float ZoomStep = 0.5f;

        /// <summary>Meters Zoom keeps the camera within, behind the target.</summary>
        public float MinDistance = 1.5f;
        public float MaxDistance = 8.0f;

        /// <summary>How fast it moves back out after a wall pushed it closer: higher is faster.</summary>
        public float ReturnSmoothing = 4.0f;

        /// <summary>
        /// Meters the camera keeps from the point it turns around: pushed closer by a wall, it rises until it is this far,
        /// looking down over the target instead of into it (a ceiling stops it).
        /// </summary>
        public float CloseDistance = 1.2f;

        /// <summary>How long a switch glides from one view to the next.</summary>
        public float SwitchSeconds = 0.4f;

        /// <summary>Behind and in front: the view turns by itself towards where the target moves.</summary>
        public bool FollowMovement = true;

        /// <summary>Degrees per second it turns at most (the faster the target, the closer to this).</summary>
        public float FollowTurnSpeed = 90.0f;

        /// <summary>Meters per second the target has to move before the view follows it.</summary>
        public float FollowMinSpeed = 1.0f;

        /// <summary>Seconds after Orbit was last used before the view follows the movement again.</summary>
        public float FollowDelay = 1.0f;

        Vector3 offset, aboveRotation;
        float yaw, pitch; // behind and in front: degrees, as the camera's rotation (X down, Y to the left)
        float blend = 1.0f;
        Vector3 fromTranslation, fromRotation;
        Vector3 lastTarget;
        float sinceOrbit = float.MaxValue;
        float zoomDistance, distance = -1.0f; // behind: the distance Zoom chose; the one now (a wall: closer)
        float controlYaw;    // ControlForward's: turned by the mouse and Orbit, the view's again at a new Move
        Vector2 controlMove; // Move's direction when controlYaw last became the view's (zero: let go)

        /// <summary>
        /// Forward on the ground for the target's controller (Move): the view's, turned by the mouse and Orbit but not by
        /// FollowMovement while Move is held the same way; the view's again once Move is let go or turned 20 degrees.
        /// </summary>
        public Vector3 ControlForward
        {
            get
            {
                if (View == CameraView.Above)
                    return Vector3.Normalize(new Vector3(Forward.X, 0.0f, Forward.Z));
                float radians = controlYaw * MathF.PI / 180.0f;
                return new Vector3(-MathF.Sin(radians), 0.0f, -MathF.Cos(radians));
            }
        }

        protected override void OnCreate()
        {
            if (MouseLook)
                Input.CursorLocked = true;
            zoomDistance = Math.Clamp(BehindDistance, MathF.Min(MinDistance, MaxDistance), MathF.Max(MinDistance, MaxDistance));
            aboveRotation = EulerAngles;
            yaw = controlYaw = aboveRotation.Y;
            pitch = BehindPitch;
            if (Target != null)
            {
                offset = Translation - Target.Translation;
                lastTarget = Target.WorldPosition;
            }
        }

        protected override void OnUpdate(float dt)
        {
            if (Target == null || !Target.Exists)
                return;

            if (Input.WasPressed("SwitchCamera"))
            {
                View = View == CameraView.FirstPerson ? CameraView.Above : View + 1;
                controlMove = Vector2.Zero;
                pitch = View == CameraView.FirstPerson ? 0.0f : BehindPitch;
                fromTranslation = Translation;
                fromRotation = EulerAngles;
                blend = 0.0f;
            }
            Vector3 targetNow = Target.WorldPosition;
            Vector3 moved = targetNow - lastTarget;
            lastTarget = targetNow;
            if (View != CameraView.Above)
            {
                // Right: clockwise seen from above; up: the view looks up. The mouse only while it is the game's: a free
                // cursor moves to the editor's panels or the window's edge
                Vector2 orbit = Input.GetAxis2D("Orbit");
                if (MouseLook && Input.MouseCaptured)
                    orbit += Input.GetAxis2D("Look") * MouseSensitivity * GameOptions.MouseSensitivity; // times the player's choice (Settings)
                yaw -= orbit.X * dt;
                controlYaw -= orbit.X * dt;
                pitch = View == CameraView.Behind ? Math.Clamp(pitch - orbit.Y * dt, -20.0f, 70.0f) // up: lower, the ground brings it closer
                    : Math.Clamp(pitch - orbit.Y * dt, -60.0f, 60.0f);
                sinceOrbit = orbit.LengthSquared() > 1e-4f ? 0.0f : sinceOrbit + dt;
                if (FollowMovement && dt > 0 && sinceOrbit >= FollowDelay)
                    FollowTheMovement(moved / dt, dt);

                // The way to steer by: the view's at each new Move (let go, then pushed; or turned 20 degrees, more than a thumb wobbles)
                Vector2 move = Input.GetAxis2D("Move");
                if (move.LengthSquared() < 0.04f)
                {
                    controlMove = Vector2.Zero;
                    controlYaw = yaw;
                }
                else if (controlMove == Vector2.Zero || Vector2.Dot(Vector2.Normalize(move), controlMove) < 0.94f) // 20 degrees
                {
                    controlMove = Vector2.Normalize(move);
                    controlYaw = yaw;
                }
            }

            Vector3 wanted;
            Vector3 rotation;
            Vector3 target = Target.WorldPosition;
            float yawRadians = yaw * MathF.PI / 180.0f, pitchRadians = pitch * MathF.PI / 180.0f;
            Vector3 forward = new Vector3(-MathF.Sin(yawRadians), 0.0f, -MathF.Cos(yawRadians)); // on the ground
            switch (View)
            {
                case CameraView.Behind:
                {
                    // Back and up from a point above the target, as far as no wall is in between: closer at once, back out
                    // smoothly (no jumps past a pillar)
                    // The wheel: its notches this frame; a gamepad's buttons: a step at each press
                    InputAction zoom = Input.GetAction("Zoom");
                    float notches = !zoom.FromGamepad ? zoom.Value.X : zoom.WasPressed ? MathF.Sign(zoom.Value.X) : 0.0f;
                    zoomDistance = Math.Clamp(zoomDistance - notches * ZoomStep,
                        MathF.Min(MinDistance, MaxDistance), MathF.Max(MinDistance, MaxDistance));
                    Vector3 pivot = target + Vector3.UnitY * BehindHeight;
                    Vector3 back = -forward * MathF.Cos(pitchRadians) + Vector3.UnitY * MathF.Sin(pitchRadians);
                    float free = zoomDistance;
                    if (Physics.SphereCast(pivot, 0.25f, back, zoomDistance, out RaycastHit hit, PhysicsLayers.Solid, Target))
                        free = MathF.Max(0.3f, hit.Distance - 0.1f);
                    if (distance < 0 || free <= distance || blend < 1.0f)
                        distance = free;
                    else
                        distance += (free - distance) * (1.0f - MathF.Exp(-ReturnSmoothing * dt));
                    wanted = pivot + back * distance;

                    // Pushed close: up over the target until CloseDistance from the pivot (as high as no ceiling is in the
                    // way), still looking at the pivot
                    if (CloseDistance > 0 && distance < CloseDistance)
                    {
                        float lift = MathF.Sqrt(CloseDistance * CloseDistance - distance * distance);
                        if (Physics.SphereCast(wanted, 0.2f, Vector3.UnitY, lift + 0.2f, out RaycastHit ceiling, PhysicsLayers.Solid, Target))
                            lift = MathF.Max(0.0f, ceiling.Distance - 0.2f);
                        wanted += Vector3.UnitY * lift;
                    }
                    Vector3 look = pivot - wanted;
                    float lookPitch = look.LengthSquared() > 1e-6f ?
                        MathF.Asin(Math.Clamp(-look.Y / look.Length(), -1.0f, 1.0f)) * 180.0f / MathF.PI : pitch;
                    rotation = new Vector3(lookPitch, yaw, 0.0f);
                    break;
                }
                case CameraView.FirstPerson:
                {
                    if (EyeHeight >= 0.0f)
                    {
                        // A character's eyes: over its feet, a little ahead along the view
                        wanted = target + forward * EyeForward + Vector3.UnitY * EyeHeight;
                    }
                    else
                    {
                        // Just in front of the ball's surface, a little above its center: the ball is out of view
                        float radius = Target.Scale.Y;
                        wanted = target + forward * (radius + 0.05f) + Vector3.UnitY * (radius * 0.3f);
                    }
                    rotation = new Vector3(pitch, yaw, 0.0f);
                    break;
                }
                default:
                    wanted = Target.Translation + offset;
                    rotation = aboveRotation;
                    if (blend >= 1.0f && Smoothing > 0)
                        wanted = Vector3.Lerp(Translation, wanted, 1.0f - MathF.Exp(-Smoothing * dt));
                    break;
            }

            // A switch glides from where the camera was
            if (blend < 1.0f)
            {
                blend = SwitchSeconds > 0 ? MathF.Min(1.0f, blend + dt / SwitchSeconds) : 1.0f;
                float t = blend * blend * (3.0f - 2.0f * blend);
                wanted = Vector3.Lerp(fromTranslation, wanted, t);
                rotation = LerpAngles(fromRotation, rotation, t);
            }
            Translation = wanted;
            EulerAngles = rotation;
        }

        // The view's turn towards where the target moves on the ground: at most FollowTurnSpeed, the faster it moves the
        // closer to that; not when it moves back towards the camera (pulling back would spin the view round), nor at a
        // teleport's speed
        void FollowTheMovement(Vector3 velocity, float dt)
        {
            velocity.Y = 0.0f;
            float speed = velocity.Length();
            if (speed < MathF.Max(FollowMinSpeed, 0.01f) || speed > 50.0f)
                return;
            float wanted = MathF.Atan2(-velocity.X, -velocity.Z) * 180.0f / MathF.PI; // a yaw: 0 along -Z, positive to the left
            float delta = ((wanted - yaw) % 360.0f + 540.0f) % 360.0f - 180.0f;
            if (MathF.Abs(delta) > 120.0f)
                return;
            float step = FollowTurnSpeed * dt * MathF.Min(1.0f, speed / (4.0f * FollowMinSpeed));
            yaw += Math.Clamp(delta, -step, step);
        }

        // Each angle the short way round (from 170 to -170 through 180, not through 0)
        static Vector3 LerpAngles(Vector3 from, Vector3 to, float t)
        {
            static float Lerp(float a, float b, float t)
            {
                float delta = ((b - a) % 360.0f + 540.0f) % 360.0f - 180.0f;
                return a + delta * t;
            }
            return new Vector3(Lerp(from.X, to.X, t), Lerp(from.Y, to.Y, t), Lerp(from.Z, to.Z, t));
        }
    }
}
