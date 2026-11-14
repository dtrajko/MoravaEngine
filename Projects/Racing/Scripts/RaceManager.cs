using System;
using System.Globalization;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The race (docs/games/Racing_Plan.html, Phase 4): start lights (red, yellow, green), laps and checkpoints, the timers and
    /// splits, respawn and restart, the wrong-way warning, the fallen-off car put back, the medals and the records.
    ///
    /// Everything it knows about the track comes from the track's center line (Tools/TrackBuilder writes it): the
    /// checkpoints are distances along it (Sectors of equal length; the start and finish line at 0), passed when the car's
    /// nearest point on it moves over them, in order (a lap with a missed one doesn't count). No triggers: nothing to
    /// place in the scene, and nothing a car at 350 km/h (1.6 m a physics step) can jump through.
    ///
    /// The car starts behind the line: the timer starts when the light turns green, the first lap ends at the second time
    /// over the line. Respawn (the key, or by itself when the car has fallen off) puts the car on the last checkpoint
    /// passed, standing still, facing along the track; the timer keeps running. Restart starts the race again.
    /// The HUD (RaceHud, Phase 5) reads the race's state from Current. The keys don't count while a menu is open
    /// (KeysBlocked: the pause menu's Enter is its own).
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
        public string Centerline = "Assets/Tracks/Circuit.centerline.json";
        public int Laps = 3;
        /// <summary>The lap's parts of equal length: Sectors - 1 checkpoints between the start and finish line.</summary>
        public int Sectors = 4;

        // ---- The start lights: red, then yellow, then green: go
        /// <summary>The panel of the three lights (its children Light 1 to Light 3, red, yellow, green: UI images).</summary>
        public Entity? StartLights;
        /// <summary>Seconds before the red light comes on (the car settles on its springs).</summary>
        public float LightsDelay = 1.0f;
        /// <summary>Seconds the red and the yellow light each stay on before the next.</summary>
        public float LightInterval = 1.0f;
        /// <summary>Seconds the green light stays on after the start, before the lights hide.</summary>
        public float GreenTime = 1.0f;
        /// <summary>The lights' colors on, red, yellow, green, and off (dimmed).</summary>
        public Vector4 RedOn = new(1.0f, 0.08f, 0.05f, 1.0f);
        public Vector4 YellowOn = new(1.0f, 0.78f, 0.08f, 1.0f);
        public Vector4 GreenOn = new(0.15f, 1.0f, 0.3f, 1.0f);
        /// <summary>Times a light's color while it is off.</summary>
        public float OffDim = 0.16f;

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

        // ---- Medals: the race's time, in seconds (0: none), with the engine's Buffed power
        public float AuthorTime;
        public float GoldTime;
        public float SilverTime;
        public float BronzeTime;
        // ---- The medals with the Realistic power (GameOptions.RealisticPower): a slower car, times of their own
        public float RealisticAuthorTime;
        public float RealisticGoldTime;
        public float RealisticSilverTime;
        public float RealisticBronzeTime;

        /// <summary>The race of the running game.</summary>
        public static RaceManager? Current { get; private set; }

        // ---- The race's state, for the HUD (Phase 5)
        public RaceState State { get; private set; } = RaceState.Lights;
        /// <summary>The start light on: 0 none, 1 red, 2 yellow, 3 green.</summary>
        public int LightsOn { get; private set; }
        /// <summary>The lap being driven, 1 to Laps (Laps when finished).</summary>
        public int Lap => Math.Clamp(lapsDone + 1, 1, Math.Max(Laps, 1));
        /// <summary>The laps finished in this race (the ghost: a lap just ended when it grows).</summary>
        public int LapsDone => lapsDone;
        /// <summary>Seconds since the light turned green.</summary>
        public float RaceTime { get; private set; }
        /// <summary>Seconds of the lap being driven.</summary>
        public float LapTime => RaceTime - lapStart;
        /// <summary>The best flying lap ever (lap 2 on, over the line at speed) on this track (saved), 0 when none yet.</summary>
        public float BestLap { get; private set; }
        /// <summary>The best lap 1 ever (from standing at the lights) on this track (saved), 0 when none yet.</summary>
        public float BestStartLap { get; private set; }
        /// <summary>The lap being driven is lap 1, from the standing start: it is compared with lap 1s only.</summary>
        public bool StartLap => lapsDone == 0;
        /// <summary>The best of the lap being driven's kind: BestStartLap in lap 1, BestLap after.</summary>
        public float BestOfThisLap => StartLap ? BestStartLap : BestLap;
        /// <summary>The best race ever on this track (saved), 0 when none yet.</summary>
        public float BestRace { get; private set; }
        /// <summary>The last split: its time in the lap, and against the best lap of its kind there (lap 1 against the best
        /// lap 1, the others against the best flying lap; negative: faster; NaN: no best yet).</summary>
        public float LastSplit { get; private set; }
        public float LastSplitDelta { get; private set; } = float.NaN;
        /// <summary>The car drives the wrong way (the warning shows).</summary>
        public bool WrongWay { get; private set; }
        /// <summary>The medal of the finished race: 0 none, 1 bronze, 2 silver, 3 gold, 4 author.</summary>
        public int Medal { get; private set; }
        /// <summary>The finished race beat the best race (saved before it).</summary>
        public bool NewRecord { get; private set; }
        /// <summary>The laps driven in this race, in order (seconds).</summary>
        public float[] LapTimes => lapTimes.ToArray();
        /// <summary>Counts the splits (checkpoints and the line): the HUD shows a new one when it changes.</summary>
        public int SplitCount { get; private set; }
        /// <summary>The last split's checkpoint, 1 to Sectors - 1; Sectors: the line (a lap).</summary>
        public int LastSplitCheckpoint { get; private set; }
        /// <summary>The last lap was the best ever of its kind (lap 1, or a flying lap).</summary>
        public bool LastLapBest { get; private set; }
        /// <summary>The last lap finished was lap 1 (the standing start).</summary>
        public bool LastLapStart { get; private set; }
        /// <summary>Counts the restarts: the HUD clears its popups when it changes.</summary>
        public int RestartCount { get; private set; }
        /// <summary>Set by the menus while they are open: Restart and Respawn wait (their keys are the menus' too).</summary>
        public static bool KeysBlocked { get; set; }

        static readonly string[] MedalNames = { "no medal", "bronze", "silver", "gold", "author" };

        /// <summary>The medal's name for the screen: "", "Bronze", "Silver", "Gold", "Author".</summary>
        public static string MedalTitle(int medal) => medal switch { 1 => "Bronze", 2 => "Silver", 3 => "Gold", 4 => "Author", _ => "" };

        /// <summary>The medal's time with the power of the records in use (RealisticRecords; 0: none).</summary>
        public float MedalTime(int medal) => RealisticRecords ?
            medal switch { 1 => RealisticBronzeTime, 2 => RealisticSilverTime, 3 => RealisticGoldTime, 4 => RealisticAuthorTime, _ => 0.0f } :
            medal switch { 1 => BronzeTime, 2 => SilverTime, 3 => GoldTime, 4 => AuthorTime, _ => 0.0f };

        Centerline? line;
        CarController? car;
        Rigidbody? body;
        readonly UIImage?[] lights = new UIImage?[3];
        UIRect? lightsPanel, wrongWayRect;

        float phaseTime;        // seconds in the lights, or since the finish
        float lightsHideAt = -1.0f;
        int nearest = -1;
        float previousS = -1.0f;
        bool crossedStart;      // over the line once: the first lap is under way
        int nextCheckpoint = 1; // 1 to Sectors - 1; Sectors: all passed, the line is next
        int lapsDone;
        float lapStart;
        float[] lapSplits = Array.Empty<float>();
        float[] bestSplits = Array.Empty<float>();      // the best flying lap's
        float[] bestStartSplits = Array.Empty<float>(); // the best lap 1's
        float offTime, wrongTime;
        bool respawnAsked, restartAsked, started;
        int respawns;
        readonly System.Collections.Generic.List<float> lapTimes = new();

        /// <summary>The records in use are the Realistic power's (else Buffed's): the car's power. A race whose power changes
        /// starts again with the other power's records (RecordsFor).</summary>
        public bool RealisticRecords { get; private set; }

        // The records are the track version's and the engine power's (Centerline.RecordKey): a rebuilt track starts with none
        string Key(string name) => line != null ? line.RecordKey(name, RealisticRecords) : $"Track{(RealisticRecords ? "-Realistic" : "")}.{name}";
        /// <summary>The save data key of one of this track's records (the ghost keeps its laps beside them).</summary>
        public string RecordKey(string name) => Key(name);

        /// <summary>The power's name for the screen and the log.</summary>
        public static string PowerTitle(bool realistic) => realistic ? "Realistic" : "Buffed";

        // The best times of this power (the saved ones)
        void LoadRecords(bool realistic)
        {
            RealisticRecords = realistic;
            BestLap = SaveData.GetFloat(Key("BestLap"));
            BestRace = SaveData.GetFloat(Key("BestRace"));
            bestSplits = ParseSplits(SaveData.GetString(Key("BestLapSplits")));
            BestStartLap = SaveData.GetFloat(Key("BestStartLap"));
            bestStartSplits = ParseSplits(SaveData.GetString(Key("BestStartLapSplits")));
        }

        string RecordsText() => $"best lap 1 {Format(BestStartLap)}, best flying lap {Format(BestLap)}, best race {Format(BestRace)} ({PowerTitle(RealisticRecords)} power)";

        protected override void OnCreate()
        {
            Current = this;
            GameOptions.ApplyAtStart(); // a race played directly in the editor: the player's choices too (the ghost, the gearbox)
            line =Racing.Centerline.Load(Centerline, Track != null ? Track.WorldPosition : Vector3.Zero);
            if (line == null)
            {
                Log.Error($"RaceManager: the center line '{Centerline}' wasn't found: no race");
                return;
            }
            Sectors = Math.Max(Sectors, 1);
            lapSplits = new float[Sectors];
            LoadRecords(GameOptions.RealisticPower); // the car's when it is there (Restart)
            for (int i = 0; i < lights.Length; i++)
                lights[i] = Scene.FindEntityByName($"Light {i + 1}")?.GetComponent<UIImage>();
            lightsPanel = StartLights?.GetComponent<UIRect>();
            wrongWayRect = WrongWayText?.GetComponent<UIRect>();
            if (wrongWayRect != null)
                wrongWayRect.Visible = false;
            Log.Info($"Race: '{line.Name}', {line.Length:0} m, {Laps} laps, {Sectors - 1} checkpoints; {RecordsText()}, records of version {line.Version}");
        }

        protected override void OnUpdate(float dt)
        {
            // The keys: read each frame, done at the next physics step
            if (KeysBlocked)
                return;
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
            else if (car.RealisticPower != RealisticRecords)
            {
                // The power changed (Settings, in the pause menu): its laps would count for neither, so the race starts again
                Log.Info($"Race: the engine's power is now {PowerTitle(car.RealisticPower)}: the race starts again");
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
                    else if (car!.TestDriverStuck)
                        Respawn("the test driver was stuck");
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

        /// <summary>The race starts again at the next physics step (the menus' Restart).</summary>
        public void RequestRestart() => restartAsked = true;

        /// <summary>The race from the start: the car behind the line, held, the lights from the first.</summary>
        public void Restart()
        {
            if (line == null || car == null)
                return;
            if (car.RealisticPower != RealisticRecords)
            {
                LoadRecords(car.RealisticPower);
                Log.Info($"Race: {RecordsText()}");
            }
            State = RaceState.Lights;
            phaseTime = 0.0f;
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
            NewRecord = false;
            LastLapBest = false;
            lapTimes.Clear();
            RestartCount++;
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
            Log.Info($"Race: restart; green {LightsDelay + 2.0f * LightInterval:0.00} s from now");
        }

        void UpdateLights()
        {
            float interval = MathF.Max(LightInterval, 0.01f);
            if (phaseTime >= LightsDelay + 2.0f * interval)
            {
                // Green: go
                SetLights(3);
                State = RaceState.Racing;
                car!.Held = false;
                lightsHideAt = phaseTime + MathF.Max(GreenTime, 0.0f);
                Log.Info("Race: green, go");
                return;
            }
            int on = phaseTime < LightsDelay ? 0 : phaseTime < LightsDelay + interval ? 1 : 2;
            if (on != LightsOn)
                SetLights(on);
        }

        // One light on (1 red, 2 yellow, 3 green), the others dimmed
        void SetLights(int on)
        {
            LightsOn = on;
            Vector4[] colors = { RedOn, YellowOn, GreenOn };
            for (int i = 0; i < lights.Length; i++)
            {
                if (lights[i] == null)
                    continue;
                Vector4 c = colors[i];
                lights[i]!.Color = i + 1 == on ? c : new Vector4(c.X * OffDim, c.Y * OffDim, c.Z * OffDim, 1.0f);
            }
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
            // Lap 1 (from standing at the lights) and the flying laps each have a best of their own
            bool start = StartLap;
            float bestBefore = start ? BestStartLap : BestLap;
            lapsDone++;
            lapTimes.Add(lap);
            bool bestLap = bestBefore <= 0.0f || lap < bestBefore;
            LastLapBest = bestLap;
            LastLapStart = start;
            Log.Info($"Race: lap {lapsDone}/{Laps}: {Format(lap)}{(bestLap ? (start ? " (best lap 1)" : " (best flying lap)") : "")}, race {Format(RaceTime)}");
            if (bestLap)
            {
                float[] splits = (float[])lapSplits.Clone();
                string name = start ? "BestStartLap" : "BestLap";
                if (start) { BestStartLap = lap; bestStartSplits = splits; }
                else { BestLap = lap; bestSplits = splits; }
                SaveData.SetFloat(Key(name), lap);
                SaveData.SetString(Key(name + "Splits"), FormatSplits(splits));
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

        // The split at sector index (0 the first checkpoint, Sectors - 1 the line) against the best lap's of its kind (the
        // lap being driven: lap 1 against the best lap 1, the others against the best flying lap)
        void Split(int index, float time)
        {
            LastSplitCheckpoint = index + 1;
            SplitCount++;
            LastSplit = time;
            float[] best = StartLap ? bestStartSplits : bestSplits;
            LastSplitDelta = index < best.Length && best[index] > 0.0f ? time - best[index] : float.NaN;
        }

        string DeltaText() => float.IsNaN(LastSplitDelta) ? "" : $" ({(LastSplitDelta <= 0.0f ? "-" : "+")}{MathF.Abs(LastSplitDelta):0.00} {(LastSplitDelta <= 0.0f ? "faster" : "slower")})";

        void Finish()
        {
            State = RaceState.Finished;
            phaseTime = 0.0f;
            car!.Held = true;
            SetWrongWay(false);
            float time = RaceTime;
            Medal = 0;
            for (int medal = 4; medal >= 1 && Medal == 0; medal--)
                if (time <= MedalTime(medal))
                    Medal = medal;
            bool record = BestRace <= 0.0f || time < BestRace;
            NewRecord = record;
            if (record)
            {
                BestRace = time;
                SaveData.SetFloat(Key("BestRace"), BestRace);
            }
            if (Medal > SaveData.GetInt(Key("Medal")))
                SaveData.SetInt(Key("Medal"), Medal);
            SaveData.Save();
            Log.Info($"Race: finished in {Format(time)}{(record ? " (new record)" : "")}, {MedalNames[Medal]}, best lap 1 {Format(BestStartLap)}, best flying lap {Format(BestLap)}, {respawns} respawns");
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

        /// <summary>A time as 1:23.456 (or 23.456 under a minute; decimals: 2 gives 1:23.45, cut, not rounded); "-" for none.</summary>
        public static string Format(float seconds, int decimals = 3)
        {
            if (seconds <= 0.0f || float.IsNaN(seconds))
                return "-";
            decimals = Math.Clamp(decimals, 0, 3);
            float scale = MathF.Pow(10.0f, decimals);
            seconds = MathF.Floor(seconds * scale + 1e-3f) / scale; // a clock shows what has passed
            int minutes = (int)(seconds / 60.0f);
            float rest = seconds - 60.0f * minutes;
            string fraction = decimals > 0 ? "." + new string('0', decimals) : "";
            return minutes > 0 ? $"{minutes}:{rest.ToString("00" + fraction, CultureInfo.InvariantCulture)}" : rest.ToString("0" + fraction, CultureInfo.InvariantCulture);
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
