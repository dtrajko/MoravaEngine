using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The pause menu (Phase 3.3, Racing's remade, on its canvas): Pause (Escape, the gamepad's Start) stops the game
    /// (Time.TimeScale 0), frees the mouse and shows Resume, Checkpoint, Settings and Main Menu at the left, Resume
    /// selected so the gamepad and the arrow keys work at once, and the objectives' full list in the middle (the HUD's
    /// panel, Hud.ShowObjectives). Checkpoint starts again at the last checkpoint passed, as after dying (Player.Respawn).
    /// While it is open, and a moment after Resume (its click mustn't fire), the game's keys wait (BlocksGame). Pause is
    /// Back while the Settings screen is open. Not while the debug fly camera has the view.
    /// </summary>
    public class PauseMenu : Entity
    {
        /// <summary>The menu's panel (a UI Rect: hidden until the game is paused).</summary>
        public Entity? Menu;
        public Entity? ResumeButton;
        public Entity? CheckpointButton;
        public Entity? SettingsButton;
        public Entity? MainMenuButton;
        /// <summary>The Settings screen (SettingsMenu); Pause is its Back.</summary>
        public Entity? SettingsScreen;
        /// <summary>The player (Player): Checkpoint starts it again.</summary>
        public Entity? Player;
        /// <summary>The HUD (Hud): its objectives' panel shows while paused.</summary>
        public Entity? Hud;
        /// <summary>A text that pulses while the menu is open.</summary>
        public Entity? Title;
        public string MainMenuScene = "Scenes/MainMenu.mscene";

        /// <summary>Seconds (real time) the game's keys still wait after Resume.</summary>
        public float ResumeBlockSeconds = 0.15f;

        /// <summary>The game is paused.</summary>
        public static bool IsPaused { get; private set; }

        /// <summary>The game's keys wait: paused, or just resumed (the Player reads it).</summary>
        public static bool BlocksGame => IsPaused || Time.UnscaledTime < resumedAt + resumeBlock;

        static double resumedAt = double.MinValue;
        static float resumeBlock;
        bool cursorWasLocked;

        protected override void OnCreate()
        {
            IsPaused = false; // (the scripts' statics outlive a Play in the editor)
            resumedAt = double.MinValue;
            Time.TimeScale = 1;
            if (Button(ResumeButton) is UIButton resume) resume.Clicked += Resume;
            if (Button(CheckpointButton) is UIButton checkpoint) checkpoint.Clicked += ToCheckpoint;
            if (Button(SettingsButton) is UIButton settings) settings.Clicked += OpenSettings;
            if (Button(MainMenuButton) is UIButton mainMenu) mainMenu.Clicked += ToMainMenu;
            Show(false);
        }

        protected override void OnUpdate(float dt)
        {
            if (Input.WasPressed("Pause"))
            {
                if (SettingsScreen is SettingsMenu screen && screen.IsOpen) screen.Back(); // back to this menu
                else if (IsPaused) Resume();
                else if (Player is not Outpost.Player { IsFlying: true }) Pause();
            }
            if (IsPaused && Title?.GetComponent<UIText>() is UIText title)
            {
                // Time.UnscaledTime goes on while TimeScale is 0
                float pulse = 0.8f + 0.2f * MathF.Sin((float)Time.UnscaledTime * 4.0f);
                title.Color = title.Color with { W = pulse };
            }
        }

        void Pause()
        {
            IsPaused = true;
            Time.TimeScale = 0;
            cursorWasLocked = Input.CursorLocked;
            Input.CursorLocked = false; // the mouse for the buttons
            Show(true);
            Log.Info("Paused");
        }

        void Resume()
        {
            IsPaused = false;
            resumedAt = Time.UnscaledTime;
            resumeBlock = ResumeBlockSeconds;
            Time.TimeScale = 1;
            Input.CursorLocked = cursorWasLocked;
            Show(false);
            Log.Info("Resumed");
        }

        void ToCheckpoint()
        {
            Log.Info("Pause menu: back to the checkpoint");
            Resume();
            if (Player is Outpost.Player player)
                player.Respawn();
        }

        void OpenSettings()
        {
            if (SettingsScreen is not SettingsMenu screen)
                return;
            Show(false);
            screen.Open(() =>
            {
                Show(true, select: false);
                Button(SettingsButton)?.Select();
            });
        }

        void ToMainMenu()
        {
            Log.Info("Main menu from the pause menu");
            IsPaused = false;
            Time.TimeScale = 1;
            Scene.Load(MainMenuScene);
        }

        void Show(bool shown, bool select = true)
        {
            if (Menu?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = shown;
            if (Hud is Outpost.Hud hud)
                hud.ShowObjectives = shown;
            if (shown && select)
                Button(ResumeButton)?.Select();
        }

        static UIButton? Button(Entity? entity) => entity?.GetComponent<UIButton>();
    }
}
