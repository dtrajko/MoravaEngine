using System;
using Morava;

namespace Outpost
{
    /// <summary>
    /// A player's health and armor (docs/games/Shooter_Plan.html, Phase 3.1): the game's rules, as the weapons' are. Health
    /// 100 and armor up to 100; while armor lasts it takes two thirds of each hit (ArmorShare) and loses as many points, the
    /// rest goes to health. Falls go through armor (a vest doesn't soften a landing). No regeneration: health comes from
    /// health packs, armor from vests (Pickup). At 0 health the player is dead until Reset (the respawn).
    /// </summary>
    public sealed class PlayerHealth
    {
        public PlayerHealth(float maxHealth, float maxArmor)
        {
            MaxHealth = MathF.Max(1.0f, maxHealth);
            MaxArmor = MathF.Max(0.0f, maxArmor);
            Health = MaxHealth;
        }

        public float MaxHealth { get; }
        public float MaxArmor { get; }

        /// <summary>The share of each hit armor takes while it lasts (two thirds).</summary>
        public float ArmorShare { get; set; } = 2.0f / 3.0f;

        public float Health { get; private set; }
        public float Armor { get; private set; }
        public bool IsDead => Health <= 0.0f;

        /// <summary>A hit taken (the damage, the health and the armor it took): what is seen and heard reacts.</summary>
        public event Action<DamageInfo, float, float>? Hurt;

        /// <summary>The hit that took the last of the health.</summary>
        public event Action<DamageInfo>? Died;

        /// <summary>Takes a hit: armor first (its share, as far as it lasts), the rest from health. Nothing once dead.</summary>
        public void TakeDamage(in DamageInfo damage)
        {
            if (IsDead || damage.Amount <= 0.0f)
                return;
            float toArmor = damage.Kind == "Fall" ? 0.0f : MathF.Min(Armor, damage.Amount * ArmorShare);
            float toHealth = MathF.Min(Health, damage.Amount - toArmor);
            Armor -= toArmor;
            Health -= toHealth;
            if (Health < 0.001f)
                Health = 0.0f;
            Hurt?.Invoke(damage, toHealth, toArmor);
            if (IsDead)
                Died?.Invoke(damage);
        }

        /// <summary>Health added (a health pack), up to MaxHealth: the points it gave (0: full, or dead).</summary>
        public float Heal(float amount)
        {
            if (IsDead)
                return 0.0f;
            float added = MathF.Min(amount, MaxHealth - Health);
            if (added <= 0.0f)
                return 0.0f;
            Health += added;
            return added;
        }

        /// <summary>Armor added (a vest), up to MaxArmor: the points it gave (0: full, or dead).</summary>
        public float AddArmor(float amount)
        {
            if (IsDead)
                return 0.0f;
            float added = MathF.Min(amount, MaxArmor - Armor);
            if (added <= 0.0f)
                return 0.0f;
            Armor += added;
            return added;
        }

        /// <summary>Alive again (the respawn): full health, this much armor.</summary>
        public void Reset(float armor = 0.0f)
        {
            Health = MaxHealth;
            Armor = Math.Clamp(armor, 0.0f, MaxArmor);
        }
    }
}
