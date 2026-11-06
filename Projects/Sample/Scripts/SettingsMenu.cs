using System;
using Morava;

namespace Sample
{
    /// <summary>
    /// The Settings screen (on its canvas; its Panel hidden until Open): sliders for the music's and the effects' volumes and
    /// the mouse's sensitivity, drop-down lists of the quality, fullscreen or a window, fullscreen's resolution and VSync,
    /// and a Controls screen where the keys and the gamepad's buttons are bound anew. A change counts at once; the
    /// engine keeps the player's choices of the window, the quality, the volumes and the bindings for the next runs
    /// (GameOptions keeps the mouse's sensitivity), and Back saves them. Opened by the main menu and the pause menu, which get
    /// the selection back when it closes; their Pause action (Escape, Start) is Back while it is open (an open list: closed
    /// first).
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

        /// <summary>Drop-down lists of the settings (UIDropdown).</summary>
        public Entity? QualityDropdown;
        public Entity? DisplayDropdown;
        public Entity? ResolutionDropdown;
        public Entity? VSyncDropdown;

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

        // The quality list's options in order: automatic (the GPU's), then the levels
        static readonly QualityLevel?[] QualityOptions = { null, QualityLevel.Low, QualityLevel.Medium, QualityLevel.High, QualityLevel.Ultra };

        Resolution[] resolutions = Array.Empty<Resolution>(); // the resolution list's options
        int settling; // frames until the lists show the engine's values again: a choice takes effect after its frame
        bool refill;  // the options again once settled (the automatic quality's level)

        protected override void OnCreate()
        {
            if (Slider(MusicSlider) is UISlider music) music.ValueChanged += v => { Audio.SetBusVolume(AudioBus.Music, v); ShowValues(); };
            if (Slider(EffectsSlider) is UISlider effects) effects.ValueChanged += v => { Audio.SetBusVolume(AudioBus.Effects, v); ShowValues(); };
            if (Slider(MouseSlider) is UISlider mouse) mouse.ValueChanged += v => { GameOptions.SetMouseSensitivity(v); ShowValues(); };
            if (Dropdown(QualityDropdown) is UIDropdown quality) quality.ValueChanged += ChooseQuality;
            if (Dropdown(DisplayDropdown) is UIDropdown display) display.ValueChanged += option => { Application.Fullscreen = option == 0; settling = 3; };
            if (Dropdown(ResolutionDropdown) is UIDropdown resolution) resolution.ValueChanged += ChooseResolution;
            if (Dropdown(VSyncDropdown) is UIDropdown vsync) vsync.ValueChanged += option => { Application.VSync = option == 1; settling = 3; };

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
            FillLists();
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
            foreach (Entity? list in Lists)
            {
                if (Dropdown(list) is UIDropdown open && open.IsOpen)
                {
                    open.Close(); // an open list: Escape closes it unchanged
                    return;
                }
            }
            if (controlsOpen)
            {
                controlsOpen = false;
                Show(ControlsPanel, false);
                Show(Panel, true);
                Dropdown(DisplayDropdown)?.Select();
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

        Entity?[] Lists => new[] { QualityDropdown, DisplayDropdown, ResolutionDropdown, VSyncDropdown };

        // The lists' choices (the shown values follow in OnUpdate: the window changes after this frame)
        void ChooseQuality(int option)
        {
            if (QualityOptions[Math.Clamp(option, 0, QualityOptions.Length - 1)] is QualityLevel level) Quality.Level = level;
            else Quality.SetAutomatic();
            settling = 3;
            refill = true; // "Automatic" names the level it chose
        }

        void ChooseResolution(int option)
        {
            if (option >= 0 && option < resolutions.Length)
                Application.Resolution = resolutions[option];
            settling = 3;
        }

        // The options that depend on the machine: the automatic quality's level, the monitor's resolutions (largest first)
        void FillLists()
        {
            if (Dropdown(QualityDropdown) is UIDropdown quality)
            {
                string automatic = Quality.IsAutomatic ? $"Automatic ({Quality.Level})" : "Automatic";
                quality.Options = new[] { automatic, "Low", "Medium", "High", "Ultra" };
            }
            if (Dropdown(ResolutionDropdown) is UIDropdown resolution)
            {
                resolutions = Application.Resolutions;
                string[] options = new string[resolutions.Length];
                for (int i = 0; i < resolutions.Length; i++)
                    options[i] = resolutions[i].ToString();
                // The editor's Play: the game draws into the Viewport, no resolution to choose
                resolution.Options = options.Length > 0 ? options : new[] { "The editor's Viewport" };
                resolution.Interactable = options.Length > 0;
            }
            ShowLists();
        }

        // The lists show what the engine has now (also after F11, or a choice that didn't take)
        void ShowLists()
        {
            if (Dropdown(QualityDropdown) is UIDropdown quality && !quality.IsOpen)
                quality.Value = Quality.IsAutomatic ? 0 : Math.Max(0, Array.IndexOf(QualityOptions, Quality.Level));
            if (Dropdown(DisplayDropdown) is UIDropdown display && !display.IsOpen)
                display.Value = Application.Fullscreen ? 0 : 1;
            if (Dropdown(ResolutionDropdown) is UIDropdown resolution && !resolution.IsOpen && resolutions.Length > 0)
                resolution.Value = Math.Max(0, Array.IndexOf(resolutions, Application.Resolution));
            if (Dropdown(VSyncDropdown) is UIDropdown vsync && !vsync.IsOpen)
                vsync.Value = Application.VSync ? 1 : 0;
        }

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

            // What the engine has now (a change shows in the next frame: the window changes after the frame); the monitor's
            // resolutions again when the window moved to another one
            if (!controlsOpen && --settling < 0)
            {
                settling = 0;
                if (refill || Application.Resolutions.Length != resolutions.Length)
                    FillLists();
                refill = false;
                ShowLists();
            }
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

        static UIDropdown? Dropdown(Entity? entity) => entity?.GetComponent<UIDropdown>();

        static void SetText(Entity? entity, string text)
        {
            if (entity?.GetComponent<UIText>() is UIText ui && ui.Text != text)
                ui.Text = text;
        }
    }
}
