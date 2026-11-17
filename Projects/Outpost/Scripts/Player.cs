using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The player (docs/games/Shooter_Plan.html, Phase 1): first person, on a Character (Inspector: Physics) whose entity
    /// stands at the feet. Each fixed step the local input becomes a PlayerCommand and the PlayerMotor moves the character
    /// by it (run, walk quietly, crouch, jump, fall damage); each frame the camera goes to the eyes (lower when crouched,
    /// eased) and the steps are heard.
    ///
    /// F2 (DebugFly) hands the view to the debug fly camera (the Camera entity's FlyCamera) and back: the player stands
    /// still meanwhile.
    /// </summary>
    public class Player : Entity, IDamageable
    {
        /// <summary>The view: the scene's camera entity (with a FlyCamera script for F2).</summary>
        public Entity? Camera;

        /// <summary>Times the input map's Look.</summary>
        public float Sensitivity = 1.0f;

        /// <summary>Meters from the feet to the eyes, standing (crouched: lower by what the capsule is).</summary>
        public float EyeHeight = 1.62f;

        public float RunSpeed = 4.5f, SprintSpeed = 6.5f, WalkSpeed = 2.2f, CrouchSpeed = 1.8f;
        public float JumpHeight = 1.0f;
        public float CrouchHeight = 1.2f;

        /// <summary>Health points; falls take from them (Phase 3: death, health packs, armor).</summary>
        public float Health = 100.0f;

        /// <summary>
        /// Steps: the sound ({0}: 1 to FootstepVariants), meters between steps and volumes by the way it moves. Crouched it
        /// makes none (with Alt or without).
        /// </summary>
        public string FootstepSound = "Assets/Audio/Footstep_{0}.wav";
        public int FootstepVariants = 4;
        public float StrideRun = 1.6f, StrideSprint = 2.1f, StrideWalk = 1.0f;
        public float RunVolume = 0.6f, SprintVolume = 0.9f, WalkVolume = 0.12f;
        public string JumpSound = "Assets/Audio/JumpTakeoff.wav";
        public string LandSound = "Assets/Audio/Land.wav";

        CharacterController character = null!;
        PlayerMotor motor = null!;
        LocalPlayerInput input = null!;
        float eyeOffset;          // the eyes below EyeHeight now (eased towards the crouch's)
        float stepDistance;       // meters walked since the last step's sound
        int lastVariant;
        Vector3 lastFeet;
        PlayerCommand lastCommand;
        readonly Random random = new Random(7);

        bool Flying => Camera is FlyCamera fly && fly.IsActive;

        /// <summary>
        /// Commands from elsewhere instead of the input (a bot, a test driver; later a remote player): called each fixed step
        /// with the step's length. Null: the local input.
        /// </summary>
        public Func<float, PlayerCommand>? CommandSource { get; set; }

        /// <summary>The motor's state, for what shows it (the HUD, a test).</summary>
        public bool IsCrouched => motor.IsCrouched;
        public bool IsGrounded => motor.IsGrounded;
        public bool IsSprinting => motor.IsSprinting;
        public float CurrentHeight => character.Height;

        protected override void OnCreate()
        {
            character = GetComponent<CharacterController>() ??
                throw new InvalidOperationException($"{Name} needs a Character (Inspector: Physics) for Player");
            PlayerMotorSettings settings = new PlayerMotorSettings
            {
                RunSpeed = RunSpeed, SprintSpeed = SprintSpeed, WalkSpeed = WalkSpeed, CrouchSpeed = CrouchSpeed, JumpHeight = JumpHeight, CrouchHeight = CrouchHeight,
            };
            motor = new PlayerMotor(character, settings);
            motor.Jumped += () => PlaySound(JumpSound, 0.5f);
            motor.Landed += OnLanded;

            // The look starts as the camera was placed in the scene
            Vector3 angles = Camera != null ? Camera.EulerAngles : EulerAngles;
            input = new LocalPlayerInput(angles.Y, angles.X) { Sensitivity = Sensitivity };
            Input.CursorLocked = true;
            lastFeet = WorldPosition;
            Log.Info($"{Name}: the player, {character.StandingHeight:0.00} m tall ({CrouchHeight:0.00} m crouched), F2 the fly camera");
        }

        protected override void OnFixedUpdate(float dt)
        {
            PlayerCommand command = CommandSource != null ? CommandSource(dt) :
                Flying ? new PlayerCommand { Yaw = input.Yaw, Pitch = input.Pitch } : input.Sample();
            motor.Step(command, dt);
            lastCommand = command;
        }

        protected override void OnUpdate(float dt)
        {
            if (Input.WasPressed("DebugFly") && Camera is FlyCamera fly)
                fly.IsActive = !fly.IsActive;
            if (Flying)
                return;

            if (CommandSource == null)
                input.UpdateLook(dt);
            PlayerCommand look = lastCommand;

            // The eyes: down with the capsule when crouched, eased so the view doesn't jump
            float crouchDrop = character.StandingHeight - character.Height;
            eyeOffset += (crouchDrop - eyeOffset) * MathF.Min(1.0f, 14.0f * dt);
            if (Camera != null)
            {
                Camera.Translation = WorldPosition + new Vector3(0.0f, EyeHeight - eyeOffset, 0.0f);
                Camera.EulerAngles = CommandSource != null ? new Vector3(look.Pitch, look.Yaw, 0.0f) : new Vector3(input.Pitch, input.Yaw, 0.0f);
            }

            // The steps: one each stride walked on the ground, louder sprinting, quieter walking, none crouched
            Vector3 feet = WorldPosition;
            Vector3 moved = feet - lastFeet;
            lastFeet = feet;
            if (!motor.IsGrounded || motor.IsCrouched)
            {
                stepDistance = 0.0f;
                return;
            }
            stepDistance += new Vector2(moved.X, moved.Z).Length();
            float stride = motor.IsWalking ? StrideWalk : motor.IsSprinting ? StrideSprint : StrideRun;
            if (stepDistance >= stride)
            {
                stepDistance = 0.0f;
                PlayFootstep(motor.IsWalking ? WalkVolume : motor.IsSprinting ? SprintVolume : RunVolume);
            }
        }

        void OnLanded(float speed, float damage)
        {
            PlaySound(LandSound, Math.Clamp(speed / 12.0f, 0.15f, 1.0f));
            stepDistance = 0.0f;
            if (damage > 0.0f)
                Damage.Apply(this, new DamageInfo { Amount = damage, Point = WorldPosition, Direction = -Vector3.UnitY, Kind = "Fall" });
        }

        public void TakeDamage(in DamageInfo damage)
        {
            Health = MathF.Max(0.0f, Health - damage.Amount);
            Log.Info($"{Name}: {damage.Amount:0} damage ({damage.Kind}), {Health:0} health left");
        }

        void PlayFootstep(float volume)
        {
            if (FootstepVariants <= 0 || string.IsNullOrEmpty(FootstepSound))
                return;
            int variant = random.Next(1, FootstepVariants + 1);
            if (variant == lastVariant && FootstepVariants > 1)
                variant = variant % FootstepVariants + 1; // not the same twice in a row
            lastVariant = variant;
            Audio.PlayOneShot(string.Format(FootstepSound, variant), WorldPosition, volume, AudioBus.Effects, 0.95f + 0.1f * (float)random.NextDouble());
        }

        void PlaySound(string clip, float volume)
        {
            if (!string.IsNullOrEmpty(clip))
                Audio.PlayOneShot(clip, WorldPosition, volume);
        }
    }
}
