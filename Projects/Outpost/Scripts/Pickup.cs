using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>What a pickup gives.</summary>
    public enum PickupKind
    {
        /// <summary>Health points (a small pack 25, a large one 50), up to the most.</summary>
        Health,
        /// <summary>Armor points (a vest: 100), up to the most.</summary>
        Armor,
        /// <summary>Spare ammo: Amount magazines for each gun carried, up to what each starts with.</summary>
        Ammo,
    }

    /// <summary>
    /// A health pack, a vest or an ammo box (docs/games/Shooter_Plan.html, Phase 3.1) on the entity's trigger (a Collider
    /// with Is Trigger on): the player walking into it takes it, if it gives anything (full health leaves a health pack
    /// lying). Standing in it, it is taken as soon as it would give something. Taken, its model (Visual: the child with
    /// it) is hidden and its sound heard; after RespawnSeconds it is back (0: never, the mission's pickups).
    /// The rules run in the fixed step (Player.TryPickUp), as the rest of the game's.
    /// </summary>
    public class Pickup : Entity
    {
        public PickupKind Kind = PickupKind.Health;

        /// <summary>Points (Health, Armor), or magazines per gun (Ammo).</summary>
        public float Amount = 25.0f;

        /// <summary>Seconds until a taken pickup is back (0: never).</summary>
        public float RespawnSeconds = 0.0f;

        /// <summary>The child with the model, hidden while taken.</summary>
        public Entity? Visual;

        /// <summary>Heard when it is taken, and how loud.</summary>
        public string Sound = "Assets/Audio/Pickup_Health.wav";
        public float Volume = 0.7f;

        /// <summary>The name the HUD's message gives it (empty: by its kind).</summary>
        public string Label = "";

        /// <summary>It is there to take (not taken, or back).</summary>
        public bool IsAvailable => backIn <= 0.0f && !gone;

        /// <summary>Times it was taken (a test reads it).</summary>
        public int Taken { get; private set; }

        Player? inside;
        Model? model;
        float backIn;  // seconds until it is back (0: here)
        bool gone;     // taken for good (RespawnSeconds 0)

        protected override void OnCreate()
        {
            model = Visual?.GetComponent<Model>();
            if (!HasComponent<Collider>())
                Log.Warn($"{Name}: no Collider (Is Trigger) for Pickup: nothing can take it");
        }

        protected override void OnTriggerEnter(Entity other)
        {
            if (other is Player player)
                inside = player;
        }

        protected override void OnTriggerExit(Entity other)
        {
            if (other is Player player && player == inside)
                inside = null;
        }

        protected override void OnFixedUpdate(float dt)
        {
            if (backIn > 0.0f)
            {
                backIn -= dt;
                if (backIn <= 0.0f)
                {
                    backIn = 0.0f;
                    Show(true);
                }
                return;
            }
            if (gone || inside == null || !inside.Exists)
                return;
            if (!inside.TryPickUp(this))
                return;
            Taken++;
            if (!string.IsNullOrEmpty(Sound))
                Audio.PlayOneShot(Sound, WorldPosition, Volume);
            Show(false);
            if (RespawnSeconds > 0.0f)
                backIn = RespawnSeconds;
            else
                gone = true;
        }

        /// <summary>Back at once (a respawn of the level's pickups; Phase 7's mission state decides when).</summary>
        public void Restore()
        {
            backIn = 0.0f;
            gone = false;
            Show(true);
        }

        void Show(bool shown)
        {
            if (model != null && model.Visible != shown)
                model.Visible = shown;
        }

        /// <summary>The HUD's name for it.</summary>
        public string DisplayName => Label.Length > 0 ? Label : Kind switch
        {
            PickupKind.Health => "HEALTH",
            PickupKind.Armor => "ARMOR",
            _ => "AMMO",
        };
    }
}
