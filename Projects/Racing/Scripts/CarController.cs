using System;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The race car's arcade handling (docs/games/Racing_Plan.html, Phase 3): it reads the driver's input and drives the
    /// car's Vehicle (set to Controls: Script) with assists that make it fast, fun and forgiving:
    /// - Steering: keys turn the wheel gradually and let it come back faster; the faster the car, the less the wheel turns
    ///   (at full speed a fraction of the slow-speed angle), so it doesn't spin out at 300 km/h.
    /// - Downforce: pressed down harder the faster it goes (the square of its speed), along its own down: it holds the road
    ///   in fast turns, on banked turns and over crests.
    /// - Drift (Handbrake at speed): the rear tires lose grip for as long as it is held and get it back gradually after; the
    ///   throttle stays on. Slow, the handbrake is a handbrake.
    /// - ABS: braking hard, the brake eases while a wheel locks, so it stops sooner and still steers.
    /// - In the air: the throttle and brake pitch it, the steering turns it, and it rights itself gently, so it lands on its
    ///   wheels.
    /// Each assist has its fields to tune in the Inspector. With TestDriver on, the test driver drives (TestDriver.cs: it
    /// follows the track's center line) through the same assists, for checks without a player.
    /// </summary>
    public class CarController : Entity
    {
        // ---- Steering
        /// <summary>How fast the keys turn the wheel (full lock per second).</summary>
        public float SteerSpeed = 5.0f;
        /// <summary>How fast the wheel comes back when the keys let go (full lock per second).</summary>
        public float SteerReturnSpeed = 8.0f;
        /// <summary>The share of the wheel's turn left at FullSpeedSteerAt and above (1: the same at any speed).</summary>
        public float FullSpeedSteer = 0.2f;
        /// <summary>km/h at which the wheel turns only FullSpeedSteer of its slow-speed angle.</summary>
        public float FullSpeedSteerAt = 250.0f;

        // ---- Downforce
        /// <summary>The downforce at DownforceSpeed, in g (1: as much again as the car's weight).</summary>
        public float Downforce = 1.5f;
        /// <summary>km/h of the downforce above (it grows with the square of the speed).</summary>
        public float DownforceSpeed = 300.0f;

        // ---- Drift
        /// <summary>km/h above which the handbrake drifts (below: it is a handbrake).</summary>
        public float DriftMinSpeed = 60.0f;
        /// <summary>The rear tires' grip while drifting (1: their usual).</summary>
        public float DriftRearGrip = 0.5f;
        /// <summary>Seconds the rear tires take to get their grip back after a drift.</summary>
        public float DriftRecovery = 0.6f;

        // ---- Brakes
        /// <summary>
        /// ABS: when a wheel starts to lock (its slip along it above this: 0 rolling, 1 locked), the brake eases off, so it
        /// stops sooner and still steers. 0: off.
        /// </summary>
        public float AbsSlip = 0.15f;

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
        public string Centerline = "Assets/Tracks/TestTrack.centerline.json";
        /// <summary>The track entity the center line's points are relative to.</summary>
        public Entity? Track;
        /// <summary>m/s^2 the test driver allows itself sideways in turns (higher: braver).</summary>
        public float TestDriverCornering = 14.0f;

        Vehicle? vehicle;
        Rigidbody? body;
        TestDriver? driver;
        float steer;          // the wheel, -1 left to 1 right, before the speed's share
        float rearGrip = 1.0f;
        float absShare = 1.0f;   // the share of the brake ABS lets through

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
            if (TestDriver)
            {
                driver = Racing.TestDriver.Load(Centerline, Track != null ? Track.WorldPosition : Vector3.Zero, TestDriverCornering);
                if (driver == null)
                    Log.Error($"CarController: the test driver has no center line ('{Centerline}' not found)");
            }
        }

        protected override void OnFixedUpdate(float dt)
        {
            if (vehicle == null || body == null)
                return;
            float speed = vehicle.Speed;                 // m/s along the car
            float kmh = MathF.Abs(speed) * 3.6f;

            // The driver: the player's actions, or the test driver's
            float throttle, brake, steerTarget;
            bool drift;
            if (driver != null)
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
                // The wheel turns towards the keys gradually, and comes back faster
                bool returning = MathF.Abs(steerTarget) < MathF.Abs(steer) || MathF.Sign(steerTarget) != MathF.Sign(steer);
                float rate = (returning ? SteerReturnSpeed : SteerSpeed) * dt;
                steer += Math.Clamp(steerTarget - steer, -rate, rate);
            }

            // Less of the wheel the faster it goes
            float share = 1.0f - (1.0f - Math.Clamp(FullSpeedSteer, 0.0f, 1.0f)) * Math.Clamp(kmh / MathF.Max(FullSpeedSteerAt, 1.0f), 0.0f, 1.0f);

            // Drift: fast, the handbrake lowers the rear grip instead of locking the wheels; the grip comes back gradually
            bool drifting = drift && kmh >= DriftMinSpeed;
            if (drifting)
                rearGrip = Math.Clamp(DriftRearGrip, 0.0f, 1.0f);
            else
                rearGrip = MathF.Min(1.0f, rearGrip + dt / MathF.Max(DriftRecovery, 0.01f));
            vehicle.SetGrip(1.0f, rearGrip);

            // ABS: the brake eases while a wheel locks, and comes back once it rolls
            if (AbsSlip > 0.0f && brake > 0.0f && kmh > 10.0f)
                absShare = vehicle.LongitudinalSlip > AbsSlip ? MathF.Max(0.2f, absShare - 6.0f * dt) : MathF.Min(1.0f, absShare + 6.0f * dt);
            else
                absShare = 1.0f;
            brake *= absShare;

            // Forward or back: brake is the reverse once it stops (the engine's own rule); the handbrake only when slow
            float forward = throttle > 0.0f ? throttle : -brake;
            float handbrake = drift && !drifting ? 1.0f : 0.0f;
            vehicle.SetInput(forward, steer * share, throttle > 0.0f ? brake : 0.0f, handbrake);

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
