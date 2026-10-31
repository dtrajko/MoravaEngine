using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A camera facing another camera (Camera: the one that follows the target) across the target, on the same view
    /// axis: the other camera's view line goes through the target, this camera stands as far beyond the target on that
    /// line and looks straight back along it. The other camera looks down at the target, this one looks up at it; behind
    /// a character, it shows the character's front (a render texture's camera: a screen showing the player's face). The
    /// target's point is Height above its origin, the one the other camera looks at (CameraFollow's BehindHeight); the
    /// mirror point is the point of the view axis nearest it, so the axes stay one even when the other camera looks a
    /// little past the target. A wall behind the target, where this camera would stand, brings it closer along the axis
    /// (WallDistance short of the wall), so it never looks out from inside a wall.
    /// </summary>
    public class OppositeCamera : Entity
    {
        /// <summary>The entity looked at.</summary>
        public Entity? Target;

        /// <summary>The camera faced: its view axis is this camera's, the other way round.</summary>
        public Entity? Camera;

        /// <summary>Meters above the target's origin the other camera looks at (a character: its feet; as CameraFollow's BehindHeight).</summary>
        public float Height = 1.2f;

        /// <summary>Meters it keeps from walls: a wall between the target and where it would stand brings it closer.</summary>
        public float WallDistance = 0.25f;

        /// <summary>The nearest it comes to the mirror point, however close a wall is.</summary>
        public float MinDistance = 0.3f;

        /// <summary>How fast it moves back out after a wall pushed it closer: higher is faster (as CameraFollow's).</summary>
        public float ReturnSmoothing = 4.0f;

        float distance = -1.0f; // from the mirror point, as last placed (-1: not yet)

        // After the followed camera moved: its script was made first, so it is called first in each frame
        protected override void OnUpdate(float dt)
        {
            if (Target == null || !Target.Exists || Camera == null || !Camera.Exists)
                return;

            Vector3 eye = Camera.WorldPosition;
            Vector3 forward = ViewDirection(Camera.EulerAngles);
            Vector3 pivot = Target.WorldPosition + Vector3.UnitY * Height;
            Vector3 center = eye + forward * Vector3.Dot(pivot - eye, forward); // on the view axis, nearest the target
            Vector3 look = -forward;

            // As far beyond the mirror point as the other camera is before it, unless a wall is nearer: then a sphere's
            // radius short of it, on the same axis (as CameraFollow keeps out of walls). Closer at once, back out smoothly
            float mirrored = Vector3.Distance(eye, center);
            float free = mirrored;
            if (Physics.SphereCast(center, WallDistance, forward, mirrored, out RaycastHit hit, PhysicsLayers.Solid, Target))
                free = MathF.Min(mirrored, MathF.Max(MinDistance, hit.Distance - 0.1f));
            if (distance < 0 || free <= distance)
                distance = free;
            else
                distance += (free - distance) * (1.0f - MathF.Exp(-ReturnSmoothing * dt));

            // As the camera's rotation: X down, Y to the left (0 looks along -Z)
            float yaw = MathF.Atan2(-look.X, -look.Z) * 180.0f / MathF.PI;
            float pitch = MathF.Asin(Math.Clamp(-look.Y, -1.0f, 1.0f)) * 180.0f / MathF.PI;
            Translation = center + forward * distance;
            EulerAngles = new Vector3(pitch, yaw, 0.0f);
        }

        // Where a camera looks, from its rotation as the renderer reads it: X down, Y to the left. Not Forward: a
        // camera's positive X looks down, while its -Z axis turns up
        static Vector3 ViewDirection(Vector3 rotation)
        {
            float pitch = rotation.X * MathF.PI / 180.0f, yaw = rotation.Y * MathF.PI / 180.0f;
            return new Vector3(-MathF.Sin(yaw) * MathF.Cos(pitch), -MathF.Sin(pitch), -MathF.Cos(yaw) * MathF.Cos(pitch));
        }
    }
}
