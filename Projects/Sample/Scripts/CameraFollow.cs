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
    /// in front, Orbit (Q, E, R, F, the right stick) turns the view; the target's controller moves it where the camera
    /// looks. Behind, the camera comes closer instead of going through a wall. A switch glides there in SwitchSeconds.
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

        /// <summary>Degrees the view looks down, behind the target, when it switches there.</summary>
        public float BehindPitch = 20.0f;

        /// <summary>How long a switch glides from one view to the next.</summary>
        public float SwitchSeconds = 0.4f;

        Vector3 offset, aboveRotation;
        float yaw, pitch; // behind and in front: degrees, as the camera's rotation (X down, Y to the left)
        float blend = 1.0f;
        Vector3 fromTranslation, fromRotation;

        protected override void OnCreate()
        {
            aboveRotation = EulerAngles;
            yaw = aboveRotation.Y;
            pitch = BehindPitch;
            if (Target != null)
                offset = Translation - Target.Translation;
        }

        protected override void OnUpdate(float dt)
        {
            if (Target == null || !Target.Exists)
                return;

            if (Input.WasPressed("SwitchCamera"))
            {
                View = View == CameraView.FirstPerson ? CameraView.Above : View + 1;
                pitch = View == CameraView.FirstPerson ? 0.0f : BehindPitch;
                fromTranslation = Translation;
                fromRotation = EulerAngles;
                blend = 0.0f;
            }
            if (View != CameraView.Above)
            {
                // Right: clockwise seen from above; up: the view looks up
                Vector2 orbit = Input.GetAxis2D("Orbit");
                yaw -= orbit.X * dt;
                pitch = View == CameraView.Behind ? Math.Clamp(pitch - orbit.Y * dt, -5.0f, 70.0f)
                    : Math.Clamp(pitch - orbit.Y * dt, -60.0f, 60.0f);
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
                    // Back and up from a point above the target, as far as no wall is in between
                    Vector3 pivot = target + Vector3.UnitY * 0.5f;
                    Vector3 back = -forward * MathF.Cos(pitchRadians) + Vector3.UnitY * MathF.Sin(pitchRadians);
                    float distance = BehindDistance;
                    if (Physics.SphereCast(pivot, 0.25f, back, BehindDistance, out RaycastHit hit, PhysicsLayers.Solid, Target))
                        distance = MathF.Max(0.3f, hit.Distance - 0.1f);
                    wanted = pivot + back * distance;
                    rotation = new Vector3(pitch, yaw, 0.0f);
                    break;
                }
                case CameraView.FirstPerson:
                {
                    // Just in front of the ball's surface, a little above its center: the ball is out of view
                    float radius = Target.Scale.Y;
                    wanted = target + forward * (radius + 0.05f) + Vector3.UnitY * (radius * 0.3f);
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
