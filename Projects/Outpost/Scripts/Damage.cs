using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>One hit of damage: how much, who or what did it, where and from which way.</summary>
    public struct DamageInfo
    {
        public float Amount;
        public Entity? Source;   // the shooter, a grenade; null for the world (a fall)
        public Vector3 Point;    // where it hit, in the world
        public Vector3 Direction; // the way it went (a shot's direction; a fall: down)
        public string Kind;      // "Fall", later "Bullet", "Explosion"...
    }

    /// <summary>What can be hurt: the player, later the guards, the dummies (Phase 2), breakable things.</summary>
    public interface IDamageable
    {
        void TakeDamage(in DamageInfo damage);
    }

    /// <summary>
    /// The one way to deal damage (docs/games/Shooter_Plan.html, keeping multiplayer possible): Apply hands it to the
    /// target and tells the rest of the game (the HUD's hit marker and damage direction, sounds, the mission's rules) through
    /// Dealt. A server would be the one to call it.
    /// </summary>
    public static class Damage
    {
        /// <summary>After each damage a target took.</summary>
        public static event Action<Entity, DamageInfo>? Dealt;

        /// <summary>False: the target can't be hurt (its script isn't an IDamageable), or nothing to deal.</summary>
        public static bool Apply(Entity target, in DamageInfo damage)
        {
            if (damage.Amount <= 0.0f || target is not IDamageable damageable)
                return false;
            damageable.TakeDamage(damage);
            Dealt?.Invoke(target, damage);
            return true;
        }
    }
}
