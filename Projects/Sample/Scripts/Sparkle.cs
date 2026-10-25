using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A bit of a collected pickup (Assets/Prefabs/Sparkle.mprefab): the ball places a few where the pickup was
    /// (Scene.Instantiate) and gives each a velocity. It flies off, falls, spins and shrinks, and is gone after Lifetime.
    /// </summary>
    public class Sparkle : Entity
    {
        /// <summary>Meters per second: set by whoever places it.</summary>
        public Vector3 Velocity;

        /// <summary>Seconds until it is gone.</summary>
        public float Lifetime = 0.8f;

        /// <summary>Meters per second squared, down.</summary>
        public float Gravity = 9.81f;

        Vector3 startScale;
        float age;

        protected override void OnCreate() => startScale = Scale;

        protected override void OnUpdate(float dt)
        {
            age += dt;
            if (age >= Lifetime)
            {
                Destroy();
                return;
            }
            Velocity.Y -= Gravity * dt;
            Translation += Velocity * dt;
            Quaternion spin = Quaternion.CreateFromAxisAngle(Vector3.Normalize(new Vector3(1, 2, 0.5f)), 10.0f * dt);
            Rotation = Quaternion.Normalize(spin * Rotation);
            Scale = startScale * (1.0f - age / Lifetime);
        }
    }
}
