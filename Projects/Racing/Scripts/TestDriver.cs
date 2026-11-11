using System;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The test driver (docs/games/Racing_Plan.html, Phase 3): drives the car along the track's center line, for checks
    /// without a player (laps, handling, frame times) and the first medal times. Later the start of AI drivers. Not a
    /// script of its own: CarController asks it for the controls each step when its TestDriver field is on.
    ///
    /// It steers at a point ahead on the center line (farther the faster it goes), and keeps to a speed it can take the
    /// curves ahead with: for each stretch ahead, the speed whose sideways acceleration in that stretch's curve is
    /// Cornering, and from which it can still brake (Braking) by then. Each lap is logged with its time and how far the car
    /// strayed from the center line. A car that hardly moves for StuckTime though the driver wants to go (against a wall, turned
    /// around after a spin) is Stuck: the race puts it back on the track (RaceManager), as the player's Respawn key would.
    /// </summary>
    public sealed class TestDriver
    {
        public struct Controls
        {
            public float Throttle, Brake, Steer;
            public bool Drift;
        }

        /// <summary>m/s^2 it brakes with, for its look ahead.</summary>
        public float Braking = 20.0f;
        /// <summary>Seconds under StuckSpeed with the throttle on before it counts as stuck.</summary>
        public float StuckTime = 3.0f;
        /// <summary>m/s: slower than this, the car isn't getting anywhere.</summary>
        public float StuckSpeed = 2.0f;

        /// <summary>The car hasn't moved for StuckTime though the driver wanted it to: the race respawns it.</summary>
        public bool Stuck => stuckFor >= StuckTime;

        readonly Centerline line;
        readonly float cornering;
        int nearest = -1;
        float time, lapStart = -1.0f, maxOff, nextLog, previousS = -1.0f, topSpeed;
        int laps;
        float steer;
        float stuckFor;
        float startTime = -1.0f;        // when it first moved: the 0-100 and 0-200 km/h times count from it
        bool logged100, logged200;

        TestDriver(Centerline line, float cornering)
        {
            this.line = line;
            this.cornering = cornering;
        }

        /// <summary>A driver for the center line in this file of the project (null when it isn't there).</summary>
        public static TestDriver? Load(string centerlineFile, Vector3 trackPosition, float cornering)
        {
            Centerline? line = Centerline.Load(centerlineFile, trackPosition);
            if (line == null)
                return null;
            Log.Info($"Test driver: {line.Count} points along {line.Length:0} m of '{line.Name}'");
            return new TestDriver(line, cornering);
        }

        Vector3 P(int i) => line.Points[i].Position;
        int Ahead(int i, float meters) => line.Ahead(i, meters);

        /// <summary>The car was put back on the track: it looks for its place on the whole line again.</summary>
        public void OnRespawn()
        {
            nearest = -1;
            stuckFor = 0.0f;
        }

        public Controls Drive(Vector3 position, Vector3 forward, Vector3 right, float speed, float dt)
        {
            time += dt;
            // The nearest point: searched around the last one (the whole line after a respawn)
            nearest = line.Nearest(position, nearest);
            float off = Vector3.Distance(position, P(nearest));
            maxOff = MathF.Max(maxOff, off);
            topSpeed = MathF.Max(topSpeed, speed);

            // The acceleration from the start: 0 to 100 and 0 to 200 km/h
            if (startTime < 0.0f && speed > 0.1f)
                startTime = time - dt;
            if (startTime >= 0.0f && !logged100 && speed * 3.6f >= 100.0f)
            {
                logged100 = true;
                Log.Info($"Test driver: 0-100 km/h in {time - startTime:0.00} s");
            }
            if (startTime >= 0.0f && !logged200 && speed * 3.6f >= 200.0f)
            {
                logged200 = true;
                Log.Info($"Test driver: 0-200 km/h in {time - startTime:0.00} s");
            }

            // Steering: at a point ahead, smoothed
            Vector3 to = P(Ahead(nearest, 10.0f + MathF.Abs(speed) * 0.45f)) - position;
            float angle = MathF.Atan2(Vector3.Dot(to, right), Vector3.Dot(to, forward));
            float wanted = Math.Clamp(angle / (12.0f * MathF.PI / 180.0f), -1.0f, 1.0f);
            steer += Math.Clamp(wanted - steer, -6.0f * dt, 6.0f * dt);

            // Speed: the curves ahead, each with the distance to brake for it
            float limit = 200.0f;
            float lookAhead = 30.0f + speed * speed / (2.0f * Braking);
            for (float look = 0.0f; look < lookAhead; look += 10.0f)
            {
                int a = Ahead(nearest, look), b = Ahead(a, 15.0f), c = Ahead(b, 15.0f);
                Vector3 d1 = Vector3.Normalize(P(b) - P(a)), d2 = Vector3.Normalize(P(c) - P(b));
                d1.Y = d2.Y = 0.0f; // the turn, not the hills
                float turn = MathF.Acos(Math.Clamp(Vector3.Dot(Vector3.Normalize(d1), Vector3.Normalize(d2)), -1.0f, 1.0f));
                float radius = turn > 1e-3f ? 15.0f / turn : 1e6f;
                float curveSpeed = MathF.Sqrt(cornering * radius);
                limit = MathF.Min(limit, MathF.Sqrt(curveSpeed * curveSpeed + 2.0f * Braking * look));
            }
            var controls = new Controls { Steer = steer };
            if (speed < limit - 1.0f)
                controls.Throttle = 1.0f;
            else if (speed > limit + 2.0f)
                controls.Brake = Math.Clamp((speed - limit) / 8.0f, 0.2f, 1.0f);
            stuckFor = controls.Throttle > 0.0f && MathF.Abs(speed) < StuckSpeed && startTime >= 0.0f ? stuckFor + dt : 0.0f;

            // Laps: the start line is the center line's distance 0
            float s = line.Points[nearest].S;
            if (previousS >= 0.0f && previousS > line.Length - 50.0f && s < 50.0f)
            {
                if (lapStart >= 0.0f)
                {
                    laps++;
                    Log.Info($"Test driver: lap {laps}: {time - lapStart:0.00} s, average {line.Length / (time - lapStart) * 3.6f:0} km/h, " +
                        $"top {topSpeed * 3.6f:0} km/h, at most {maxOff:0.0} m from the center line");
                }
                lapStart = time;
                maxOff = 0.0f;
                topSpeed = 0.0f;
            }
            previousS = s;
            if (time >= nextLog)
            {
                nextLog = time + 2.0f;
                Log.Info($"Test driver: {time:0} s, at {s:0} m, {speed * 3.6f:0} km/h (limit {limit * 3.6f:0}), {off:0.0} m off");
            }
            return controls;
        }
    }
}
