using System;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The race's HUD (docs/games/Racing_Plan.html, Phase 5), on its canvas: two analog dials (the speedometer to MaxSpeed
    /// km/h and the rev counter to the engine's limit, Max RPM plus Over-Rev RPM, its last RedZoneRpm red: the car's 6000 to
    /// 8000, the top of the throttle's range and the over-rev band only downshifts reach), the lap and the times, a
    /// popup at each split (the time and the difference to the best lap's there), banners (GO, FINAL LAP), the pulsing
    /// wrong-way warning and the finish screen (the time, the laps, the medal, a new record; Race again, Main menu).
    /// With the manual gearbox, two shift paddles over the rev counter (- down, + up) light up at each press (PaddleShifted;
    /// PaddleRefused when the gear can't change: the top gear, first, or too fast to go down).
    ///
    /// It only reads: the race's state from RaceManager.Current, the car's from CarController.Current.
    ///
    /// The dials' ticks and numbers are UI entities made in the scene, as many as the longest scale needs (Speed Tick 1...,
    /// Speed Label 1..., Rev Tick 1..., Rev Label 1...): this script turns and colors them for the car, and hides the rest.
    /// A tick is a rectangle whose pivot lies at the dial's center (Pivot.Y beyond 1), so its Rotation swings it around the
    /// dial; the needles turn the same way.
    /// </summary>
    public class RaceHud : Entity
    {
        // ---- The dials
        /// <summary>The speedometer's end, km/h.</summary>
        public float MaxSpeed = 360.0f;
        /// <summary>km/h between the speedometer's long ticks (numbered every second one) and its short ones.</summary>
        public float SpeedMajor = 20.0f;
        public float SpeedMinor = 10.0f;
        /// <summary>Degrees the dials sweep, clockwise from the bottom left: 0 at -Sweep/2 (up is 0).</summary>
        public float Sweep = 270.0f;
        /// <summary>How fast the needles follow: higher is faster.</summary>
        public float NeedleSmoothing = 18.0f;

        public Entity? SpeedNeedle;
        public Entity? SpeedText;
        public Entity? RevNeedle;
        public Entity? GearText;

        // ---- The shift paddles (shown with the manual gearbox)
        public Entity? PaddleDown;
        public Entity? PaddleDownText;
        public Entity? PaddleUp;
        public Entity? PaddleUpText;
        public Vector4 PaddleIdle = new(0.3f, 0.32f, 0.38f, 0.85f);
        public Vector4 PaddleShifted = new(0.92f, 0.12f, 0.16f, 1.0f);
        public Vector4 PaddleRefused = new(1.0f, 0.62f, 0.12f, 1.0f);
        /// <summary>Seconds a paddle stays lit after a press (its second half fades), and how far it moves in while pressed.</summary>
        public float PaddleSeconds = 0.35f;
        public float PaddlePress = 5.0f;
        /// <summary>The ticks' distance from a dial's center to their outer end, and the numbers' (reference pixels).</summary>
        public float SpeedTickRadius = 142.0f;
        public float SpeedLabelRadius = 104.0f;
        public float RevTickRadius = 110.0f;
        public float RevLabelRadius = 66.0f;
        /// <summary>
        /// The red zone's band under the rev counter's ticks (Rev Zone 1..., drawn before the ticks, which stay light over
        /// it): its outer radius, and its width at the zone's start and at the end of the scale (equal: a band of even depth).
        /// </summary>
        public float RevZoneRadius = 111.0f;
        /// <summary>The red zone's span: the rev counter's last this many rpm (the scale ends at Max RPM, up to a thousand).</summary>
        public float RedZoneRpm = 2000.0f;
        public float RevZoneWidth = 19.0f;
        public float RevZoneWidthEnd = 19.0f;

        public Vector4 TickColor = new(0.92f, 0.94f, 1.0f, 0.95f);
        public Vector4 MinorTickColor = new(0.7f, 0.74f, 0.82f, 0.7f);
        public Vector4 RedZone = new(1.0f, 0.18f, 0.16f, 1.0f);

        // ---- The lap and the times
        public Entity? LapText;
        public Entity? RaceTimeText;
        public Entity? LapTimeText;
        /// <summary>The best of the lap being driven's kind (lap 1's, then the flying laps'), and its label (BEST LAP 1, BEST LAP).</summary>
        public Entity? BestLapText;
        public Entity? BestLapLabel;

        // ---- The popups
        /// <summary>The split popup's panel (hidden between splits), its texts: the split's name and time, the difference.</summary>
        public Entity? SplitPanel;
        public Entity? SplitText;
        public Entity? SplitDeltaText;
        /// <summary>Seconds a split shows (the last half second fades).</summary>
        public float SplitSeconds = 3.0f;
        public Vector4 Faster = new(0.3f, 0.95f, 0.45f, 1.0f);
        public Vector4 Slower = new(1.0f, 0.32f, 0.26f, 1.0f);

        /// <summary>A big text in the middle: GO, FINAL LAP.</summary>
        public Entity? Banner;
        public Entity? WrongWayText;

        // ---- The finish screen
        public Entity? FinishScreen;
        public Entity? FinishTime;
        public Entity? FinishRecord;
        public Entity? FinishMedal;
        public Entity? FinishLaps;
        public Entity? RaceAgainButton;
        public Entity? MainMenuButton;
        public string MainMenuScene = "Scenes/MainMenu.mscene";
        /// <summary>Seconds after the line before the finish screen shows.</summary>
        public float FinishDelay = 1.5f;

        /// <summary>The finish screen shows (the pause menu waits while it does).</summary>
        public static bool FinishShown { get; private set; }

        UIRect? speedNeedle, revNeedle, splitPanel, banner, finishScreen;
        UIText? speedText, gearText, lapText, raceTimeText, lapTimeText, bestLapText, splitText, splitDeltaText, bannerText, wrongWayText;
        Entity[] speedTicks = Array.Empty<Entity>(), speedLabels = Array.Empty<Entity>();
        Entity[] revTicks = Array.Empty<Entity>(), revLabels = Array.Empty<Entity>(), revZone = Array.Empty<Entity>();
        float revEnd;           // the rev counter's end (the car's Max RPM up to a whole thousand)
        bool revLaidOut;
        float shownSpeed, shownRpm;
        int splitCount = -1, restartCount = -1;
        float splitAge = float.MaxValue, bannerAge = float.MaxValue, bannerLength;
        Vector4 bannerColor;
        RaceManager.RaceState lastState = RaceManager.RaceState.Lights;
        int lastLap = 1;
        float finishedFor = -1.0f;
        bool cursorWasLocked;

        // A paddle: its image, its text, where it rests, its last press
        sealed class Paddle
        {
            public UIRect? Rect, TextRect;
            public UIImage? Image;
            public Vector2 Rest, TextRest, In; // In: towards the dial's center
            public int Presses = -1;
            public float Age = float.MaxValue;
            public bool Refused;
        }
        Paddle down = new(), up = new();

        protected override void OnCreate()
        {
            FinishShown = false;
            speedNeedle = SpeedNeedle?.GetComponent<UIRect>();
            revNeedle = RevNeedle?.GetComponent<UIRect>();
            splitPanel = SplitPanel?.GetComponent<UIRect>();
            banner = Banner?.GetComponent<UIRect>();
            finishScreen = FinishScreen?.GetComponent<UIRect>();
            speedText = SpeedText?.GetComponent<UIText>();
            gearText = GearText?.GetComponent<UIText>();
            lapText = LapText?.GetComponent<UIText>();
            raceTimeText = RaceTimeText?.GetComponent<UIText>();
            lapTimeText = LapTimeText?.GetComponent<UIText>();
            bestLapText = BestLapText?.GetComponent<UIText>();
            splitText = SplitText?.GetComponent<UIText>();
            splitDeltaText = SplitDeltaText?.GetComponent<UIText>();
            bannerText = Banner?.GetComponent<UIText>();
            wrongWayText = WrongWayText?.GetComponent<UIText>();
            speedTicks = FindNumbered("Speed Tick");
            speedLabels = FindNumbered("Speed Label");
            revTicks = FindNumbered("Rev Tick");
            revLabels = FindNumbered("Rev Label");
            revZone = FindNumbered("Rev Zone");
            LayOutSpeedometer();
            down = MakePaddle(PaddleDown, PaddleDownText);
            up = MakePaddle(PaddleUp, PaddleUpText);

            if (RaceAgainButton?.GetComponent<UIButton>() is UIButton again)
                again.Clicked += RaceAgain;
            if (MainMenuButton?.GetComponent<UIButton>() is UIButton menu)
                menu.Clicked += ToMainMenu;
            SetVisible(splitPanel, false);
            SetVisible(banner, false);
            SetVisible(finishScreen, false);
        }

        // Entities named "<prefix> 1", "<prefix> 2"... up to the first missing one
        static Entity[] FindNumbered(string prefix)
        {
            var found = new System.Collections.Generic.List<Entity>();
            for (int i = 1; ; i++)
            {
                Entity? e = Scene.FindEntityByName($"{prefix} {i}");
                if (e == null)
                    break;
                found.Add(e);
            }
            return found.ToArray();
        }

        float Angle(float t) => -0.5f * Sweep + Sweep * Math.Clamp(t, 0.0f, 1.0f);

        // A tick at t (0 to 1) of a dial: its outer end radius from the center, length long, width wide
        void PlaceTick(Entity tick, float t, float radius, Vector2 size, Vector4 color)
        {
            if (tick.GetComponent<UIRect>() is not UIRect rect)
                return;
            rect.Visible = true;
            rect.Size = size;
            rect.Pivot = new Vector2(0.5f, radius / MathF.Max(size.Y, 1.0f)); // the dial's center
            rect.Position = Vector2.Zero;
            rect.Rotation = Angle(t);
            if (tick.GetComponent<UIImage>() is UIImage image)
                image.Color = color;
        }

        // A number at t of a dial, upright, at radius from the center
        void PlaceLabel(Entity label, float t, float radius, string text, Vector4 color)
        {
            if (label.GetComponent<UIRect>() is not UIRect rect)
                return;
            float a = Angle(t) * MathF.PI / 180.0f;
            rect.Visible = true;
            rect.Position = new Vector2(MathF.Sin(a), -MathF.Cos(a)) * radius; // y down: up is -y
            if (label.GetComponent<UIText>() is UIText ui)
            {
                ui.Text = text;
                ui.Color = color;
            }
        }

        // A band along a dial's edge from t0 to t1, its outer edge at radius, growing inward from width to widthEnd: its
        // segments side by side, each turned to the circle
        void LayOutBand(Entity[] segments, float t0, float t1, float radius, float width, float widthEnd, Vector4 color)
        {
            int n = segments.Length;
            if (n == 0 || t1 <= t0)
            {
                Hide(segments, 0);
                return;
            }
            float step = Sweep * (t1 - t0) / n; // degrees per segment
            float length = 2.0f * radius * MathF.Sin(step * 0.5f * MathF.PI / 180.0f) + 1.0f; // a pixel over: no gaps
            for (int i = 0; i < n; i++)
            {
                if (segments[i].GetComponent<UIRect>() is not UIRect rect)
                    continue;
                rect.Visible = true;
                float w = width + (widthEnd - width) * (i + 0.5f) / n;
                rect.Size = new Vector2(length, w);
                rect.Pivot = new Vector2(0.5f, radius / w);
                rect.Position = Vector2.Zero;
                rect.Rotation = Angle(t0) + step * (i + 0.5f);
                if (segments[i].GetComponent<UIImage>() is UIImage image)
                    image.Color = color;
            }
        }

        static void Hide(Entity[] entities, int from)
        {
            for (int i = from; i < entities.Length; i++)
                SetVisible(entities[i].GetComponent<UIRect>(), false);
        }

        void LayOutSpeedometer()
        {
            int ticks = (int)MathF.Round(MaxSpeed / MathF.Max(SpeedMinor, 1.0f)) + 1;
            int perMajor = Math.Max(1, (int)MathF.Round(SpeedMajor / MathF.Max(SpeedMinor, 1.0f)));
            int used = Math.Min(ticks, speedTicks.Length), labels = 0;
            for (int i = 0; i < used; i++)
            {
                float t = i / (float)(ticks - 1);
                bool major = i % perMajor == 0;
                PlaceTick(speedTicks[i], t, SpeedTickRadius, major ? new Vector2(5, 24) : new Vector2(3, 13), major ? TickColor : MinorTickColor);
                if (major && (i / perMajor) % 2 == 0 && labels < speedLabels.Length)
                    PlaceLabel(speedLabels[labels++], t, SpeedLabelRadius, $"{i * SpeedMinor:0}", TickColor);
            }
            Hide(speedTicks, used);
            Hide(speedLabels, labels);
        }

        // The rev counter to the car's Max RPM (up to a whole thousand), as a Smiths rev counter: a long tick and a number per
        // thousand, a middle one per 500 and a short one per 100 (per 500 only when the scene has too few ticks for that).
        // Red for the scale's last RedZoneRpm (8000: from 6), not tied to the shift point: a red band under the ticks there,
        // the numbers red
        void LayOutRevCounter(float maxRpm, float shiftUpRpm)
        {
            revEnd = MathF.Max(1000.0f, MathF.Ceiling(maxRpm / 1000.0f) * 1000.0f);
            float step = revTicks.Length >= (int)(revEnd / 100.0f) + 1 ? 100.0f : 500.0f;
            int ticks = (int)MathF.Round(revEnd / step) + 1;
            int used = Math.Min(ticks, revTicks.Length), labels = 0;
            float red = revEnd - MathF.Max(RedZoneRpm, 0.0f);
            for (int i = 0; i < used; i++)
            {
                float rpm = i * step, t = rpm / revEnd;
                bool thousand = MathF.Abs(rpm % 1000.0f) < 1.0f, half = MathF.Abs(rpm % 500.0f) < 1.0f;
                bool inRed = rpm >= red - 1.0f;
                Vector2 size = thousand ? new Vector2(5, 18) : half ? new Vector2(3.5f, 12) : new Vector2(2, 7);
                Vector4 color = thousand || half ? TickColor : MinorTickColor; // light over the red band too
                PlaceTick(revTicks[i], t, RevTickRadius, size, color);
                if (thousand && labels < revLabels.Length)
                    PlaceLabel(revLabels[labels++], t, RevLabelRadius, $"{rpm / 1000.0f:0}", inRed ? RedZone : TickColor);
            }
            Hide(revTicks, used);
            Hide(revLabels, labels);
            LayOutBand(revZone, red / revEnd, 1.0f, RevZoneRadius, RevZoneWidth, RevZoneWidthEnd, RedZone);
            revLaidOut = true;
            Log.Info($"HUD: rev counter to {revEnd:0} rpm (red from {red:0}, ticks every {step:0}), speedometer to {MaxSpeed:0} km/h");
        }

        protected override void OnUpdate(float dt)
        {
            RaceManager? race = RaceManager.Current;
            CarController? car = CarController.Current;
            float real = Time.UnscaledDeltaTime;
            if (car?.GetComponent<Vehicle>() is Vehicle vehicle)
                UpdateDials(vehicle, dt);
            UpdatePaddles(car, real);
            if (race == null)
                return;

            if (race.RestartCount != restartCount)
            {
                // A new start: no popups left from the last race
                restartCount = race.RestartCount;
                splitCount = race.SplitCount;
                splitAge = bannerAge = float.MaxValue;
                SetVisible(splitPanel, false);
                SetVisible(banner, false);
                ShowFinish(false);
                finishedFor = -1.0f;
                lastLap = race.Lap;
            }

            // The lap and the times
            SetText(lapText, $"{race.Lap}/{race.Laps}");
            SetText(raceTimeText, Clock(race.RaceTime));
            SetText(lapTimeText, race.State == RaceManager.RaceState.Racing ? Clock(race.LapTime) : "-");
            SetText(bestLapText, RaceManager.Format(race.BestOfThisLap));
            SetText(BestLapLabel?.GetComponent<UIText>(), race.StartLap ? "BEST LAP 1" : "BEST LAP");

            // GO when the light turns green, FINAL LAP when it starts
            if (lastState == RaceManager.RaceState.Lights && race.State == RaceManager.RaceState.Racing)
                ShowBanner("GO!", Faster, 1.0f);
            if (race.State == RaceManager.RaceState.Racing && race.Lap != lastLap && race.Lap == race.Laps && race.Laps > 1)
                ShowBanner("FINAL LAP", new Vector4(1.0f, 0.85f, 0.25f, 1.0f), 2.0f);
            lastLap = race.Lap;
            lastState = race.State;

            if (race.SplitCount != splitCount)
            {
                splitCount = race.SplitCount;
                ShowSplit(race);
            }
            UpdatePopups(real);

            if (wrongWayText != null && race.WrongWay)
            {
                float pulse = 0.6f + 0.4f * MathF.Abs(MathF.Sin((float)Time.UnscaledTime * 5.0f));
                wrongWayText.Color = new Vector4(1.0f, 0.22f, 0.12f, pulse);
            }

            // The finish screen, a moment after the line
            if (race.State == RaceManager.RaceState.Finished)
            {
                bool before = finishedFor < FinishDelay;
                finishedFor = MathF.Max(finishedFor, 0.0f) + real;
                if (before && finishedFor >= FinishDelay)
                    FillFinish(race);
            }
        }

        void UpdateDials(Vehicle vehicle, float dt)
        {
            if (!revLaidOut)
                LayOutRevCounter(MathF.Max(vehicle.MaxRpm, vehicle.RevLimitRpm), vehicle.ShiftUpRpm);
            float speed = MathF.Abs(vehicle.Speed) * 3.6f;
            float rpm = vehicle.EngineRpm;
            float k = 1.0f - MathF.Exp(-NeedleSmoothing * MathF.Max(dt, 0.0f)); // paused: the needles stand
            shownSpeed += (speed - shownSpeed) * k;
            shownRpm += (rpm - shownRpm) * k;
            if (speedNeedle != null)
                speedNeedle.Rotation = Angle(shownSpeed / MathF.Max(MaxSpeed, 1.0f));
            if (revNeedle != null)
                revNeedle.Rotation = Angle(shownRpm / MathF.Max(revEnd, 1.0f));
            SetText(speedText, $"{speed:0}");
            int gear = vehicle.Gear;
            SetText(gearText, gear < 0 ? "R" : gear == 0 ? "N" : gear.ToString());
        }

        Paddle MakePaddle(Entity? image, Entity? text)
        {
            var paddle = new Paddle { Rect = image?.GetComponent<UIRect>(), Image = image?.GetComponent<UIImage>(), TextRect = text?.GetComponent<UIRect>() };
            if (paddle.Rect != null)
            {
                paddle.Rest = paddle.Rect.Position;
                paddle.In = paddle.Rest.LengthSquared() > 0.0f ? -Vector2.Normalize(paddle.Rest) : Vector2.Zero;
            }
            if (paddle.TextRect != null)
                paddle.TextRest = paddle.TextRect.Position;
            return paddle;
        }

        // The paddles: shown with the manual gearbox, lit at each press (refused: the other color), fading back
        void UpdatePaddles(CarController? car, float real)
        {
            bool shown = car != null && car.ManualGearbox;
            Light(down, shown, car?.DownPresses ?? 0, car?.LastDownRefused ?? false, real);
            Light(up, shown, car?.UpPresses ?? 0, car?.LastUpRefused ?? false, real);
        }

        void Light(Paddle paddle, bool shown, int presses, bool refused, float real)
        {
            SetVisible(paddle.Rect, shown);
            SetVisible(paddle.TextRect, shown);
            if (presses != paddle.Presses)
            {
                if (paddle.Presses >= 0)
                    paddle.Age = 0.0f; // not at the start: the count from before the HUD
                paddle.Presses = presses;
                paddle.Refused = refused;
            }
            else
                paddle.Age += real;
            if (!shown)
                return;
            float seconds = MathF.Max(PaddleSeconds, 0.01f);
            float lit = paddle.Age < seconds ? Math.Clamp(2.0f * (1.0f - paddle.Age / seconds), 0.0f, 1.0f) : 0.0f; // its second half fades
            if (paddle.Image != null)
                paddle.Image.Color = Vector4.Lerp(PaddleIdle, paddle.Refused ? PaddleRefused : PaddleShifted, lit);
            Vector2 pressed = paddle.In * (PaddlePress * lit);
            if (paddle.Rect != null)
                paddle.Rect.Position = paddle.Rest + pressed;
            if (paddle.TextRect != null)
                paddle.TextRect.Position = paddle.TextRest + pressed;
        }

        void ShowSplit(RaceManager race)
        {
            bool lap = race.LastSplitCheckpoint >= race.Sectors;
            string name = lap ? (race.LastLapBest ? (race.LastLapStart ? "BEST LAP 1" : "BEST LAP") : "LAP") : $"CHECKPOINT {race.LastSplitCheckpoint}";
            SetText(splitText, $"{name}   {RaceManager.Format(race.LastSplit)}");
            float delta = race.LastSplitDelta;
            if (splitDeltaText != null)
            {
                // No best lap to compare with (the first lap ever, or this lap is now the best): no difference shown
                splitDeltaText.Text = float.IsNaN(delta) ? "" : $"{(delta <= 0.0f ? "-" : "+")}{MathF.Abs(delta):0.000}";
                splitDeltaText.Color = delta <= 0.0f ? Faster : Slower;
            }
            splitAge = 0.0f;
            SetVisible(splitPanel, true);
        }

        void ShowBanner(string text, Vector4 color, float seconds)
        {
            SetText(bannerText, text);
            bannerColor = color;
            bannerAge = 0.0f;
            bannerLength = seconds;
            SetVisible(banner, true);
        }

        void UpdatePopups(float real)
        {
            if (splitAge < float.MaxValue)
            {
                splitAge += real;
                float fade = Math.Clamp((SplitSeconds - splitAge) / 0.5f, 0.0f, 1.0f);
                if (splitText != null) splitText.Color = new Vector4(1, 1, 1, fade);
                if (splitDeltaText != null) { Vector4 c = splitDeltaText.Color; splitDeltaText.Color = new Vector4(c.X, c.Y, c.Z, fade); }
                if (splitPanel != null && SplitPanel?.GetComponent<UIImage>() is UIImage image)
                    image.Color = new Vector4(0.03f, 0.035f, 0.05f, 0.78f * fade);
                if (splitAge >= SplitSeconds)
                {
                    splitAge = float.MaxValue;
                    SetVisible(splitPanel, false);
                }
            }
            if (bannerAge < float.MaxValue)
            {
                bannerAge += real;
                float fade = Math.Clamp((bannerLength - bannerAge) / 0.4f, 0.0f, 1.0f);
                if (bannerText != null)
                    bannerText.Color = new Vector4(bannerColor.X, bannerColor.Y, bannerColor.Z, fade);
                if (bannerAge >= bannerLength)
                {
                    bannerAge = float.MaxValue;
                    SetVisible(banner, false);
                }
            }
        }

        // ---- The finish screen

        void FillFinish(RaceManager race)
        {
            SetText(FinishTime?.GetComponent<UIText>(), RaceManager.Format(race.RaceTime));
            SetText(FinishRecord?.GetComponent<UIText>(), race.NewRecord ? "NEW RECORD" : $"Best race {RaceManager.Format(race.BestRace)}");
            if (FinishMedal?.GetComponent<UIText>() is UIText medal)
            {
                string name = RaceManager.MedalTitle(race.Medal);
                // The next medal's time to beat, when there is one
                int next = race.Medal + 1;
                float nextTime = next <= 4 ? race.MedalTime(next) : 0.0f;
                string goal = nextTime > 0.0f ? $"   ({RaceManager.MedalTitle(next)} {RaceManager.Format(nextTime)})" : "";
                medal.Text = (name.Length > 0 ? $"{name.ToUpperInvariant()} MEDAL" : "No medal") + goal;
                medal.Color = race.Medal switch
                {
                    1 => new Vector4(0.86f, 0.55f, 0.3f, 1.0f),
                    2 => new Vector4(0.82f, 0.86f, 0.92f, 1.0f),
                    3 => new Vector4(1.0f, 0.82f, 0.25f, 1.0f),
                    4 => new Vector4(0.35f, 0.95f, 0.55f, 1.0f),
                    _ => new Vector4(1.0f, 1.0f, 1.0f, 0.75f),
                };
            }
            if (FinishLaps?.GetComponent<UIText>() is UIText laps)
            {
                // "fastest": of the flying laps (lap 1, from standing, is always the slowest)
                float[] times = race.LapTimes;
                float fastest = float.MaxValue;
                for (int i = 1; i < times.Length; i++)
                    fastest = MathF.Min(fastest, times[i]);
                var lines = new System.Text.StringBuilder();
                for (int i = 0; i < times.Length; i++)
                    lines.Append($"Lap {i + 1}   {RaceManager.Format(times[i])}{(i > 0 && times[i] == fastest && times.Length > 2 ? "   fastest" : "")}\n");
                lines.Append($"Best lap 1 ever   {RaceManager.Format(race.BestStartLap)}\n");
                lines.Append($"Best flying lap ever   {RaceManager.Format(race.BestLap)}");
                laps.Text = lines.ToString();
            }
            ShowFinish(true);
            Log.Info($"HUD: the finish screen ({RaceManager.Format(race.RaceTime)}, {(race.NewRecord ? "a new record" : "no record")})");
        }

        void ShowFinish(bool shown)
        {
            if (FinishShown == shown)
                return;
            FinishShown = shown;
            RaceManager.KeysBlocked = shown;
            SetVisible(finishScreen, shown);
            if (shown)
            {
                cursorWasLocked = Input.CursorLocked;
                Input.CursorLocked = false; // the mouse for the buttons
                RaceAgainButton?.GetComponent<UIButton>()?.Select();
            }
        }

        void RaceAgain()
        {
            Log.Info("Finish screen: race again");
            ShowFinish(false);
            Input.CursorLocked = cursorWasLocked;
            RaceManager.Current?.RequestRestart();
        }

        void ToMainMenu()
        {
            Log.Info("Finish screen: main menu");
            FinishShown = false;
            RaceManager.KeysBlocked = false;
            Scene.Load(MainMenuScene);
        }

        // ---- Helpers

        // A running clock, in thousandths as every time: 0.000 at the start (RaceManager.Format shows "-" for none)
        static string Clock(float seconds) => seconds > 0.0f ? RaceManager.Format(seconds) : "0.000";

        static void SetVisible(UIRect? rect, bool shown)
        {
            if (rect != null && rect.Visible != shown)
                rect.Visible = shown;
        }

        static void SetText(UIText? text, string value)
        {
            if (text != null && text.Text != value)
                text.Text = value;
        }
    }
}
