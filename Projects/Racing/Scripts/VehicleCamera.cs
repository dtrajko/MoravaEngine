using System;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// A chase camera for a vehicle: behind it and a little above, turning after it as it turns (smoothly: a quick swerve
    /// doesn't swing the view), along its heading, so it stays behind while it reverses. The mouse (Look, with MouseLook:
    /// the game captures the cursor, Escape gives it back) and Orbit (Q, E, R, F, the right stick) look around it; let go
    /// for ReturnDelay seconds, the view glides back behind it. Zoom (the mouse wheel, the D-pad's up and down) brings it
    /// closer or farther. A hill or a wall between the vehicle and the camera brings the camera closer at once; it glides
    /// back out once it is passed. Put it on a camera at the top of the scene (no parent).
    /// </summary>
    public class VehicleCamera : Entity
    {
        /// <summary>The vehicle followed.</summary>
        public Entity? Target;

        /// <summary>Meters behind the point the view turns around.</summary>
        public float Distance = 6.5f;

        /// <summary>Meters above the vehicle's origin the view turns around (and looks at).</summary>
        public float Height = 1.2f;

        /// <summary>Degrees the view looks down from behind.</summary>
        public float Pitch = 12.0f;

        /// <summary>How fast the view turns after the vehicle's heading: higher is tighter.</summary>
        public float TurnSmoothing = 3.0f;

        /// <summary>
        /// How fast the camera catches up with the vehicle's ups and downs (bumps, a jump): higher is tighter (0: at once).
        /// Along the ground the point it turns around stays on the vehicle (smoothed, it trailed meters behind at speed).
        /// </summary>
        public float Smoothing = 12.0f;

        /// <summary>The mouse looks around the vehicle (the game captures the cursor while it has the input).</summary>
        public bool MouseLook = true;

        /// <summary>Times the mouse's turn (the input map's Look: 0.1 degrees per pixel).</summary>
        public float MouseSensitivity = 1.0f;

        /// <summary>Seconds after the mouse or Orbit last turned the view before it glides back behind the vehicle.</summary>
        public float ReturnDelay = 1.5f;

        /// <summary>How fast it glides back behind the vehicle: higher is faster.</summary>
        public float ReturnSmoothing = 2.0f;

        /// <summary>Meters closer or farther per notch of Zoom, and the distances it keeps within.</summary>
        public float ZoomStep = 0.75f;
        public float MinDistance = 3.0f;
        public float MaxDistance = 14.0f;

        /// <summary>How fast it moves back out after a hill or a wall brought it closer: higher is faster.</summary>
        public float OutSmoothing = 4.0f;

        // ---- The racing feel (docs/games/Racing_Plan.html, Phase 6)
        /// <summary>Degrees the field of view widens at FovSpeed km/h (with the square root of the speed: most of it early).</summary>
        public float SpeedFov = 12.0f;
        public float FovSpeed = 300.0f;
        /// <summary>Seconds of the vehicle's turning the view looks ahead into a turn (0: none), and its most, in degrees.</summary>
        public float LookAhead = 0.25f;
        public float MaxLookAhead = 12.0f;
        /// <summary>Meters the view dips per m/s the vehicle was falling when it lands, its most, and how fast it comes back.</summary>
        public float LandingDip = 0.04f;
        public float MaxLandingDip = 0.6f;
        public float DipRecovery = 6.0f;

        float yaw;                 // the vehicle's heading, followed: degrees, as the camera's rotation (0 along -Z, positive to the left)
        float lookYaw, lookPitch;  // the player's turn around it, on top
        float sinceLook = float.MaxValue;
        float zoomDistance, distance = -1.0f; // the distance Zoom chose; the one now (a hill: closer)
        Vector3 pivot;
        bool started;
        Camera? camera;
        Rigidbody? body;
        float baseFov, fov;          // the camera's own field of view; the one now
        float lastHeading, turnLook; // the heading in the last frame; the look into the turn now (degrees)
        float lastFall, dip;         // m/s the vehicle fell in the last frame; the dip now (meters)

        protected override void OnCreate()
        {
            if (MouseLook)
                Input.CursorLocked = true;
            camera = GetComponent<Camera>();
            baseFov = fov = camera?.FieldOfView ?? 60.0f;
            zoomDistance = Math.Clamp(Distance, MathF.Min(MinDistance, MaxDistance), MathF.Max(MinDistance, MaxDistance));
        }

        protected override void OnUpdate(float dt)
        {
            if (Target == null || !Target.Exists)
                return;

            // The heading on the ground: kept while the vehicle points (nearly) straight up or down
            Vector3 forward = Target.Forward;
            forward.Y = 0.0f;
            bool hasHeading = forward.LengthSquared() > 0.01f;
            float heading = hasHeading ? MathF.Atan2(-forward.X, -forward.Z) * 180.0f / MathF.PI : yaw;
            Vector3 wantedPivot = Target.WorldPosition + Vector3.UnitY * Height;
            if (!started)
            {
                yaw = heading;
                lastHeading = heading;
                pivot = wantedPivot;
                started = true;
            }
            yaw += WrapAngle(heading - yaw) * Ease(TurnSmoothing, dt);
            // On top of the vehicle: exactly over it along the ground, only its height smoothed
            float height = Smoothing > 0 ? pivot.Y + (wantedPivot.Y - pivot.Y) * Ease(Smoothing, dt) : wantedPivot.Y;
            pivot = new Vector3(wantedPivot.X, height, wantedPivot.Z);

            // Looking around: the mouse only while it is the game's (a free cursor moves to the editor's panels)
            Vector2 orbit = Input.GetAxis2D("Orbit");
            if (MouseLook && Input.MouseCaptured)
                orbit += Input.GetAxis2D("Look") * MouseSensitivity * GameOptions.MouseSensitivity; // times the player's choice (Settings)
            lookYaw = WrapAngle(lookYaw - orbit.X * dt);
            lookPitch = Math.Clamp(lookPitch - orbit.Y * dt, -10.0f - Pitch, 70.0f - Pitch);
            sinceLook = orbit.LengthSquared() > 1e-4f ? 0.0f : sinceLook + dt;
            if (sinceLook >= ReturnDelay)
            {
                float back = Ease(ReturnSmoothing, dt);
                lookYaw -= lookYaw * back;
                lookPitch -= lookPitch * back;
            }

            // The wheel: its notches this frame; a gamepad's buttons: a step at each press
            InputAction zoom = Input.GetAction("Zoom");
            float notches = !zoom.FromGamepad ? zoom.Value.X : zoom.WasPressed ? MathF.Sign(zoom.Value.X) : 0.0f;
            zoomDistance = Math.Clamp(zoomDistance - notches * ZoomStep, MathF.Min(MinDistance, MaxDistance), MathF.Max(MinDistance, MaxDistance));

            // Back and up from the pivot, as far as nothing is in between: closer at once, back out smoothly
            UpdateFeel(heading, dt);
            float viewYaw = yaw + lookYaw + turnLook, viewPitch = Pitch + lookPitch;
            float yawRadians = viewYaw * MathF.PI / 180.0f, pitchRadians = viewPitch * MathF.PI / 180.0f;
            Vector3 ahead = new Vector3(-MathF.Sin(yawRadians), 0.0f, -MathF.Cos(yawRadians));
            Vector3 backward = -ahead * MathF.Cos(pitchRadians) + Vector3.UnitY * MathF.Sin(pitchRadians);
            float free = zoomDistance;
            if (Physics.SphereCast(pivot, 0.3f, backward, zoomDistance, out RaycastHit hit, PhysicsLayers.Solid, Target))
                free = MathF.Max(0.5f, hit.Distance - 0.1f);
            if (distance < 0 || free <= distance)
                distance = free;
            else
                distance += (free - distance) * Ease(OutSmoothing, dt);

            Translation = pivot + backward * distance - Vector3.UnitY * dip;
            EulerAngles = new Vector3(viewPitch, viewYaw, 0.0f);
        }

        // The field of view widens with the speed, the view looks into the turn, and dips when the vehicle lands
        void UpdateFeel(float heading, float dt)
        {
            if (dt <= 0.0f)
                return; // paused
            body ??= Target!.GetComponent<Rigidbody>();
            Vector3 velocity = body?.LinearVelocity ?? Vector3.Zero;
            float kmh = velocity.Length() * 3.6f;

            float wantedFov = baseFov + SpeedFov * MathF.Sqrt(Math.Clamp(kmh / MathF.Max(FovSpeed, 1.0f), 0.0f, 1.5f));
            fov += (wantedFov - fov) * Ease(3.0f, dt);
            if (camera != null && MathF.Abs(camera.FieldOfView - fov) > 0.01f)
                camera.FieldOfView = fov;

            // The heading's turn rate (degrees per second), so many seconds ahead; not while reversing or nearly still
            float rate = WrapAngle(heading - lastHeading) / dt;
            lastHeading = heading;
            float wantedLook = kmh > 20.0f ? Math.Clamp(rate * LookAhead, -MaxLookAhead, MaxLookAhead) : 0.0f;
            turnLook += (wantedLook - turnLook) * Ease(4.0f, dt);

            // Landing: the fall stopped at once (a jump, a crest): dip by how fast it fell, then come back
            float fall = MathF.Max(-velocity.Y, 0.0f);
            if (lastFall - fall > 4.0f)
                dip = MathF.Min(MaxLandingDip, dip + (lastFall - fall) * LandingDip);
            lastFall = fall;
            dip -= dip * Ease(DipRecovery, dt);
        }

        // The part of the way to go in this frame, for a smoothing rate (frame-rate independent)
        static float Ease(float rate, float dt) => rate > 0 ? 1.0f - MathF.Exp(-rate * dt) : 1.0f;

        // An angle's difference the short way round: -180 to 180
        static float WrapAngle(float degrees) => ((degrees % 360.0f) + 540.0f) % 360.0f - 180.0f;
    }
}
