using System;
using System.Numerics;
using Morava;

namespace Racing
{
    /// <summary>
    /// The race's pause menu (on its canvas, from Sample's): Pause (Escape, the gamepad's Start) stops the game
    /// (Time.TimeScale 0) and shows Resume, Restart, Settings and Main Menu, Resume selected, so the gamepad and the arrow
    /// keys work at once. Restart starts the race again without loading the scene (RaceManager.RequestRestart). While it is
    /// open the race's keys wait (RaceManager.KeysBlocked: Enter is the menu's here, not Restart). Not while the finish
    /// screen shows: it has its own buttons.
    /// </summary>
    public class PauseMenu : Entity
    {
        /// <summary>The menu's panel (a UI Rect: hidden until the game is paused).</summary>
        public Entity? Menu;
        public Entity? ResumeButton;
        public Entity? RestartButton;
        public Entity? SettingsButton;
        public Entity? MainMenuButton;
        /// <summary>The Settings screen (SettingsMenu); Pause is its Back.</summary>
        public Entity? SettingsScreen;
        /// <summary>A text that pulses while the menu is open.</summary>
        public Entity? Title;
        public string MainMenuScene = "Scenes/MainMenu.mscene";

        /// <summary>The game is paused.</summary>
        public static bool IsPaused { get; private set; }

        bool cursorWasLocked;

        protected override void OnCreate()
        {
            IsPaused = false;
            RaceManager.KeysBlocked = false;
            if (ResumeButton?.GetComponent<UIButton>() is UIButton resume) resume.Clicked += Resume;
            if (RestartButton?.GetComponent<UIButton>() is UIButton restart) restart.Clicked += Restart;
            if (SettingsButton?.GetComponent<UIButton>() is UIButton settings) settings.Clicked += OpenSettings;
            if (MainMenuButton?.GetComponent<UIButton>() is UIButton mainMenu) mainMenu.Clicked += ToMainMenu;
            Show(false);
        }

        protected override void OnUpdate(float dt)
        {
            if (Input.WasPressed("Pause"))
            {
                if (SettingsScreen is SettingsMenu screen && screen.IsOpen) screen.Back(); // back to this menu
                else if (IsPaused) Resume();
                else if (!RaceHud.FinishShown) Pause();
            }
            if (IsPaused && Title?.GetComponent<UIText>() is UIText title)
            {
                // Time.UnscaledTime goes on while TimeScale is 0
                float pulse = 0.8f + 0.2f * MathF.Sin((float)Time.UnscaledTime * 4.0f);
                title.Color = new Vector4(1.0f, 1.0f, 1.0f, pulse);
            }
        }

        void Pause()
        {
            IsPaused = true;
            RaceManager.KeysBlocked = true;
            Time.TimeScale = 0;
            cursorWasLocked = Input.CursorLocked;
            Input.CursorLocked = false; // the mouse for the buttons
            Show(true);
            ResumeButton?.GetComponent<UIButton>()?.Select();
            Log.Info("Paused");
        }

        void Resume()
        {
            IsPaused = false;
            RaceManager.KeysBlocked = false;
            Time.TimeScale = 1;
            Input.CursorLocked = cursorWasLocked;
            Show(false);
            Log.Info("Resumed");
        }

        void Restart()
        {
            Log.Info("Restart from the pause menu");
            Resume();
            RaceManager.Current?.RequestRestart();
        }

        void OpenSettings()
        {
            if (SettingsScreen is not SettingsMenu screen)
                return;
            Show(false);
            screen.Open(() =>
            {
                Show(true);
                SettingsButton?.GetComponent<UIButton>()?.Select();
            });
        }

        void ToMainMenu()
        {
            Log.Info("Main menu from the pause menu");
            IsPaused = false;
            RaceManager.KeysBlocked = false;
            Time.TimeScale = 1;
            Scene.Load(MainMenuScene);
        }

        void Show(bool shown)
        {
            if (Menu?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = shown;
        }
    }
}
