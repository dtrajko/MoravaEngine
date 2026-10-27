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
            return new InputAction(state.Value, state.Held != 0, state.Pressed != 0, state.Released != 0);
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
    }

    /// <summary>An input action's state (Input.GetAction).</summary>
    public readonly struct InputAction
    {
        internal InputAction(Vector2 value, bool held, bool pressed, bool released)
        {
            Value = value;
            IsHeld = held;
            WasPressed = pressed;
            WasReleased = released;
        }

        /// <summary>A button: X is 0 or 1. An axis: X. Two axes: X and Y.</summary>
        public Vector2 Value { get; }

        /// <summary>A button held down (an axis: moved away from 0).</summary>
        public bool IsHeld { get; }

        /// <summary>Went down in this update.</summary>
        public bool WasPressed { get; }

        /// <summary>Went up in this update.</summary>
        public bool WasReleased { get; }
    }
}
