using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The main menu (Phase 3.3, on its canvas): Play, Settings (SettingsMenu) and Quit at the left, the camera circling the
    /// base slowly behind them. The saved options are put in place here (GameOptions); the mouse is free and Play is
    /// selected, so the gamepad works at once.
    /// </summary>
    public class MainMenu : Entity
    {
        /// <summary>The menu (a UI Rect: hidden while the Settings screen is open).</summary>
        public Entity? Menu;
        public Entity? PlayButton;
        public Entity? SettingsButton;
        public Entity? QuitButton;
        public Entity? SettingsScreen;

        /// <summary>The level Play loads.</summary>
        public string PlayScene = "Scenes/Greybox.mscene";

        // ---- The camera behind the menu: round Center (the greybox's buildings' middle) at Radius meters, Height above it,
        // looking at the center LookHeight up and Shift meters to its left, so the base sits right of the menu;
        // DegreesPerSecond around (positive: counterclockwise from above), starting at StartAngle (0: on +Z)
        public Entity? MenuCamera;
        public Vector3 Center = new Vector3(5.0f, 0.0f, -6.0f);
        public float Radius = 44.0f;
        public float Height = 17.0f;
        public float LookHeight = 3.0f;
        public float Shift = 9.0f;
        public float DegreesPerSecond = 3.0f;
        public float StartAngle = 35.0f;

        float angle;

        protected override void OnCreate()
        {
            GameOptions.ApplyAtStart();
            Input.CursorLocked = false;
            Time.TimeScale = 1;
            if (Button(PlayButton) is UIButton play) play.Clicked += Play;
            if (Button(SettingsButton) is UIButton settings) settings.Clicked += OpenSettings;
            if (Button(QuitButton) is UIButton quit) quit.Clicked += Quit;
            angle = StartAngle;
            Show(true);
            PlaceCamera();
        }

        protected override void OnUpdate(float dt)
        {
            if (SettingsScreen is SettingsMenu settings && settings.IsOpen && Input.WasPressed("Pause"))
                settings.Back();
            angle += DegreesPerSecond * Time.UnscaledDeltaTime;
            PlaceCamera();
        }

        void PlaceCamera()
        {
            if (MenuCamera == null)
                return;
            float a = angle * MathF.PI / 180.0f;
            Vector3 position = Center + new Vector3(MathF.Sin(a) * Radius, Height, MathF.Cos(a) * Radius);
            Vector3 toCenter = Center - position;
            Vector3 right = Vector3.Normalize(new Vector3(-toCenter.Z, 0.0f, toCenter.X)); // (looking along toCenter)
            Vector3 d = Center + new Vector3(0.0f, LookHeight, 0.0f) - right * Shift - position;
            MenuCamera.Translation = position; // a root entity: its own transform is the world's
            // In the Inspector's angles: pitch positive down, yaw positive to the left (0 along -Z)
            float yaw = MathF.Atan2(-d.X, -d.Z) * 180.0f / MathF.PI;
            float pitch = MathF.Atan2(-d.Y, MathF.Sqrt(d.X * d.X + d.Z * d.Z)) * 180.0f / MathF.PI;
            MenuCamera.EulerAngles = new Vector3(pitch, yaw, 0.0f);
        }

        void Play()
        {
            Log.Info("Main menu: play");
            Scene.Load(PlayScene);
        }

        void OpenSettings()
        {
            if (SettingsScreen is not SettingsMenu settings)
                return;
            Show(false);
            settings.Open(() =>
            {
                Show(true, select: false);
                Button(SettingsButton)?.Select();
            });
        }

        void Quit()
        {
            Log.Info("Main menu: quit");
            Application.Quit();
        }

        void Show(bool shown, bool select = true)
        {
            if (Menu?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = shown;
            if (shown && select)
                Button(PlayButton)?.Select();
        }

        static UIButton? Button(Entity? entity) => entity?.GetComponent<UIButton>();
    }
}
