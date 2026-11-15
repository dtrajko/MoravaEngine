using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// A free camera for looking around a level while it is built (docs/games/Shooter_Plan.html, Phase 0): Look (the mouse,
    /// the right stick) turns it, Move (W A S D, the left stick) flies along the view, Jump and Crouch (Space, Ctrl) rise and
    /// sink, Walk (Shift) is faster. It goes through walls. Kept as a debug camera once the player comes (Phase 1).
    /// </summary>
    public class FlyCamera : Entity
    {
        /// <summary>Meters per second, and times that while Walk is held.</summary>
        public float Speed = 6.0f;
        public float FastFactor = 4.0f;

        /// <summary>Times the input map's Look (0.1 degrees per pixel, 180 degrees per second on the stick).</summary>
        public float Sensitivity = 1.0f;

        float yaw, pitch; // degrees, as the camera's rotation (yaw 0 looks along -Z, positive to the left)

        protected override void OnCreate()
        {
            Input.CursorLocked = true;
            Vector3 angles = EulerAngles;
            pitch = angles.X;
            yaw = angles.Y;
        }

        protected override void OnUpdate(float dt)
        {
            // The mouse only while it is the game's (a free cursor moves to the editor's panels); the stick always
            InputAction look = Input.GetAction("Look");
            if (Input.MouseCaptured || look.FromGamepad)
            {
                Vector2 turn = look.Value * Sensitivity;
                yaw = ((yaw - turn.X * dt) % 360.0f + 360.0f) % 360.0f;
                pitch = Math.Clamp(pitch - turn.Y * dt, -89.0f, 89.0f);
            }
            EulerAngles = new Vector3(pitch, yaw, 0.0f);

            float yawRadians = yaw * MathF.PI / 180.0f, pitchRadians = pitch * MathF.PI / 180.0f;
            Vector3 forward = new Vector3(-MathF.Sin(yawRadians) * MathF.Cos(pitchRadians), -MathF.Sin(pitchRadians),
                -MathF.Cos(yawRadians) * MathF.Cos(pitchRadians));
            Vector3 right = new Vector3(MathF.Cos(yawRadians), 0.0f, -MathF.Sin(yawRadians));

            Vector2 move = Input.GetAxis2D("Move");
            float rise = (Input.IsHeld("Jump") ? 1.0f : 0.0f) - (Input.IsHeld("Crouch") ? 1.0f : 0.0f);
            Vector3 velocity = forward * move.Y + right * move.X + Vector3.UnitY * rise;
            if (velocity.LengthSquared() > 1.0f)
                velocity = Vector3.Normalize(velocity);
            float speed = Speed * (Input.IsHeld("Walk") ? FastFactor : 1.0f);
            Translation += velocity * speed * dt;
        }
    }
}
