using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>What a bullet hits, by the entity's tag (Metal, Dirt; none: Concrete), a body's hitbox: Flesh.</summary>
    public enum Surface { Concrete, Metal, Dirt, Flesh }

    /// <summary>
    /// What a shot looks like (docs/games/Shooter_Plan.html, Phase 2.3), on an entity whose children are the particle emitters
    /// it uses (each off, Rate 0: it only bursts): the muzzle's flash (particles and a short light: E8; in first person drawn
    /// with the view model, so it stays at the arms' gun), the muzzle's smoke (third person), a tracer every TracerEvery shots
    /// (a streak from a little way out of the muzzle to the hit), and the hit by its surface: concrete's dust, chips and hole,
    /// metal's sparks (and a spark's light) and dent, dirt's puff and clods, a soldier's small dark-red puff (the user's choice:
    /// stylized, no marks). The holes are decals (E6, Decals.Spawn). The sounds of the hits come from where they are.
    /// </summary>
    public class WeaponEffects : Entity
    {
        public Entity? MuzzleFlash, MuzzleFlashView, MuzzleSmoke, Tracers;
        public Entity? ImpactDust, ImpactChips, ImpactSparks, ImpactDirt, ImpactClods, ImpactBlood;

        /// <summary>A tracer on every this many shots (the 1st, 4th, 7th...: the user's choice, every third).</summary>
        public int TracerEvery = 3;
        /// <summary>
        /// The tracer: meters a second, meters from the muzzle it starts (no streak through the first person's own view), and the
        /// seconds it flies at the least: a short shot's tracer flies slower, else it would be gone before a frame shows it.
        /// </summary>
        public float TracerSpeed = 400.0f, TracerStart = 2.5f, TracerMinSeconds = 0.05f;

        /// <summary>The muzzle's light: color, intensity, meters, seconds.</summary>
        public Vector3 FlashColor = new Vector3(1.0f, 0.72f, 0.42f);
        public float FlashIntensity = 7.0f, FlashRange = 7.0f, FlashSeconds = 0.06f;

        /// <summary>The holes' textures (project paths) and sizes across (meters).</summary>
        public string ConcreteHole = "Assets/Textures/Effects/BulletHole_Concrete.png";
        public string MetalHole = "Assets/Textures/Effects/BulletHole_Metal.png";
        public float ConcreteHoleSize = 0.12f, MetalHoleSize = 0.09f, DirtMarkSize = 0.14f;

        /// <summary>The hits' sounds ({0}: the variant) and how many of each.</summary>
        public string ConcreteSound = "Assets/Audio/Impact_Concrete_{0}.wav", MetalSound = "Assets/Audio/Impact_Metal_{0}.wav";
        public string DirtSound = "Assets/Audio/Impact_Dirt_{0}.wav", FleshSound = "Assets/Audio/Impact_Flesh_{0}.wav";
        public int ConcreteVariants = 3, MetalVariants = 3, DirtVariants = 2, FleshVariants = 2;
        public float ImpactVolume = 0.7f;

        ParticleEmitter? muzzleFlash, muzzleFlashView, muzzleSmoke, tracers;
        ParticleEmitter? dust, chips, sparks, dirt, clods, blood;
        readonly Random random = new Random(11);

        /// <summary>What the effects showed so far (a test reads them).</summary>
        public int Flashes { get; private set; }
        public int TracersShown { get; private set; }
        public int Impacts { get; private set; }
        public Surface LastSurface { get; private set; }

        protected override void OnCreate()
        {
            muzzleFlash = MuzzleFlash?.GetComponent<ParticleEmitter>();
            muzzleFlashView = MuzzleFlashView?.GetComponent<ParticleEmitter>();
            muzzleSmoke = MuzzleSmoke?.GetComponent<ParticleEmitter>();
            tracers = Tracers?.GetComponent<ParticleEmitter>();
            dust = ImpactDust?.GetComponent<ParticleEmitter>();
            chips = ImpactChips?.GetComponent<ParticleEmitter>();
            sparks = ImpactSparks?.GetComponent<ParticleEmitter>();
            dirt = ImpactDirt?.GetComponent<ParticleEmitter>();
            clods = ImpactClods?.GetComponent<ParticleEmitter>();
            blood = ImpactBlood?.GetComponent<ParticleEmitter>();
            if (muzzleFlash == null || tracers == null || dust == null)
                Log.Warn($"{Name}: some of the emitters are missing (a child with a Particle Emitter each): those effects aren't shown");
        }

        /// <summary>
        /// The surface a shot hit: a body's hitbox (or a body that takes damage itself) is flesh, else the entity's tag (Concrete
        /// when it has none).
        /// </summary>
        public static Surface SurfaceOf(in ShotResult shot)
        {
            if (shot.Entity is Hitbox || shot.Target is IDamageable)
                return Surface.Flesh;
            Entity? entity = shot.Entity;
            if (entity == null)
                return Surface.Concrete;
            if (entity.HasTag("Metal"))
                return Surface.Metal;
            if (entity.HasTag("Dirt"))
                return Surface.Dirt;
            return Surface.Concrete;
        }

        /// <summary>
        /// A shot fired: the flash at the muzzle (muzzle and its direction; firstPerson: at the view model's gun, drawn with it,
        /// viewMuzzle and viewDirection being its), the smoke, the tracer from muzzle, and the hit.
        /// </summary>
        public void Shot(in ShotResult shot, Vector3 muzzle, Vector3 direction, bool firstPerson, Vector3 viewMuzzle, Vector3 viewDirection)
        {
            Vector3 flashAt = firstPerson ? viewMuzzle : muzzle;
            Vector3 flashDirection = firstPerson ? viewDirection : direction;
            ParticleEmitter? flash = firstPerson ? muzzleFlashView : muzzleFlash;
            flash?.Emit(3, new ParticleBurst(flashAt + flashDirection * 0.04f, flashDirection));
            if (!firstPerson)
                muzzleSmoke?.Emit(2, new ParticleBurst(muzzle + direction * 0.08f, direction));
            Lights.Flash(flashAt + flashDirection * 0.1f, FlashColor, FlashIntensity * (0.85f + 0.3f * (float)random.NextDouble()), FlashRange, FlashSeconds);
            Flashes++;

            // The tracer: from a little way out to the hit (or far into the sky)
            float distance = shot.Hit ? Vector3.Distance(shot.From, shot.Point) : 150.0f;
            if (tracers != null && TracerEvery > 0 && shot.Index % TracerEvery == 0 && distance > TracerStart + 1.0f)
            {
                Vector3 start = shot.From + shot.Direction * TracerStart;
                float flight = distance - TracerStart;
                float speed = MathF.Min(TracerSpeed, flight / TracerMinSeconds);
                tracers.Emit(1, new ParticleBurst(start, shot.Direction, speed, flight / speed));
                TracersShown++;
            }

            if (shot.Hit)
                Impact(shot);
        }

        void Impact(in ShotResult shot)
        {
            Surface surface = SurfaceOf(shot);
            LastSurface = surface;
            Impacts++;
            Vector3 point = shot.Point, normal = shot.Normal;
            // The spray goes out of the surface, somewhat back along the shot's way (a glancing shot throws forward)
            Vector3 bounce = Vector3.Normalize(Vector3.Reflect(shot.Direction, normal) + normal);
            float angle = (float)random.NextDouble() * 360.0f;
            switch (surface)
            {
                case Surface.Concrete:
                    dust?.Emit(5, new ParticleBurst(point + normal * 0.03f, normal));
                    chips?.Emit(6, new ParticleBurst(point + normal * 0.02f, bounce));
                    Decals.Spawn(ConcreteHole, point, normal, ConcreteHoleSize, 0.1f, angle, Vector4.One);
                    PlayImpact(ConcreteSound, ConcreteVariants, point);
                    break;
                case Surface.Metal:
                    sparks?.Emit(10, new ParticleBurst(point + normal * 0.01f, bounce));
                    dust?.Emit(1, new ParticleBurst(point + normal * 0.03f, normal));
                    Lights.Flash(point + normal * 0.08f, new Vector3(1.0f, 0.6f, 0.3f), 1.2f, 1.5f, 0.05f);
                    Decals.Spawn(MetalHole, point, normal, MetalHoleSize, 0.1f, angle, Vector4.One);
                    PlayImpact(MetalSound, MetalVariants, point);
                    break;
                case Surface.Dirt:
                    dirt?.Emit(5, new ParticleBurst(point + normal * 0.03f, normal));
                    clods?.Emit(6, new ParticleBurst(point + normal * 0.02f, Vector3.Normalize(normal * 2.0f + bounce)));
                    Decals.Spawn("", point, normal, DirtMarkSize, 0.1f, angle, new Vector4(0.35f, 0.3f, 0.24f, 0.7f));
                    PlayImpact(DirtSound, DirtVariants, point);
                    break;
                case Surface.Flesh:
                    // A small dark-red puff, out of the body toward the shooter and a little through
                    blood?.Emit(4, new ParticleBurst(point, normal));
                    blood?.Emit(2, new ParticleBurst(point, shot.Direction));
                    PlayImpact(FleshSound, FleshVariants, point);
                    break;
            }
        }

        void PlayImpact(string sound, int variants, Vector3 point)
        {
            if (variants > 0 && !string.IsNullOrEmpty(sound))
                Audio.PlayOneShot(string.Format(sound, random.Next(1, variants + 1)), point, ImpactVolume, AudioBus.Effects,
                    0.94f + 0.12f * (float)random.NextDouble());
        }
    }
}
