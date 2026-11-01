using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A level's goal: a gate that opens when all the level's coins are collected (its Glow grows in), and finishes the
    /// level when the player walks into it then. The entity needs a Collider with Is Trigger on (the gate's opening). A
    /// level with a goal is done at the goal, a level without one with its last coin (CollectorLevel).
    /// </summary>
    public class LevelGoal : Entity
    {
        /// <summary>What lights up (its scale grows from 0 to the scale it was placed with).</summary>
        public Entity? Glow;

        /// <summary>Seconds the glow takes to grow in.</summary>
        public float OpenSeconds = 0.8f;

        /// <summary>Played when it opens (relative to the project; empty: none).</summary>
        public string OpenSound = "Assets/Audio/Elevator_Clank.wav";

        public bool IsOpen { get; private set; }

        Vector3 glowScale;
        float openTime = -1;

        protected override void OnCreate()
        {
            if (Glow != null)
            {
                glowScale = Glow.Scale;
                Glow.Scale = Vector3.Zero;
            }
        }

        /// <summary>All coins are in: the gate opens.</summary>
        public void Open()
        {
            if (IsOpen)
                return;
            IsOpen = true;
            openTime = 0;
            if (OpenSound.Length > 0)
                Audio.PlayOneShot(OpenSound, WorldPosition, 0.9f);
        }

        protected override void OnUpdate(float dt)
        {
            if (openTime < 0 || Glow == null)
                return;
            openTime += dt;
            float t = Math.Clamp(openTime / MathF.Max(OpenSeconds, 0.01f), 0.0f, 1.0f);
            float pulse = t < 1.0f ? t * t * (3.0f - 2.0f * t) : 1.0f + 0.04f * MathF.Sin((float)Time.GameTime * 4.0f);
            Glow.Scale = glowScale * pulse;
        }

        protected override void OnTriggerEnter(Entity other)
        {
            if (IsOpen && other.HasTag("Player"))
                CollectorLevel.Current?.ReachGoal();
        }
    }
}
