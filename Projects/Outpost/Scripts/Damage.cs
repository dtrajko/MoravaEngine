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
        public string Kind;      // "Fall", "Bullet", later "Explosion"...
        public string Zone;      // where on the body: a hitbox's ("Head", "Torso"...); "" when it has none
    }

    /// <summary>
    /// A part of a body that shots hit (Phase 2.2: the dummies; Phase 5: the guards): a child entity with a Collider and a
    /// Kinematic Rigid Body, on one of its body's bones (Inspector: Bone Attachment), so it follows the animation, a fallen
    /// body hit where it lies. The damage goes to Owner (its IDamageable script), times DamageScale ("Head": the weapon's
    /// own head multiplier instead).
    /// </summary>
    public class Hitbox : Entity
    {
        public Entity? Owner;
        public string Zone = "Body";
        public float DamageScale = 1.0f;

        /// <summary>What a hit entity's damage goes to and the zone: a hitbox's owner, else the entity itself (Zone "").</summary>
        public static Entity Resolve(Entity hit, out string zone, out float scale)
        {
            if (hit is Hitbox box && box.Owner != null)
            {
                zone = box.Zone;
                scale = box.DamageScale;
                return box.Owner;
            }
            zone = "";
            scale = 1.0f;
            return hit;
        }
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
