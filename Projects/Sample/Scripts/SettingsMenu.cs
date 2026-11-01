using System;
using Morava;

namespace Sample
{
    /// <summary>
    /// The Settings screen (on its canvas; its Panel hidden until Open): sliders for the music's and the effects' volumes
    /// and the mouse's sensitivity, a Fullscreen button, and Back. A change counts at once (GameOptions) and is saved at
    /// Back (Settings.Save). Opened by the main menu and the pause menu, which get the selection back when it closes; their
    /// Pause action (Escape, Start) is Back while it is open.
    /// </summary>
    public class SettingsMenu : Entity
    {
        /// <summary>The screen (a UI Rect, hidden while closed).</summary>
        public Entity? Panel;

        public Entity? MusicSlider;
        public Entity? EffectsSlider;
        public Entity? MouseSlider;

        /// <summary>UI Texts showing the sliders' values.</summary>
        public Entity? MusicValue;
        public Entity? EffectsValue;
        public Entity? MouseValue;

        /// <summary>Its On Click: ToggleFullscreen; its label (a UI Text child) says the state.</summary>
        public Entity? FullscreenButton;
        public Entity? FullscreenLabel;

        public bool IsOpen { get; private set; }

        Action? closed;

        protected override void OnCreate()
        {
            if (Slider(MusicSlider) is UISlider music) music.ValueChanged += v => { GameOptions.SetMusic(v); ShowValues(); };
            if (Slider(EffectsSlider) is UISlider effects) effects.ValueChanged += v => { GameOptions.SetEffects(v); ShowValues(); };
            if (Slider(MouseSlider) is UISlider mouse) mouse.ValueChanged += v => { GameOptions.SetMouseSensitivity(v); ShowValues(); };
            Show(false);
        }

        /// <summary>Shows the screen with the values as they are; onClosed when Back closes it.</summary>
        public void Open(Action? onClosed)
        {
            closed = onClosed;
            // Set from code: no ValueChanged (nothing saved just by opening)
            if (Slider(MusicSlider) is UISlider music) music.Value = Audio.GetBusVolume(AudioBus.Music);
            if (Slider(EffectsSlider) is UISlider effects) effects.Value = Audio.GetBusVolume(AudioBus.Effects);
            if (Slider(MouseSlider) is UISlider mouse) mouse.Value = GameOptions.MouseSensitivity;
            ShowValues();
            Show(true);
            Slider(MusicSlider)?.Select();
            Log.Info("Settings opened");
        }

        // The Back button's On Click, and Pause while open
        public void Back()
        {
            if (!IsOpen)
                return;
            Settings.Save();
            Show(false);
            Log.Info($"Settings saved ({Settings.File})");
            Action? then = closed;
            closed = null;
            then?.Invoke();
        }

        // The Fullscreen button's On Click (the label follows in the next frames: the window changes after this one)
        void ToggleFullscreen() => GameOptions.SetFullscreen(!Application.Fullscreen);

        protected override void OnUpdate(float dt)
        {
            if (IsOpen && FullscreenLabel?.GetComponent<UIText>() is UIText label)
                label.Text = Application.Fullscreen ? "Fullscreen: On" : "Fullscreen: Off";
        }

        void ShowValues()
        {
            SetText(MusicValue, $"{Audio.GetBusVolume(AudioBus.Music) * 100:0} %");
            SetText(EffectsValue, $"{Audio.GetBusVolume(AudioBus.Effects) * 100:0} %");
            SetText(MouseValue, $"x {GameOptions.MouseSensitivity:0.00}");
        }

        void Show(bool shown)
        {
            IsOpen = shown;
            if (Panel?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = shown;
        }

        static UISlider? Slider(Entity? entity) => entity?.GetComponent<UISlider>();

        static void SetText(Entity? entity, string text)
        {
            if (entity?.GetComponent<UIText>() is UIText ui)
                ui.Text = text;
        }
    }
}
