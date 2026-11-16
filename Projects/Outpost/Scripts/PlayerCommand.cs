using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// What a player wants in one fixed step (docs/games/Shooter_Plan.html, keeping multiplayer possible: input as
    /// commands). The player's character is driven only from these: the local player's come from the input
    /// (LocalPlayerInput), and a remote player or a bot would send the same. Phase 2 adds firing, aiming and the weapons.
    /// </summary>
    public struct PlayerCommand
    {
        /// <summary>x right, y forward, as the look's yaw turns them; length 1 at most.</summary>
        public Vector2 Move;

        /// <summary>Where it looks, in degrees: yaw 0 along -Z (positive turns left), pitch positive looks down.</summary>
        public float Yaw, Pitch;

        /// <summary>Jump was pressed since the last step.</summary>
        public bool Jump;

        /// <summary>It wants to be crouched (Crouch held, or CrouchToggle on).</summary>
        public bool Crouch;

        /// <summary>Walking quietly: slower, and steps the guards hear less.</summary>
        public bool Walk;

        /// <summary>The horizontal direction the yaw faces (length 1).</summary>
        public Vector3 Forward
        {
            get
            {
                float yaw = Yaw * MathF.PI / 180.0f;
                return new Vector3(-MathF.Sin(yaw), 0.0f, -MathF.Cos(yaw));
            }
        }

        /// <summary>Right of Forward (length 1).</summary>
        public Vector3 Right
        {
            get
            {
                float yaw = Yaw * MathF.PI / 180.0f;
                return new Vector3(MathF.Cos(yaw), 0.0f, -MathF.Sin(yaw));
            }
        }
    }

    /// <summary>
    /// The local player's commands, from the input actions. The look turns each frame (the camera follows the mouse
    /// smoothly) and goes into the next step's command; the buttons are read in the fixed step (a short tap is never
    /// missed there: Input keeps a press until the next step).
    /// </summary>
    public sealed class LocalPlayerInput
    {
        /// <summary>Times the input map's Look (0.1 degrees per pixel, 180 degrees per second on the stick).</summary>
        public float Sensitivity = 1.0f;

        float yaw, pitch;
        bool crouchToggled;

        public LocalPlayerInput(float yaw, float pitch)
        {
            this.yaw = yaw;
            this.pitch = pitch;
        }

        public float Yaw => yaw;
        public float Pitch => pitch;

        /// <summary>Each frame (OnUpdate): the look. The mouse only while it is the game's (a free cursor works the editor).</summary>
        public void UpdateLook(float dt)
        {
            InputAction look = Input.GetAction("Look");
            if (!Input.MouseCaptured && !look.FromGamepad)
                return;
            Vector2 turn = look.Value * Sensitivity;
            yaw = ((yaw - turn.X * dt) % 360.0f + 360.0f) % 360.0f;
            pitch = Math.Clamp(pitch - turn.Y * dt, -89.0f, 89.0f);
        }

        /// <summary>Each fixed step (OnFixedUpdate): the step's command.</summary>
        public PlayerCommand Sample()
        {
            if (Input.WasPressed("CrouchToggle"))
                crouchToggled = !crouchToggled;
            bool crouchHeld = Input.IsHeld("Crouch");
            if (crouchHeld)
                crouchToggled = false; // holding Crouch takes over from the toggle
            Vector2 move = Input.GetAxis2D("Move");
            if (move.LengthSquared() > 1.0f)
                move = Vector2.Normalize(move);
            return new PlayerCommand
            {
                Move = move,
                Yaw = yaw,
                Pitch = pitch,
                Jump = Input.WasPressed("Jump"),
                Crouch = crouchHeld || crouchToggled,
                Walk = Input.IsHeld("Walk"),
            };
        }
    }
}
