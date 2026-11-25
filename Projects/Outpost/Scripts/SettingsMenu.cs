using System;
using System.Collections.Generic;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The Settings screen (Phase 3.3, Racing's remade), on its canvas, its Panel hidden until Open: sliders for the music's
    /// and the effects' volumes, the mouse's and the scope's sensitivity and the field of view; drop-down lists of the
    /// quality, fullscreen or a window, fullscreen's resolution, VSync, the frame rate limit and the limit on a laptop's
    /// battery; and a Controls screen where every action the player uses is bound anew, the keyboard's (and the mouse's
    /// buttons) and the gamepad's, in two columns of rows. The sliders count at once; the lists' choices wait for Apply
    /// (Back without it gives them up). The engine keeps the window, the quality, the volumes and the bindings for the next
    /// runs, GameOptions the sensitivities and the field of view; Back saves them. Opened by the main menu and the pause
    /// menu, which get the selection back when it closes; their Pause (Escape, Start) is Back while it is open (an open
    /// list: closed first).
    /// <para>
    /// The Controls rows are the scene's "Control {n} Label", "Control {n} Key" and "Control {n} Pad" (n from 1, in
    /// Rows' order): a row whose action has no binding of a column's kind shows a dash and can't be chosen.
    /// </para>
    /// </summary>
    public class SettingsMenu : Entity
    {
        /// <summary>The screen (a UI Rect, hidden while closed).</summary>
        public Entity? Panel;

        public Entity? MusicSlider;
        public Entity? EffectsSlider;
        public Entity? MouseSlider;
        public Entity? ScopeSlider;
        public Entity? FieldOfViewSlider;

        /// <summary>UI Texts showing the sliders' values.</summary>
        public Entity? MusicValue;
        public Entity? EffectsValue;
        public Entity? MouseValue;
        public Entity? ScopeValue;
        public Entity? FieldOfViewValue;

        /// <summary>Drop-down lists of the settings (UIDropdown).</summary>
        public Entity? QualityDropdown;
        public Entity? DisplayDropdown;
        public Entity? ResolutionDropdown;
        public Entity? VSyncDropdown;
        /// <summary>The frame rate limit: Off, 60, 120, 144, 240 (FrameRateOptions).</summary>
        public Entity? FrameRateDropdown;
        /// <summary>On a laptop's battery: 60 fps (0), As plugged in (1).</summary>
        public Entity? BatteryDropdown;

        /// <summary>The button that puts the lists' choices in place (shown dimmed while there is nothing to apply).</summary>
        public Entity? ApplyButton;

        /// <summary>The Controls screen (a UI Rect, hidden until the Controls button opens it).</summary>
        public Entity? ControlsPanel;

        /// <summary>A UI Text that says what to do while a binding waits for a press.</summary>
        public Entity? ControlsHint;

        public bool IsOpen { get; private set; }
        bool controlsOpen;
        Action? closed;

        // The frame rate limit's choices (0: none), and the battery's limit when it has one
        static readonly int[] FrameRateOptions = { 0, 60, 120, 144, 240 };
        const int BatteryLimit = 60;

        // The quality list's options in order: automatic (the GPU's), then the levels
        static readonly QualityLevel?[] QualityOptions = { null, QualityLevel.Low, QualityLevel.Medium, QualityLevel.High, QualityLevel.Ultra };

        /// <summary>The Controls screen's rows in order: the title, the action and, for a composite (WASD), which of its keys.</summary>
        public static readonly (string Title, string Action, BindingPart Part)[] Rows =
        {
            ("Forward", "Move", BindingPart.Up),
            ("Back", "Move", BindingPart.Down),
            ("Left", "Move", BindingPart.Left),
            ("Right", "Move", BindingPart.Right),
            ("Jump", "Jump", BindingPart.Whole),
            ("Crouch (hold)", "Crouch", BindingPart.Whole),
            ("Crouch (toggle)", "CrouchToggle", BindingPart.Whole),
            ("Sprint", "Sprint", BindingPart.Whole),
            ("Walk quietly", "Walk", BindingPart.Whole),
            ("Use", "Use", BindingPart.Whole),
            ("Objectives", "Objectives", BindingPart.Whole),
            ("Switch view", "SwitchView", BindingPart.Whole),
            ("Swap shoulder", "SwapShoulder", BindingPart.Whole),
            ("Fire", "Fire", BindingPart.Whole),
            ("Aim / scope", "Aim", BindingPart.Whole),
            ("Reload", "Reload", BindingPart.Whole),
            ("Fire mode", "FireMode", BindingPart.Whole),
            ("Rifle", "Weapon1", BindingPart.Whole),
            ("Sniper rifle", "Weapon2", BindingPart.Whole),
            ("Pistol", "Weapon3", BindingPart.Whole),
            ("Knife", "Weapon4", BindingPart.Whole),
            ("Last weapon", "LastWeapon", BindingPart.Whole),
            ("Next weapon", "NextWeapon", BindingPart.Whole),
            ("Previous weapon", "PreviousWeapon", BindingPart.Whole),
        };

        // A binding the Controls screen shows: its button, the row, which column (the keyboard's takes the mouse's buttons too)
        readonly struct Binding
        {
            public Binding(Entity? button, int row, bool gamepad)
            {
                Button = button;
                Row = row;
                Gamepad = gamepad;
            }

            public Entity? Button { get; }
            public int Row { get; }
            public bool Gamepad { get; }
            public string Action => Rows[Row].Action;
            public BindingPart Part => Rows[Row].Part;
        }

        readonly List<Binding> bindings = new();
        int rebinding = -1; // the binding waiting for a press

        Resolution[] resolutions = Array.Empty<Resolution>(); // the resolution list's options
        int settling; // frames until the lists show the engine's values again: an applied choice takes effect after its frame
        bool refill;  // the options again once settled (the automatic quality's level)
        bool pending; // a list has a choice not applied yet: the lists keep showing the choices
        UIDropdown? lastChosen; // where the selection goes after Apply (it dims, so it can't keep the selection)

        protected override void OnCreate()
        {
            if (Slider(MusicSlider) is UISlider music) music.ValueChanged += v => { Audio.SetBusVolume(AudioBus.Music, v); ShowValues(); };
            if (Slider(EffectsSlider) is UISlider effects) effects.ValueChanged += v => { Audio.SetBusVolume(AudioBus.Effects, v); ShowValues(); };
            if (Slider(MouseSlider) is UISlider mouse) mouse.ValueChanged += v => { GameOptions.SetMouseSensitivity(v); ShowValues(); };
            if (Slider(ScopeSlider) is UISlider scope) scope.ValueChanged += v => { GameOptions.SetScopeSensitivity(v); ShowValues(); };
            if (Slider(FieldOfViewSlider) is UISlider fov) fov.ValueChanged += v => { GameOptions.SetFieldOfView(v); ShowValues(); };
            foreach (Entity? list in Lists)
            {
                if (Dropdown(list) is UIDropdown dropdown)
                    dropdown.ValueChanged += _ => { lastChosen = dropdown; pending = Differs(); }; // put in place by Apply
            }

            for (int row = 0; row < Rows.Length; row++)
            {
                string label = $"Control {row + 1} Label";
                if (Scene.FindEntityByName(label)?.GetComponent<UIText>() is UIText title)
                    title.Text = Rows[row].Title;
                else
                    Log.Warn($"Settings: the scene has no '{label}': the Controls screen shows {row} of its {Rows.Length} rows");
                for (int column = 0; column < 2; column++)
                {
                    Entity? button = Scene.FindEntityByName($"Control {row + 1} {(column == 0 ? "Key" : "Pad")}");
                    if (button == null)
                        continue;
                    int index = bindings.Count;
                    bindings.Add(new Binding(button, row, column == 1));
                    if (Button(button) is UIButton ui)
                        ui.Clicked += () => Rebind(index);
                }
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
            if (Slider(ScopeSlider) is UISlider scope) scope.Value = GameOptions.ScopeSensitivity;
            if (Slider(FieldOfViewSlider) is UISlider fov) fov.Value = GameOptions.FieldOfView;
            ShowValues();
            pending = false;
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
                Button(Scene.FindEntityByName("Controls Button"))?.Select();
                return;
            }
            if (pending)
            {
                pending = false; // not applied: given up, the lists show what is in place again
                ShowLists();
                Log.Info("Settings: the choices not applied were given up");
            }
            Settings.Save();
            IsOpen = false;
            Show(Panel, false);
            Log.Info($"Settings saved ({Settings.File})");
            Action? then = closed;
            closed = null;
            then?.Invoke();
        }

        Entity?[] Lists => new[] { QualityDropdown, DisplayDropdown, ResolutionDropdown, VSyncDropdown, FrameRateDropdown, BatteryDropdown };

        // The frame rate limit in force as the list's entry (a limit the list doesn't have: the nearest above it, else Off)
        static int FrameRateIndex()
        {
            int limit = Application.FrameRateLimit;
            if (limit <= 0)
                return 0;
            for (int i = 1; i < FrameRateOptions.Length; i++)
                if (FrameRateOptions[i] >= limit)
                    return i;
            return 0;
        }

        static int BatteryIndex() => Application.BatteryFrameRateLimit > 0 ? 0 : 1;

        // A list's choice differs from what is in place (a list set back to it: nothing to apply)
        bool Differs()
        {
            if (Dropdown(QualityDropdown) is UIDropdown quality &&
                quality.Value != (Quality.IsAutomatic ? 0 : Math.Max(0, Array.IndexOf(QualityOptions, Quality.Level))))
                return true;
            if (Dropdown(DisplayDropdown) is UIDropdown display && (display.Value == 0) != Application.Fullscreen)
                return true;
            if (Dropdown(ResolutionDropdown) is UIDropdown resolution && resolution.Value >= 0 && resolution.Value < resolutions.Length &&
                !resolutions[resolution.Value].Equals(Application.Resolution))
                return true;
            if (Dropdown(FrameRateDropdown) is UIDropdown frameRate && frameRate.Value != FrameRateIndex())
                return true;
            if (Dropdown(BatteryDropdown) is UIDropdown battery && battery.Value != BatteryIndex())
                return true;
            return Dropdown(VSyncDropdown) is UIDropdown vsync && (vsync.Value == 1) != Application.VSync;
        }

        // The Apply button's On Click: the lists' choices in place, those that differ from what is (the window changes after
        // this frame, so the lists show the engine's values again a few frames later)
        void Apply()
        {
            if (!pending)
                return;
            if (Dropdown(QualityDropdown) is UIDropdown quality)
            {
                QualityLevel? chosen = QualityOptions[Math.Clamp(quality.Value, 0, QualityOptions.Length - 1)];
                if (chosen is QualityLevel level && (Quality.IsAutomatic || Quality.Level != level)) Quality.Level = level;
                else if (chosen == null && !Quality.IsAutomatic) Quality.SetAutomatic();
            }
            if (Dropdown(ResolutionDropdown) is UIDropdown resolution && resolution.Value >= 0 && resolution.Value < resolutions.Length &&
                !resolutions[resolution.Value].Equals(Application.Resolution))
                Application.Resolution = resolutions[resolution.Value];
            if (Dropdown(DisplayDropdown) is UIDropdown display && (display.Value == 0) != Application.Fullscreen)
                Application.Fullscreen = display.Value == 0;
            if (Dropdown(VSyncDropdown) is UIDropdown vsync && (vsync.Value == 1) != Application.VSync)
                Application.VSync = vsync.Value == 1;
            if (Dropdown(FrameRateDropdown) is UIDropdown frameRate && frameRate.Value != FrameRateIndex())
                Application.FrameRateLimit = FrameRateOptions[Math.Clamp(frameRate.Value, 0, FrameRateOptions.Length - 1)];
            if (Dropdown(BatteryDropdown) is UIDropdown battery && battery.Value != BatteryIndex())
                Application.BatteryFrameRateLimit = battery.Value == 0 ? BatteryLimit : 0;
            pending = false;
            settling = 3;
            refill = true; // "Automatic" names the level it chose
            if (Button(ApplyButton) is UIButton button)
                button.Interactable = false;
            lastChosen?.Select();
            Log.Info("Settings: applied");
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
            if (Dropdown(FrameRateDropdown) is UIDropdown frameRate && !frameRate.IsOpen)
                frameRate.Value = FrameRateIndex();
            if (Dropdown(BatteryDropdown) is UIDropdown battery && !battery.IsOpen)
                battery.Value = BatteryIndex();
        }

        // The Controls button's On Click
        void OpenControls()
        {
            controlsOpen = true;
            Show(Panel, false);
            Show(ControlsPanel, true);
            ShowBindings();
            foreach (Binding b in bindings)
                if (Button(b.Button)?.Select() == true)
                    break;
        }

        // The Controls screen's Reset button's On Click
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
                Binding done = bindings[rebinding];
                Log.Info($"Controls: {done.Action}{(done.Part != BindingPart.Whole ? $" {done.Part}" : "")} " +
                    $"{(Input.RebindState == RebindState.Done ? "bound anew" : "kept")}");
                Button(done.Button)?.Select();
                rebinding = -1;
                ShowBindings();
            }

            if (Button(ApplyButton) is UIButton apply && apply.Interactable != pending)
                apply.Interactable = pending;

            // What the engine has now (an applied change shows a few frames later: the window changes after the frame); the
            // monitor's resolutions again when the window moved to another one. Not while there are choices to apply
            if (!controlsOpen && !pending && --settling < 0)
            {
                settling = 0;
                if (refill || Application.Resolutions.Length != resolutions.Length)
                    FillLists();
                refill = false;
                ShowLists();
            }
        }

        // The binding of the row's column: the keyboard's first key (a composite's, for a part) or mouse button (not the
        // mouse's motion or wheel), the gamepad's first button (not a stick). -1: none
        static int FindBinding(Binding row)
        {
            int count = Input.GetBindingCount(row.Action);
            for (int i = 0; i < count; i++)
            {
                InputDevice device = Input.GetBindingDevice(row.Action, i);
                bool column = row.Gamepad ? device == InputDevice.Gamepad : device != InputDevice.Gamepad;
                if (column && (row.Part == BindingPart.Whole || PartText(row.Action, i, row.Part).Length > 0))
                    return i;
            }
            return -1;
        }

        // A composite's key ("" when the binding isn't a composite: the gamepad's stick has no Up key of its own)
        static string PartText(string action, int binding, BindingPart part)
        {
            try
            {
                return Input.GetBindingText(action, binding, part);
            }
            catch (ArgumentException)
            {
                return "";
            }
        }

        void ShowBindings()
        {
            foreach (Binding row in bindings)
            {
                int found = FindBinding(row);
                string shown = found >= 0 ? Input.GetBindingText(row.Action, found, row.Part) : "-";
                if (row.Gamepad && shown.StartsWith("Gamepad ", StringComparison.Ordinal))
                    shown = shown.Substring("Gamepad ".Length); // (the column's header says it: "Right Bumper" fits)
                SetText(row.Button, shown);
                if (Button(row.Button) is UIButton button && button.Interactable != found >= 0)
                    button.Interactable = found >= 0;
            }
            SetText(ControlsHint, "Click a binding, then press the key or button for it");
        }

        void ShowValues()
        {
            SetText(MusicValue, $"{Audio.GetBusVolume(AudioBus.Music) * 100:0} %");
            SetText(EffectsValue, $"{Audio.GetBusVolume(AudioBus.Effects) * 100:0} %");
            SetText(MouseValue, $"x {GameOptions.MouseSensitivity:0.00}");
            SetText(ScopeValue, $"x {GameOptions.ScopeSensitivity:0.00}");
            SetText(FieldOfViewValue, $"{GameOptions.FieldOfView:0}°");
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
