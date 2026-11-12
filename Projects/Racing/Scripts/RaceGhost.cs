using System;
using System.Collections.Generic;
using System.Globalization;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The ghost of the best lap (docs/games/Racing_Plan.html, Phase 9): a see-through copy of the car (glass, no physics,
    /// no collider) driving the player's best lap ever on this track, from the start line at each lap's start. Every lap is
    /// recorded (the car's body and its four wheels, SampleRate times a second, on the lap's clock); when a lap is the best,
    /// its samples are saved beside the best lap's time (SaveData, the track's "GhostLap") and the ghost drives it from the
    /// next lap on. Between samples the ghost is interpolated (positions straight, turns along the shortest arc).
    /// Before the green light, after the finish, and once it has finished its lap, the ghost is parked out of sight.
    /// </summary>
    public class RaceGhost : Entity
    {
        /// <summary>The player's car: its root entity (the body), with the wheels named in WheelNames.</summary>
        public Entity? Car;
        public float SampleRate = 20.0f;
        /// <summary>Off: no ghost shown (it still records and saves).</summary>
        public bool Show = true;

        static readonly string[] WheelNames = { "Wheel FL", "Wheel FR", "Wheel RL", "Wheel RR" };
        const int Floats = 7 + 7 * 4; // the body's position and turn, then each wheel's (in the car)
        const int FormatVersion = 1;
        static readonly Vector3 Parked = new(0.0f, -1000.0f, 0.0f);

        readonly Entity?[] carWheels = new Entity?[4];
        readonly Entity?[] ghostWheels = new Entity?[4];
        readonly List<float> recording = new();   // this lap's samples
        float[] best = Array.Empty<float>();      // the best lap's, as saved
        int lapsSeen = -1, restartsSeen = -1;
        bool loaded, parked;

        protected override void OnCreate()
        {
            for (int i = 0; i < 4; i++)
            {
                carWheels[i] = Scene.FindEntityByName(WheelNames[i]);
                ghostWheels[i] = Scene.FindEntityByName("Ghost " + WheelNames[i]);
            }
            Park();
        }

        protected override void OnUpdate(float dt)
        {
            RaceManager? race = RaceManager.Current;
            if (race == null || Car == null)
                return;
            if (!loaded)
            {
                loaded = true;
                best = Load(SaveData.GetString(race.RecordKey("GhostLap")));
                Log.Info(best.Length > 0 ? $"Ghost: the best lap's, {Samples(best)} samples ({Samples(best) / SampleRate:0.0} s)" : "Ghost: none saved yet");
            }
            // A restart: a new recording. A lap finished: what was recorded is the best lap, or nothing
            if (race.RestartCount != restartsSeen)
            {
                restartsSeen = race.RestartCount;
                lapsSeen = race.LapsDone;
                recording.Clear();
            }
            if (race.LapsDone != lapsSeen)
            {
                lapsSeen = race.LapsDone;
                if (race.LastLapBest && recording.Count >= Floats * 2)
                {
                    best = recording.ToArray();
                    SaveData.SetString(race.RecordKey("GhostLap"), Save(best));
                    SaveData.Save();
                    Log.Info($"Ghost: the new best lap saved, {Samples(best)} samples");
                }
                recording.Clear();
            }

            if (race.State == RaceManager.RaceState.Racing)
            {
                Record(race.LapTime);
                if (Show && Replay(race.LapTime))
                    return;
            }
            Park();
        }

        // ---- Recording: a sample each time the lap's clock passes the next sample's time
        void Record(float lapTime)
        {
            int next = recording.Count / Floats;
            if (lapTime < next / SampleRate)
                return;
            Add(Car!.Translation);
            Add(Car.Rotation);
            for (int i = 0; i < 4; i++)
            {
                Entity? wheel = carWheels[i];
                Add(wheel != null ? wheel.Translation : Vector3.Zero);
                Add(wheel != null ? wheel.Rotation : Quaternion.Identity);
            }
        }

        void Add(Vector3 v) { recording.Add(v.X); recording.Add(v.Y); recording.Add(v.Z); }
        void Add(Quaternion q) { recording.Add(q.X); recording.Add(q.Y); recording.Add(q.Z); recording.Add(q.W); }

        // ---- Replay: the best lap at this lap time (false: none, or it has finished its lap)
        bool Replay(float lapTime)
        {
            int count = Samples(best);
            float at = lapTime * SampleRate;
            if (count < 2 || at >= count - 1)
                return false;
            int i = (int)at;
            float t = at - i;
            int a = i * Floats, b = (i + 1) * Floats;
            Translation = Vector3.Lerp(Position(a), Position(b), t);
            Rotation = Quaternion.Slerp(Turn(a + 3), Turn(b + 3), t);
            for (int w = 0; w < 4; w++)
            {
                Entity? wheel = ghostWheels[w];
                if (wheel == null)
                    continue;
                int offset = 7 + 7 * w;
                wheel.Translation = Vector3.Lerp(Position(a + offset), Position(b + offset), t);
                wheel.Rotation = Quaternion.Slerp(Turn(a + offset + 3), Turn(b + offset + 3), t);
            }
            parked = false;
            return true;
        }

        Vector3 Position(int i) => new(best[i], best[i + 1], best[i + 2]);
        Quaternion Turn(int i) => Quaternion.Normalize(new Quaternion(best[i], best[i + 1], best[i + 2], best[i + 3]));

        void Park()
        {
            if (parked)
                return;
            Translation = Parked;
            parked = true;
        }

        // ---- Saved as text: "1;rate;samples;" and the floats in base64 (little-endian)
        static int Samples(float[] data) => data.Length / Floats;

        string Save(float[] data)
        {
            byte[] bytes = new byte[data.Length * 4];
            Buffer.BlockCopy(data, 0, bytes, 0, bytes.Length);
            return $"{FormatVersion};{SampleRate.ToString(CultureInfo.InvariantCulture)};{Samples(data)};{Convert.ToBase64String(bytes)}";
        }

        float[] Load(string text)
        {
            string[] parts = text.Split(';');
            if (parts.Length != 4 || parts[0] != FormatVersion.ToString(CultureInfo.InvariantCulture) ||
                !float.TryParse(parts[1], NumberStyles.Float, CultureInfo.InvariantCulture, out float rate) ||
                MathF.Abs(rate - SampleRate) > 1e-3f)
                return Array.Empty<float>(); // none, or of another format or rate: the next best lap records one
            try
            {
                byte[] bytes = Convert.FromBase64String(parts[3]);
                float[] data = new float[bytes.Length / 4];
                Buffer.BlockCopy(bytes, 0, data, 0, data.Length * 4);
                return Samples(data) * Floats == data.Length ? data : Array.Empty<float>();
            }
            catch (FormatException)
            {
                return Array.Empty<float>();
            }
        }
    }
}
