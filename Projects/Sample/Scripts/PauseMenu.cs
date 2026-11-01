using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A pause menu (on its canvas): Pause (Escape, the gamepad's Start) stops the game (Time.TimeScale 0) and shows the
    /// menu with its first button selected, so the gamepad and the arrow keys work at once. Resume is wired in the
    /// Inspector (the button's On Click: Resume, found on this script, its canvas), as Exit is (On Click: Exit); Restart
    /// listens in code (RestartButton.Clicked). The title pulses in real time: the menu lives while the game stands still.
    /// </summary>
    public class PauseMenu : Entity
    {
        /// <summary>The menu's panel (a UI Rect: hidden until the game is paused).</summary>
        public Entity? Menu;

        /// <summary>Selected when the menu opens.</summary>
        public Entity? ResumeButton;

        /// <summary>Starts the scene again (listened to in code).</summary>
        public Entity? RestartButton;

        /// <summary>A text that pulses while the menu is open (none: nothing pulses).</summary>
        public Entity? Title;

        bool paused;
        bool cursorWasLocked;

        protected override void OnCreate()
        {
            if (RestartButton?.GetComponent<UIButton>() is UIButton restart)
                restart.Clicked += Restart;
            Show(false);
        }

        protected override void OnUpdate(float dt)
        {
            if (Input.WasPressed("Pause"))
            {
                if (paused) Resume();
                else Pause();
            }
            if (paused && Title?.GetComponent<UIText>() is UIText title)
            {
                // Time.UnscaledTime goes on while TimeScale is 0 (Time.GameTime and dt don't)
                float pulse = 0.75f + 0.25f * MathF.Sin((float)Time.UnscaledTime * 4.0f);
                title.Color = new Vector4(1.0f, 0.86f, 0.3f, pulse);
            }
        }

        void Pause()
        {
            paused = true;
            Time.TimeScale = 0;
            cursorWasLocked = Input.CursorLocked;
            Input.CursorLocked = false; // the mouse for the buttons
            Show(true);
            ResumeButton?.GetComponent<UIButton>()?.Select();
            Log.Info("Paused");
        }

        // The Resume button's On Click (Inspector), and Pause again
        void Resume()
        {
            paused = false;
            Time.TimeScale = 1;
            Input.CursorLocked = cursorWasLocked;
            Show(false);
            Log.Info("Resumed");
        }

        // The Exit button's On Click (Inspector): the player closes, the editor stops Play
        void Exit()
        {
            Log.Info("Exit from the pause menu");
            Application.Quit();
        }

        void Restart()
        {
            Log.Info("Restart from the pause menu");
            Time.TimeScale = 1; // kept by Scene.Reload, as the clock is
            Scene.Reload();
        }

        void Show(bool shown)
        {
            if (Menu?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = shown;
        }
    }
}
