using System;
using System.Collections.Generic;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// Something the player uses with Use (docs/games/Shooter_Plan.html, Phase 4.1): a door now, switches and the like
    /// later. The player uses the one it looks at within reach (Usables.Find); the HUD shows the key and what it does
    /// ("E  OPEN", "LOCKED").
    /// </summary>
    public interface IUsable
    {
        /// <summary>The point to reach and look at, nearest to where the eyes are.</summary>
        Vector3 UsePoint(Vector3 eyes);

        /// <summary>Meters from the eyes to UsePoint it can be used from.</summary>
        float UseReach { get; }

        /// <summary>What Use does now, for the HUD (OPEN, CLOSE); a word without the key when it can't (LOCKED).</summary>
        string UseText { get; }

        /// <summary>Use can do something (the HUD shows the key before UseText only then).</summary>
        bool CanUse { get; }

        /// <summary>Use pressed by a player (in the fixed step).</summary>
        void Use(Player player);
    }

    /// <summary>The usable things in the scene (they add themselves in OnCreate), and the one a player looks at.</summary>
    public static class Usables
    {
        /// <summary>Degrees off the look's direction a usable thing's point may be.</summary>
        public const float MaxAngle = 35.0f;

        static readonly List<(Entity Entity, IUsable Usable)> list = new();

        public static int Count
        {
            get
            {
                list.RemoveAll(u => !u.Entity.Exists);
                return list.Count;
            }
        }

        internal static void Add<T>(T usable) where T : Entity, IUsable
        {
            list.RemoveAll(u => !u.Entity.Exists);
            if (!list.Exists(u => u.Entity == usable))
                list.Add((usable, usable));
        }

        /// <summary>
        /// The usable thing looked at from the eyes (look: the direction, length 1): within its reach and MaxAngle of the
        /// look, the nearest to the look's line (by angle); null none.
        /// </summary>
        public static IUsable? Find(Vector3 eyes, Vector3 look)
        {
            list.RemoveAll(u => !u.Entity.Exists);
            IUsable? best = null;
            float bestCos = MathF.Cos(MaxAngle * MathF.PI / 180.0f);
            foreach (var (_, usable) in list)
            {
                Vector3 to = usable.UsePoint(eyes) - eyes;
                float distance = to.Length();
                if (distance > usable.UseReach)
                    continue;
                // Very near (the point at the eyes): any look will do
                float cos = distance < 0.05f ? 1.0f : Vector3.Dot(to / distance, look);
                if (cos >= bestCos)
                {
                    bestCos = cos;
                    best = usable;
                }
            }
            return best;
        }
    }
}
