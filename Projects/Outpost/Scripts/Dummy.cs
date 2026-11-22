using System;
using Morava;

namespace Outpost
{
    /// <summary>
    /// A training dummy (docs/games/Shooter_Plan.html, Phase 2.2): a soldier in the training uniform (Soldier_Training, the
    /// guards' mesh) to shoot in the greybox. Its Body child has the model and Dummy.manim; the hitboxes (Hitbox scripts on
    /// the body's bones, Owner this) take the hits. A hit plays a flinch (the head's or the chest's); out of health it falls
    /// (Death_Back: the library's Death01, the left arm settling relaxed on the floor) and, after LieSeconds, gets up
    /// (Phase 2.4: our own Get_Up, Tools/CharacterPoses/make_get_up.py, from where Death_Back leaves it lying to Idle's
    /// stance: before, a cross-fade to Idle swung a leg through the floor; since 2026-11-22 up onto the elbows, then the
    /// hands), its health back once it stands.
    /// </summary>
    public class Dummy : Entity, IDamageable
    {
        public Entity? Body;
        public float MaxHealth = 100.0f;
        public float LieSeconds = 4.0f;

        /// <summary>Seconds the get-up takes (the Get_Up clip's length): down until it has stood up.</summary>
        public float GetUpSeconds = 2.55f;

        /// <summary>Health points now; 0 while it lies and gets up.</summary>
        public float Health { get; private set; }
        public bool IsDown => down > 0.0f || gettingUp > 0.0f;

        /// <summary>It is getting up (after lying, before it stands).</summary>
        public bool IsGettingUp => gettingUp > 0.0f;

        /// <summary>Hits and falls since the start (a test reads them).</summary>
        public int Hits { get; private set; }
        public int Falls { get; private set; }
        public string LastZone { get; private set; } = "";

        Animator? animator;
        float flinch;   // seconds of the flinch left
        float down;     // seconds lying left
        float gettingUp; // seconds of the get-up left

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
                    gettingUp = GetUpSeconds;
                    animator?.Play("GetUp", 0.0f); // it starts where Death_Back ends
                }
                return;
            }
            if (gettingUp > 0.0f)
            {
                gettingUp -= dt;
                if (gettingUp <= 0.0f)
                {
                    gettingUp = 0.0f;
                    Health = MaxHealth;
                    animator?.Play("Idle", 0.2f); // Get_Up ends in Idle's first pose
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
