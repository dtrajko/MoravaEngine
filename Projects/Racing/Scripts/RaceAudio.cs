using System;
using System.Collections.Generic;
using System.Globalization;
using Morava;

namespace Racing
{
    /// <summary>
    /// The race's sounds (docs/games/Racing_Plan.html, Phase 6). Loops on the car's sound entities, set every frame:
    /// - The engine: two banks of loops, on and off the throttle, each a loop every 500 rpm (Tools/Audio/GenerateEngineSounds.py).
    ///   In each bank the two loops nearest the engine's rpm play, cross-faded, each pitched by rpm / its rpm (a few percent
    ///   at most, so nothing is stretched far); the banks are cross-faded by the throttle. An upshift dips the engine for
    ///   a moment and clicks.
    /// - The tires: louder the more they slide or spin. The wall: while the car scrapes one, louder the faster it goes.
    /// One-shots from the race's state: a beep at each start light (a higher, longer one at green), a chime at each split
    /// (higher when faster than the best lap's), the finish. The short sounds are made by Tools/Audio/GenerateRaceSounds.py.
    /// A loop whose AudioSource has no Clip stays silent.
    /// </summary>
    public class RaceAudio : Entity
    {
        /// <summary>Entities with a looping AudioSource each (children of the car): two per engine bank (its loops blend).</summary>
        public Entity? EngineOnA;
        public Entity? EngineOnB;
        public Entity? EngineOffA;
        public Entity? EngineOffB;
        public Entity? TireSound;
        public Entity? WallSound;

        // ---- The engine
        /// <summary>
        /// The loops on and off the throttle, low rpm to high, each as file@rpm: the rpm it was made at (the generator writes
        /// the list in Assets/Audio/EngineLoops.txt).
        /// </summary>
        public string EngineLoopsOn = Bank("On");
        public string EngineLoopsOff = Bank("Off");
        /// <summary>The engine's volume.</summary>
        public float EngineVolume = 0.85f;
        /// <summary>How fast the sound follows the throttle (the banks' cross-fade): higher is faster.</summary>
        public float ThrottleSmoothing = 10.0f;
        /// <summary>An upshift: the engine's volume dips to this share for DipSeconds, and the click plays.</summary>
        public float ShiftDip = 0.45f;
        public float DipSeconds = 0.25f;
        public string ShiftClick = "Assets/Audio/ShiftClick.wav";
        public float ShiftClickVolume = 0.35f;

        // ---- The tires: degrees of sideways slide where the squeal starts and where it is loudest
        public float SlipStart = 5.0f;
        public float SlipFull = 16.0f;
        public float TireVolume = 0.7f;

        // ---- The wall: km/h at which the scrape is loudest
        public float ScrapeFullSpeed = 150.0f;
        public float WallVolume = 0.8f;

        // ---- The one-shots
        public string LightRed = "Assets/Audio/LightRed.wav";
        public string LightYellow = "Assets/Audio/LightYellow.wav";
        public string LightGreen = "Assets/Audio/LightGreen.wav";
        public string Checkpoint = "Assets/Audio/Checkpoint.wav";
        public string CheckpointFaster = "Assets/Audio/CheckpointFaster.wav";
        public string Finish = "Assets/Audio/Finish.wav";
        public float OneShotVolume = 0.8f;

        // The generator's loops: every 500 rpm from 1000 to 7000 (its exact rpms differ by under 0.1 %)
        static string Bank(string load)
        {
            var items = new List<string>();
            for (int rpm = 1000; rpm <= 7000; rpm += 500)
                items.Add($"Assets/Audio/Engine_{load}_{rpm}.wav@{rpm}");
            return string.Join(", ", items);
        }

        // A bank: its loops and its two sources (even loops in A, odd ones in B: a pair always has one in each, and a
        // source changes its loop only while its weight is 0)
        sealed class EngineBank
        {
            public string[] Files = Array.Empty<string>();
            public float[] Rpms = Array.Empty<float>();
            public AudioSource? A, B;
            int loopA = -1, loopB = -1;

            public static EngineBank Parse(string list, Entity? a, Entity? b, string field)
            {
                var bank = new EngineBank { A = a?.GetComponent<AudioSource>(), B = b?.GetComponent<AudioSource>() };
                var files = new List<string>();
                var rpms = new List<float>();
                foreach (string item in list.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
                {
                    int at = item.LastIndexOf('@');
                    if (at <= 0 || !float.TryParse(item[(at + 1)..], NumberStyles.Float, CultureInfo.InvariantCulture, out float rpm) || rpm <= 0.0f)
                    {
                        Log.Error($"RaceAudio: '{item}' in {field} isn't file@rpm");
                        continue;
                    }
                    files.Add(item[..at]);
                    rpms.Add(rpm);
                }
                bank.Files = files.ToArray();
                bank.Rpms = rpms.ToArray();
                return bank;
            }

            void Set(AudioSource? source, ref int current, int loop)
            {
                if (source == null || loop == current || loop < 0 || loop >= Files.Length)
                    return;
                current = loop;
                source.Loop = true;
                source.Clip = Files[loop];
                source.Play();
            }

            /// <summary>The two loops around the rpm, cross-faded (equal power), at this volume, each pitched by rpm / its rpm.</summary>
            public void Update(float rpm, float volume)
            {
                int n = Files.Length;
                if (n == 0 || A == null || B == null)
                    return;
                int low = 0;
                while (low < n - 2 && rpm > Rpms[low + 1])
                    low++;
                int high = Math.Min(low + 1, n - 1);
                float blend = high == low ? 0.0f : Math.Clamp((rpm - Rpms[low]) / (Rpms[high] - Rpms[low]), 0.0f, 1.0f);
                bool lowInA = low % 2 == 0;
                Set(A, ref loopA, lowInA ? low : high);
                Set(B, ref loopB, lowInA ? high : low);
                float weightA = lowInA ? 1.0f - blend : blend;
                if (high == low)
                    weightA = 1.0f;
                A.Volume = volume * MathF.Sqrt(weightA);
                B.Volume = volume * MathF.Sqrt(1.0f - weightA);
                float r = MathF.Max(rpm, 1.0f);
                if (loopA >= 0) A.Pitch = Math.Clamp(r / Rpms[loopA], 0.5f, 2.0f);
                if (loopB >= 0) B.Pitch = Math.Clamp(r / Rpms[loopB], 0.5f, 2.0f);
            }
        }

        EngineBank? on, off;
        AudioSource? tires, wall;
        float tireVolume, wallVolume, engineRpm = 1000.0f, load, dipTime = float.MaxValue;
        int gear;
        int lightsOn, splitCount = -1, restartCount = -1;
        RaceManager.RaceState state = RaceManager.RaceState.Lights;

        protected override void OnCreate()
        {
            on = EngineBank.Parse(EngineLoopsOn, EngineOnA, EngineOnB, nameof(EngineLoopsOn));
            off = EngineBank.Parse(EngineLoopsOff, EngineOffA, EngineOffB, nameof(EngineLoopsOff));
            tires = Start(TireSound);
            wall = Start(WallSound);
        }

        // The loop playing at no volume (a source without a clip: none)
        static AudioSource? Start(Entity? entity)
        {
            if (entity?.GetComponent<AudioSource>() is not AudioSource source || source.Clip.Length == 0)
                return null;
            source.Loop = true;
            source.Volume = 0.0f;
            source.Play();
            return source;
        }

        protected override void OnUpdate(float dt)
        {
            bool paused = dt <= 0.0f;
            UpdateLoops(paused);
            if (RaceManager.Current is RaceManager race)
                UpdateOneShots(race);
        }

        void UpdateLoops(bool paused)
        {
            CarController? car = CarController.Current;
            Vehicle? vehicle = car?.GetComponent<Vehicle>();
            float real = Time.UnscaledDeltaTime;
            float k = 1.0f - MathF.Exp(-12.0f * real);
            float wantedTires = 0.0f, wantedWall = 0.0f, volume = 0.0f;
            if (vehicle != null && !paused)
            {
                float throttle = Math.Clamp(vehicle.AppliedForward, 0.0f, 1.0f);
                load += (throttle - load) * (1.0f - MathF.Exp(-ThrottleSmoothing * real));
                engineRpm += (vehicle.EngineRpm - engineRpm) * (1.0f - MathF.Exp(-25.0f * real));
                // An upshift: the dip and the click
                int g = vehicle.Gear;
                if (g > gear && gear > 0)
                {
                    dipTime = 0.0f;
                    Audio.PlayOneShot(ShiftClick, ShiftClickVolume);
                }
                gear = g;
                dipTime += real;
                float dip = dipTime < DipSeconds ? 1.0f - (1.0f - ShiftDip) * MathF.Sin(MathF.PI * dipTime / DipSeconds) : 1.0f;
                volume = EngineVolume * dip;

                float kmh = MathF.Abs(vehicle.Speed) * 3.6f;
                if (!vehicle.IsInAir && kmh > 10.0f)
                {
                    float slide = Math.Clamp((vehicle.LateralSlip - SlipStart) / MathF.Max(SlipFull - SlipStart, 0.1f), 0.0f, 1.0f);
                    float spin = Math.Clamp((vehicle.LongitudinalSlip - 0.2f) / 0.4f, 0.0f, 1.0f);
                    wantedTires = TireVolume * MathF.Max(slide, spin);
                }
                if (car!.TouchingWall)
                    wantedWall = WallVolume * Math.Clamp(kmh / MathF.Max(ScrapeFullSpeed, 1.0f), 0.2f, 1.0f);
            }
            // Paused or no car: the engine silent (the loops keep their places)
            on?.Update(engineRpm, volume * MathF.Sqrt(load));
            off?.Update(engineRpm, volume * MathF.Sqrt(1.0f - load));
            tireVolume += (wantedTires - tireVolume) * k;
            wallVolume += (wantedWall - wallVolume) * k;
            if (tires != null) { tires.Volume = tireVolume; tires.Pitch = 0.9f + 0.2f * tireVolume / MathF.Max(TireVolume, 0.01f); }
            if (wall != null) wall.Volume = wallVolume;
        }

        void UpdateOneShots(RaceManager race)
        {
            if (race.RestartCount != restartCount)
            {
                restartCount = race.RestartCount;
                splitCount = race.SplitCount;
                lightsOn = race.LightsOn;
                state = race.State;
            }
            if (race.LightsOn != lightsOn)
            {
                lightsOn = race.LightsOn;
                string clip = lightsOn switch { 1 => LightRed, 2 => LightYellow, 3 => LightGreen, _ => "" };
                if (clip.Length > 0)
                    Audio.PlayOneShot(clip, OneShotVolume);
            }
            if (race.SplitCount != splitCount)
            {
                splitCount = race.SplitCount;
                bool faster = !float.IsNaN(race.LastSplitDelta) && race.LastSplitDelta <= 0.0f;
                if (race.State != RaceManager.RaceState.Finished)
                    Audio.PlayOneShot(faster ? CheckpointFaster : Checkpoint, OneShotVolume);
            }
            if (race.State != state)
            {
                state = race.State;
                if (state == RaceManager.RaceState.Finished)
                    Audio.PlayOneShot(Finish, OneShotVolume);
            }
        }
    }
}
