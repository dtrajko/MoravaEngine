using System;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The racing game's main menu (on its canvas): Race, Settings (SettingsMenu) and Quit, the track's records under them
    /// (SaveData, as RaceManager keeps them), and the camera flying along the track behind. The saved settings are put in place
    /// here (GameOptions); the mouse is free and Race is selected, so the gamepad works at once.
    /// </summary>
    public class MainMenu : Entity
    {
        /// <summary>The menu's buttons (a UI Rect: hidden while the Settings screen is open).</summary>
        public Entity? Menu;
        public Entity? RaceButton;
        public Entity? SettingsButton;
        public Entity? QuitButton;
        public Entity? SettingsScreen;
        /// <summary>A UI Text for the track's best race, best lap and medal.</summary>
        public Entity? RecordsText;

        /// <summary>The race's scene (its track's records: those of the center line's version, Centerline.RecordKey).</summary>
        public string RaceScene = "Scenes/Circuit.mscene";

        // ---- The camera behind the menu: along the track's center line (the race's file, its Track entity's place), Height
        // above the road and Side meters to its right, looking LookAhead meters ahead on the road, Speed meters per second;
        // Smoothing: how quickly it follows (higher is tighter), so the turns sweep by instead of jerking
        public Entity? MenuCamera;
        public string Centerline = "Assets/Tracks/Circuit.centerline.json";
        public Entity? Track;
        public float Height = 25.0f;
        public float Side = 12.0f;
        public float LookAhead = 60.0f;
        public float Speed = 30.0f;
        public float Smoothing = 1.5f;
        /// <summary>Meters along the track where the flight begins (0: the start line).</summary>
        public float StartDistance = 0.0f;

        Racing.Centerline? line;
        float distance;
        Vector3 cameraPosition, cameraTarget;
        bool placed;

        protected override void OnCreate()
        {
            GameOptions.ApplyAtStart();
            Input.CursorLocked = false;
            Time.TimeScale = 1;
            if (RaceButton?.GetComponent<UIButton>() is UIButton race) race.Clicked += Race;
            if (SettingsButton?.GetComponent<UIButton>() is UIButton settings) settings.Clicked += OpenSettings;
            if (QuitButton?.GetComponent<UIButton>() is UIButton quit) quit.Clicked += Quit;
            line = Racing.Centerline.Load(Centerline, Track != null ? Track.WorldPosition : Vector3.Zero);
            if (RecordsText?.GetComponent<UIText>() is UIText records)
                records.Text = Records();
            if (line == null)
                Log.Warn($"Main menu: the center line '{Centerline}' was not found: the camera stays where it is");
            distance = StartDistance;
            Show(true);
        }

        // Where the camera wants to be and look at, at this distance along the track
        void Goal(float s, out Vector3 position, out Vector3 target)
        {
            Vector3 road = line!.PositionAt(s, out Vector3 right);
            position = road + right * Side + Vector3.UnitY * Height;
            target = line.PositionAt(s + LookAhead, out _);
        }

        string Records()
        {
            if (line == null)
                return "";
            float bestRace = SaveData.GetFloat(line.RecordKey("BestRace"));
            float bestStart = SaveData.GetFloat(line.RecordKey("BestStartLap"));
            float bestLap = SaveData.GetFloat(line.RecordKey("BestLap"));
            string medal = RaceManager.MedalTitle(SaveData.GetInt(line.RecordKey("Medal")));
            if (bestRace <= 0.0f && bestStart <= 0.0f && bestLap <= 0.0f)
                return "No records yet: 3 laps, beat the medal times";
            return $"Best race {RaceManager.Format(bestRace)}     Lap 1 {RaceManager.Format(bestStart)}     Flying lap {RaceManager.Format(bestLap)}" +
                (medal.Length > 0 ? $"     {medal} medal" : "");
        }

        protected override void OnUpdate(float dt)
        {
            if (SettingsScreen is SettingsMenu settings && settings.IsOpen && Input.WasPressed("Pause"))
                settings.Back();
            if (MenuCamera != null && line != null)
            {
                float unscaled = Time.UnscaledDeltaTime;
                distance += Speed * unscaled;
                Goal(distance, out Vector3 goalPosition, out Vector3 goalTarget);
                if (!placed)
                {
                    cameraPosition = goalPosition;
                    cameraTarget = goalTarget;
                    placed = true;
                }
                // Following its goal smoothly (the same share each second at any frame rate)
                float follow = 1.0f - MathF.Exp(-Smoothing * unscaled);
                cameraPosition = Vector3.Lerp(cameraPosition, goalPosition, follow);
                cameraTarget = Vector3.Lerp(cameraTarget, goalTarget, follow);
                MenuCamera.Translation = cameraPosition; // a root entity: its own transform is the world's
                // Looking at the target, in the Inspector's angles: pitch positive down, yaw positive to the left (0 along -Z)
                Vector3 d = cameraTarget - cameraPosition;
                float yaw = MathF.Atan2(-d.X, -d.Z) * 180.0f / MathF.PI;
                float pitch = MathF.Atan2(-d.Y, MathF.Sqrt(d.X * d.X + d.Z * d.Z)) * 180.0f / MathF.PI;
                MenuCamera.EulerAngles = new Vector3(pitch, yaw, 0.0f);
            }
        }

        void Race()
        {
            Log.Info("Main menu: race");
            Scene.Load(RaceScene);
        }

        void OpenSettings()
        {
            if (SettingsScreen is not SettingsMenu settings)
                return;
            Show(false);
            settings.Open(() =>
            {
                Show(true);
                SettingsButton?.GetComponent<UIButton>()?.Select();
            });
        }

        void Quit()
        {
            Log.Info("Main menu: quit");
            Application.Quit();
        }

        void Show(bool shown)
        {
            if (Menu?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = shown;
            if (shown)
                RaceButton?.GetComponent<UIButton>()?.Select();
        }
    }
}
