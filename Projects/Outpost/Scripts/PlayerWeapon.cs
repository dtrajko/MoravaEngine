using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>Where a shot aims from, each fixed step: the view's place (the camera's: first person the eyes), the eyes, the muzzle.</summary>
    public struct AimContext
    {
        public Vector3 Camera, Eyes, Muzzle;
        public float Speed;          // horizontal, meters per second
        public bool Grounded, Crouched, Sprinting;
    }

    /// <summary>One shot (or a knife's strike): where it went and what it hit (Hit false: nothing in range), and the kick it gives the look.</summary>
    public struct ShotResult
    {
        public Vector3 From, Direction, Point, Normal;
        public bool Hit;
        public Entity? Entity, Target;
        public string Zone;
        public float Damage;
        public bool Dealt;             // the target took the damage (an IDamageable)
        public bool Behind;            // a knife's strike from behind the target (a takedown)
        public float KickUp, KickLeft; // degrees
        public int Index;              // the shot's number (its random numbers' seed)
    }

    /// <summary>
    /// A weapon in a player's hands (docs/games/Shooter_Plan.html, Phases 2.2 and 2.4): the game's rules, run in the fixed
    /// step from the commands. A gun: the rate of fire and the fire modes, the magazine and the reserve, reloading, the
    /// spread (still, moving, crouched, in the air, growing with each shot, narrower aiming down the sights) and the kick,
    /// and the hit test of the plan (both views alike): a ray from the view through the crosshair finds the aim point, a
    /// ray from the muzzle to it what is really hit (a corner the camera sees past), and one from the eyes to the muzzle
    /// that the gun isn't through a wall. Each shot's random numbers (the spread, the kick's side) come from its number and
    /// the owner's, so a server and a client would agree. A knife (Melee): a strike on Fire, its blade arriving HitAt
    /// seconds in, a sphere swept from the eyes along the look finds what it reaches; from behind the target a takedown.
    /// What is seen and heard (sounds, the arms' clips, the HUD) reacts to its events and state.
    ///
    /// Phase 2.5, the sniper rifle: a bolt worked after each shot (meanwhile the scope is down, no reload starts), and the
    /// scope: Aim pressed steps through its zooms and back to none, the wheel zooms between, a reload, a sprint or putting
    /// it away takes it down. Scoped the spread is the scope's (tiny standing, wide moving or in the air) and the look
    /// sways with breathing, a slow figure of eight; held breath (Sprint or Walk held) steadies it for a few seconds, then
    /// it sways more until the breath is back. The sway is part of the rules: the shot goes where the swaying view looks.
    /// </summary>
    public sealed class PlayerWeapon
    {
        readonly WeaponData data;
        readonly Entity owner;
        float cooldown;        // seconds until the next shot (or strike) may go
        float reloadLeft;      // seconds of the reload left (0: not reloading)
        float spreadExtra;     // degrees the shots added, recovering once the shots pause
        float sinceShot = 10.0f;
        float strikeLeft;      // seconds of the knife's strike left (0: not striking)
        float hitIn;           // seconds until the strike's blade arrives (negative: it has)
        int fireMode;
        int shots;
        float boltIn, boltLeft;  // seconds until the bolt is worked after a shot, and of working it left
        float zoom;              // the scope's magnification wanted (0: no scope)
        float scopedSeconds;     // how long it has been scoped (the sway's time)
        float breath = 1.0f;     // the breath to hold (1 full, 0 out of it)
        bool tired;              // held until out of breath: the sway is larger until the breath is back
        float swayScale = 1.0f;  // the sway now (eased: steadied, tired, normal)

        public PlayerWeapon(WeaponData data, Entity owner)
        {
            this.data = data;
            this.owner = owner;
            Ammo = data.Kind == WeaponKind.Gun ? data.Magazine : 0;
            Reserve = data.Kind == WeaponKind.Gun ? data.Reserve : 0;
        }

        public WeaponData Data => data;
        public bool IsMelee => data.Kind == WeaponKind.Melee;
        public int Ammo { get; private set; }
        public int Reserve { get; private set; }
        public string FireMode => data.FireModes[fireMode];
        public bool IsReloading => reloadLeft > 0.0f;

        /// <summary>The knife is striking (from the Fire until the strike's end).</summary>
        public bool IsAttacking => strikeLeft > 0.0f;

        /// <summary>Aiming down the sights (Aim held with a gun that can, not reloading or sprinting).</summary>
        public bool IsAiming { get; private set; }

        /// <summary>0 to 1 through the reload (0 when not reloading).</summary>
        public float ReloadProgress => IsReloading ? 1.0f - reloadLeft / MathF.Max(0.01f, data.ReloadSeconds) : 0.0f;

        /// <summary>The spread now (degrees, the cone's half angle), from the last step: the HUD's crosshair shows it.</summary>
        public float Spread { get; private set; }

        /// <summary>The bolt is being worked after a shot, or is about to be (no scope, no reload meanwhile).</summary>
        public bool IsWorkingBolt => boltIn > 0.0f || boltLeft > 0.0f;

        /// <summary>The scope's magnification wanted (0: none): the view zooms to it while IsAiming.</summary>
        public float Zoom => zoom;

        /// <summary>Holding breath to steady the scope, and how much breath is left (0 to 1).</summary>
        public bool HoldingBreath { get; private set; }
        public float Breath => breath;

        /// <summary>The sway's size now: 1 normal, less steadied, more tired (eased between).</summary>
        public float SwayScale => swayScale;

        /// <summary>
        /// The sway's turn of the look now (degrees: yaw, pitch), ahead seconds after the last step (the frames between two
        /// steps): zero unless scoped.
        /// </summary>
        public Vector2 SwayAt(float ahead)
        {
            if (!data.HasScope || !IsAiming)
                return Vector2.Zero;
            float t = (scopedSeconds + ahead) * 2.0f * MathF.PI / MathF.Max(0.5f, data.SwaySeconds);
            // A slow figure of eight (side to side once, up and down twice) with a little of a faster drift in it
            float yaw = MathF.Sin(t) + 0.3f * MathF.Sin(2.7f * t + 1.1f);
            float pitch = 0.8f * MathF.Sin(2.0f * t + 0.6f) + 0.25f * MathF.Sin(3.3f * t);
            // (it starts from the middle: eased in over the first second scoped)
            float ease = MathF.Min(1.0f, (scopedSeconds + ahead) / 1.0f);
            return new Vector2(yaw, pitch) * data.SwayDegrees * swayScale * ease;
        }

        public event Action<ShotResult>? Fired;
        public event Action? DryFired, ReloadStarted, ReloadFinished;
        public event Action<string>? FireModeChanged;

        /// <summary>The bolt's working started (BoltAfter seconds after a shot), and ended (the scope may come back up).</summary>
        public event Action? BoltStarted, BoltFinished;

        /// <summary>The scope's zoom wanted changed (Aim stepped it; 0: down by Aim). The wheel's smooth zoom doesn't count.</summary>
        public event Action<float>? ZoomStepped;

        /// <summary>The knife: a strike started; its blade arrived (what it hit, Hit false: nothing in reach); the strike ended.</summary>
        public event Action? Swung, AttackEnded;
        public event Action<ShotResult>? Struck;

        /// <summary>Put away (another weapon comes out): a reload or a strike stops, the sights come down.</summary>
        public void Holster()
        {
            reloadLeft = 0.0f;
            strikeLeft = 0.0f;
            IsAiming = false;
            zoom = 0.0f;
            scopedSeconds = 0.0f;
            HoldingBreath = false;
            // A bolt being worked is finished on the way down (the round is in when it comes out again)
            if (IsWorkingBolt)
            {
                boltIn = boltLeft = 0.0f;
                BoltFinished?.Invoke();
            }
        }

        public void Step(in PlayerCommand command, in AimContext aim, float dt)
        {
            cooldown = MathF.Max(0.0f, cooldown - dt);
            if (IsMelee)
            {
                StepMelee(command, aim, dt);
                return;
            }
            sinceShot += dt;
            if (sinceShot >= data.SpreadRecoveryDelay)
                spreadExtra = MathF.Max(0.0f, spreadExtra - data.SpreadRecovery * dt);
            StepBolt(dt);
            if (data.HasScope)
            {
                StepScope(command, aim, dt);
                Spread = IsAiming ? ScopeSpread(aim) : BaseSpread(aim) + spreadExtra;
            }
            else
            {
                IsAiming = command.Aim && data.CanAimDownSights && !IsReloading && !aim.Sprinting;
                Spread = (BaseSpread(aim) + spreadExtra) * (IsAiming ? data.AimSpreadScale : 1.0f);
            }

            if (command.FireMode && data.FireModes.Length > 1)
            {
                fireMode = (fireMode + 1) % data.FireModes.Length;
                FireModeChanged?.Invoke(FireMode);
            }

            if (IsReloading)
            {
                reloadLeft -= dt;
                if (reloadLeft <= 0.0f)
                {
                    reloadLeft = 0.0f;
                    int taken = Math.Min(data.Magazine - Ammo, Reserve);
                    Ammo += taken;
                    Reserve -= taken;
                    ReloadFinished?.Invoke();
                }
                return; // no shots while reloading
            }
            if (command.Reload && Ammo < data.Magazine && Reserve > 0 && !IsWorkingBolt)
            {
                StartReload();
                return;
            }

            // Sprinting the weapon is lowered: no shots
            bool wants = FireMode == "Auto" ? command.Fire : command.FirePressed;
            if (!wants || aim.Sprinting || cooldown > 0.0f)
                return;
            if (Ammo <= 0)
            {
                if (command.FirePressed)
                {
                    DryFired?.Invoke();
                    cooldown = 0.25f;
                    if (Reserve > 0)
                        StartReload(); // an empty magazine: the click, then the reload
                }
                return;
            }
            Ammo--;
            cooldown += data.ShotInterval;
            sinceShot = 0.0f;
            Fired?.Invoke(Shoot(command, aim));
            spreadExtra = MathF.Min(data.SpreadMax, spreadExtra + data.SpreadPerShot);
            // A bolt action: worked after the shot (not after the last round: the reload opens it anyway)
            if (data.HasBolt && Ammo > 0)
                boltIn = MathF.Max(0.001f, data.BoltAfter);
        }

        void StepBolt(float dt)
        {
            if (boltIn > 0.0f)
            {
                boltIn -= dt;
                if (boltIn <= 0.0f)
                {
                    boltIn = 0.0f;
                    boltLeft = MathF.Max(0.001f, data.BoltSeconds);
                    BoltStarted?.Invoke();
                }
            }
            else if (boltLeft > 0.0f)
            {
                boltLeft -= dt;
                if (boltLeft <= 0.0f)
                {
                    boltLeft = 0.0f;
                    BoltFinished?.Invoke();
                }
            }
        }

        /// <summary>Seconds into the bolt's working (negative: not started yet; meaningless when not working it).</summary>
        public float BoltSeconds => boltLeft > 0.0f ? data.BoltSeconds - boltLeft : -boltIn;

        // The scope: Aim pressed steps to the next zoom (from a zoom the wheel set: the next one above it), past the last
        // one down; the wheel zooms smoothly; a reload or a sprint takes it down. Scoped (not during the bolt) it sways
        void StepScope(in PlayerCommand command, in AimContext aim, float dt)
        {
            if (command.AimPressed && !IsReloading && !aim.Sprinting)
            {
                float next = 0.0f;
                foreach (float level in data.ScopeZooms)
                    if (level > zoom + 0.01f)
                    {
                        next = level;
                        break;
                    }
                zoom = next;
                ZoomStepped?.Invoke(zoom);
            }
            if (zoom > 0.0f && command.Zoom != 0.0f)
                zoom = Math.Clamp(zoom * MathF.Pow(data.ScopeWheelStep, command.Zoom), data.ScopeMinZoom, data.ScopeMaxZoom);
            if (IsReloading || aim.Sprinting)
                zoom = 0.0f;
            IsAiming = zoom > 0.0f && !IsReloading && !aim.Sprinting && !IsWorkingBolt;

            // Breath: held (Sprint or Walk) while scoped and not out of it, used up in SteadySeconds; otherwise back in
            // RecoverSeconds; once out of it, tired until most of it is back
            HoldingBreath = IsAiming && (command.Sprint || command.Walk) && !tired && breath > 0.0f;
            if (HoldingBreath)
            {
                breath = MathF.Max(0.0f, breath - dt / MathF.Max(0.1f, data.SteadySeconds));
                if (breath <= 0.0f)
                    tired = true;
            }
            else
            {
                breath = MathF.Min(1.0f, breath + dt / MathF.Max(0.1f, data.SteadyRecoverSeconds));
                if (tired && breath >= 0.6f)
                    tired = false;
            }
            float wanted = HoldingBreath ? data.SteadyScale : tired ? data.TiredScale : 1.0f;
            swayScale += (wanted - swayScale) * MathF.Min(1.0f, 3.0f * dt);
            scopedSeconds = IsAiming ? scopedSeconds + dt : 0.0f;
        }

        // Scoped: the scope's own spread, tiny standing still, wide moving or in the air (the scope's view is no help then)
        float ScopeSpread(in AimContext aim)
        {
            if (!aim.Grounded)
                return data.ScopeAir;
            float moving = Math.Clamp(aim.Speed / MathF.Max(0.1f, data.ScopeMovingSpeed), 0.0f, 1.0f);
            float spread = data.ScopeStill + (data.ScopeMoving - data.ScopeStill) * moving;
            return aim.Crouched ? spread * data.SpreadCrouched : spread;
        }

        void StepMelee(in PlayerCommand command, in AimContext aim, float dt)
        {
            IsAiming = false;
            Spread = 0.0f;
            if (IsAttacking)
            {
                strikeLeft -= dt;
                if (hitIn > 0.0f)
                {
                    hitIn -= dt;
                    if (hitIn <= 0.0f)
                        Struck?.Invoke(Strike(command, aim));
                }
                if (strikeLeft <= 0.0f)
                {
                    strikeLeft = 0.0f;
                    AttackEnded?.Invoke();
                }
                return;
            }
            if (command.FirePressed && !aim.Sprinting && cooldown <= 0.0f)
            {
                strikeLeft = data.MeleeSeconds;
                hitIn = data.MeleeHitAt;
                cooldown = data.MeleeSeconds;
                Swung?.Invoke();
            }
        }

        // The knife's blade arrives: a sphere swept from the eyes along the look as far as it reaches. A target struck from
        // behind (the striker within BehindAngle of its back, seen from above) takes the takedown's damage
        ShotResult Strike(in PlayerCommand command, in AimContext aim)
        {
            Vector3 look = LookDirection(command.Yaw, command.Pitch);
            ShotResult strike = new ShotResult { From = aim.Eyes, Direction = look, Zone = "", Index = shots++ };
            if (!Physics.SphereCast(aim.Eyes, data.MeleeRadius, look, data.MeleeRange, out RaycastHit hit, PhysicsLayers.Solid, owner))
            {
                strike.Point = aim.Eyes + look * data.MeleeRange;
                return strike;
            }
            strike.Hit = true;
            strike.Point = hit.Point;
            strike.Normal = hit.Normal;
            strike.Entity = hit.Entity;
            Entity target = Hitbox.Resolve(hit.Entity, out string zone, out float _);
            strike.Target = target;
            strike.Zone = zone;
            strike.Behind = IsBehind(target, owner.WorldPosition, data.MeleeBehindAngle);
            strike.Damage = strike.Behind ? data.MeleeBehindDamage : data.MeleeDamage;
            strike.Dealt = Damage.Apply(target, new DamageInfo
            {
                Amount = strike.Damage, Source = owner, Point = hit.Point, Direction = look, Kind = strike.Behind ? "Takedown" : "Knife", Zone = zone,
            });
            return strike;
        }

        /// <summary>
        /// The striker at from is behind the target: within angle degrees of the target's back (its model faces +Z), seen
        /// from above.
        /// </summary>
        public static bool IsBehind(Entity target, Vector3 from, float angle)
        {
            Matrix4x4 m = target.WorldTransform;
            Vector2 facing = new Vector2(m.M31, m.M33);
            Vector2 toStriker = new Vector2(from.X - m.M41, from.Z - m.M43);
            if (facing.LengthSquared() < 1e-6f || toStriker.LengthSquared() < 1e-6f)
                return false;
            float cosine = Vector2.Dot(Vector2.Normalize(facing), Vector2.Normalize(toStriker));
            return cosine <= -MathF.Cos(angle * MathF.PI / 180.0f);
        }

        void StartReload()
        {
            reloadLeft = data.ReloadSeconds;
            IsAiming = false;
            zoom = 0.0f;
            ReloadStarted?.Invoke();
        }

        float BaseSpread(in AimContext aim)
        {
            if (!aim.Grounded)
                return data.SpreadAir;
            float moving = Math.Clamp(aim.Speed / MathF.Max(0.1f, data.SpreadMovingSpeed), 0.0f, 1.0f);
            float spread = data.SpreadStill + (data.SpreadMoving - data.SpreadStill) * moving;
            return aim.Crouched ? spread * data.SpreadCrouched : spread;
        }

        /// <summary>The look's direction (yaw 0 along -Z, positive left; pitch positive down).</summary>
        public static Vector3 LookDirection(float yaw, float pitch)
        {
            float y = yaw * MathF.PI / 180.0f, p = pitch * MathF.PI / 180.0f;
            return new Vector3(-MathF.Sin(y) * MathF.Cos(p), -MathF.Sin(p), -MathF.Cos(y) * MathF.Cos(p));
        }

        ShotResult Shoot(in PlayerCommand command, in AimContext aim)
        {
            int index = shots++;
            Random random = new Random(Seed(owner.Id, index));
            // (scoped, where the swaying view looks: the reticle's middle)
            Vector2 sway = SwayAt(0.0f);
            Vector3 look = LookDirection(command.Yaw + sway.X, command.Pitch + sway.Y);
            Vector3 direction = InCone(look, Spread, random);

            ShotResult shot = new ShotResult { Index = index, Direction = direction, Zone = "" };
            shot.KickUp = data.RecoilPitch;
            shot.KickLeft = data.RecoilYaw * (float)(random.NextDouble() * 2.0 - 1.0);

            // 1. From the view through the crosshair (spread included): the aim point. The ray starts level with the eyes,
            //    so in third person nothing between the camera and the player is hit
            float along = MathF.Max(0.0f, Vector3.Dot(aim.Eyes - aim.Camera, direction));
            Vector3 start = aim.Camera + direction * along;
            Vector3 aimPoint = start + direction * data.Range;
            if (Physics.Raycast(start, direction, data.Range, out RaycastHit viewHit, PhysicsLayers.Solid, owner))
                aimPoint = viewHit.Point;

            // 2. The gun isn't through a wall: the eyes to the muzzle; then 3. the muzzle to the aim point: what is hit
            Vector3 from = aim.Muzzle;
            Vector3 toMuzzle = aim.Muzzle - aim.Eyes;
            if (toMuzzle.Length() > 0.01f && Physics.Raycast(aim.Eyes, Vector3.Normalize(toMuzzle), toMuzzle.Length(), out RaycastHit blocked, PhysicsLayers.Solid, owner))
            {
                shot.From = aim.Eyes;
                return Hit(shot, blocked, Vector3.Normalize(toMuzzle));
            }
            Vector3 toAim = aimPoint - from;
            float distance = toAim.Length();
            shot.From = from;
            if (distance < 0.01f)
            {
                shot.Point = aimPoint;
                return shot;
            }
            Vector3 way = toAim / distance;
            shot.Direction = way;
            if (Physics.Raycast(from, way, distance + 0.05f, out RaycastHit hit, PhysicsLayers.Solid, owner))
                return Hit(shot, hit, way);
            shot.Point = aimPoint;
            return shot;
        }

        ShotResult Hit(ShotResult shot, in RaycastHit hit, Vector3 way)
        {
            shot.Hit = true;
            shot.Direction = way;
            shot.Point = hit.Point;
            shot.Normal = hit.Normal;
            shot.Entity = hit.Entity;
            Entity target = Hitbox.Resolve(hit.Entity, out string zone, out float scale);
            shot.Target = target;
            shot.Zone = zone;
            shot.Damage = data.Damage * (zone == "Head" ? data.HeadMultiplier : scale);
            shot.Dealt = Damage.Apply(target, new DamageInfo
            {
                Amount = shot.Damage, Source = owner, Point = hit.Point, Direction = way, Kind = "Bullet", Zone = zone,
            });
            return shot;
        }

        /// <summary>A shot's seed from its owner and number, the same everywhere (HashCode is randomized per process)</summary>
        static int Seed(ulong id, int index)
        {
            unchecked
            {
                ulong h = id * 0x9E3779B97F4A7C15UL ^ (ulong)index * 0xC2B2AE3D27D4EB4FUL;
                h ^= h >> 31;
                return (int)(h ^ (h >> 32)) & 0x7FFFFFFF;
            }
        }

        /// <summary>A direction within a cone of this half angle (degrees) round axis, spread evenly over its area.</summary>
        static Vector3 InCone(Vector3 axis, float degrees, Random random)
        {
            if (degrees <= 0.0f)
                return axis;
            float angle = degrees * MathF.PI / 180.0f * MathF.Sqrt((float)random.NextDouble());
            float around = 2.0f * MathF.PI * (float)random.NextDouble();
            Vector3 side = Vector3.Normalize(Vector3.Cross(axis, MathF.Abs(axis.Y) < 0.99f ? Vector3.UnitY : Vector3.UnitX));
            Vector3 up = Vector3.Cross(side, axis);
            Vector3 offset = (side * MathF.Cos(around) + up * MathF.Sin(around)) * MathF.Tan(angle);
            return Vector3.Normalize(axis + offset);
        }
    }
}
