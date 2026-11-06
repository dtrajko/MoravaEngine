using System;
using Morava;

namespace Sample
{
    /// <summary>
    /// The Settings screen (on its canvas; its Panel hidden until Open): sliders for the music's and the effects' volumes and
    /// the mouse's sensitivity, buttons that step through the quality, fullscreen or a window, fullscreen's resolution and
    /// VSync, and a Controls screen where the keys and the gamepad's buttons are bound anew. A change counts at once; the
    /// engine keeps the player's choices of the window, the quality, the volumes and the bindings for the next runs
    /// (GameOptions keeps the mouse's sensitivity), and Back saves them. Opened by the main menu and the pause menu, which get
    /// the selection back when it closes; their Pause action (Escape, Start) is Back while it is open.
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

        /// <summary>Buttons that step through a setting, each showing it with a UI Text of its own.</summary>
        public Entity? QualityButton;
        public Entity? DisplayButton;
        public Entity? ResolutionButton;
        public Entity? VSyncButton;

        /// <summary>The Controls screen (a UI Rect, hidden until the Controls button opens it).</summary>
        public Entity? ControlsPanel;

        /// <summary>The Controls screen's buttons, each showing its binding with a UI Text of its own.</summary>
        public Entity? ForwardKey;
        public Entity? BackKey;
        public Entity? LeftKey;
        public Entity? RightKey;
        public Entity? JumpKey;
        public Entity? JumpPad;
        public Entity? CameraKey;
        public Entity? CameraPad;

        /// <summary>A UI Text that says what to do while a binding waits for a press.</summary>
        public Entity? ControlsHint;

        public bool IsOpen { get; private set; }
        bool controlsOpen;

        Action? closed;

        // A binding the Controls screen shows: the action, which column (the keyboard's takes the mouse's buttons too), and
        // for a composite (WASD) which of its keys
        readonly struct Binding
        {
            public Binding(Entity? button, string action, bool gamepad, BindingPart part = BindingPart.Whole)
            {
                Button = button;
                Action = action;
                Gamepad = gamepad;
                Part = part;
            }

            public Entity? Button { get; }
            public string Action { get; }
            public bool Gamepad { get; }
            public BindingPart Part { get; }
        }

        Binding[] bindings = Array.Empty<Binding>();
        int rebinding = -1; // the binding waiting for a press

        static readonly QualityLevel?[] QualitySteps = { null, QualityLevel.Low, QualityLevel.Medium, QualityLevel.High, QualityLevel.Ultra };

        protected override void OnCreate()
        {
            if (Slider(MusicSlider) is UISlider music) music.ValueChanged += v => { Audio.SetBusVolume(AudioBus.Music, v); ShowValues(); };
            if (Slider(EffectsSlider) is UISlider effects) effects.ValueChanged += v => { Audio.SetBusVolume(AudioBus.Effects, v); ShowValues(); };
            if (Slider(MouseSlider) is UISlider mouse) mouse.ValueChanged += v => { GameOptions.SetMouseSensitivity(v); ShowValues(); };

            bindings = new[]
            {
                new Binding(ForwardKey, "Move", false, BindingPart.Up),
                new Binding(BackKey, "Move", false, BindingPart.Down),
                new Binding(LeftKey, "Move", false, BindingPart.Left),
                new Binding(RightKey, "Move", false, BindingPart.Right),
                new Binding(JumpKey, "Jump", false),
                new Binding(JumpPad, "Jump", true),
                new Binding(CameraKey, "SwitchCamera", false),
                new Binding(CameraPad, "SwitchCamera", true),
            };
            for (int i = 0; i < bindings.Length; i++)
            {
                int index = i;
                if (bindings[i].Button?.GetComponent<UIButton>() is UIButton button)
                    button.Clicked += () => Rebind(index);
            }
            Show(Panel, false);
            Show(ControlsPanel, false);
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
            IsOpen = true;
            Show(Panel, true);
            Slider(MusicSlider)?.Select();
            Log.Info("Settings opened");
        }

        // The Back buttons' On Click, and Pause while open: the Controls screen back to Settings, Settings closed
        public void Back()
        {
            if (!IsOpen || rebinding >= 0)
                return; // a binding waiting for a press: Escape gives it up first
            if (controlsOpen)
            {
                controlsOpen = false;
                Show(ControlsPanel, false);
                Show(Panel, true);
                Button(DisplayButton)?.Select();
                return;
            }
            Settings.Save();
            IsOpen = false;
            Show(Panel, false);
            Log.Info($"Settings saved ({Settings.File})");
            Action? then = closed;
            closed = null;
            then?.Invoke();
        }

        // The buttons' On Click (the label follows in OnUpdate: the window changes after this frame)
        void StepQuality()
        {
            int current = Quality.IsAutomatic ? 0 : Array.IndexOf(QualitySteps, Quality.Level);
            QualityLevel? next = QualitySteps[(current + 1) % QualitySteps.Length];
            if (next is QualityLevel level) Quality.Level = level;
            else Quality.SetAutomatic();
        }

        void ToggleDisplay() => Application.Fullscreen = !Application.Fullscreen;

        void StepResolution()
        {
            Resolution[] resolutions = Application.Resolutions;
            if (resolutions.Length == 0)
                return;
            int current = Array.IndexOf(resolutions, Application.Resolution);
            Application.Resolution = resolutions[(current + 1) % resolutions.Length]; // largest first: after the smallest, the largest again
        }

        void ToggleVSync() => Application.VSync = !Application.VSync;

        void OpenControls()
        {
            controlsOpen = true;
            Show(Panel, false);
            Show(ControlsPanel, true);
            ShowBindings();
            Button(ForwardKey)?.Select();
        }

        void ResetControls()
        {
            Input.ResetBindings();
            ShowBindings();
            Log.Info("Controls: the game's keys again");
        }

        void Rebind(int index)
        {
            Binding binding = bindings[index];
            int found = FindBinding(binding);
            if (found < 0 || !Input.Rebind(binding.Action, found, binding.Part))
                return;
            rebinding = index;
            SetText(binding.Button, binding.Gamepad ? "Press a button..." : "Press a key...");
            SetText(ControlsHint, binding.Gamepad ? "Back on the gamepad, or Escape: keep the old one" : "Escape: keep the old one");
        }

        protected override void OnUpdate(float dt)
        {
            if (!IsOpen)
                return;
            if (rebinding >= 0 && Input.RebindState != RebindState.Listening)
            {
                Log.Info($"Controls: {bindings[rebinding].Action} {(Input.RebindState == RebindState.Done ? "bound anew" : "kept")}");
                Button(bindings[rebinding].Button)?.Select();
                rebinding = -1;
                ShowBindings();
            }

            // What the engine has now (a change shows in the next frame: the window changes after the frame)
            SetText(QualityButton, $"{Quality.Level}" + (Quality.IsAutomatic ? " (automatic)" : ""));
            SetText(DisplayButton, Application.Fullscreen ? "Fullscreen" : "Window");
            Resolution[] resolutions = Application.Resolutions;
            SetText(ResolutionButton, resolutions.Length > 0 ? Application.Resolution.ToString() : "The editor's Viewport");
            if (Button(ResolutionButton) is UIButton resolution)
                resolution.Interactable = resolutions.Length > 0;
            SetText(VSyncButton, Application.VSync ? "On" : "Off");
        }

        // The binding of the row's column: the keyboard's first key (a composite's, for a part) or mouse button, the
        // gamepad's first button. -1: none
        static int FindBinding(Binding row)
        {
            int count = Input.GetBindingCount(row.Action);
            for (int i = 0; i < count; i++)
            {
                InputDevice device = Input.GetBindingDevice(row.Action, i);
                bool column = row.Gamepad ? device == InputDevice.Gamepad : device != InputDevice.Gamepad;
                if (column && (row.Part == BindingPart.Whole || Input.GetBindingText(row.Action, i, row.Part).Length > 0))
                    return i;
            }
            return -1;
        }

        void ShowBindings()
        {
            foreach (Binding row in bindings)
            {
                int found = FindBinding(row);
                SetText(row.Button, found >= 0 ? Input.GetBindingText(row.Action, found, row.Part) : "(none)");
            }
            SetText(ControlsHint, "Click a binding, then press the key or button for it");
        }

        void ShowValues()
        {
            SetText(MusicValue, $"{Audio.GetBusVolume(AudioBus.Music) * 100:0} %");
            SetText(EffectsValue, $"{Audio.GetBusVolume(AudioBus.Effects) * 100:0} %");
            SetText(MouseValue, $"x {GameOptions.MouseSensitivity:0.00}");
        }

        static void Show(Entity? panel, bool shown)
        {
            if (panel?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = shown;
        }

        static UISlider? Slider(Entity? entity) => entity?.GetComponent<UISlider>();

        static UIButton? Button(Entity? entity) => entity?.GetComponent<UIButton>();

        static void SetText(Entity? entity, string text)
        {
            if (entity?.GetComponent<UIText>() is UIText ui && ui.Text != text)
                ui.Text = text;
        }
    }
}
