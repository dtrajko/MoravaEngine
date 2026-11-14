using System;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The race car's arcade handling (docs/games/Racing_Plan.html, Phase 3): it reads the driver's input and drives the
    /// car's Vehicle (set to Controls: Script) with assists that make it fast, fun and forgiving:
    /// - Steering: keys turn the wheel gradually and let it come back faster. Full input turns the front wheels only as far
    ///   as the tires can hold at this speed: the angle of the tightest turn their grip allows (SteerGrip, more with the
    ///   downforce) plus the slip at which a tire grips best (SteerSlip). Slow, that's the full lock; fast, a few degrees,
    ///   so the car turns as hard as it can without skidding. If a tire still slides past SlipLimit (a bump, a crest), the
    ///   steering eases until it grips again.
    /// - Downforce: pressed down harder the faster it goes (the square of its speed), along its own down: it holds the road
    ///   in fast turns, on banked turns and over crests.
    /// - Grip: in ordinary driving the tires grip DrivingGrip times the Inspector's (Front Grip, Rear Grip), so the car
    ///   doesn't slide under throttle and braking; braking, the rear grips more still (BrakingRearGrip); the handbrake's drift
    ///   keeps the Inspector's grip (DriftRearGrip at the rear).
    /// - Drift (Handbrake at speed): the rear tires lose grip for as long as it is held and get it back gradually after; the
    ///   throttle stays on. Slow, the handbrake is a handbrake.
    /// - ABS: braking hard, the brake eases while a wheel locks, so it stops sooner and still steers.
    /// - In the air: the throttle and brake pitch it, the steering turns it, and it rights itself gently, so it lands on its
    ///   wheels.
    /// - Walls: touching one (its tag: WallTag), it scrapes along it instead of bouncing off and spinning.
    /// - The gearbox: automatic, or manual when the player chose it (Settings: Gearbox; GameOptions.ManualGearbox): ShiftUp
    ///   and ShiftDown shift (the clutch works by itself), and the arrow keys shift instead of steering (SteerArrows: the
    ///   arrows' steering, read only with the automatic). The HUD's paddles show each press (UpPresses, DownPresses). At
    ///   the start and after each respawn it is in first gear.
    /// - The engine's power: the car's own (Buffed), or a GT40 Mk II's (Realistic; Settings: Engine power;
    ///   GameOptions.EngineTorque). The test driver always has the car's own.
    /// Each assist has its fields to tune in the Inspector. With TestDriver on, the test driver drives (TestDriver.cs: it
    /// follows the track's center line) through the same assists, for checks without a player.
    /// The race (RaceManager.cs, Phase 4) holds it at the start (Held), puts it back on the track (Respawn) and asks
    /// whether it touched the terrain (TouchedTerrain): through Current, the car of the running game.
    /// </summary>
    public class CarController : Entity
    {
        // ---- Steering
        /// <summary>How fast the keys turn the wheel (full lock per second).</summary>
        public float SteerSpeed = 5.0f;
        /// <summary>How fast the wheel comes back when the keys let go (full lock per second).</summary>
        public float SteerReturnSpeed = 8.0f;
        /// <summary>The Vehicle's Max Steer Angle (Inspector), degrees: full input's angle when slow.</summary>
        public float MaxSteerAngle = 30.0f;
        /// <summary>Meters between the front and the rear axle (the Vehicle's wheels).</summary>
        public float Wheelbase = 3.04f;
        /// <summary>The sideways g the steering aims for at full input, without the downforce (which adds to it as it presses
        /// the tires down). Higher: more angle at speed (and skids sooner); lower: wider turns.</summary>
        public float SteerGrip = 3.0f;
        /// <summary>Degrees the front tires may slip on top of the turn's angle: where they grip best (the tire curve's peak, 3).</summary>
        public float SteerSlip = 3.0f;
        /// <summary>Degrees of sideways slide (any wheel) above which the steering eases, until the tires grip again. 0: off.</summary>
        public float SlipLimit = 6.0f;

        // ---- Downforce
        /// <summary>The downforce at DownforceSpeed, in g (1: as much again as the car's weight).</summary>
        public float Downforce = 1.8f;
        /// <summary>km/h of the downforce above (it grows with the square of the speed).</summary>
        public float DownforceSpeed = 300.0f;

        // ---- Grip
        /// <summary>The tires' grip in ordinary driving, times the Inspector's (1: as set there). The drift doesn't use it.</summary>
        public float DrivingGrip = 1.65f;
        /// <summary>Braking: the rear tires' sideways grip rises by this share at full brake (the weight moves to the front and
        /// the rear would slide out), so the car brakes straight and turns in under braking without skidding. 0: off.</summary>
        public float BrakingRearGrip = 0.5f;

        // ---- Drift
        /// <summary>km/h above which the handbrake drifts (below: it is a handbrake).</summary>
        public float DriftMinSpeed = 60.0f;
        /// <summary>The rear tires' grip while drifting, times the Inspector's (the front's is the Inspector's then).</summary>
        public float DriftRearGrip = 0.5f;
        /// <summary>Seconds the rear tires take to get their grip back after a drift.</summary>
        public float DriftRecovery = 0.6f;

        // ---- Brakes
        /// <summary>
        /// ABS: when a wheel starts to lock (its slip along it above this: 0 rolling, 1 locked), the brake eases off, so it
        /// stops sooner and still steers. 0: off.
        /// </summary>
        public float AbsSlip = 0.15f;

        // ---- Walls
        /// <summary>The tag of the walls (the track's Walls entity has it): touching one, the car scrapes along it.</summary>
        public string WallTag = "Wall";
        /// <summary>The share of its speed the car keeps per second while it scrapes a wall.</summary>
        public float WallSpeedKept = 0.85f;
        /// <summary>Degrees per second it may still turn while it scrapes a wall (its spin from the hit beyond this goes).</summary>
        public float WallMaxTurn = 60.0f;
        /// <summary>
        /// The tag of the terrain: on it, the car has fallen off the track (the race puts it back). Found by a short ray down
        /// (standing on its wheels the car's body doesn't touch the ground: the wheels are casts, not bodies) or by its
        /// body touching it.
        /// </summary>
        public string TerrainTag = "Terrain";

        // ---- In the air
        /// <summary>Degrees per second squared the throttle and brake pitch it in the air.</summary>
        public float AirPitch = 120.0f;
        /// <summary>Degrees per second squared the steering turns it in the air.</summary>
        public float AirYaw = 90.0f;
        /// <summary>How strongly it rights itself in the air (0: not at all).</summary>
        public float AirLevel = 2.0f;

        // ---- The test driver
        /// <summary>The test driver drives instead of the player (TestDriver.cs), along the track's center line.</summary>
        public bool TestDriver = false;
        /// <summary>The center line the test driver follows: a file of the project (Tools/TrackBuilder writes it).</summary>
        public string Centerline = "Assets/Tracks/Circuit.centerline.json";
        /// <summary>The track entity the center line's points are relative to.</summary>
        public Entity? Track;
        /// <summary>m/s^2 the test driver allows itself sideways in turns (higher: braver).</summary>
        public float TestDriverCornering = 14.0f;

        /// <summary>The car of the running game (the race's: one car).</summary>
        public static CarController? Current { get; private set; }

        /// <summary>The player shifts (the manual gearbox): for the HUD's paddles.</summary>
        public bool ManualGearbox => manual;

        /// <summary>The shift paddles' presses so far, each way, and whether the last one was refused (top gear, first, too fast).</summary>
        public int UpPresses { get; private set; }
        public int DownPresses { get; private set; }
        public bool LastUpRefused { get; private set; }
        public bool LastDownRefused { get; private set; }

        /// <summary>The car scrapes a wall now (the wall assist holds: for its sound).</summary>
        public bool TouchingWall => wallTime > 0.0f;

        /// <summary>Held at the start: brakes on, the throttle ignored (the race, until the lights go out).</summary>
        public bool Held { get; set; }

        /// <summary>The test driver drives it (TestDriver on, and the center line found).</summary>
        public bool TestDriving => driver != null;
        /// <summary>The test driver is stuck (TestDriver.Stuck): the race respawns the car.</summary>
        public bool TestDriverStuck => driver != null && driver.Stuck;
        /// <summary>The engine has the Realistic power now (the player's choice; the test driver always has the car's own).</summary>
        public bool RealisticPower => driver == null && GameOptions.RealisticPower;

        /// <summary>It touched the terrain since the last time this was read (and cleared): it has fallen off the track.</summary>
        public bool TouchedTerrain { get; set; }

        Vehicle? vehicle;
        Rigidbody? body;
        TestDriver? driver;
        float steer;          // the wheel, -1 left to 1 right, before the speed's share
        float rearGrip = 1.0f;
        float absShare = 1.0f;   // the share of the brake ABS lets through
        float slipShare = 1.0f;  // the share of the steering the slide lets through (SlipLimit)
        float wallTime;          // seconds the wall assist still holds after the last touch
        Vector3 wallNormal;      // towards the wall, level
        bool manual;             // the vehicle's gearbox is manual (as GameOptions says, the test driver aside)
        bool toFirstGear = true; // put the manual gearbox in first at the next step (the start, a respawn)
        float torque = 1.0f;     // the engine's torque times the car's (as GameOptions says, the test driver aside)

        protected override void OnCreate()
        {
            vehicle = GetComponent<Vehicle>();
            body = GetComponent<Rigidbody>();
            if (vehicle == null || body == null)
            {
                Log.Error($"CarController on '{Name}': the entity needs a Vehicle and a Rigid Body");
                return;
            }
            vehicle.Controls = VehicleControls.Script;
            Current = this;
            if (TestDriver)
            {
                driver = Racing.TestDriver.Load(Centerline, Track != null ? Track.WorldPosition : Vector3.Zero, TestDriverCornering);
                if (driver == null)
                    Log.Error($"CarController: the test driver has no center line ('{Centerline}' not found)");
            }
        }

        /// <summary>
        /// Puts the car here, standing still, its steering straight and its tires gripping (a respawn, the start); its
        /// body follows at the next physics step.
        /// </summary>
        public void Respawn(Vector3 position, Quaternion rotation)
        {
            Translation = position;
            Rotation = rotation;
            if (body != null)
            {
                body.LinearVelocity = Vector3.Zero;
                body.AngularVelocity = Vector3.Zero;
            }
            steer = 0.0f;
            rearGrip = MathF.Max(DrivingGrip, 0.0f);
            absShare = 1.0f;
            wallTime = 0.0f;
            toFirstGear = true;
            TouchedTerrain = false;
            driver?.OnRespawn();
        }

        protected override void OnCollisionEnter(Collision collision)
        {
            if (!string.IsNullOrEmpty(TerrainTag) && collision.Other.HasTag(TerrainTag))
                TouchedTerrain = true;
            if (collision.Other.HasTag(WallTag))
            {
                Vector3 normal = collision.Normal;
                normal.Y = 0.0f;
                if (normal.LengthSquared() > 1e-4f)
                {
                    wallNormal = Vector3.Normalize(normal);
                    wallTime = 0.3f;
                }
            }
        }

        // The gearbox as the player chose it (also changed in the pause menu's Settings), and the paddles' presses
        // The share of the wheel's full lock that full input turns at this speed: the turn the tires can hold (a car turning
        // on a circle of radius R pulls v² / R sideways; its front wheels point Wheelbase / R off its way), plus the slip at
        // which they grip best. The downforce presses them down as the square of the speed, so they hold more. Then less
        // while a tire slides past SlipLimit (not in a drift, nor while the rear gets its grip back after one)
        float SteerShare(float speed, bool drift, float dt)
        {
            float lockAngle = MathF.Max(MaxSteerAngle, 1.0f);
            float v = MathF.Abs(speed);
            float angle = lockAngle;
            if (v > 1.0f && SteerGrip > 0.0f)
            {
                float ratio = v / MathF.Max(DownforceSpeed / 3.6f, 1.0f);
                float sideways = SteerGrip * 9.81f * (1.0f + MathF.Max(Downforce, 0.0f) * ratio * ratio); // m/s²
                float turn = MathF.Atan(sideways * MathF.Max(Wheelbase, 0.5f) / (v * v)) * 180.0f / MathF.PI;
                angle = MathF.Min(lockAngle, turn + MathF.Max(SteerSlip, 0.0f));
            }
            bool sliding = SlipLimit > 0.0f && !drift && rearGrip >= MathF.Max(DrivingGrip, 0.0f) && v > 10.0f &&
                vehicle!.LateralSlip > SlipLimit;
            slipShare = sliding ? MathF.Max(0.4f, slipShare - 3.0f * dt) : MathF.Min(1.0f, slipShare + 2.0f * dt);
            return angle / lockAngle * slipShare;
        }

        void UpdateGearbox()
        {
            float wantedTorque = driver == null ? GameOptions.EngineTorque : 1.0f;
            if (wantedTorque != torque)
            {
                vehicle!.SetEngineTorque(wantedTorque);
                torque = wantedTorque;
                Log.Info($"CarController: the engine's torque x{torque:0.00}");
            }
            bool wanted = GameOptions.ManualGearbox && driver == null;
            if (wanted != manual)
            {
                vehicle!.ManualGearbox = wanted;
                manual = wanted;
                Log.Info($"CarController: the gearbox is {(manual ? "manual" : "automatic")}");
            }
            if (!manual)
                return;
            // At the start and after a respawn, standing still: first gear, whatever it was in (the automatic waits in
            // neutral and takes first with the throttle by itself)
            if (toFirstGear)
            {
                vehicle!.SetGear(1);
                toFirstGear = false;
            }
            if (Input.WasPressed("ShiftUp"))
            {
                UpPresses++;
                LastUpRefused = !vehicle!.ShiftUp();
            }
            if (Input.WasPressed("ShiftDown"))
            {
                DownPresses++;
                LastDownRefused = !vehicle!.ShiftDown();
            }
        }

        protected override void OnFixedUpdate(float dt)
        {
            if (vehicle == null || body == null)
                return;
            float speed = vehicle.Speed;                 // m/s along the car
            float kmh = MathF.Abs(speed) * 3.6f;

            // On the terrain: a ray down a little past the wheels
            if (!string.IsNullOrEmpty(TerrainTag) && Physics.Raycast(WorldPosition, -Vector3.UnitY, 1.8f, out RaycastHit ground, PhysicsLayers.Static) &&
                ground.Entity.HasTag(TerrainTag))
                TouchedTerrain = true;

            // The driver: the player's actions, or the test driver's
            float throttle, brake, steerTarget;
            bool drift;
            if (driver != null && !Held) // held at the start: its clock starts with the race
            {
                TestDriver.Controls c = driver.Drive(WorldPosition, Forward, Right, speed, dt);
                (throttle, brake, steerTarget, drift) = (c.Throttle, c.Brake, c.Steer, c.Drift);
                steer = steerTarget; // it steers smoothly itself
            }
            else
            {
                throttle = Math.Clamp(Input.GetValue("Accelerate"), 0.0f, 1.0f);
                brake = Math.Clamp(Input.GetValue("Brake"), 0.0f, 1.0f);
                steerTarget = Math.Clamp(Input.GetValue("Steer"), -1.0f, 1.0f);
                drift = Input.IsHeld("Handbrake");
                if (!manual)
                    steerTarget = Math.Clamp(steerTarget + Input.GetValue("SteerArrows"), -1.0f, 1.0f);
                // The wheel turns towards the keys gradually, and comes back faster
                bool returning = MathF.Abs(steerTarget) < MathF.Abs(steer) || MathF.Sign(steerTarget) != MathF.Sign(steer);
                float rate = (returning ? SteerReturnSpeed : SteerSpeed) * dt;
                steer += Math.Clamp(steerTarget - steer, -rate, rate);
            }

            UpdateGearbox();

            float share = SteerShare(speed, drift, dt);

            // Drift: fast, the handbrake lowers the rear grip instead of locking the wheels; the grip comes back gradually
            // (from DriftRearGrip up to DrivingGrip); the front keeps the Inspector's grip while the handbrake is held
            bool drifting = drift && kmh >= DriftMinSpeed;
            float driving = MathF.Max(DrivingGrip, 0.0f);
            if (drifting)
                rearGrip = Math.Clamp(DriftRearGrip, 0.0f, 1.0f);
            else
                rearGrip = MathF.Min(driving, rearGrip + driving * dt / MathF.Max(DriftRecovery, 0.01f));
            // Braking forward (not drifting): more grip at the rear, as much more as the brake is pressed
            float braking = !drifting && throttle <= 0.0f && speed > 1.0f ? brake : 0.0f;
            vehicle.SetGrip(drift ? 1.0f : driving, rearGrip * (1.0f + MathF.Max(BrakingRearGrip, 0.0f) * braking));

            // ABS: the brake eases while a wheel locks, and comes back once it rolls
            if (AbsSlip > 0.0f && brake > 0.0f && kmh > 10.0f)
                absShare = vehicle.LongitudinalSlip > AbsSlip ? MathF.Max(0.2f, absShare - 6.0f * dt) : MathF.Min(1.0f, absShare + 6.0f * dt);
            else
                absShare = 1.0f;
            brake *= absShare;

            // Forward or back: brake is the reverse once it stops (the engine's own rule); the handbrake only when slow
            float forward = throttle > 0.0f ? throttle : -brake;
            float handbrake = drift && !drifting ? 1.0f : 0.0f;
            if (Held)
                vehicle.SetInput(0.0f, 0.0f, 1.0f, 1.0f); // the start: still, brakes on
            else
                vehicle.SetInput(forward, steer * share, throttle > 0.0f ? brake : 0.0f, handbrake);

            // Scraping a wall: no speed into it, its spin, roll and pitch from the hit taken away, a little speed lost
            if (wallTime > 0.0f)
            {
                wallTime -= dt;
                Vector3 velocity = body.LinearVelocity;
                float into = Vector3.Dot(velocity, wallNormal);
                if (into > 0.0f)
                    velocity -= wallNormal * into;
                velocity *= MathF.Pow(Math.Clamp(WallSpeedKept, 0.0f, 1.0f), dt);
                body.LinearVelocity = velocity;
                Vector3 angular = body.AngularVelocity;
                float maxTurn = WallMaxTurn * MathF.PI / 180.0f;
                body.AngularVelocity = new Vector3(angular.X * 0.2f, Math.Clamp(angular.Y, -maxTurn, maxTurn), angular.Z * 0.2f);
            }

            // Downforce: along the car's down, the square of its speed
            if (!vehicle.IsInAir)
            {
                float ratio = speed / MathF.Max(DownforceSpeed / 3.6f, 1.0f);
                body.AddForce(-Up * (Downforce * 9.81f * ratio * ratio), ForceMode.Acceleration);
            }
            else
            {
                // In the air: pitch with the throttle and brake, turn with the steering, and right itself gently
                const float toRadians = MathF.PI / 180.0f;
                Vector3 spin = Right * ((brake - throttle) * AirPitch * toRadians * dt) + Up * (-steerTarget * AirYaw * toRadians * dt);
                // Righting itself: a spring towards upright, damped so it doesn't swing past it (its tilting turn slows; a
                // turn around the vertical stays)
                Vector3 angular = body.AngularVelocity;
                Vector3 tilting = angular - Vector3.UnitY * Vector3.Dot(angular, Vector3.UnitY);
                Vector3 level = Vector3.Cross(Up, Vector3.UnitY) * (AirLevel * dt) - tilting * (2.0f * MathF.Sqrt(MathF.Max(AirLevel, 0.0f)) * dt);
                body.AngularVelocity = angular + spin + level;
            }
        }
    }
}
