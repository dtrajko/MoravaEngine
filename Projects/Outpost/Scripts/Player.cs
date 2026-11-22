using System;
using System.Collections.Generic;
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
    /// settings. In first person the body (and the weapon in its hand) is hidden from the camera, its shadow stays, and the
    /// view model (the arms and the weapon, models marked View Model: E2) is shown at the eyes instead. In third person
    /// the body holds the weapon in its hand and aims where the camera looks (Phase 2.1: E3, E5).
    ///
    /// The weapons (Phase 2.4: PlayerArsenal; each a PlayerWeapon, its numbers from Assets/Weapons/&lt;name&gt;.weapon.json)
    /// are the rifle, the pistol and the knife, switched with the number keys, the wheel and Q; each has its model on the
    /// body's hand and on the view model's (the one out is visible: Model.Visible), and its own animator states. The
    /// weapon out fires, reloads and switches its fire mode (the knife strikes) in the fixed step from the same commands;
    /// its shots kick the look, and are heard here (Phase 2.2) and seen through Effects (WeaponEffects, Phase 2.3: the
    /// flash, the tracers, the hits), with a hit marker on the crosshair. Aim held aims down the sights: the view narrows,
    /// the first person's eye goes onto the sight line, the third person's camera comes closer.
    ///
    /// Phase 2.5, the sniper rifle (slot 2): its bolt worked after each shot (the body's and the arms' clip, its sounds),
    /// and its scope. Aim steps the zoom (4x, 10x, off), the wheel zooms between 2x and 12x. Scoping, the camera goes to
    /// the eyes (in third person too, back after), the view model comes up to the eye, then the scope's overlay (the HUD's)
    /// shows with a short darkening while the view narrows to the zoom; the view sways with the weapon's breathing (the
    /// shot goes where it looks), the look turns slower in proportion to the zoom (times the ScopeSensitivity setting).
    /// The camera's Normal Field of View is the unzoomed one, so far terrain and trees keep their detail (E10).
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

        /// <summary>The weapons in the body's right hand: the body's children on its hand_r bone (Inspector: Bone Attachment).</summary>
        public Entity? BodyGun, BodySniper, BodyPistol, BodyKnife;

        /// <summary>
        /// The sniper rifle on the left hand's bone (hand_l): shown instead of the right hand's while the right hand works
        /// the bolt (the weapon's LeftHand: its place there, by the clip). The body's, and the view model's below.
        /// </summary>
        public Entity? BodySniperLeft, ViewModelSniperLeft;

        /// <summary>
        /// First person's view model (Inspector: Model, View Model): the arms (ViewModel.manim), placed at the eyes each frame,
        /// and the weapons they hold, its children. Shown only in first person.
        /// </summary>
        public Entity? ViewModel;
        public Entity? ViewModelGun, ViewModelSniper, ViewModelPistol, ViewModelKnife;

        /// <summary>Taking a weapon out, the view model comes up from this far below the view (meters), turned this far down (degrees).</summary>
        public float LoweredDrop = 0.28f, LoweredTilt = 35.0f;

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

        /// <summary>
        /// The weapons carried (Phase 2.4), their data files' names (Assets/Weapons/&lt;name&gt;.weapon.json) separated by
        /// commas; the first one is out at the start. The shots' volume, the knife's.
        /// </summary>
        public string WeaponNames = "Rifle,Sniper,Pistol,Knife";
        public float ShotVolume = 0.8f, KnifeVolume = 0.7f;

        /// <summary>The scope's zoom stepped (Aim): its click, and the volume.</summary>
        public string ScopeSound = "Assets/Audio/Rifle_FireMode.wav";
        public float ScopeVolume = 0.25f;

        /// <summary>
        /// What the shots look like (Phase 2.3): the entity with the WeaponEffects script and its emitters. The hit marker's
        /// sound, its volume, and how long the marker shows (seconds; a kill's a little longer).
        /// </summary>
        public Entity? Effects;
        public string HitMarkerSound = "Assets/Audio/HitMarker.wav";
        public float HitMarkerVolume = 0.5f;
        public float HitMarkerSeconds = 0.18f, KillMarkerSeconds = 0.35f;

        CharacterController character = null!;
        PlayerMotor motor = null!;
        LocalPlayerInput input = null!;
        PlayerBody? body;
        PlayerView view = null!;
        PlayerViewSettings viewSettings = null!;
        Model? viewModelArms;
        float eyeOffset;          // the eyes below EyeHeight now (eased towards the crouch's)
        float stepDistance;       // meters walked since the last step's sound
        int lastVariant;
        Vector3 lastFeet;
        PlayerCommand lastCommand;
        readonly Random random = new Random(7);
        PlayerArsenal arsenal = null!;
        Animator? viewModelAnimator;
        Morava.Camera? camera;
        float baseFieldOfView = 70.0f, baseViewModelFieldOfView = 55.0f;
        float aimBlend;               // 0 at the hip, 1 down the sights (eased, the weapon's AimSeconds)
        Vector3 lastEyes, lastCamera; // the last frame's: where the shots of the next fixed steps aim from
        int reloadSounds;             // the reload's sounds played so far (its cues)
        int boltSounds;               // the bolt's sounds played so far (its cues)
        float scopeBlend;             // 0 down, 1 up: the scope coming up (the view model to the eye, then the overlay)
        float shownZoom = 1.0f;       // the zoom the view shows (eased toward the weapon's)
        float sinceStep;              // seconds since the last fixed step (the sway between steps)
        float scopeSensitivity = 1.0f;
        WeaponEffects? effects;
        readonly List<ShotResult> shotsToShow = new(); // fired in fixed steps, shown in the frame
        float hitMarker;              // seconds the hit marker still shows

        /// <summary>Each weapon's models: on the body's hand, on the view model's.</summary>
        readonly Dictionary<PlayerWeapon, (Entity? Body, Entity? View)> held = new();

        /// <summary>The weapons' models on the left hand (the sniper rifle's: while the right hand works the bolt).</summary>
        readonly Dictionary<PlayerWeapon, (Entity? Body, Entity? View)> heldLeft = new();
        WeaponData.LeftHold? leftPlaced; // the left models' place now (the bolt's or the reload's)
        bool inLeftHand;

        bool Flying => Camera is FlyCamera fly && fly.IsActive;

        /// <summary>The weapon out (null: none could be read).</summary>
        public PlayerWeapon? Weapon => arsenal.Current;

        /// <summary>The weapons carried, the switching.</summary>
        public PlayerArsenal Arsenal => arsenal;

        /// <summary>Down the sights now (eased: 0 at the hip, 1 aiming), for the HUD and a test.</summary>
        public float AimBlend => aimBlend;

        /// <summary>
        /// The scope's overlay (Phase 2.5): shown (the view through the scope), how dark it still is coming up (0 to 1), and
        /// the zoom the view shows.
        /// </summary>
        public bool ScopeShown { get; private set; }
        public float ScopeDarkness { get; private set; }
        public float ScopeZoom => ScopeShown ? shownZoom : 1.0f;

        /// <summary>The setting the scope's turning is times (1: in proportion to the zoom alone).</summary>
        public const string ScopeSensitivitySetting = "ScopeSensitivity";

        /// <summary>The debug fly camera has the view (F2): the player stands still, the HUD is hidden.</summary>
        public bool IsFlying => Flying;

        /// <summary>The last shot, and the knife's last strike (a test reads them).</summary>
        public ShotResult LastShot { get; private set; }
        public ShotResult LastStrike { get; private set; }

        /// <summary>The hit marker on the crosshair: how much it still shows (1 at a hit, 0 gone), and whether the hit downed its target.</summary>
        public float HitMarker => KillMarker ? hitMarker / KillMarkerSeconds : hitMarker / HitMarkerSeconds;
        public bool KillMarker { get; private set; }

        /// <summary>Where the muzzle is now (the body's weapon, in both views: the same rules), the eyes if there is none.</summary>
        public Vector3 MuzzlePosition
        {
            get
            {
                PlayerWeapon? weapon = Weapon;
                Entity? gun = weapon != null && held.TryGetValue(weapon, out var models) ? models.Body : null;
                return gun != null && weapon != null ? Vector3.Transform(weapon.Data.Muzzle, gun.WorldTransform) : lastEyes;
            }
        }

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
        public string BodyUpperState => body?.UpperState ?? "";
        public float BodyHeading => body?.Heading ?? 0.0f;
        public float BodyMoveAngle => body?.MoveAngle ?? 0.0f;
        public bool BodyHidden => body?.HiddenInMainView ?? false;
        public Vector3 CameraPosition => view.Position;
        public bool ViewModelShown => viewModelArms != null && !viewModelArms.HiddenInMainView;
        public string ViewModelState => viewModelAnimator?.GetLayerState("") ?? "";

        /// <summary>The model of a weapon on the body's hand and on the view model's (a test reads them).</summary>
        public (Entity? Body, Entity? View) ModelsOf(PlayerWeapon weapon) => held.TryGetValue(weapon, out var models) ? models : (null, null);

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
            viewModelAnimator = ViewModel?.GetComponent<Animator>();
            camera = Camera?.GetComponent<Morava.Camera>();
            if (camera != null)
            {
                baseFieldOfView = camera.FieldOfView;
                baseViewModelFieldOfView = camera.ViewModelFieldOfView;
                // The levels of detail are meant for the normal view: zoomed in, far things keep the detail of their size (E10)
                camera.NormalFieldOfView = baseFieldOfView;
            }
            scopeSensitivity = Math.Clamp(Settings.GetFloat(ScopeSensitivitySetting, 1.0f), 0.1f, 3.0f);
            effects = Effects as WeaponEffects;
            if (Effects != null && effects == null)
                Log.Warn($"{Name}: Effects has no WeaponEffects script: the shots show nothing");

            CreateArsenal();
            viewSettings = new PlayerViewSettings { Distance = ThirdPersonDistance, ShoulderOffset = ShoulderOffset };
            view = new PlayerView(viewSettings, Settings.GetBool(ThirdPersonSetting) ? ViewMode.ThirdPerson : ViewMode.FirstPerson,
                Settings.GetBool(LeftShoulderSetting));

            // The look starts as the camera was placed in the scene
            Vector3 angles = Camera != null ? Camera.EulerAngles : EulerAngles;
            input = new LocalPlayerInput(angles.Y, angles.X) { Sensitivity = Sensitivity };
            Input.CursorLocked = true;
            lastFeet = WorldPosition;
            lastEyes = lastCamera = WorldPosition + new Vector3(0.0f, EyeHeight, 0.0f);
            List<string> carried = new();
            foreach (PlayerWeapon w in arsenal.Weapons)
                carried.Add($"{w.Data.Slot} {w.Data.Name}");
            Log.Info($"{Name}: the player, {character.StandingHeight:0.00} m tall ({CrouchHeight:0.00} m crouched), {view.Mode}; " +
                $"{string.Join(", ", carried)}; V the view, X the shoulder, F2 the fly camera");
        }

        // The weapons by their names, each with its models (the scene's, by the weapon's name), the first one out
        void CreateArsenal()
        {
            Dictionary<string, (Entity? Body, Entity? View)> models = new()
            {
                ["Rifle"] = (BodyGun, ViewModelGun), ["Sniper"] = (BodySniper, ViewModelSniper),
                ["Pistol"] = (BodyPistol, ViewModelPistol), ["Knife"] = (BodyKnife, ViewModelKnife),
            };
            List<PlayerWeapon> carried = new();
            foreach (string name in WeaponNames.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
            {
                WeaponData? data = WeaponData.Load(name);
                if (data == null)
                    continue;
                PlayerWeapon weapon = new PlayerWeapon(data, this);
                weapon.Fired += OnFired;
                weapon.DryFired += () => PlaySound(data.EmptySound, 0.6f);
                weapon.FireModeChanged += mode => { PlaySound(data.FireModeSound, 0.5f); Log.Info($"{Name}: {data.Name} {mode}"); };
                weapon.ReloadStarted += () => { reloadSounds = 0; body?.Reloading(true); PlayView(data.ViewReload, 0.1f); };
                weapon.ReloadFinished += () => { body?.Reloading(false); PlayView(data.ViewAim, 0.15f); };
                weapon.Swung += () => OnSwung(weapon);
                weapon.Struck += OnStruck;
                weapon.AttackEnded += () => { body?.Attack(null); PlayView(data.ViewAim, 0.15f); };
                weapon.BoltStarted += () => { boltSounds = 0; body?.Attack(data.BodyBolt); PlayView(data.ViewBolt, 0.08f); };
                weapon.BoltFinished += () => { body?.Attack(null); PlayView(data.ViewAim, 0.12f); };
                weapon.ZoomStepped += zoom => PlaySound(ScopeSound, ScopeVolume);
                carried.Add(weapon);
                if (!models.TryGetValue(data.Name, out var pair))
                    Log.Warn($"{Name}: no models for the weapon {data.Name}");
                held[weapon] = pair;
                if (data.Name == "Sniper")
                {
                    heldLeft[weapon] = (BodySniperLeft, ViewModelSniperLeft);
                    if (BodySniperLeft == null || ViewModelSniperLeft == null)
                        Log.Warn($"{Name}: no left-hand models for the {data.Name}: it moves with the right hand on the bolt");
                }
            }
            arsenal = new PlayerArsenal(carried);
            arsenal.Holstering += weapon => PlayView(weapon.Data.ViewAim, 0.1f);
            arsenal.Switched += (from, to) => ShowWeapon(to);
            arsenal.Drawn += weapon => Log.Info($"{Name}: {weapon.Data.Name} out");
            // The first weapon named is out at the start (the arsenal sorts them by slot)
            if (carried.Count > 0)
                arsenal.Choose(arsenal.IndexOfSlot(carried[0].Data.Slot));
            // Only the one out: the others' models hidden everywhere (nothing of them, not even a shadow)
            foreach (var (weapon, pair) in held)
                SetVisible(pair, weapon == arsenal.Current);
            foreach (var pair in heldLeft.Values)
                SetVisible(pair, false);
            if (arsenal.Current != null)
                ShowWeapon(arsenal.Current, quiet: true);
        }

        // A weapon comes out: its models shown, the other ones' hidden, the body's and the view model's states its own
        void ShowWeapon(PlayerWeapon weapon, bool quiet = false)
        {
            foreach (var (other, pair) in held)
                SetVisible(pair, other == weapon);
            foreach (var pair in heldLeft.Values)
                SetVisible(pair, false);
            inLeftHand = false;
            Entity? bodyModel = held.TryGetValue(weapon, out var models) ? models.Body : null;
            WeaponData data = weapon.Data;
            body?.SetWeapon(bodyModel, data.BodyAim, data.BodyReady, data.BodyReload);
            PlayView(data.ViewAim, 0.0f);
            if (!quiet)
                PlaySound(data.DrawSound, 0.5f);
        }

        static void SetVisible((Entity? Body, Entity? View) pair, bool visible)
        {
            foreach (Entity? e in new[] { pair.Body, pair.View })
                if (e?.GetComponent<Model>() is Model model && model.Visible != visible)
                    model.Visible = visible;
        }

        void PlayView(string state, float fade)
        {
            if (!string.IsNullOrEmpty(state))
                viewModelAnimator?.Play(state, fade);
        }

        protected override void OnFixedUpdate(float dt)
        {
            PlayerCommand command = CommandSource != null ? CommandSource(dt) :
                Flying ? new PlayerCommand { Yaw = input.Yaw, Pitch = input.Pitch } : input.Sample();
            arsenal.Step(command, dt);
            PlayerWeapon? weapon = arsenal.Current;
            // Aiming down the sights slows the walk and stops the sprint: Aim held wins over Shift (with a weapon that can
            // aim, not reloading, not switching). A scope: while its zoom is wanted (Aim pressed it), also during the bolt
            bool aiming = weapon != null && !weapon.IsReloading && !arsenal.IsSwitching &&
                (weapon.Data.HasScope ? weapon.Zoom > 0.0f || command.AimPressed : command.Aim && weapon.Data.CanAimDownSights);
            motor.AimSpeedScale = aiming ? weapon!.Data.AimSpeedScale : 1.0f;
            motor.CarrySpeedScale = weapon?.Data.MoveSpeedScale ?? 1.0f;
            motor.Step(command, dt);
            lastCommand = command;
            sinceStep = 0.0f;
            if (weapon != null)
            {
                Vector3 velocity = motor.Velocity;
                // While a switch is under way the weapon doesn't fire (nor aims): no buttons for it
                PlayerCommand forWeapon = command;
                if (arsenal.IsSwitching)
                {
                    forWeapon.Fire = forWeapon.FirePressed = forWeapon.Reload = forWeapon.FireMode = forWeapon.Aim = forWeapon.AimPressed = false;
                    forWeapon.Zoom = 0.0f;
                }
                weapon.Step(forWeapon, new AimContext
                {
                    Camera = lastCamera, Eyes = lastEyes, Muzzle = MuzzlePosition,
                    Speed = new Vector2(velocity.X, velocity.Z).Length(),
                    Grounded = motor.IsGrounded, Crouched = motor.IsCrouched, Sprinting = motor.IsWeaponLowered,
                }, dt);
            }
        }

        void OnFired(ShotResult shot)
        {
            LastShot = shot;
            WeaponData data = Weapon!.Data;
            if (data.ShotVariants > 0 && !string.IsNullOrEmpty(data.ShotSound))
                Audio.PlayOneShot(string.Format(data.ShotSound, random.Next(1, data.ShotVariants + 1)), shot.From, ShotVolume,
                    AudioBus.Effects, 0.97f + 0.06f * (float)random.NextDouble());
            if (CommandSource == null)
                input.Kick(shot.KickUp, shot.KickLeft); // the local player's look (a bot's or a test's commands bring their own)
            PlayView(data.ViewFire, 0.02f);
            shotsToShow.Add(shot); // its flash and its hit: in the frame, where the view model is then
            if (shot.Dealt)
                ShowHitMarker(shot.Target);
        }

        // The knife: the strike starts (its clip on the body and the arms, the swish), its blade arrives (what it struck)
        void OnSwung(PlayerWeapon weapon)
        {
            WeaponData data = weapon.Data;
            body?.Attack(data.BodyAttack);
            PlayView(data.ViewAttack, 0.05f);
            if (data.SwingVariants > 0 && !string.IsNullOrEmpty(data.SwingSound))
                Audio.PlayOneShot(string.Format(data.SwingSound, random.Next(1, data.SwingVariants + 1)), lastEyes, KnifeVolume * 0.7f,
                    AudioBus.Effects, 0.95f + 0.1f * (float)random.NextDouble());
        }

        void OnStruck(ShotResult strike)
        {
            LastStrike = strike;
            if (!strike.Hit)
                return;
            WeaponData data = Weapon!.Data;
            bool flesh = WeaponEffects.SurfaceOf(strike) == Surface.Flesh;
            string sound = flesh ? data.HitFleshSound : data.HitHardSound;
            if (!string.IsNullOrEmpty(sound))
                Audio.PlayOneShot(sound, strike.Point, KnifeVolume, AudioBus.Effects, 0.95f + 0.1f * (float)random.NextDouble());
            effects?.Strike(strike);
            if (strike.Dealt)
                ShowHitMarker(strike.Target);
            if (strike.Behind && strike.Dealt)
                Log.Info($"{Name}: a takedown ({strike.Target?.Name})");
        }

        void ShowHitMarker(Entity? target)
        {
            bool kill = target is Dummy dummy && dummy.IsDown;
            KillMarker = kill || (KillMarker && hitMarker > 0.0f);
            hitMarker = KillMarker ? KillMarkerSeconds : HitMarkerSeconds;
            PlaySound(HitMarkerSound, HitMarkerVolume * (kill ? 1.3f : 1.0f));
        }

        // The shots fired since the last frame: their flashes at the muzzle (in first person the view model's, placed this
        // frame), their tracers and hits
        void ShowShots(bool firstPerson)
        {
            if (shotsToShow.Count == 0)
                return;
            PlayerWeapon? weapon = Weapon;
            if (effects != null && weapon != null && held.TryGetValue(weapon, out var models))
            {
                Vector3 muzzle = MuzzlePosition;
                Vector3 direction = models.Body != null ? GunForward(models.Body) : shotsToShow[^1].Direction;
                Vector3 viewMuzzle = models.View != null ? Vector3.Transform(weapon.Data.Muzzle, models.View.WorldTransform) : muzzle;
                Vector3 viewDirection = models.View != null ? GunForward(models.View) : direction;
                foreach (ShotResult shot in shotsToShow)
                    effects.Shot(shot, muzzle, direction, firstPerson, viewMuzzle, viewDirection);
            }
            shotsToShow.Clear();
        }

        // A gun model's barrel points along its own +X
        static Vector3 GunForward(Entity gun)
        {
            Matrix4x4 m = gun.WorldTransform;
            Vector3 x = new Vector3(m.M11, m.M12, m.M13);
            return x.LengthSquared() > 1e-10f ? Vector3.Normalize(x) : Vector3.UnitZ;
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
                ShowShots(false);
                return;
            }
            if (Input.WasPressed("SwitchView"))
                View = View == ViewMode.FirstPerson ? ViewMode.ThirdPerson : ViewMode.FirstPerson;
            if (Input.WasPressed("SwapShoulder"))
                LeftShoulder = !LeftShoulder;

            PlayerWeapon? weapon = Weapon;
            sinceStep += dt;
            UpdateScope(weapon, dt);
            if (CommandSource == null)
            {
                input.UpdateLook(dt);
                if (weapon != null && !weapon.IsMelee)
                    input.RecoverKick(dt, weapon.Data.RecoilRecovery, weapon.Data.RecoilRecoveryDelay);
            }
            ReloadSounds();
            BoltSounds();
            float yaw = CommandSource != null ? lastCommand.Yaw : input.Yaw;
            float pitch = CommandSource != null ? lastCommand.Pitch : input.Pitch;

            // Down the sights: eased in the weapon's time, the view and the view model narrowed, the third person's camera
            // closer. A scope: the view model up to the eye in the first half of its coming up, then the scope's view
            float aimSeconds = weapon != null ? MathF.Max(0.01f, weapon.Data.AimSeconds) : 0.2f;
            bool aiming = weapon != null && weapon.IsAiming && !arsenal.IsSwitching;
            aimBlend = Math.Clamp(aimBlend + (aiming ? dt : -dt) / aimSeconds, 0.0f, 1.0f);
            float aimEase = aimBlend * aimBlend * (3.0f - 2.0f * aimBlend);
            float ads = 0.0f;
            if (weapon != null)
            {
                WeaponData data = weapon.Data;
                float raise = Math.Clamp(scopeBlend * 2.0f, 0.0f, 1.0f);
                ads = data.HasScope ? raise * raise * (3.0f - 2.0f * raise) : data.CanAimDownSights ? aimEase : 0.0f;
                if (camera != null)
                {
                    float sightsFov = data.HasScope ? baseFieldOfView : data.AimFieldOfView;
                    float fov = ScopeShown ? ZoomedFieldOfView(shownZoom) : baseFieldOfView + (sightsFov - baseFieldOfView) * ads;
                    float viewFov = baseViewModelFieldOfView + (data.AimViewModelFieldOfView - baseViewModelFieldOfView) * ads;
                    if (MathF.Abs(camera.FieldOfView - fov) > 0.01f || MathF.Abs(camera.ViewModelFieldOfView - viewFov) > 0.01f)
                    {
                        camera.FieldOfView = fov;
                        camera.ViewModelFieldOfView = viewFov;
                    }
                }
                viewSettings.Distance = ThirdPersonDistance + (data.AimDistance - ThirdPersonDistance) * ads;
            }

            // The body shows what the motor does (the same in both views)
            body?.Update(dt, motor.Velocity, motor.IsGrounded, motor.IsCrouched, motor.IsWalking, motor.IsSprinting, yaw, pitch);

            // The eyes: down with the capsule when crouched, eased so the view doesn't jump; the camera there or over the shoulder
            float crouchDrop = character.StandingHeight - character.Height;
            eyeOffset += (crouchDrop - eyeOffset) * MathF.Min(1.0f, 14.0f * dt);
            Vector3 eyes = WorldPosition + new Vector3(0.0f, EyeHeight - eyeOffset, 0.0f);
            Vector3 cameraPosition = view.Update(dt, eyes, yaw, pitch, this);
            lastEyes = eyes;
            lastCamera = cameraPosition;
            if (body != null)
                body.HiddenInMainView = view.HidesBody;
            UpdateHand(weapon);
            // The view model in first person (scoping in third person too), once the camera is at the eyes (not while a
            // switch glides); the scope's view (from halfway up) hides it
            bool atEyes = Vector3.DistanceSquared(cameraPosition, eyes) < 0.0001f;
            bool firstPerson = (view.Mode == ViewMode.FirstPerson || view.AtEyes) && atEyes;
            ScopeShown = weapon != null && weapon.Data.HasScope && scopeBlend >= 0.5f && atEyes;
            ScopeDarkness = ScopeShown ? 0.85f * Math.Clamp(1.0f - (scopeBlend - 0.5f) / 0.5f, 0.0f, 1.0f) : 0.0f;
            if (Camera != null)
            {
                // Scoped, the look sways with the breathing (the weapon's, as the shots aim)
                Vector2 sway = ScopeShown && weapon != null ? weapon.SwayAt(sinceStep) : Vector2.Zero;
                Camera.Translation = cameraPosition;
                Camera.EulerAngles = new Vector3(pitch + sway.Y, yaw + sway.X, 0.0f);
            }
            ShowViewModel(firstPerson && !ScopeShown);
            if (firstPerson && weapon != null)
                PlaceViewModel(cameraPosition, yaw, pitch, weapon.Data, ads);
            ShowShots(firstPerson && !ScopeShown);
            hitMarker = MathF.Max(0.0f, hitMarker - dt);

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

        // The view model's model faces +Z (the camera looks along -Z): turned half round, then by the look. Its eye at the
        // camera, moved by the offset in the camera's space (the model's X is the camera's left, its Z the camera's forward);
        // down the sights the eye is the weapon's AimEye (on the sight line) and nothing is moved. Switching, it drops below
        // the view, turned down
        void PlaceViewModel(Vector3 cameraPosition, float yaw, float pitch, WeaponData data, float ads)
        {
            if (ViewModel == null)
                return;
            Vector3 eye = Vector3.Lerp(data.ViewModelEye, data.AimEye, ads);
            Vector3 offset = Vector3.Lerp(data.ViewModelOffset, Vector3.Zero, ads);
            float lowered = arsenal.Lowered;
            lowered = lowered * lowered * (3.0f - 2.0f * lowered);
            offset.Y -= LoweredDrop * lowered;
            Quaternion rotation = Quaternion.CreateFromYawPitchRoll((yaw + 180.0f) * MathF.PI / 180.0f, pitch * MathF.PI / 180.0f, 0.0f);
            // (turned down about the model's X, the camera's side, while it is lowered)
            Quaternion tilt = Quaternion.CreateFromAxisAngle(Vector3.UnitX, LoweredTilt * lowered * MathF.PI / 180.0f);
            // Down the sights the sight line looks along the view (the front sight's tip on the crosshair): the model turned
            // by the little turn from the weapon's SightDirection to its forward, as far as the sights are up
            tilt *= Quaternion.Slerp(Quaternion.Identity, SightTurn(data), ads);
            Vector3 local = new Vector3(-offset.X, offset.Y, offset.Z);
            ViewModel.Rotation = rotation * tilt;
            ViewModel.Translation = cameraPosition + Vector3.Transform(local, rotation) - Vector3.Transform(eye, rotation * tilt);
        }

        // The turn of the arms' model that brings a weapon's sight line onto the model's forward (+Z: the view's)
        static Quaternion SightTurn(WeaponData data)
        {
            if (data.SightDirection.LengthSquared() < 1e-8f)
                return Quaternion.Identity;
            Vector3 from = Vector3.Normalize(data.SightDirection);
            Vector3 axis = Vector3.Cross(from, Vector3.UnitZ);
            float sine = axis.Length();
            if (sine < 1e-6f)
                return Quaternion.Identity;
            float angle = MathF.Atan2(sine, Vector3.Dot(from, Vector3.UnitZ));
            return Quaternion.CreateFromAxisAngle(axis / sine, angle);
        }

        void ShowViewModel(bool shown)
        {
            Show(viewModelArms, shown);
            foreach (var (_, view) in held.Values)
                Show(view?.GetComponent<Model>(), shown);
            foreach (var (_, view) in heldLeft.Values)
                Show(view?.GetComponent<Model>(), shown);
        }

        // The hand that holds the weapon (Phase 2.5): the sniper rifle is in the left hand while the right works the bolt
        // (its LeftHand windows, by the bolt's or the reload's time): the left hand's models shown, placed for that clip,
        // the right hand's hidden; otherwise the other way round. The body's left model hidden from the main view with it
        void UpdateHand(PlayerWeapon? weapon)
        {
            if (weapon == null || !heldLeft.TryGetValue(weapon, out var left))
                return;
            WeaponData data = weapon.Data;
            WeaponData.LeftHold? hold = null;
            // The clip's own time when the arms play it (the pose shown: the weapon's clock runs in the fixed step, ahead of
            // the animation by up to a frame, more after a slow one; a crossfade into the clip still mixes the last pose),
            // else the weapon's clock
            float ClipTime(string state, float clock)
            {
                if (viewModelAnimator == null || string.IsNullOrEmpty(state) || viewModelAnimator.CurrentState != state)
                    return clock;
                return viewModelAnimator.IsInTransition ? -1.0f : viewModelAnimator.StateTime;
            }
            if (arsenal.IsSwitching)
                hold = null; // (put away or taken out in the right hand)
            else if (weapon.IsWorkingBolt && weapon.BoltSeconds >= 0.0f && data.LeftInBolt != null &&
                data.LeftInBolt.Holds(ClipTime(data.ViewBolt, weapon.BoltSeconds)))
                hold = data.LeftInBolt;
            else if (weapon.IsReloading && data.LeftInReload != null &&
                data.LeftInReload.Holds(ClipTime(data.ViewReload, weapon.ReloadProgress * data.ReloadSeconds)))
                hold = data.LeftInReload;
            if (hold != null && hold != leftPlaced)
            {
                foreach (Entity? e in new[] { left.Body, left.View })
                    if (e != null)
                    {
                        e.Translation = hold.Translation;
                        e.Rotation = hold.Rotation;
                    }
                leftPlaced = hold;
            }
            bool wanted = hold != null;
            if (wanted != inLeftHand)
            {
                inLeftHand = wanted;
                SetVisible(left, wanted);
                SetVisible(held[weapon], !wanted);
            }
            if (body != null && left.Body?.GetComponent<Model>() is Model leftModel && leftModel.HiddenInMainView != body.HiddenInMainView)
                leftModel.HiddenInMainView = body.HiddenInMainView;
        }

        /// <summary>The weapon is held in the left hand now (the sniper's bolt, a test reads it).</summary>
        public bool InLeftHand => inLeftHand;

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

        // The reload's sounds at their moments in the clip (its cues: the magazine out, then a new one in; the sniper's
        // bolt open, the rounds in, the bolt shut)
        void ReloadSounds()
        {
            PlayerWeapon? weapon = Weapon;
            if (weapon == null || !weapon.IsReloading)
                return;
            float seconds = weapon.ReloadProgress * weapon.Data.ReloadSeconds;
            var cues = weapon.Data.ReloadCues;
            while (reloadSounds < cues.Count && seconds >= cues[reloadSounds].At)
            {
                PlaySound(cues[reloadSounds].Sound, cues[reloadSounds].Volume);
                reloadSounds++;
            }
        }

        // The bolt's sounds at their moments (open, shut) while it is worked
        void BoltSounds()
        {
            PlayerWeapon? weapon = Weapon;
            if (weapon == null || !weapon.IsWorkingBolt)
                return;
            float seconds = weapon.BoltSeconds;
            var cues = weapon.Data.BoltCues;
            while (boltSounds < cues.Count && seconds >= 0.0f && seconds >= cues[boltSounds].At)
            {
                PlaySound(cues[boltSounds].Sound, cues[boltSounds].Volume);
                boltSounds++;
            }
        }

        // The scope (Phase 2.5): coming up while the weapon is scoped (its ScopeSeconds), down at once (halfway: the
        // overlay goes, the view model comes back down); the zoom shown eased to the wanted one (in proportion: as fast
        // from 4x to 10x as from 2x to 5x); the camera at the eyes meanwhile, the wheel zooms, the look turns slower
        void UpdateScope(PlayerWeapon? weapon, float dt)
        {
            bool scope = weapon != null && weapon.Data.HasScope;
            bool scoped = scope && weapon!.IsAiming && !arsenal.IsSwitching;
            float seconds = scope ? MathF.Max(0.01f, weapon!.Data.ScopeSeconds) : 0.25f;
            if (scoped)
                scopeBlend = MathF.Min(1.0f, scopeBlend + dt / seconds);
            else
                scopeBlend = MathF.Max(0.0f, MathF.Min(scopeBlend, 0.49f) - dt / seconds);
            float wanted = scoped ? MathF.Max(1.0f, weapon!.Zoom) : 1.0f;
            shownZoom = ScopeShown && scopeBlend < 1.0f ? wanted // (coming up: the darkening hides the jump)
                : MathF.Exp(MathF.Log(shownZoom) + (MathF.Log(wanted) - MathF.Log(shownZoom)) * MathF.Min(1.0f, 12.0f * dt));
            view.AtEyes = scope && (scoped || scopeBlend > 0.0f);
            input.WheelZooms = scope && weapon!.Zoom > 0.0f;
            input.LookScale = ScopeShown ? MathF.Tan(ZoomedFieldOfView(shownZoom) * MathF.PI / 360.0f) /
                MathF.Tan(baseFieldOfView * MathF.PI / 360.0f) * scopeSensitivity : 1.0f;
        }

        // The field of view zoom times as narrow as the normal one (as a lens: the tangent of half of it)
        float ZoomedFieldOfView(float zoom) =>
            2.0f * MathF.Atan(MathF.Tan(baseFieldOfView * MathF.PI / 360.0f) / MathF.Max(1.0f, zoom)) * 180.0f / MathF.PI;

        void PlaySound(string clip, float volume)
        {
            if (!string.IsNullOrEmpty(clip))
                Audio.PlayOneShot(clip, WorldPosition, volume);
        }
    }
}
