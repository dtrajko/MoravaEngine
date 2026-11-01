using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A coin of the collector (Assets/Prefabs/Coin.mprefab): it turns and bobs where it is placed, and when the player
    /// (an entity tagged Player) touches it, it counts for the level (CollectorLevel.Collect) and is gone. The entity needs
    /// a Collider with Is Trigger on and the tag Coin (the level counts its coins by it).
    /// </summary>
    public class Coin : Entity
    {
        /// <summary>Degrees per second around the world's up axis.</summary>
        public float DegreesPerSecond = 120.0f;

        /// <summary>Meters up and down from where it was placed.</summary>
        public float BobHeight = 0.08f;

        /// <summary>Bobs per second.</summary>
        public float BobSpeed = 0.6f;

        Vector3 home;
        float phase;
        bool collected;

        protected override void OnCreate()
        {
            home = Translation;
            // Not all in step: the phase from where it is
            phase = (home.X * 0.37f + home.Z * 0.61f) % 1.0f;
        }

        protected override void OnUpdate(float dt)
        {
            Quaternion turn = Quaternion.CreateFromAxisAngle(Vector3.UnitY, DegreesPerSecond * dt * MathF.PI / 180.0f);
            Rotation = Quaternion.Normalize(turn * Rotation);
            float bob = MathF.Sin(((float)Time.GameTime * BobSpeed + phase) * 2.0f * MathF.PI) * BobHeight;
            Translation = home + new Vector3(0, bob, 0);
        }

        protected override void OnTriggerEnter(Entity other)
        {
            if (collected || !other.HasTag("Player"))
                return;
            collected = true; // the character's inner body may touch it twice before it is gone
            CollectorLevel.Current?.Collect(this);
            Destroy();
        }
    }
}
