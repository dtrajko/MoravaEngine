using System;
using System.Collections.Generic;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// A place that hurts while one stands in it (docs/games/Shooter_Plan.html, Phase 3.1: a live electric floor in the
    /// greybox, to try health, armor and death before the guards fight back; later the base's hazards), on the entity's
    /// trigger (a Collider with Is Trigger on). Whatever can be hurt inside it (an IDamageable: the player; a hitbox's
    /// owner) takes DamagePerSecond, dealt each TickSeconds through Damage.Apply (Kind: the zone's, its direction up from
    /// the floor). It sparks meanwhile, more often with someone in it: bursts of the Sparks emitter at random points in the
    /// Area (half sizes, meters, around the entity), each with a light's flash and a crackle; the hum is the entity's
    /// AudioSource (looping, played from the start).
    /// </summary>
    public class HazardZone : Entity
    {
        public float DamagePerSecond = 20.0f;
        public float TickSeconds = 0.25f;
        public string Kind = "Electric";

        /// <summary>The sparks' emitter (its bursts), and the area they come from: half sizes around the entity, meters.</summary>
        public Entity? Sparks;
        public Vector3 Area = new Vector3(1.4f, 0.05f, 1.4f);

        /// <summary>Seconds between bursts: idle, and with someone in it (each a random time up to it).</summary>
        public float SparkEvery = 1.2f, SparkEveryInside = 0.35f;

        /// <summary>The crackle ({0}: 1 to SparkVariants) and its volume; the flash's color, brightness and reach.</summary>
        public string SparkSound = "Assets/Audio/Electric_Spark_{0}.wav";
        public int SparkVariants = 3;
        public float SparkVolume = 0.6f;
        public Vector3 FlashColor = new Vector3(0.55f, 0.7f, 1.0f);
        public float FlashIntensity = 6.0f, FlashRange = 4.0f;

        /// <summary>Damage dealt so far (a test reads it).</summary>
        public float Dealt { get; private set; }

        readonly Dictionary<Entity, int> inside = new(); // the damageable, and how many of its bodies are in (hitboxes)
        readonly Random random = new Random(11);
        ParticleEmitter? sparks;
        float tick;
        float sparkIn;

        protected override void OnCreate()
        {
            sparks = Sparks?.GetComponent<ParticleEmitter>();
            if (!HasComponent<Collider>())
                Log.Warn($"{Name}: no Collider (Is Trigger) for HazardZone: it hurts nothing");
            sparkIn = SparkEvery * (float)random.NextDouble();
        }

        protected override void OnTriggerEnter(Entity other)
        {
            Entity target = Hitbox.Resolve(other, out _, out _);
            if (target is not IDamageable)
                return;
            inside[target] = inside.TryGetValue(target, out int count) ? count + 1 : 1;
        }

        protected override void OnTriggerExit(Entity other)
        {
            Entity target = Hitbox.Resolve(other, out _, out _);
            if (!inside.TryGetValue(target, out int count))
                return;
            if (count <= 1)
                inside.Remove(target);
            else
                inside[target] = count - 1;
        }

        /// <summary>Someone that can be hurt is in it now.</summary>
        public bool IsOccupied => inside.Count > 0;

        protected override void OnFixedUpdate(float dt)
        {
            if (inside.Count == 0)
            {
                tick = 0.0f; // the first tick a full TickSeconds after stepping in
                return;
            }
            tick += dt;
            if (tick < TickSeconds)
                return;
            tick -= TickSeconds;
            float amount = DamagePerSecond * TickSeconds;
            foreach (Entity target in new List<Entity>(inside.Keys))
            {
                if (!target.Exists)
                {
                    inside.Remove(target);
                    continue;
                }
                Vector3 at = target.WorldPosition;
                if (Damage.Apply(target, new DamageInfo { Amount = amount, Source = this, Point = at, Direction = Vector3.UnitY, Kind = Kind, Zone = "" }))
                    Dealt += amount;
            }
        }

        protected override void OnUpdate(float dt)
        {
            sparkIn -= dt;
            if (sparkIn > 0.0f)
                return;
            float every = inside.Count > 0 ? SparkEveryInside : SparkEvery;
            sparkIn = every * (0.3f + 0.7f * (float)random.NextDouble());
            Spark();
        }

        // A burst at a random point of the area: sparks thrown up, a blue-white flash, a crackle
        void Spark()
        {
            Vector3 local = new Vector3(Signed() * Area.X, Area.Y, Signed() * Area.Z);
            Vector3 at = Vector3.Transform(local, WorldTransform);
            Vector3 up = Vector3.Normalize(new Vector3(Signed() * 0.4f, 1.0f, Signed() * 0.4f));
            sparks?.Emit(6 + random.Next(10), new ParticleBurst(at, up));
            Lights.Flash(at + new Vector3(0.0f, 0.15f, 0.0f), FlashColor, FlashIntensity * (0.6f + 0.4f * (float)random.NextDouble()), FlashRange, 0.12f);
            if (SparkVariants > 0 && !string.IsNullOrEmpty(SparkSound))
                Audio.PlayOneShot(string.Format(SparkSound, random.Next(1, SparkVariants + 1)), at, SparkVolume, AudioBus.Effects,
                    0.9f + 0.2f * (float)random.NextDouble());
        }

        float Signed() => (float)random.NextDouble() * 2.0f - 1.0f;
    }
}
