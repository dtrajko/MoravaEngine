using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A bit of a collected pickup or coin (Assets/Prefabs/Sparkle.mprefab): it flies off with its Velocity, falls, spins
    /// and shrinks, and is done after Lifetime. A sparkle of a SparklePool hides then, to be launched again; one placed on
    /// its own (Scene.Instantiate, then Velocity set) is destroyed.
    /// </summary>
    public class Sparkle : Entity
    {
        /// <summary>Where a hidden sparkle waits: far below anything a level has.</summary>
        public static readonly Vector3 HiddenPosition = new Vector3(0, -1000, 0);

        /// <summary>Meters per second: set by whoever places or launches it.</summary>
        public Vector3 Velocity;

        /// <summary>Seconds until it is done.</summary>
        public float Lifetime = 0.8f;

        /// <summary>Meters per second squared, down.</summary>
        public float Gravity = 9.81f;

        /// <summary>It belongs to a pool: done, it hides instead of being destroyed.</summary>
        public bool Pooled { get; set; }

        /// <summary>It flies now (a pool's hidden sparkle doesn't).</summary>
        public bool Flying { get; private set; } = true;

        Vector3 startScale;
        float age;

        protected override void OnCreate() => startScale = Scale;

        /// <summary>From position with velocity, as new.</summary>
        public void Launch(Vector3 position, Vector3 velocity)
        {
            Translation = position;
            Rotation = Quaternion.Identity;
            Scale = startScale;
            Velocity = velocity;
            age = 0;
            Flying = true;
        }

        /// <summary>Out of sight (no size, far below), not flying.</summary>
        public void Hide()
        {
            Flying = false;
            Scale = Vector3.Zero;
            Translation = HiddenPosition;
        }

        protected override void OnUpdate(float dt)
        {
            if (!Flying)
                return;
            age += dt;
            if (age >= Lifetime)
            {
                if (Pooled) Hide();
                else Destroy();
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
