using System;
using System.Numerics;

namespace Morava
{
    /// <summary>
    /// The game's input as actions, not keys: the project's input file (Settings/Input.minput) names them and binds them to
    /// keys, the mouse and gamepads, and the View > Input panel shows their values. Pressed and released are the frame's
    /// in OnUpdate and the fixed step's in OnFixedUpdate, so a short tap is never missed in either.
    /// </summary>
    public static unsafe class Input
    {
        /// <summary>An action's state now.</summary>
        public static InputAction GetAction(string name)
        {
            NativeInputAction state;
            fixed (char* chars = name) Interop.Game->Input_GetAction(chars, name.Length, &state);
            if (state.Exists == 0)
                throw new ArgumentException($"The input map has no action '{name}' (the project's Settings/Input.minput; the View > Input panel lists them)");
            return new InputAction(state.Value, state.Held != 0, state.Pressed != 0, state.Released != 0, state.Gamepad != 0);
        }

        /// <summary>A button's or an axis' value: 0 or 1 for a button, -1 to 1 for an axis.</summary>
        public static float GetValue(string name) => GetAction(name).Value.X;

        /// <summary>A two-axis action (a stick, WASD): X right, Y up (forward), each -1 to 1.</summary>
        public static Vector2 GetAxis2D(string name) => GetAction(name).Value;

        /// <summary>The button is held down.</summary>
        public static bool IsHeld(string name) => GetAction(name).IsHeld;

        /// <summary>The button went down: in this frame (OnUpdate), or since the last fixed step (OnFixedUpdate).</summary>
        public static bool WasPressed(string name) => GetAction(name).WasPressed;

        /// <summary>The button went up: in this frame (OnUpdate), or since the last fixed step (OnFixedUpdate).</summary>
        public static bool WasReleased(string name) => GetAction(name).WasReleased;

        /// <summary>The action's value comes from a gamepad now (a stick: how far it is pushed means something).</summary>
        public static bool FromGamepad(string name) => GetAction(name).FromGamepad;

        /// <summary>
        /// The game wants the mouse: the cursor hidden and its motion unlimited (a camera turned by the mouse), from the next
        /// frame, while the game has the input. Kept when a scene is loaded. The user can take the cursor back (Escape, in
        /// the editor and the player); MouseCaptured says whether the game has it now.
        /// </summary>
        public static bool CursorLocked
        {
            get => (Interop.Game->Input_GetCursorState() & 1) != 0;
            set => Interop.Game->Input_SetCursorLocked(value ? 1 : 0);
        }

        /// <summary>
        /// The mouse is the game's now (CursorLocked, the game has the input, and the user didn't take it back): its motion
        /// is the player's. False: the cursor is free (moving to a window's edge or the editor's panels), so mouse motion
        /// shouldn't turn a camera.
        /// </summary>
        public static bool MouseCaptured => (Interop.Game->Input_GetCursorState() & 2) != 0;

        /// <summary>How many bindings an action has (the input file's, or the player's once rebound).</summary>
        public static int GetBindingCount(string action)
        {
            int count;
            fixed (char* chars = action) count = Interop.Game->Input_GetBindingCount(chars, action.Length);
            if (count < 0)
                throw new ArgumentException($"The input map has no action '{action}'");
            return count;
        }

        /// <summary>
        /// A binding as the player reads it: "Space", "Left Shift", "Mouse Left", "Gamepad A"; a composite's keys
        /// ("W, S, A, D"), or one of them (part). Empty when the composite has no key there.
        /// </summary>
        public static string GetBindingText(string action, int binding, BindingPart part = BindingPart.Whole) =>
            GetBinding(action, binding, part, out _);

        /// <summary>Which device a binding is for: a Controls screen's keyboard and gamepad columns.</summary>
        public static InputDevice GetBindingDevice(string action, int binding)
        {
            GetBinding(action, binding, BindingPart.Whole, out InputDevice device);
            return device;
        }

        /// <summary>
        /// The next key, mouse button or gamepad button pressed replaces the binding (a composite's key: part): a keyboard
        /// binding takes a key or a mouse button, a gamepad binding a gamepad button. Escape or the gamepad's Back gives
        /// up. Until then the actions read released (RebindState says where it is). The player's bindings are kept for the
        /// next runs too. False: a binding nothing can be pressed for (a stick, the mouse's motion), or no such binding.
        /// </summary>
        public static bool Rebind(string action, int binding, BindingPart part = BindingPart.Whole)
        {
            fixed (char* chars = action) return Interop.Game->Input_Rebind(chars, action.Length, binding, (int)part) != 0;
        }

        /// <summary>Where the last Rebind is.</summary>
        public static RebindState RebindState => (RebindState)Interop.Game->Input_GetRebindState();

        /// <summary>Gives up a Rebind that is still listening (a Back button clicked).</summary>
        public static void CancelRebind() => Interop.Game->Input_CancelRebind();

        /// <summary>The input file's bindings again, for every action: the player's are forgotten.</summary>
        public static void ResetBindings() => Interop.Game->Input_ResetBindings();

        static string GetBinding(string action, int binding, BindingPart part, out InputDevice device)
        {
            char* buffer = stackalloc char[256];
            int length, kind = 0;
            fixed (char* chars = action) length = Interop.Game->Input_GetBinding(chars, action.Length, binding, (int)part, &kind, buffer, 256);
            if (length == -2)
                throw new ArgumentException($"The action '{action}' has no binding {binding}" + (part != BindingPart.Whole ? $" with a {part} key" : ""));
            device = (InputDevice)kind;
            return length >= 0 ? new string(buffer, 0, length) : "?";
        }
    }

    /// <summary>A binding, or one key of a composite (WASD's W is Up).</summary>
    public enum BindingPart
    {
        /// <summary>The whole binding (a key, a button; a composite's keys together).</summary>
        Whole = 0,
        /// <summary>A composite's Up key (WASD's W).</summary>
        Up = 1,
        /// <summary>A composite's Down key (WASD's S).</summary>
        Down = 2,
        /// <summary>A composite's Left key (WASD's A; an Axis1D's Negative).</summary>
        Left = 3,
        /// <summary>A composite's Right key (WASD's D; an Axis1D's Positive).</summary>
        Right = 4,
    }

    /// <summary>What a binding is for.</summary>
    public enum InputDevice
    {
        /// <summary>A key, or keys (a composite).</summary>
        Keyboard = 0,
        /// <summary>A mouse button, the mouse's motion or its wheel.</summary>
        Mouse = 1,
        /// <summary>A gamepad's button, axis or stick.</summary>
        Gamepad = 2,
    }

    /// <summary>Where a rebind is (Input.Rebind).</summary>
    public enum RebindState
    {
        /// <summary>None was started.</summary>
        None = 0,
        /// <summary>Waiting for a press.</summary>
        Listening = 1,
        /// <summary>The binding changed.</summary>
        Done = 2,
        /// <summary>Given up: Escape, the gamepad's Back, or CancelRebind.</summary>
        Cancelled = 3,
    }

    /// <summary>An input action's state (Input.GetAction).</summary>
    public readonly struct InputAction
    {
        internal InputAction(Vector2 value, bool held, bool pressed, bool released, bool fromGamepad)
        {
            Value = value;
            IsHeld = held;
            WasPressed = pressed;
            WasReleased = released;
            FromGamepad = fromGamepad;
        }

        /// <summary>A button: X is 0 or 1. An axis: X. Two axes: X and Y.</summary>
        public Vector2 Value { get; }

        /// <summary>A button held down (an axis: moved away from 0).</summary>
        public bool IsHeld { get; }

        /// <summary>Went down in this update.</summary>
        public bool WasPressed { get; }

        /// <summary>Went up in this update.</summary>
        public bool WasReleased { get; }

        /// <summary>The value is a gamepad's: the binding that gave it (the largest) is a gamepad's button, axis or stick.</summary>
        public bool FromGamepad { get; }
    }
}
