using System;
using Morava;

namespace Sample
{
    /// <summary>
    /// The coin collector's main menu (on its canvas): Play (a new run from the first level), Settings (the Settings
    /// screen, SettingsMenu) and Quit, wired in the Inspector (On Click: Play, OpenSettings, Quit). The saved settings are
    /// put in place here (GameOptions); the mouse is free and Play is selected, so the gamepad works at once.
    /// </summary>
    public class MainMenu : Entity
    {
        /// <summary>The menu's buttons (a UI Rect: hidden while the Settings screen is open).</summary>
        public Entity? Menu;

        /// <summary>Selected when the menu shows.</summary>
        public Entity? FirstButton;

        /// <summary>The Settings screen.</summary>
        public Entity? SettingsScreen;

        /// <summary>A text that pulses (none: nothing pulses).</summary>
        public Entity? Title;

        protected override void OnCreate()
        {
            GameOptions.ApplyAtStart();
            Input.CursorLocked = false;
            Show(true);
        }

        protected override void OnUpdate(float dt)
        {
            if (SettingsScreen is SettingsMenu settings && settings.IsOpen && Input.WasPressed("Pause"))
                settings.Back();
            if (Title?.GetComponent<UIText>() is UIText title)
            {
                float pulse = 0.85f + 0.15f * MathF.Sin((float)Time.UnscaledTime * 2.0f);
                title.Color = new System.Numerics.Vector4(1.0f, 0.82f, 0.3f, pulse);
            }
        }

        // The buttons' On Click
        void Play()
        {
            Log.Info("Main menu: play");
            CollectorGame.Reset();
            Scene.Load(CollectorGame.FirstLevel);
        }

        void OpenSettings()
        {
            if (SettingsScreen is not SettingsMenu settings)
                return;
            Show(false);
            settings.Open(() => Show(true));
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
                FirstButton?.GetComponent<UIButton>()?.Select();
        }
    }
}
