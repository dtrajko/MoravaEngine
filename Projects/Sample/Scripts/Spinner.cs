using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>The axes an entity can turn around.</summary>
    public enum Axis { X, Y, Z }

    /// <summary>Turns the entity around one of its axes, at a steady speed.</summary>
    public class Spinner : Entity
    {
        /// <summary>The entity's own axis it turns around.</summary>
        public Axis Around = Axis.Y;

        /// <summary>Degrees per second; negative turns the other way.</summary>
        public float DegreesPerSecond = 45.0f;

        protected override void OnUpdate(float dt)
        {
            Vector3 axis = Around switch { Axis.X => Vector3.UnitX, Axis.Y => Vector3.UnitY, _ => Vector3.UnitZ };
            float angle = DegreesPerSecond * dt * MathF.PI / 180.0f;
            Rotation = Quaternion.Normalize(Rotation * Quaternion.CreateFromAxisAngle(axis, angle));
        }
    }
}
