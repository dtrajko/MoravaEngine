using System;
using System.Collections.Generic;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// Sparkles placed once and used again (an object pool): Scene.Instantiate loads the prefab's model each time, which
    /// takes a moment (about 60 ms a sparkle in a Debug build), so a burst of new ones in the middle of the game pauses it.
    /// The pool places Size sparkles when it is made (at the level's start), hidden; a burst launches free ones, or, when
    /// all are flying, the ones launched first; a finished sparkle hides again instead of being destroyed.
    /// </summary>
    public sealed class SparklePool
    {
        readonly List<Sparkle> sparkles = new();
        readonly Random random = new();
        int next; // the one launched longest ago, when none is free

        /// <summary>Places size sparkles of the prefab, hidden (none when the prefab is null).</summary>
        public SparklePool(Prefab? prefab, int size)
        {
            if (prefab == null)
                return;
            for (int i = 0; i < size; i++)
            {
                var sparkle = (Sparkle)Scene.Instantiate(prefab, Sparkle.HiddenPosition);
                sparkle.Pooled = true;
                sparkle.Hide();
                sparkles.Add(sparkle);
            }
        }

        public int Size => sparkles.Count;

        /// <summary>
        /// count sparkles from position, each its own way out around it (sideways meters per second) and up (meters per
        /// second, plus up to one more at random).
        /// </summary>
        public void Burst(Vector3 position, int count, float sideways, float up)
        {
            for (int i = 0; i < count && sparkles.Count > 0; i++)
            {
                Sparkle sparkle = Take();
                float angle = (i + (float)random.NextDouble() * 0.5f) * 2.0f * MathF.PI / count;
                sparkle.Launch(position, new Vector3(MathF.Cos(angle) * sideways, up + (float)random.NextDouble(), MathF.Sin(angle) * sideways));
            }
        }

        Sparkle Take()
        {
            foreach (Sparkle sparkle in sparkles)
            {
                if (!sparkle.Flying)
                    return sparkle;
            }
            Sparkle oldest = sparkles[next];
            next = (next + 1) % sparkles.Count;
            return oldest;
        }
    }
}
