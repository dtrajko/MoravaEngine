using System;
using Morava;

namespace Outpost
{
    /// <summary>
    /// A training dummy (docs/games/Shooter_Plan.html, Phase 2.2): a soldier in the training uniform (Soldier_Training, the
    /// guards' mesh) to shoot in the greybox. Its Body child has the model and Dummy.manim; the hitboxes (Hitbox scripts on
    /// the body's bones, Owner this) take the hits. A hit plays a flinch (the head's or the chest's); out of health it falls
    /// (Death01) and, after LieSeconds, stands up again with its health back.
    /// </summary>
    public class Dummy : Entity, IDamageable
    {
        public Entity? Body;
        public float MaxHealth = 100.0f;
        public float LieSeconds = 4.0f;

        /// <summary>Health points now; 0 while it lies.</summary>
        public float Health { get; private set; }
        public bool IsDown => down > 0.0f;

        /// <summary>Hits and falls since the start (a test reads them).</summary>
        public int Hits { get; private set; }
        public int Falls { get; private set; }
        public string LastZone { get; private set; } = "";

        Animator? animator;
        float flinch;   // seconds of the flinch left
        float down;     // seconds lying left

        protected override void OnCreate()
        {
            animator = Body?.GetComponent<Animator>();
            if (animator == null)
                Log.Warn($"{Name}: no Body with an Animator, the dummy won't move");
            Health = MaxHealth;
        }

        public void TakeDamage(in DamageInfo damage)
        {
            if (IsDown)
                return;
            Hits++;
            LastZone = damage.Zone;
            Health = MathF.Max(0.0f, Health - damage.Amount);
            if (Health <= 0.0f)
            {
                Falls++;
                down = LieSeconds;
                flinch = 0.0f;
                animator?.Play("Death", 0.1f);
                Log.Info($"{Name}: down ({damage.Zone} hit, {damage.Amount:0} damage)");
                return;
            }
            bool head = damage.Zone == "Head";
            flinch = head ? 0.43f : 0.33f;
            animator?.Play(head ? "HitHead" : "HitChest", 0.05f);
        }

        protected override void OnUpdate(float dt)
        {
            if (down > 0.0f)
            {
                down -= dt;
                if (down <= 0.0f)
                {
                    down = 0.0f;
                    Health = MaxHealth;
                    animator?.Play("Idle", 0.8f); // up again
                }
                return;
            }
            if (flinch > 0.0f)
            {
                flinch -= dt;
                if (flinch <= 0.0f)
                    animator?.Play("Idle", 0.25f);
            }
        }
    }
}
