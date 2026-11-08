using System;
using System.Globalization;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The race (docs/games/Racing_Plan.html, Phase 4): F1-style start lights, laps and checkpoints, the timers and
    /// splits, respawn and restart, the wrong-way warning, the fallen-off car put back, the medals and the records.
    ///
    /// Everything it knows about the track comes from the track's center line (Tools/TrackBuilder writes it): the
    /// checkpoints are distances along it (Sectors of equal length; the start and finish line at 0), passed when the car's
    /// nearest point on it moves over them, in order (a lap with a missed one doesn't count). No triggers: nothing to
    /// place in the scene, and nothing a car at 350 km/h (1.6 m a physics step) can jump through.
    ///
    /// The car starts behind the line: the timer starts when the lights go out, the first lap ends at the second time
    /// over the line. Respawn (the key, or by itself when the car has fallen off) puts the car on the last checkpoint
    /// passed, standing still, facing along the track; the timer keeps running. Restart starts the race again.
    /// The HUD (Phase 5) reads the race's state from Current.
    /// </summary>
    public class RaceManager : Entity
    {
        public enum RaceState { Lights, Racing, Finished }

        // ---- The race
        /// <summary>The car (its CarController).</summary>
        public Entity? Car;
        /// <summary>The track entity the center line's points are relative to.</summary>
        public Entity? Track;
        /// <summary>The track's center line: a file of the project (Tools/TrackBuilder writes it).</summary>
        public string Centerline = "Assets/Tracks/TestTrack.centerline.json";
        public int Laps = 3;
        /// <summary>The lap's parts of equal length: Sectors - 1 checkpoints between the start and finish line.</summary>
        public int Sectors = 4;

        // ---- The start lights
        /// <summary>The panel of the five lights (its children Light 1 to Light 5: UI images).</summary>
        public Entity? StartLights;
        /// <summary>Seconds before the first light comes on (the car settles on its springs).</summary>
        public float LightsDelay = 1.0f;
        /// <summary>Seconds between the lights.</summary>
        public float LightInterval = 1.0f;
        /// <summary>The shortest and longest the five lights stay on before they go out (a random time between).</summary>
        public float LightsHoldMin = 0.2f;
        public float LightsHoldMax = 1.0f;
        public Vector4 LightOn = new(1.0f, 0.06f, 0.04f, 1.0f);
        public Vector4 LightOff = new(0.16f, 0.03f, 0.03f, 1.0f);

        // ---- Off the track
        /// <summary>Meters below the road at which the car has fallen off.</summary>
        public float FallBelow = 3.0f;
        /// <summary>Meters beyond the road's edge (past its walls) at which the car has left the track.</summary>
        public float FallOutside = 2.0f;
        /// <summary>Seconds it may be off the track before it is put back (touching the terrain: at once).</summary>
        public float FallTime = 1.0f;

        // ---- Wrong way
        /// <summary>The text shown while the car drives against the track's direction.</summary>
        public Entity? WrongWayText;
        /// <summary>Degrees between the car's way and the track's beyond which it drives the wrong way.</summary>
        public float WrongWayAngle = 100.0f;
        /// <summary>km/h below which it doesn't count (turning around on the spot).</summary>
        public float WrongWaySpeed = 15.0f;
        /// <summary>Seconds of the wrong way before the warning (a spin doesn't set it off).</summary>
        public float WrongWayTime = 1.0f;

        // ---- Medals: the race's time, in seconds (0: none)
        public float AuthorTime;
        public float GoldTime;
        public float SilverTime;
        public float BronzeTime;

        /// <summary>The race of the running game.</summary>
        public static RaceManager? Current { get; private set; }

        // ---- The race's state, for the HUD (Phase 5)
        public RaceState State { get; private set; } = RaceState.Lights;
        /// <summary>The start lights on (0 to 5); 0 after they went out.</summary>
        public int LightsOn { get; private set; }
        /// <summary>The lap being driven, 1 to Laps (Laps when finished).</summary>
        public int Lap => Math.Clamp(lapsDone + 1, 1, Math.Max(Laps, 1));
        /// <summary>Seconds since the lights went out.</summary>
        public float RaceTime { get; private set; }
        /// <summary>Seconds of the lap being driven.</summary>
        public float LapTime => RaceTime - lapStart;
        /// <summary>The best lap ever on this track (saved), 0 when none yet.</summary>
        public float BestLap { get; private set; }
        /// <summary>The best race ever on this track (saved), 0 when none yet.</summary>
        public float BestRace { get; private set; }
        /// <summary>The last split: its time in the lap, and against the best lap's there (negative: faster; NaN: no best yet).</summary>
        public float LastSplit { get; private set; }
        public float LastSplitDelta { get; private set; } = float.NaN;
        /// <summary>The car drives the wrong way (the warning shows).</summary>
        public bool WrongWay { get; private set; }
        /// <summary>The medal of the finished race: 0 none, 1 bronze, 2 silver, 3 gold, 4 author.</summary>
        public int Medal { get; private set; }

        static readonly string[] MedalNames = { "no medal", "bronze", "silver", "gold", "author" };

        Centerline? line;
        CarController? car;
        Rigidbody? body;
        readonly UIImage?[] lights = new UIImage?[5];
        UIRect? lightsPanel, wrongWayRect;
        readonly Random random = new();

        float phaseTime;        // seconds in the lights, or since the finish
        float lightsHold;       // the random hold of this start
        float lightsHideAt = -1.0f;
        int nearest = -1;
        float previousS = -1.0f;
        bool crossedStart;      // over the line once: the first lap is under way
        int nextCheckpoint = 1; // 1 to Sectors - 1; Sectors: all passed, the line is next
        int lapsDone;
        float lapStart;
        float[] lapSplits = Array.Empty<float>();
        float[] bestSplits = Array.Empty<float>();
        float offTime, wrongTime;
        bool respawnAsked, restartAsked, started;
        int respawns;

        string Key(string name) => $"{line?.Name ?? "Track"}.{name}";

        protected override void OnCreate()
        {
            Current = this;
            line = Racing.Centerline.Load(Centerline, Track != null ? Track.WorldPosition : Vector3.Zero);
            if (line == null)
            {
                Log.Error($"RaceManager: the center line '{Centerline}' wasn't found: no race");
                return;
            }
            Sectors = Math.Max(Sectors, 1);
            lapSplits = new float[Sectors];
            BestLap = SaveData.GetFloat(Key("BestLap"));
            BestRace = SaveData.GetFloat(Key("BestRace"));
            bestSplits = ParseSplits(SaveData.GetString(Key("BestLapSplits")));
            for (int i = 0; i < lights.Length; i++)
                lights[i] = Scene.FindEntityByName($"Light {i + 1}")?.GetComponent<UIImage>();
            lightsPanel = StartLights?.GetComponent<UIRect>();
            wrongWayRect = WrongWayText?.GetComponent<UIRect>();
            if (wrongWayRect != null)
                wrongWayRect.Visible = false;
            Log.Info($"Race: '{line.Name}', {line.Length:0} m, {Laps} laps, {Sectors - 1} checkpoints; best lap {Format(BestLap)}, best race {Format(BestRace)}");
        }

        protected override void OnUpdate(float dt)
        {
            // The keys: read each frame, done at the next physics step
            if (Input.WasPressed("Restart"))
                restartAsked = true;
            if (Input.WasPressed("Respawn"))
                respawnAsked = true;
        }

        protected override void OnFixedUpdate(float dt)
        {
            if (line == null)
                return;
            if (car == null)
            {
                car = CarController.Current;
                body = Car?.GetComponent<Rigidbody>();
                if (car == null || body == null)
                    return; // the car's script starts in this step
            }
            if (!started || restartAsked)
            {
                started = true;
                restartAsked = false;
                Restart();
            }

            phaseTime += dt;
            if (lightsHideAt >= 0.0f && phaseTime >= lightsHideAt)
            {
                lightsHideAt = -1.0f;
                if (lightsPanel != null)
                    lightsPanel.Visible = false;
            }
            switch (State)
            {
                case RaceState.Lights:
                    UpdateLights();
                    break;
                case RaceState.Racing:
                    RaceTime += dt;
                    if (respawnAsked)
                        Respawn("the key");
                    UpdateProgress();
                    UpdateOffTrack(dt);
                    UpdateWrongWay(dt);
                    break;
                case RaceState.Finished:
                    break;
            }
            respawnAsked = false;
        }

        // ---- The start

        /// <summary>The race from the start: the car behind the line, held, the lights from the first.</summary>
        public void Restart()
        {
            if (line == null || car == null)
                return;
            State = RaceState.Lights;
            phaseTime = 0.0f;
            lightsHold = LightsHoldMin + (float)random.NextDouble() * MathF.Max(LightsHoldMax - LightsHoldMin, 0.0f);
            lightsHideAt = -1.0f;
            RaceTime = 0.0f;
            lapStart = 0.0f;
            lapsDone = 0;
            crossedStart = false;
            nextCheckpoint = 1;
            Array.Clear(lapSplits);
            LastSplit = 0.0f;
            LastSplitDelta = float.NaN;
            Medal = 0;
            respawns = 0;
            SetWrongWay(false);
            wrongTime = offTime = 0.0f;

            int at = line.Nearest(line.StartPosition, -1);
            Vector3 up = line.Points[at].Up;
            car.Respawn(line.StartPosition + up * 1.0f, Racing.Centerline.Facing(line.StartForward, up));
            car.Held = true;
            nearest = at;
            previousS = -1.0f;
            SetLights(0);
            if (lightsPanel != null)
                lightsPanel.Visible = true;
            Log.Info($"Race: restart; the lights go out {LightsDelay + 4.0f * LightInterval + lightsHold:0.00} s from now");
        }

        void UpdateLights()
        {
            float allOn = LightsDelay + 4.0f * LightInterval;
            if (phaseTime >= allOn + lightsHold)
            {
                // Lights out: go
                SetLights(0);
                State = RaceState.Racing;
                car!.Held = false;
                lightsHideAt = phaseTime + 1.0f;
                Log.Info("Race: lights out, go");
                return;
            }
            int on = phaseTime < LightsDelay ? 0 : Math.Min(5, 1 + (int)((phaseTime - LightsDelay) / MathF.Max(LightInterval, 0.01f)));
            if (on != LightsOn)
                SetLights(on);
        }

        void SetLights(int on)
        {
            LightsOn = on;
            for (int i = 0; i < lights.Length; i++)
                if (lights[i] != null)
                    lights[i]!.Color = i < on ? LightOn : LightOff;
        }

        // ---- Laps and checkpoints

        float CheckpointS(int k) => line!.Length * k / Sectors;

        void UpdateProgress()
        {
            nearest = line!.Nearest(Car!.WorldPosition, nearest);
            float s = line.Points[nearest].S;
            if (previousS >= 0.0f)
            {
                float length = line.Length;
                if (previousS > length - 50.0f && s < 50.0f)
                    CrossLine();
                else if (crossedStart && nextCheckpoint < Sectors)
                {
                    float c = CheckpointS(nextCheckpoint);
                    if (previousS < c && s >= c && s - previousS < 50.0f)
                        PassCheckpoint();
                }
            }
            previousS = s;
        }

        void CrossLine()
        {
            if (!crossedStart)
            {
                crossedStart = true; // the first lap: its time counts from the lights
                return;
            }
            if (nextCheckpoint < Sectors)
            {
                Log.Info($"Race: over the line with checkpoint {nextCheckpoint} missed: the lap doesn't count");
                return;
            }
            float lap = RaceTime - lapStart;
            lapSplits[Sectors - 1] = lap;
            Split(Sectors - 1, lap);
            lapsDone++;
            bool bestLap = BestLap <= 0.0f || lap < BestLap;
            Log.Info($"Race: lap {lapsDone}/{Laps}: {Format(lap)}{(bestLap ? " (best lap)" : "")}, race {Format(RaceTime)}");
            if (bestLap)
            {
                BestLap = lap;
                bestSplits = (float[])lapSplits.Clone();
                SaveData.SetFloat(Key("BestLap"), BestLap);
                SaveData.SetString(Key("BestLapSplits"), FormatSplits(bestSplits));
                SaveData.Save();
            }
            lapStart = RaceTime;
            nextCheckpoint = 1;
            Array.Clear(lapSplits);
            if (lapsDone >= Laps)
                Finish();
        }

        void PassCheckpoint()
        {
            float split = RaceTime - lapStart;
            lapSplits[nextCheckpoint - 1] = split;
            Split(nextCheckpoint - 1, split);
            Log.Info($"Race: lap {Lap} checkpoint {nextCheckpoint}/{Sectors - 1}: {Format(split)}{DeltaText()}");
            nextCheckpoint++;
        }

        // The split at sector index (0 the first checkpoint, Sectors - 1 the line) against the best lap's
        void Split(int index, float time)
        {
            LastSplit = time;
            LastSplitDelta = index < bestSplits.Length && bestSplits[index] > 0.0f ? time - bestSplits[index] : float.NaN;
        }

        string DeltaText() => float.IsNaN(LastSplitDelta) ? "" : $" ({(LastSplitDelta <= 0.0f ? "-" : "+")}{MathF.Abs(LastSplitDelta):0.00} {(LastSplitDelta <= 0.0f ? "faster" : "slower")})";

        void Finish()
        {
            State = RaceState.Finished;
            phaseTime = 0.0f;
            car!.Held = true;
            SetWrongWay(false);
            float time = RaceTime;
            Medal = time <= AuthorTime ? 4 : time <= GoldTime ? 3 : time <= SilverTime ? 2 : time <= BronzeTime ? 1 : 0;
            bool record = BestRace <= 0.0f || time < BestRace;
            if (record)
            {
                BestRace = time;
                SaveData.SetFloat(Key("BestRace"), BestRace);
            }
            if (Medal > SaveData.GetInt(Key("Medal")))
                SaveData.SetInt(Key("Medal"), Medal);
            SaveData.Save();
            Log.Info($"Race: finished in {Format(time)}{(record ? " (new record)" : "")}, {MedalNames[Medal]}, best lap {Format(BestLap)}, {respawns} respawns");
        }

        // ---- Respawn: the last checkpoint passed

        /// <summary>The car back on the last checkpoint passed (the start when none), standing still, facing along the track.</summary>
        public void Respawn(string reason)
        {
            if (line == null || car == null || State != RaceState.Racing)
                return;
            respawns++;
            if (!crossedStart)
            {
                int at = line.Nearest(line.StartPosition, -1);
                Vector3 up = line.Points[at].Up;
                car.Respawn(line.StartPosition + up * 1.0f, Racing.Centerline.Facing(line.StartForward, up));
                nearest = at;
                previousS = line.Points[at].S;
                Log.Info($"Race: respawn ({reason}) at the start");
            }
            else
            {
                // The last checkpoint passed (0: the line), a little past it, so it isn't passed again
                int passed = nextCheckpoint - 1;
                int i = line.IndexAt(CheckpointS(passed) + 2.0f);
                Centerline.Point p = line.Points[i];
                car.Respawn(p.Position + p.Up * 1.0f, line.RotationAt(i));
                nearest = i;
                previousS = p.S;
                Log.Info($"Race: respawn ({reason}) at {(passed == 0 ? "the line" : $"checkpoint {passed}")}, {p.S:0} m");
            }
            offTime = wrongTime = 0.0f;
            SetWrongWay(false);
        }

        void UpdateOffTrack(float dt)
        {
            Centerline.Point p = line!.Points[nearest];
            Vector3 d = Car!.WorldPosition - p.Position;
            float below = -Vector3.Dot(d, p.Up);
            float aside = MathF.Abs(Vector3.Dot(d, p.Right)) - 0.5f * p.Width;
            bool off = below > FallBelow || aside > FallOutside;
            offTime = off ? offTime + dt : 0.0f;
            if (car!.TouchedTerrain)
            {
                car.TouchedTerrain = false;
                Respawn("touched the terrain");
            }
            else if (offTime >= FallTime)
                Respawn(below > FallBelow ? $"fell {below:0} m below the road" : $"{aside:0} m outside the road");
        }

        // ---- Wrong way

        void UpdateWrongWay(float dt)
        {
            Centerline.Point p = line!.Points[nearest];
            Vector3 velocity = body!.LinearVelocity;
            velocity -= p.Up * Vector3.Dot(velocity, p.Up);
            float speed = velocity.Length();
            if (speed * 3.6f >= WrongWaySpeed)
            {
                bool wrong = Vector3.Dot(velocity / speed, line.Direction(nearest)) < MathF.Cos(WrongWayAngle * MathF.PI / 180.0f);
                wrongTime = wrong ? wrongTime + dt : 0.0f; // the right way: gone at once
            }
            bool show = wrongTime >= WrongWayTime;
            if (show != WrongWay)
            {
                SetWrongWay(show);
                Log.Info(show ? $"Race: wrong way (at {p.S:0} m)" : "Race: the right way again");
            }
        }

        void SetWrongWay(bool show)
        {
            WrongWay = show;
            if (wrongWayRect != null)
                wrongWayRect.Visible = show;
        }

        // ---- Text

        /// <summary>A time as 1:23.456 (or 23.456 under a minute); "-" for none.</summary>
        public static string Format(float seconds)
        {
            if (seconds <= 0.0f || float.IsNaN(seconds))
                return "-";
            int minutes = (int)(seconds / 60.0f);
            float rest = seconds - 60.0f * minutes;
            return minutes > 0 ? $"{minutes}:{rest.ToString("00.000", CultureInfo.InvariantCulture)}" : rest.ToString("0.000", CultureInfo.InvariantCulture);
        }

        static string FormatSplits(float[] splits) =>
            string.Join(";", Array.ConvertAll(splits, t => t.ToString("0.000", CultureInfo.InvariantCulture)));

        float[] ParseSplits(string text)
        {
            var result = new float[Sectors];
            string[] parts = text.Split(';', StringSplitOptions.RemoveEmptyEntries);
            if (parts.Length != Sectors)
                return result; // none yet, or another number of sectors
            for (int i = 0; i < parts.Length; i++)
                float.TryParse(parts[i], NumberStyles.Float, CultureInfo.InvariantCulture, out result[i]);
            return result;
        }
    }
}
