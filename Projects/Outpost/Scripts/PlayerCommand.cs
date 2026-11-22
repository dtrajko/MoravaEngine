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

        /// <summary>Walking quietly: slower, and steps the guards hear less (Alt; a gamepad: a light push of the stick).</summary>
        public bool Walk;

        /// <summary>Sprinting: faster forward, louder steps (Shift). Walking quietly or crouching takes over from it.</summary>
        public bool Sprint;

        /// <summary>Fire is held (the automatic rifle fires on), and was pressed since the last step (a semi-automatic shot).</summary>
        public bool Fire, FirePressed;

        /// <summary>Reload was pressed; FireMode was pressed (the rifle: automatic or semi-automatic).</summary>
        public bool Reload, FireMode;

        /// <summary>Aim is held: down the sights (Phase 2.4); was pressed since the last step: the scope's next zoom (Phase 2.5).</summary>
        public bool Aim, AimPressed;

        /// <summary>The wheel's notches while scoped (Phase 2.5): positive zooms in, negative out (0 none).</summary>
        public float Zoom;

        /// <summary>
        /// Switching weapons (Phase 2.4): a slot's key was pressed (1 to 4; 0 none), the weapons were stepped through
        /// (+1 next, -1 previous: the wheel, the bumpers; 0 none), back to the one held before (Q).
        /// </summary>
        public int WeaponSlot, WeaponCycle;
        public bool LastWeapon;

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

        /// <summary>The look's turn times this too, set each frame (scoped: slower in proportion to the zoom; 1 otherwise).</summary>
        public float LookScale = 1.0f;

        /// <summary>Scoped (set each frame): the wheel zooms instead of switching weapons.</summary>
        public bool WheelZooms;

        float yaw, pitch;
        bool crouchToggled;
        float kickUp;      // degrees the shots' kicks raised the look, not yet taken back
        float sinceKick;   // seconds since the last kick
        float wheel;       // the wheel's notches since the last step (read each frame: a step may come after several)

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
            if (Input.MouseCaptured)
                wheel += Input.GetValue("WeaponScroll");
            InputAction look = Input.GetAction("Look");
            if (!Input.MouseCaptured && !look.FromGamepad)
                return;
            Vector2 turn = look.Value * Sensitivity * LookScale;
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
            InputAction moveAction = Input.GetAction("Move");
            Vector2 move = moveAction.Value;
            if (move.LengthSquared() > 1.0f)
                move = Vector2.Normalize(move);
            // A gamepad's stick pushed a little walks quietly (a key is all the way)
            bool lightPush = moveAction.FromGamepad && move.LengthSquared() > 0.0f && move.LengthSquared() < 0.55f * 0.55f;
            if (lightPush)
                move /= 0.55f; // the quiet walk's full speed at the edge of the light push
            int slot = 0;
            for (int key = 1; key <= 4 && slot == 0; key++)
                if (Input.WasPressed($"Weapon{key}"))
                    slot = key;
            // The wheel down (negative notches) takes the next weapon, up the previous one
            int cycle = Input.WasPressed("NextWeapon") ? 1 : Input.WasPressed("PreviousWeapon") ? -1 : 0;
            // Scoped the wheel zooms (up: in), and doesn't switch weapons
            float zoom = WheelZooms ? wheel : 0.0f;
            if (cycle == 0 && !WheelZooms && MathF.Abs(wheel) >= 0.5f)
                cycle = wheel < 0.0f ? 1 : -1;
            wheel = 0.0f;
            return new PlayerCommand
            {
                Move = move,
                Yaw = yaw,
                Pitch = pitch,
                Jump = Input.WasPressed("Jump"),
                Crouch = crouchHeld || crouchToggled,
                Walk = Input.IsHeld("Walk") || lightPush,
                Sprint = Input.IsHeld("Sprint"),
                Fire = Input.IsHeld("Fire"),
                FirePressed = Input.WasPressed("Fire"),
                Reload = Input.WasPressed("Reload"),
                FireMode = Input.WasPressed("FireMode"),
                Aim = Input.IsHeld("Aim"),
                AimPressed = Input.WasPressed("Aim"),
                Zoom = zoom,
                WeaponSlot = slot,
                WeaponCycle = cycle,
                LastWeapon = Input.WasPressed("LastWeapon"),
            };
        }

        /// <summary>
        /// A shot's kick to the look (degrees: up, and to the left positive), from the fixed step. The climb it added is taken
        /// back in RecoverKick (the sideways part stays: the player corrects it).
        /// </summary>
        public void Kick(float up, float left)
        {
            pitch = Math.Clamp(pitch - up, -89.0f, 89.0f); // pitch positive looks down
            yaw = ((yaw + left) % 360.0f + 360.0f) % 360.0f;
            kickUp += up;
            sinceKick = 0.0f;
        }

        /// <summary>Each frame: the kick's climb taken back at rate degrees a second, once delay seconds passed without one.</summary>
        public void RecoverKick(float dt, float rate, float delay)
        {
            sinceKick += dt;
            if (sinceKick < delay || kickUp <= 0.0f)
                return;
            float back = MathF.Min(kickUp, rate * dt);
            kickUp -= back;
            pitch = Math.Clamp(pitch + back, -89.0f, 89.0f);
        }
    }
}
