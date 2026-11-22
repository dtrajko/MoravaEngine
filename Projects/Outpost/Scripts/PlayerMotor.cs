using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>How a player's character moves: the numbers (the same on every machine: the rules, characters as data).</summary>
    public sealed class PlayerMotorSettings
    {
        /// <summary>Meters per second: moving (the default, a fast walk), sprinting (Sprint, forward), walking quietly (Walk), crouched.</summary>
        public float MoveSpeed = 3.0f, SprintSpeed = 6.5f, WalkSpeed = 2.2f, CrouchSpeed = 1.8f;

        /// <summary>
        /// The speeds above are forward and back; straight to the side (the fast walk and walking quietly; crouched as
        /// before, its clip has no side walks) this times as fast, in between scaled by how far
        /// the move is to the side (the angle's, linearly). People step sideways leading with one leg and closing with the
        /// other, never crossing them, which covers about a third of the walk (the fast walk's side clips: 1.15 m/s).
        /// </summary>
        public float SideSpeedScale = 1.15f / 3.0f;

        /// <summary>Meters per second squared to the wanted speed: on the ground, and the little control in the air.</summary>
        public float GroundAcceleration = 45.0f, AirAcceleration = 10.0f;

        /// <summary>Meters a jump lifts the feet; meters per second squared down.</summary>
        public float JumpHeight = 1.0f, Gravity = 20.0f;

        /// <summary>The capsule's height crouched (standing: the Character's, the Inspector's).</summary>
        public float CrouchHeight = 1.2f;

        /// <summary>
        /// Falls: landing faster than SafeFallSpeed (meters per second; 9 is a fall of about 2 m) hurts, DamagePerSpeed
        /// health points per meter per second over it (a fall of 5 m: about 50).
        /// </summary>
        public float SafeFallSpeed = 9.0f, DamagePerSpeed = 11.0f;
    }

    /// <summary>
    /// A player's character in the fixed step: from a PlayerCommand to the velocity the CharacterController walks with,
    /// jumps, gravity, crouching (E7: the capsule lower, standing up only with room above) and landing (fall damage).
    /// The game's rules, in the fixed step; what is seen and heard (the camera, the steps) reacts to its state and events.
    /// </summary>
    public sealed class PlayerMotor
    {
        readonly PlayerMotorSettings settings;
        readonly CharacterController character;
        Vector3 velocity;   // what it asked for in the last step
        bool wasGrounded = true;
        float fallSpeed;    // meters per second down, the fastest of this time in the air

        public PlayerMotor(CharacterController character, PlayerMotorSettings settings)
        {
            this.character = character;
            this.settings = settings;
        }

        public bool IsCrouched { get; private set; }
        public bool IsGrounded => character.IsGrounded;
        public bool IsWalking { get; private set; }

        /// <summary>Sprinting (Shift, forward): the weapon lowered, no shots.</summary>
        public bool IsSprinting { get; private set; }
        public bool IsWeaponLowered => IsSprinting;

        /// <summary>
        /// Aiming down the sights (Phase 2.4, set by the player each step): the moving speeds times this, and no sprint
        /// (1: not aiming).
        /// </summary>
        public float AimSpeedScale { get; set; } = 1.0f;

        /// <summary>The weapon out (Phase 2.5, set by the player each step): every moving speed times this (the sniper rifle's: slower).</summary>
        public float CarrySpeedScale { get; set; } = 1.0f;

        /// <summary>How it really moves (after the last step), meters per second in the world.</summary>
        public Vector3 Velocity => character.Velocity;

        /// <summary>It left the ground by jumping, in this step.</summary>
        public event Action? Jumped;

        /// <summary>It came down: the speed it landed with (meters per second) and the damage that does (0: a safe landing).</summary>
        public event Action<float, float>? Landed;

        /// <summary>
        /// How far a move (the command's: x right, y forward) is to the side, 0 ahead or back to 1 straight to the side:
        /// its angle from the forward-back line over 90 degrees. The body's clips blend by the same (Player.manim's Move).
        /// </summary>
        public static float Sideways(Vector2 move)
        {
            if (move.LengthSquared() < 1e-6f)
                return 0.0f;
            return MathF.Atan2(MathF.Abs(move.X), MathF.Abs(move.Y)) / (MathF.PI * 0.5f);
        }

        /// <summary>The speed's scale for a move's direction: 1 ahead or back, sideScale straight to the side.</summary>
        public static float SideScale(Vector2 move, float sideScale) => 1.0f + (sideScale - 1.0f) * Sideways(move);

        public void Step(in PlayerCommand command, float dt)
        {
            bool grounded = character.IsGrounded;

            // Landing: down from the air, as fast as it was falling
            if (grounded && !wasGrounded)
            {
                float damage = fallSpeed > settings.SafeFallSpeed ? (fallSpeed - settings.SafeFallSpeed) * settings.DamagePerSpeed : 0.0f;
                Landed?.Invoke(fallSpeed, damage);
                fallSpeed = 0.0f;
            }
            wasGrounded = grounded;

            // Crouching: down at once; up when there is room (asked again each step until it stands)
            if (command.Crouch && !IsCrouched)
                IsCrouched = character.SetHeight(settings.CrouchHeight);
            else if (!command.Crouch && IsCrouched)
                IsCrouched = !character.SetHeight(character.StandingHeight);

            // Along the ground: towards the wanted velocity, quickly on the ground and a little in the air
            IsWalking = command.Walk && !IsCrouched;
            // Sprinting: forward only (the user, 2026-11-21: back and to the side looked bad), not while walking quietly,
            // crouched or aiming down the sights
            IsSprinting = command.Sprint && !IsWalking && !IsCrouched && command.Move.Y > 0.5f && AimSpeedScale >= 1.0f;
            float speed = IsCrouched ? settings.CrouchSpeed : IsWalking ? settings.WalkSpeed : IsSprinting ? settings.SprintSpeed : settings.MoveSpeed;
            if (!IsCrouched && !IsSprinting)
                speed *= SideScale(command.Move, settings.SideSpeedScale); // (crouched: the library's walk, forward only, no side clips yet)
            speed *= AimSpeedScale * CarrySpeedScale;
            Vector3 wanted = (command.Forward * command.Move.Y + command.Right * command.Move.X) * speed;
            Vector2 horizontal = new Vector2(velocity.X, velocity.Z);
            horizontal = MoveTowards(horizontal, new Vector2(wanted.X, wanted.Z), (grounded ? settings.GroundAcceleration : settings.AirAcceleration) * dt);
            // (Into a wall the character slides along it, and stops when it can't: it keeps asking, as Sample's do)

            // Up and down: on the ground standing (or a jump), in the air falling; a ceiling stops the rise
            float up = velocity.Y;
            if (grounded)
            {
                up = 0.0f;
                if (command.Jump && !IsCrouched)
                {
                    up = MathF.Sqrt(2.0f * settings.Gravity * settings.JumpHeight);
                    Jumped?.Invoke();
                }
            }
            else
            {
                if (up > 0.0f && character.Velocity.Y <= 0.0f)
                    up = 0.0f; // its head hit something
                up -= settings.Gravity * dt;
                fallSpeed = MathF.Max(fallSpeed, -up);
            }

            velocity = new Vector3(horizontal.X, up, horizontal.Y);
            character.Move(velocity);
        }

        static Vector2 MoveTowards(Vector2 from, Vector2 to, float maxDelta)
        {
            Vector2 delta = to - from;
            float length = delta.Length();
            return length <= maxDelta || length < 1e-6f ? to : from + delta / length * maxDelta;
        }
    }
}
