using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The player (docs/games/Shooter_Plan.html, Phase 1), on a Character (Inspector: Physics) whose entity stands at the
    /// feet. Each fixed step the local input becomes a PlayerCommand and the PlayerMotor moves the character by it (a
    /// fast walk, sprint, walk quietly, crouch, jump, fall damage); each frame the body (PlayerBody: the Body child's model
    /// and animator) shows it, the camera goes to the eyes (lower when crouched, eased) or over the shoulder (PlayerView),
    /// and the steps are heard. SwitchView (V) changes the view, SwapShoulder (X) the shoulder; both are kept in the
    /// settings. In first person the body (and the rifle in its hand) is hidden from the camera, its shadow stays, and the
    /// view model (the arms and the placeholder gun, models marked View Model: E2) is shown at the eyes instead. In third
    /// person the body holds the rifle in its hand and aims where the camera looks (Phase 2.1: E3, E5).
    ///
    /// F2 (DebugFly) hands the view to the debug fly camera (the Camera entity's FlyCamera) and back: the player stands
    /// still meanwhile.
    /// </summary>
    public class Player : Entity, IDamageable
    {
        /// <summary>The view: the scene's camera entity (with a FlyCamera script for F2).</summary>
        public Entity? Camera;

        /// <summary>The body: a child at the feet with the model and its animator (Player.manim).</summary>
        public Entity? Body;

        /// <summary>The rifle in the body's right hand: the body's child on its hand_r bone (Inspector: Bone Attachment).</summary>
        public Entity? BodyGun;

        /// <summary>
        /// First person's view model (Inspector: Model, View Model): the arms (ViewModel.manim), placed at the eyes each frame,
        /// and the gun they hold, its child. Shown only in first person.
        /// </summary>
        public Entity? ViewModel;
        public Entity? ViewModelGun;

        /// <summary>
        /// Where the view model's eye is in its model (the aim pose's right eye, by the sight line) and how far it is moved from
        /// the camera in the camera's space (meters: right, up, forward), so the gun sits low and to the right of the crosshair.
        /// The aim pose holds the gun 6.5 cm forward and 4.5 cm lower than the sight line at the eye (the face off the gun in
        /// third person): the eye here moves with the gun, so the first-person view keeps its picture.
        /// </summary>
        public Vector3 ViewModelEye = new Vector3(-0.118f, 1.529f, 0.180f);
        public Vector3 ViewModelOffset = new Vector3(0.11f, -0.09f, 0.0f);

        /// <summary>Third person: meters behind the shoulder, and to the side of the eyes.</summary>
        public float ThirdPersonDistance = 2.5f;
        public float ShoulderOffset = 0.5f;

        /// <summary>Times the input map's Look.</summary>
        public float Sensitivity = 1.0f;

        /// <summary>Meters from the feet to the eyes, standing (crouched: lower by what the capsule is).</summary>
        public float EyeHeight = 1.62f;

        public float MoveSpeed = 3.0f, SprintSpeed = 6.5f, WalkSpeed = 2.2f, CrouchSpeed = 1.8f;
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
        public float StrideMove = 1.0f, StrideSprint = 2.1f, StrideWalk = 1.0f;
        /// <summary>Straight to the side the strides are this times as long (a step-together's: 0.36 m), in between blended as Move's clips are.</summary>
        public float StrideSideScale = 0.364f;
        public float MoveVolume = 0.45f, SprintVolume = 0.9f, WalkVolume = 0.12f;
        public string JumpSound = "Assets/Audio/JumpTakeoff.wav";
        public string LandSound = "Assets/Audio/Land.wav";

        CharacterController character = null!;
        PlayerMotor motor = null!;
        LocalPlayerInput input = null!;
        PlayerBody? body;
        PlayerView view = null!;
        Model? viewModelArms, viewModelGun;
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

        /// <summary>The view (V switches it) and the shoulder (X): kept in the settings.</summary>
        public ViewMode View
        {
            get => view.Mode;
            set
            {
                view.Mode = value;
                Settings.SetBool(ThirdPersonSetting, value == ViewMode.ThirdPerson);
            }
        }

        public bool LeftShoulder
        {
            get => view.LeftShoulder;
            set
            {
                view.LeftShoulder = value;
                Settings.SetBool(LeftShoulderSetting, value);
            }
        }

        /// <summary>The body's state (Player.manim's), its heading, and where the camera is: for what shows them (a test).</summary>
        public string BodyState => body?.State ?? "";
        public float BodyHeading => body?.Heading ?? 0.0f;
        public float BodyMoveAngle => body?.MoveAngle ?? 0.0f;
        public bool BodyHidden => body?.HiddenInMainView ?? false;
        public Vector3 CameraPosition => view.Position;
        public bool ViewModelShown => viewModelArms != null && !viewModelArms.HiddenInMainView;

        const string ThirdPersonSetting = "ThirdPerson", LeftShoulderSetting = "LeftShoulder";

        protected override void OnCreate()
        {
            character = GetComponent<CharacterController>() ??
                throw new InvalidOperationException($"{Name} needs a Character (Inspector: Physics) for Player");
            PlayerMotorSettings settings = new PlayerMotorSettings
            {
                MoveSpeed = MoveSpeed, SprintSpeed = SprintSpeed, WalkSpeed = WalkSpeed, CrouchSpeed = CrouchSpeed, JumpHeight = JumpHeight, CrouchHeight = CrouchHeight,
            };
            motor = new PlayerMotor(character, settings);
            motor.Jumped += () => PlaySound(JumpSound, 0.5f);
            motor.Jumped += () => body?.OnJumped();
            motor.Landed += OnLanded;

            if (Body != null)
                body = new PlayerBody(Body, new PlayerBodySettings(), BodyGun);
            else
                Log.Warn($"{Name}: no Body, the player has no model (and no shadow)");
            viewModelArms = ViewModel?.GetComponent<Model>();
            viewModelGun = ViewModelGun?.GetComponent<Model>();
            view = new PlayerView(new PlayerViewSettings { Distance = ThirdPersonDistance, ShoulderOffset = ShoulderOffset },
                Settings.GetBool(ThirdPersonSetting) ? ViewMode.ThirdPerson : ViewMode.FirstPerson, Settings.GetBool(LeftShoulderSetting));

            // The look starts as the camera was placed in the scene
            Vector3 angles = Camera != null ? Camera.EulerAngles : EulerAngles;
            input = new LocalPlayerInput(angles.Y, angles.X) { Sensitivity = Sensitivity };
            Input.CursorLocked = true;
            lastFeet = WorldPosition;
            Log.Info($"{Name}: the player, {character.StandingHeight:0.00} m tall ({CrouchHeight:0.00} m crouched), {view.Mode}; V the view, X the shoulder, F2 the fly camera");
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
            {
                body?.Update(dt, Vector3.Zero, motor.IsGrounded, motor.IsCrouched, false, false, lastCommand.Yaw, lastCommand.Pitch);
                if (body != null)
                    body.HiddenInMainView = false; // the fly camera sees the player standing there
                ShowViewModel(false);
                return;
            }
            if (Input.WasPressed("SwitchView"))
                View = View == ViewMode.FirstPerson ? ViewMode.ThirdPerson : ViewMode.FirstPerson;
            if (Input.WasPressed("SwapShoulder"))
                LeftShoulder = !LeftShoulder;

            if (CommandSource == null)
                input.UpdateLook(dt);
            float yaw = CommandSource != null ? lastCommand.Yaw : input.Yaw;
            float pitch = CommandSource != null ? lastCommand.Pitch : input.Pitch;

            // The body shows what the motor does (the same in both views)
            body?.Update(dt, motor.Velocity, motor.IsGrounded, motor.IsCrouched, motor.IsWalking, motor.IsSprinting, yaw, pitch);

            // The eyes: down with the capsule when crouched, eased so the view doesn't jump; the camera there or over the shoulder
            float crouchDrop = character.StandingHeight - character.Height;
            eyeOffset += (crouchDrop - eyeOffset) * MathF.Min(1.0f, 14.0f * dt);
            Vector3 eyes = WorldPosition + new Vector3(0.0f, EyeHeight - eyeOffset, 0.0f);
            Vector3 cameraPosition = view.Update(dt, eyes, yaw, pitch, this);
            if (body != null)
                body.HiddenInMainView = view.HidesBody;
            if (Camera != null)
            {
                Camera.Translation = cameraPosition;
                Camera.EulerAngles = new Vector3(pitch, yaw, 0.0f);
            }
            // The view model in first person, once the camera is at the eyes (not while a switch glides)
            bool firstPerson = view.Mode == ViewMode.FirstPerson && Vector3.DistanceSquared(cameraPosition, eyes) < 0.0001f;
            ShowViewModel(firstPerson);
            if (firstPerson)
                PlaceViewModel(cameraPosition, yaw, pitch);

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
            float stride = motor.IsWalking ? StrideWalk : motor.IsSprinting ? StrideSprint : StrideMove;
            if (!motor.IsSprinting && body != null)
            {
                float angle = MathF.Abs(body.MoveAngle);
                float side = (angle <= 90.0f ? angle : 180.0f - angle) / 90.0f;
                stride *= 1.0f + (StrideSideScale - 1.0f) * side;
            }
            if (stepDistance >= stride)
            {
                stepDistance = 0.0f;
                PlayFootstep(motor.IsWalking ? WalkVolume : motor.IsSprinting ? SprintVolume : MoveVolume);
            }
        }

        // The view model's model faces +Z (the camera looks along -Z): turned half round, then by the look. Its eye at the camera,
        // moved by the offset in the camera's space (the model's X is the camera's left, its Z the camera's forward)
        void PlaceViewModel(Vector3 cameraPosition, float yaw, float pitch)
        {
            if (ViewModel == null)
                return;
            Quaternion rotation = Quaternion.CreateFromYawPitchRoll((yaw + 180.0f) * MathF.PI / 180.0f, pitch * MathF.PI / 180.0f, 0.0f);
            Vector3 offset = new Vector3(-ViewModelOffset.X, ViewModelOffset.Y, ViewModelOffset.Z);
            ViewModel.Rotation = rotation;
            ViewModel.Translation = cameraPosition + Vector3.Transform(offset - ViewModelEye, rotation);
        }

        void ShowViewModel(bool shown)
        {
            Show(viewModelArms, shown);
            Show(viewModelGun, shown);
        }

        static void Show(Model? model, bool shown)
        {
            if (model != null && model.HiddenInMainView == shown)
                model.HiddenInMainView = !shown;
        }

        void OnLanded(float speed, float damage)
        {
            PlaySound(LandSound, Math.Clamp(speed / 12.0f, 0.15f, 1.0f));
            body?.OnLanded(speed);
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
