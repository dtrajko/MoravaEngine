using System;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The racing game's main menu (on its canvas): Race, Settings (SettingsMenu) and Quit, the track's records under them
    /// (SaveData, as RaceManager keeps them), and the camera circling the track behind. The saved settings are put in place
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

        /// <summary>The race's scene, and its track's name in the records (the center line's Name).</summary>
        public string RaceScene = "Scenes/TestTrack.mscene";
        public string TrackName = "TestTrack";

        // ---- The camera behind the menu: around Center, Radius away, Height above it, Speed degrees per second
        public Entity? MenuCamera;
        public Vector3 Center = new(0.0f, 46.0f, 40.0f);
        public float Radius = 150.0f;
        public float Height = 40.0f;
        public float Speed = 4.0f;

        float angle = 30.0f;

        protected override void OnCreate()
        {
            GameOptions.ApplyAtStart();
            Input.CursorLocked = false;
            Time.TimeScale = 1;
            if (RaceButton?.GetComponent<UIButton>() is UIButton race) race.Clicked += Race;
            if (SettingsButton?.GetComponent<UIButton>() is UIButton settings) settings.Clicked += OpenSettings;
            if (QuitButton?.GetComponent<UIButton>() is UIButton quit) quit.Clicked += Quit;
            if (RecordsText?.GetComponent<UIText>() is UIText records)
                records.Text = Records();
            Show(true);
        }

        string Records()
        {
            float bestRace = SaveData.GetFloat($"{TrackName}.BestRace");
            float bestLap = SaveData.GetFloat($"{TrackName}.BestLap");
            string medal = RaceManager.MedalTitle(SaveData.GetInt($"{TrackName}.Medal"));
            if (bestRace <= 0.0f && bestLap <= 0.0f)
                return "No records yet: 3 laps, beat the medal times";
            return $"Best race {RaceManager.Format(bestRace)}     Best lap {RaceManager.Format(bestLap)}" +
                (medal.Length > 0 ? $"     {medal} medal" : "");
        }

        protected override void OnUpdate(float dt)
        {
            if (SettingsScreen is SettingsMenu settings && settings.IsOpen && Input.WasPressed("Pause"))
                settings.Back();
            if (MenuCamera != null)
            {
                angle += Speed * Time.UnscaledDeltaTime;
                float a = angle * MathF.PI / 180.0f;
                Vector3 position = Center + new Vector3(MathF.Sin(a) * Radius, Height, MathF.Cos(a) * Radius);
                MenuCamera.Translation = position; // a root entity: its own transform is the world's
                // Looking at the center, in the Inspector's angles: pitch positive down, yaw positive to the left (0 along -Z)
                Vector3 d = Center - position;
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
