using System;
using System.Numerics;

namespace Morava
{
    /// <summary>A part of an entity a script works with (Entity.GetComponent&lt;T&gt;).</summary>
    public abstract class Component
    {
        /// <summary>The entity it belongs to.</summary>
        public Entity Entity { get; internal set; } = null!;

        // The engine's numbers for the kinds (ScriptComponent in src/Scripting/ScriptApi.h)
        internal enum Kind { Rigidbody = 0, CharacterController = 1, Collider = 2, AudioSource = 3, AudioListener = 4, Camera = 5, Animator = 6,
            UIRect = 7, UIText = 8, UIImage = 9, UIButton = 10, UISlider = 11, UIDropdown = 12, Vehicle = 13, ParticleEmitter = 14, Trail = 15,
            NavAgent = 16, NavArea = 17, NavLink = 18, Model = 19, PointLight = 20, SpotLight = 21, Decal = 22 }

        internal static Kind KindOf(Type type)
        {
            if (type == typeof(Rigidbody)) return Kind.Rigidbody;
            if (type == typeof(CharacterController)) return Kind.CharacterController;
            if (type == typeof(Collider)) return Kind.Collider;
            if (type == typeof(AudioSource)) return Kind.AudioSource;
            if (type == typeof(AudioListener)) return Kind.AudioListener;
            if (type == typeof(Camera)) return Kind.Camera;
            if (type == typeof(Animator)) return Kind.Animator;
            if (type == typeof(UIRect)) return Kind.UIRect;
            if (type == typeof(UIText)) return Kind.UIText;
            if (type == typeof(UIImage)) return Kind.UIImage;
            if (type == typeof(UIButton)) return Kind.UIButton;
            if (type == typeof(UISlider)) return Kind.UISlider;
            if (type == typeof(UIDropdown)) return Kind.UIDropdown;
            if (type == typeof(Vehicle)) return Kind.Vehicle;
            if (type == typeof(ParticleEmitter)) return Kind.ParticleEmitter;
            if (type == typeof(Trail)) return Kind.Trail;
            if (type == typeof(NavAgent)) return Kind.NavAgent;
            if (type == typeof(NavArea)) return Kind.NavArea;
            if (type == typeof(NavLink)) return Kind.NavLink;
            if (type == typeof(Model)) return Kind.Model;
            if (type == typeof(PointLight)) return Kind.PointLight;
            if (type == typeof(SpotLight)) return Kind.SpotLight;
            if (type == typeof(Decal)) return Kind.Decal;
            throw new ArgumentException($"{type.Name} is not a component of the engine");
        }

        private protected InvalidOperationException Missing(string what) =>
            new InvalidOperationException($"{Entity} has no {what} in the running game");
    }

    /// <summary>How a force is given (Rigidbody.AddForce, AddTorque).</summary>
    public enum ForceMode
    {
        /// <summary>A push over the next fixed step, in newtons (torque: newton meters): heavier bodies move less.</summary>
        Force = 0,
        /// <summary>An acceleration over the next fixed step, in meters per second squared, whatever the mass.</summary>
        Acceleration = 1,
        /// <summary>A kick at once, in newton seconds: heavier bodies move less.</summary>
        Impulse = 2,
        /// <summary>A speed added at once, in meters per second, whatever the mass.</summary>
        VelocityChange = 3,
    }

    /// <summary>A dynamic physics body (Inspector: Rigid Body, Dynamic): gravity and collisions move it.</summary>
    public sealed unsafe class Rigidbody : Component
    {
        /// <summary>Pushes the body (in the world's directions). Call it in OnFixedUpdate for a continuous force.</summary>
        public void AddForce(Vector3 force, ForceMode mode = ForceMode.Force)
        {
            if (Interop.Game->Rigidbody_AddForce(Entity.Id, &force, (int)mode) == 0) throw Missing("dynamic rigid body");
        }

        /// <summary>Turns the body around the world's axes.</summary>
        public void AddTorque(Vector3 torque, ForceMode mode = ForceMode.Force)
        {
            if (Interop.Game->Rigidbody_AddTorque(Entity.Id, &torque, (int)mode) == 0) throw Missing("dynamic rigid body");
        }

        /// <summary>How fast it moves, in meters per second in the world.</summary>
        public Vector3 LinearVelocity
        {
            get { Vector3 value; if (Interop.Game->Rigidbody_GetLinearVelocity(Entity.Id, &value) == 0) throw Missing("dynamic rigid body"); return value; }
            set { if (Interop.Game->Rigidbody_SetLinearVelocity(Entity.Id, &value) == 0) throw Missing("dynamic rigid body"); }
        }

        /// <summary>How fast it turns, in radians per second around the world's axes.</summary>
        public Vector3 AngularVelocity
        {
            get { Vector3 value; if (Interop.Game->Rigidbody_GetAngularVelocity(Entity.Id, &value) == 0) throw Missing("dynamic rigid body"); return value; }
            set { if (Interop.Game->Rigidbody_SetAngularVelocity(Entity.Id, &value) == 0) throw Missing("dynamic rigid body"); }
        }
    }

    /// <summary>Where a character stands (CharacterController.GroundState). The engine's numbers (ScenePhysics::CharacterGround).</summary>
    public enum GroundState
    {
        /// <summary>On something it can stand on.</summary>
        OnGround = 1,
        /// <summary>On a slope too steep to stand on: it slides down.</summary>
        OnSteepGround = 2,
        /// <summary>Touching something that doesn't hold it.</summary>
        NotSupported = 3,
        /// <summary>Falling or jumping.</summary>
        InAir = 0,
    }

    /// <summary>
    /// A walking figure (Inspector: Character): moved by the velocity the script asks for, stopped by walls, up
    /// slopes and steps. It has no gravity of its own: the script adds it, and jumps.
    /// </summary>
    public sealed unsafe class CharacterController : Component
    {
        /// <summary>The velocity it should move with from the next fixed step on, in meters per second in the world.</summary>
        public void Move(Vector3 velocity)
        {
            if (Interop.Game->Character_SetVelocity(Entity.Id, &velocity) == 0) throw Missing("character controller");
        }

        NativeCharacterState State
        {
            get
            {
                NativeCharacterState state;
                if (Interop.Game->Character_GetState(Entity.Id, &state) == 0) throw Missing("character controller");
                return state;
            }
        }

        /// <summary>How it really moved in the last step: zero towards a wall it ran into, zero downward when it landed.</summary>
        public Vector3 Velocity => State.Velocity;

        /// <summary>Where it stands, after the last step.</summary>
        public GroundState GroundState => (GroundState)State.Ground;

        /// <summary>It stands on the ground.</summary>
        public bool IsGrounded => GroundState == GroundState.OnGround;

        /// <summary>The ground's direction up where it stands, of length 1.</summary>
        public Vector3 GroundNormal => State.GroundNormal;

        /// <summary>
        /// How fast what it stands on moves (a moving platform: its velocity; still ground: zero), in meters per second in
        /// the world. Velocity minus this is how it moves on the ground: an animation's speed (it stands still on a ferry).
        /// </summary>
        public Vector3 GroundVelocity => State.GroundVelocity;

        /// <summary>
        /// How fast what it stands on turns, in radians per second around the world's axes (a turning platform; zero
        /// otherwise). The physics carries the character around with it but doesn't turn it: a script turns its heading by
        /// the Y part (AnimatedCharacter does).
        /// </summary>
        public Vector3 GroundAngularVelocity => State.GroundAngularVelocity;

        /// <summary>The capsule's height now, top to bottom (SetHeight changes it: crouching).</summary>
        public float Height => State.Height;

        /// <summary>Its height standing: the Inspector's.</summary>
        public float StandingHeight => State.StandingHeight;

        /// <summary>
        /// The capsule another height, its bottom where it is (crouching; at least as tall as it is wide). Lower always
        /// works; higher only with room above: false when something is in the way (standing up under a table), and it
        /// keeps the height it has. Try again later (each fixed step while the player wants to stand).
        /// </summary>
        public bool SetHeight(float height)
        {
            int result = Interop.Game->Character_SetHeight(Entity.Id, height);
            if (result == 0) throw Missing("character controller");
            return result > 0;
        }
    }

    /// <summary>Who drives a vehicle (Vehicle.Controls). The engine's numbers (VehicleComponent::ControlMode).</summary>
    public enum VehicleControls
    {
        /// <summary>The player, from the input's actions: Accelerate, Brake, Steer and Handbrake (without them: Move and Jump).</summary>
        Player = 0,
        /// <summary>A script, with SetInput: an AI driver, a cutscene, a car the player gets into.</summary>
        Script = 1,
    }

    /// <summary>
    /// A wheeled vehicle (Inspector: Vehicle): a dynamic body on wheels with springs, driven by an engine through an
    /// automatic gearbox (or a manual one: ManualGearbox). Its forward is the entity's -Z. The player drives it, or a script (Controls).
    /// </summary>
    public sealed unsafe class Vehicle : Component
    {
        /// <summary>
        /// The driver's input, kept until set again; used while Controls is Script. forward: -1 (full reverse) to 1 (full
        /// throttle): asking for the other way while it still rolls brakes first. steer: -1 left to 1 right. brake and
        /// handbrake: 0 to 1.
        /// </summary>
        public void SetInput(float forward, float steer, float brake = 0.0f, float handbrake = 0.0f)
        {
            NativeVehicleInput input = new NativeVehicleInput { Forward = forward, Steer = steer, Brake = brake, Handbrake = handbrake };
            if (Interop.Game->Vehicle_SetInput(Entity.Id, &input) == 0) throw Missing("vehicle");
        }

        /// <summary>Who drives it. A script that takes over sets Script, then SetInput each fixed step.</summary>
        public VehicleControls Controls
        {
            get { int value; if (Interop.Game->Vehicle_GetControls(Entity.Id, &value) == 0) throw Missing("vehicle"); return (VehicleControls)value; }
            set { if (Interop.Game->Vehicle_SetControls(Entity.Id, (int)value) == 0) throw Missing("vehicle"); }
        }

        NativeVehicleState State
        {
            get
            {
                NativeVehicleState state;
                if (Interop.Game->Vehicle_GetState(Entity.Id, &state) == 0) throw Missing("vehicle");
                return state;
            }
        }

        /// <summary>How fast it goes along its forward, in meters per second (negative: backward). Times 3.6: km/h.</summary>
        public float Speed => State.Speed;

        /// <summary>The engine's revolutions per minute: for an engine sound's pitch, a rev counter.</summary>
        public float EngineRpm => State.Rpm;

        /// <summary>The engine's highest revolutions per minute (Inspector: Max RPM): a rev counter's end, its red zone.</summary>
        public float MaxRpm => State.MaxRpm;

        /// <summary>The revolutions per minute at which it shifts up (Inspector: Shift Up RPM).</summary>
        public float ShiftUpRpm => State.ShiftUpRpm;

        /// <summary>The gear it is in: 1, 2... forward, -1 reverse, 0 neutral.</summary>
        public int Gear => State.Gear;

        /// <summary>How many wheels it has.</summary>
        public int WheelCount => (int)State.Wheels;

        /// <summary>The wheel (in the Inspector's order) touches something.</summary>
        public bool IsWheelOnGround(int wheel) => wheel >= 0 && wheel < 32 && (State.WheelsOnGround & (1u << wheel)) != 0;

        /// <summary>All its wheels are off the ground: it jumps, or lies on its side.</summary>
        public bool IsInAir => State.WheelsOnGround == 0;

        /// <summary>The throttle the wheels got in the last step (0 while it brakes before reversing).</summary>
        public float AppliedForward => State.Applied.Forward;

        /// <summary>The brake the wheels got in the last step (also when reversing brakes first).</summary>
        public float AppliedBrake => State.Applied.Brake;

        /// <summary>
        /// Degrees: the most a wheel on the ground slides sideways (the angle between where it points and where it goes): a
        /// few in a turn, 10 and more in a drift. For tire sounds and drift effects.
        /// </summary>
        public float LateralSlip => State.LateralSlip;

        /// <summary>The most a wheel on the ground spins or locks: 0 rolling, 1 locked (or spinning twice as fast as it rolls).</summary>
        public float LongitudinalSlip => State.LongitudinalSlip;

        /// <summary>
        /// One wheel (in the Inspector's order, 0 to WheelCount - 1) after the last fixed step: where it touches the ground
        /// and how its tire slides there. For tire smoke, skid marks, tire sounds.
        /// </summary>
        public WheelState GetWheel(int wheel)
        {
            NativeWheelState state;
            if (Interop.Game->Vehicle_GetWheel(Entity.Id, wheel, &state) == 0)
            {
                if (wheel < 0 || wheel >= WheelCount) throw new ArgumentOutOfRangeException(nameof(wheel), $"{Entity} has {WheelCount} wheels");
                throw Missing("vehicle");
            }
            return new WheelState(state);
        }

        /// <summary>
        /// The tires' sideways grip, front and rear, times the Inspector's (Front Grip, Rear Grip): 1, 1 as set there. A drift
        /// lowers the rear's for a moment. Kept until set again.
        /// </summary>
        public void SetGrip(float front, float rear)
        {
            if (Interop.Game->Vehicle_SetGrip(Entity.Id, front, rear) == 0) throw Missing("vehicle");
        }

        /// <summary>
        /// The engine's torque times the Inspector's (Engine Torque): its whole torque curve scaled, so its power too (the
        /// engine braking stays as it is). 1: as set there. Kept until set again.
        /// </summary>
        public void SetEngineTorque(float times)
        {
            if (Interop.Game->Vehicle_SetEngineTorque(Entity.Id, times) == 0) throw Missing("vehicle");
        }

        /// <summary>
        /// The gearbox is manual: the script shifts (ShiftUp, ShiftDown) and the clutch works by itself (out through a shift,
        /// and when slow without throttle, so it doesn't creep in gear). The throttle from neutral still puts it in first,
        /// the brake once stopped in reverse. False (the start): automatic, it shifts by the rpm.
        /// </summary>
        public bool ManualGearbox
        {
            get => State.Manual != 0;
            set { if (Interop.Game->Vehicle_SetManual(Entity.Id, value ? 1 : 0) == 0) throw Missing("vehicle"); }
        }

        /// <summary>
        /// The most the engine turns: Max RPM, plus the Inspector's Over-Rev RPM (a band only a manual downshift reaches: the
        /// throttle is cut at Max RPM). A rev counter's end.
        /// </summary>
        public float RevLimitRpm => State.RevLimit;

        /// <summary>How many forward gears it has.</summary>
        public int GearCount => State.Gears;

        /// <summary>
        /// One gear up in a manual gearbox (from neutral or reverse: first). False: refused (automatic, or in the top gear).
        /// </summary>
        public bool ShiftUp() => Shift(1);

        /// <summary>
        /// One gear down in a manual gearbox. False: refused (automatic, in first, or the engine would turn past its limit in
        /// the lower gear: Max RPM plus Over-Rev RPM, RevLimitRpm).
        /// </summary>
        public bool ShiftDown() => Shift(-1);

        /// <summary>
        /// Straight into a gear (1, 2... forward, -1 reverse, 0 neutral; clamped to the gearbox's), without a shift's checks:
        /// a car put at the start or respawned. An automatic gearbox shifts on from there by itself.
        /// </summary>
        public void SetGear(int gear)
        {
            if (Interop.Game->Vehicle_SetGear(Entity.Id, gear) == 0) throw Missing("vehicle");
        }

        bool Shift(int direction)
        {
            int result = Interop.Game->Vehicle_Shift(Entity.Id, direction);
            if (result == 0) throw Missing("vehicle");
            return result > 0;
        }
    }

    /// <summary>
    /// One wheel of a vehicle after the last fixed step (Vehicle.GetWheel). The rubbing speeds compare the tire's surface with
    /// the ground under it, in meters a second: what makes smoke and skid marks (a big slip angle at walking pace makes
    /// neither).
    /// </summary>
    public readonly struct WheelState
    {
        internal WheelState(NativeWheelState state)
        {
            IsOnGround = state.OnGround != 0;
            Ground = state.Ground != 0 ? Interop.EntityFor(state.Ground) : null;
            Point = state.Point;
            Normal = state.Normal;
            LocalPoint = state.LocalPoint;
            LocalNormal = state.LocalNormal;
            LateralSlip = state.LateralSlip;
            LongitudinalSlip = state.LongitudinalSlip;
            SideSlide = state.SideSlide;
            SpinSlide = state.SpinSlide;
            SlideSpeed = state.SlideSpeed;
            Radius = state.Radius;
        }

        /// <summary>It touches something. The rest but Radius is zero when it doesn't.</summary>
        public bool IsOnGround { get; }

        /// <summary>The entity it touches (the road, the grass), or null.</summary>
        public Entity? Ground { get; }

        /// <summary>Where it touches, in the world, at the last fixed step.</summary>
        public Vector3 Point { get; }

        /// <summary>The ground's up there, in the world, of length 1.</summary>
        public Vector3 Normal { get; }

        /// <summary>
        /// Where it touches, in the vehicle's own space. A child of the vehicle put here stays under the drawn wheel: the
        /// vehicle is drawn between two fixed steps, so the world Point would be up to a step's way behind it.
        /// </summary>
        public Vector3 LocalPoint { get; }

        /// <summary>The ground's up, in the vehicle's own space.</summary>
        public Vector3 LocalNormal { get; }

        /// <summary>Degrees between where the wheel points and where it goes (Vehicle.LateralSlip, this wheel's).</summary>
        public float LateralSlip { get; }

        /// <summary>How much it spins or locks: 0 rolling, 1 locked or spinning twice as fast (Vehicle.LongitudinalSlip, this wheel's).</summary>
        public float LongitudinalSlip { get; }

        /// <summary>Meters a second it slides across itself (positive: to its right): a drift, a slide.</summary>
        public float SideSlide { get; }

        /// <summary>
        /// Meters a second its surface turns faster than the ground goes by: positive spins (a start at full throttle),
        /// negative slows toward locked (hard braking).
        /// </summary>
        public float SpinSlide { get; }

        /// <summary>Meters a second the tire rubs over the ground, both ways together: how much smoke, how dark a mark.</summary>
        public float SlideSpeed { get; }

        /// <summary>The wheel's radius in meters.</summary>
        public float Radius { get; }
    }

    /// <summary>A physics shape (Inspector: Collider): alone it is a static body, like a floor or a wall.</summary>
    public sealed class Collider : Component
    {
    }

    /// <summary>
    /// A sound the entity plays (Inspector: Audio Source), from where it is when Spatial. A changed Volume, Pitch, Loop or
    /// distance is heard at once; a new Clip, Bus or Spatial from the next Play.
    /// </summary>
    public sealed unsafe class AudioSource : Component
    {
        /// <summary>Plays it from the beginning, or on from where Pause left it.</summary>
        public void Play()
        {
            if (Interop.Game->AudioSource_Play(Entity.Id) == 0) throw Missing("playable audio source (the log says why)");
        }

        /// <summary>Stops it where it is: Play goes on from there.</summary>
        public void Pause()
        {
            if (Interop.Game->AudioSource_Pause(Entity.Id) == 0) throw Missing("audio source");
        }

        /// <summary>Stops it, back to its beginning.</summary>
        public void Stop()
        {
            if (Interop.Game->AudioSource_Stop(Entity.Id) == 0) throw Missing("audio source");
        }

        /// <summary>It is playing now (a sound without Loop stops at its end).</summary>
        public bool IsPlaying
        {
            get
            {
                int result = Interop.Game->AudioSource_IsPlaying(Entity.Id);
                if (result < 0) throw Missing("audio source");
                return result != 0;
            }
        }

        /// <summary>The sound file, relative to the project: "Assets/Audio/Pickup.wav". A new one plays from the next Play.</summary>
        public string Clip
        {
            get
            {
                char* buffer = stackalloc char[512];
                int length = Interop.Game->AudioSource_GetClip(Entity.Id, buffer, 512);
                if (length == -2) throw Missing("audio source");
                if (length >= 0) return new string(buffer, 0, length);
                char[] large = new char[8192];
                fixed (char* chars = large)
                    length = Interop.Game->AudioSource_GetClip(Entity.Id, chars, large.Length);
                return length >= 0 ? new string(large, 0, length) : "";
            }
            set
            {
                fixed (char* chars = value)
                    if (Interop.Game->AudioSource_SetClip(Entity.Id, chars, value.Length) == 0) throw Missing("audio source");
            }
        }

        NativeAudioSettings Settings
        {
            get
            {
                NativeAudioSettings settings;
                if (Interop.Game->AudioSource_GetSettings(Entity.Id, &settings) == 0) throw Missing("audio source");
                return settings;
            }
            set
            {
                if (Interop.Game->AudioSource_SetSettings(Entity.Id, &value) == 0) throw Missing("audio source");
            }
        }

        /// <summary>0 to 1 (above 1 amplifies).</summary>
        public float Volume { get => Settings.Volume; set { var s = Settings; s.Volume = value; Settings = s; } }

        /// <summary>1 as recorded; 2 an octave higher and twice as fast.</summary>
        public float Pitch { get => Settings.Pitch; set { var s = Settings; s.Pitch = value; Settings = s; } }

        /// <summary>It starts again at its end.</summary>
        public bool Loop { get => Settings.Loop != 0; set { var s = Settings; s.Loop = value ? 1 : 0; Settings = s; } }

        /// <summary>The group it is mixed in. A new one from the next Play.</summary>
        public AudioBus Bus { get => (AudioBus)Settings.Bus; set { var s = Settings; s.Bus = (int)value; Settings = s; } }

        /// <summary>Heard from where the entity is (louder near the listener, from its side), not the same everywhere.</summary>
        public bool Spatial { get => Settings.Spatial != 0; set { var s = Settings; s.Spatial = value ? 1 : 0; Settings = s; } }

        /// <summary>Meters: full volume up to here, then softer.</summary>
        public float MinDistance { get => Settings.MinDistance; set { var s = Settings; s.MinDistance = value; Settings = s; } }

        /// <summary>Meters: no softer beyond.</summary>
        public float MaxDistance { get => Settings.MaxDistance; set { var s = Settings; s.MaxDistance = value; Settings = s; } }
    }

    /// <summary>Where the game is heard from (Inspector: Audio Listener), usually the camera. The first active one listens.</summary>
    public sealed unsafe class AudioListener : Component
    {
        /// <summary>It listens (when it is the first active one); none active: the primary camera does.</summary>
        public bool Active
        {
            get { int value; if (Interop.Game->AudioListener_GetActive(Entity.Id, &value) == 0) throw Missing("audio listener"); return value != 0; }
            set { if (Interop.Game->AudioListener_SetActive(Entity.Id, value ? 1 : 0) == 0) throw Missing("audio listener"); }
        }
    }

    /// <summary>
    /// A game camera (Inspector: Camera): the game is seen through the primary one. It is placed and aimed by its entity
    /// (Translation, EulerAngles: X down, Y to the left). Setting Primary switches the game's view to it. A camera with a
    /// Target draws into that render texture instead (a monitor, a mirror), which materials and UI images show.
    /// </summary>
    public sealed unsafe class Camera : Component
    {
        NativeCameraSettings Settings
        {
            get
            {
                NativeCameraSettings settings;
                if (Interop.Game->Camera_GetSettings(Entity.Id, &settings) == 0) throw Missing("camera");
                return settings;
            }
            set
            {
                if (Interop.Game->Camera_SetSettings(Entity.Id, &value) == 0) throw Missing("camera");
            }
        }

        /// <summary>The game is seen through it. Set: the other cameras stop being primary.</summary>
        public bool Primary { get => Settings.Primary != 0; set { var s = Settings; s.Primary = value ? 1 : 0; Settings = s; } }

        /// <summary>Degrees from the view's bottom edge to its top edge, 10 to 120: smaller zooms in.</summary>
        public float FieldOfView { get => Settings.FieldOfView; set { var s = Settings; s.FieldOfView = value; Settings = s; } }

        /// <summary>Meters: nothing nearer is drawn.</summary>
        public float NearClip { get => Settings.NearClip; set { var s = Settings; s.NearClip = value; Settings = s; } }

        /// <summary>Meters: nothing farther is drawn.</summary>
        public float FarClip { get => Settings.FarClip; set { var s = Settings; s.FarClip = value; Settings = s; } }

        /// <summary>
        /// The render texture it draws into (a .mrtex file, relative to the project); "" for the screen. A camera with a
        /// target never shows the game. Several cameras may take turns on one texture: the one drawing last wins.
        /// </summary>
        public string Target
        {
            get => UIStrings.Get(Entity, "camera", (char* buffer, int capacity) => Interop.Game->Camera_GetTarget(Entity.Id, buffer, capacity));
            set
            {
                string target = value ?? "";
                fixed (char* chars = target)
                    if (Interop.Game->Camera_SetTarget(Entity.Id, chars, target.Length) == 0) throw Missing("camera");
            }
        }

        /// <summary>
        /// A camera with a Target draws only while enabled: off, its texture keeps the last picture and the camera costs
        /// nothing (a monitor nobody looks at, a closed menu). Enabled again, it draws at once. The screen's cameras don't use it.
        /// </summary>
        public bool Enabled { get => Settings.Enabled != 0; set { var s = Settings; s.Enabled = value ? 1 : 0; Settings = s; } }

        /// <summary>A camera with a Target draws every this many frames, 1 to 120 (1: every frame).</summary>
        public int DrawEvery { get => (int)Settings.DrawEvery; set { var s = Settings; s.DrawEvery = (uint)Math.Clamp(value, 1, 120); Settings = s; } }

        /// <summary>
        /// Degrees the view models (Model.ViewModel: the first person's arms and gun) span from bottom to top while this camera
        /// shows the game, 10 to 120: aiming down the sights narrows it with the view's.
        /// </summary>
        public float ViewModelFieldOfView { get => Settings.ViewModelFieldOfView; set { var s = Settings; s.ViewModelFieldOfView = value; Settings = s; } }
    }

    /// <summary>
    /// Plays the entity's animated model by its controller (Inspector: Animator; the .manim file in the Animator panel):
    /// the state it is in, and the transitions to others when the parameters say so. Set the parameters from the game
    /// (SetFloat("Speed", ...), SetTrigger("Jump")); the animator crossfades on its own. The names are the controller's:
    /// a name it doesn't have throws.
    /// </summary>
    public sealed unsafe class Animator : Component
    {
        // The engine's numbers (AnimatorParameterType in src/Animation/AnimatorController.h)
        const int FloatType = 0, BoolType = 1, TriggerType = 2;

        void Set(string name, int type, float value, string typeName)
        {
            int result;
            fixed (char* chars = name)
                result = Interop.Game->Animator_SetParameter(Entity.Id, chars, name.Length, type, value);
            Check(result, $"{typeName} parameter '{name}'");
        }

        float Get(string name, int type, string typeName)
        {
            float value;
            int result;
            fixed (char* chars = name)
                result = Interop.Game->Animator_GetParameter(Entity.Id, chars, name.Length, type, &value);
            Check(result, $"{typeName} parameter '{name}'");
            return value;
        }

        void Check(int result, string what)
        {
            if (result == 0) throw Missing("animator");
            if (result < 0) throw new ArgumentException($"The animator of {Entity} has no {what} (see its controller in the Animator panel)");
        }

        /// <summary>A Float parameter: a transition may test it (Speed greater than 0.1), a state's speed follow it.</summary>
        public void SetFloat(string name, float value) => Set(name, FloatType, value, "Float");
        /// <summary>A Float parameter's value.</summary>
        public float GetFloat(string name) => Get(name, FloatType, "Float");

        /// <summary>A Bool parameter: a transition may test it (Grounded true).</summary>
        public void SetBool(string name, bool value) => Set(name, BoolType, value ? 1 : 0, "Bool");
        /// <summary>A Bool parameter's value.</summary>
        public bool GetBool(string name) => Get(name, BoolType, "Bool") != 0;

        /// <summary>A Trigger parameter: set until a transition that tests it is taken (Jump: once per press).</summary>
        public void SetTrigger(string name) => Set(name, TriggerType, 1, "Trigger");
        /// <summary>A trigger no transition took yet: unset again.</summary>
        public void ResetTrigger(string name) => Set(name, TriggerType, 0, "Trigger");

        /// <summary>To the state at once, whatever the transitions say, crossfading for fade seconds (0: a cut).</summary>
        public void Play(string state, float fade = 0.0f) => Play(state, fade, "");

        /// <summary>
        /// The same in a layer: the state of that layer ("": the base), as an upper body's Reload while the legs walk.
        /// </summary>
        public void Play(string state, float fade, string layer)
        {
            int result;
            fixed (char* chars = state)
            fixed (char* layerChars = layer)
                result = Interop.Game->Animator_Play(Entity.Id, chars, state.Length, layerChars, layer.Length, fade);
            Check(result, layer.Length == 0 ? $"state '{state}'" : $"state '{state}' in a layer '{layer}'");
        }

        /// <summary>The state it is in ("" when its controller has none, or can't be read).</summary>
        public string CurrentState => GetLayerState("");

        /// <summary>The state a layer is in ("": the base, as CurrentState).</summary>
        public string GetLayerState(string layer)
        {
            char* buffer = stackalloc char[256];
            int length;
            fixed (char* layerChars = layer)
                length = Interop.Game->Animator_GetState(Entity.Id, layerChars, layer.Length, buffer, 256);
            if (length == -2) throw Missing("animator");
            if (length == -3) throw new ArgumentException($"The animator of {Entity} has no layer '{layer}' (see its controller in the Animator panel)");
            return length >= 0 ? new string(buffer, 0, length) : "";
        }

        /// <summary>
        /// A bone (a node of the model: hand_r) in the world, as the frame's pose has it: where a gun's muzzle or a hit is
        /// (Shooter E3). Its translation is the bone's place, its rotation the bone's.
        /// </summary>
        public Matrix4x4 GetBoneTransform(string bone)
        {
            Matrix4x4 value;
            int result;
            fixed (char* chars = bone)
                result = Interop.Game->Animator_GetBoneTransform(Entity.Id, chars, bone.Length, &value);
            if (result == 0) throw new ArgumentException($"The model of {Entity} has no bone '{bone}' (or no animator)");
            return value;
        }

        NativeAnimatorInfo Info
        {
            get
            {
                NativeAnimatorInfo info;
                if (Interop.Game->Animator_GetInfo(Entity.Id, &info) == 0) throw Missing("animator");
                return info;
            }
        }

        /// <summary>Seconds played in the current state (times its speed).</summary>
        public float StateTime => Info.StateTime;

        /// <summary>How far the current state's clip has played: 1 at its end, more after loops.</summary>
        public float NormalizedTime => Info.NormalizedTime;

        /// <summary>A crossfade into the current state runs.</summary>
        public bool IsInTransition => Info.InTransition != 0;

        /// <summary>How far that crossfade is, 0 to 1.</summary>
        public float TransitionProgress => Info.FadeProgress;
    }

    /// <summary>
    /// Particles that start at the entity: smoke, dust, sparks (the Inspector's Particle Emitter; the renderer moves and draws
    /// them on the GPU). A change here shows from the next frame on; the particles alive keep the color and size curve of
    /// the frame they are drawn in.
    /// </summary>
    public sealed unsafe class ParticleEmitter : Component
    {
        NativeParticleSettings Settings
        {
            get
            {
                NativeParticleSettings settings;
                if (Interop.Game->ParticleEmitter_GetSettings(Entity.Id, &settings) == 0) throw Missing("particle emitter");
                return settings;
            }
            set
            {
                if (Interop.Game->ParticleEmitter_SetSettings(Entity.Id, &value) == 0) throw Missing("particle emitter");
            }
        }

        /// <summary>Starts particles at its rates (false: only Emit starts some; those alive live on).</summary>
        public bool Emitting { get => Settings.Emitting != 0; set { var s = Settings; s.Emitting = value ? 1 : 0; Settings = s; } }

        /// <summary>Particles started a second.</summary>
        public float Rate { get => Settings.Rate; set { var s = Settings; s.Rate = value; Settings = s; } }

        /// <summary>Particles started for each meter the emitter moves (added to Rate).</summary>
        public float RatePerMeter { get => Settings.RatePerMeter; set { var s = Settings; s.RatePerMeter = value; Settings = s; } }

        /// <summary>The lower end of the speed a particle starts with: random between SpeedMin and SpeedMax (meters a second).</summary>
        public float SpeedMin { get => Settings.SpeedMin; set { var s = Settings; s.SpeedMin = value; Settings = s; } }
        /// <summary>The upper end of the start speed (meters a second).</summary>
        public float SpeedMax { get => Settings.SpeedMax; set { var s = Settings; s.SpeedMax = value; Settings = s; } }

        /// <summary>Meters across at the start of a particle's life.</summary>
        public float SizeStart { get => Settings.SizeStart; set { var s = Settings; s.SizeStart = value; Settings = s; } }
        /// <summary>Meters across at the end of a particle's life.</summary>
        public float SizeEnd { get => Settings.SizeEnd; set { var s = Settings; s.SizeEnd = value; Settings = s; } }

        /// <summary>Color and alpha at the start of a particle's life (x, y, z: red, green, blue; w: alpha).</summary>
        public Vector4 ColorStart { get => Settings.ColorStart; set { var s = Settings; s.ColorStart = value; Settings = s; } }
        /// <summary>Color and alpha at the end of a particle's life (alpha 0 fades it out).</summary>
        public Vector4 ColorEnd { get => Settings.ColorEnd; set { var s = Settings; s.ColorEnd = value; Settings = s; } }

        /// <summary>Starts this many particles at once (in the next frame), whether Emitting or not.</summary>
        public void Emit(int count)
        {
            if (Interop.Game->ParticleEmitter_Emit(Entity.Id, count) == 0) throw Missing("particle emitter");
        }

        /// <summary>
        /// Starts this many particles at once (in the next frame) from a place of the script's: from burst.Position, within the
        /// emitter's cone around burst.Direction (world), at burst.Speed for burst.Lifetime seconds when those are above 0 (else
        /// the emitter's ranges). One emitter serves every shot: a muzzle's flash, a hit's sparks, a tracer.
        /// </summary>
        public void Emit(int count, ParticleBurst burst)
        {
            NativeParticleBurst native = new NativeParticleBurst
            {
                Position = burst.Position, Direction = burst.Direction, Speed = burst.Speed, Lifetime = burst.Lifetime,
            };
            if (Interop.Game->ParticleEmitter_EmitBurst(Entity.Id, count, &native) == 0) throw Missing("particle emitter");
        }

        /// <summary>The particles alive go (a restart).</summary>
        public void Clear()
        {
            if (Interop.Game->ParticleEmitter_Clear(Entity.Id) == 0) throw Missing("particle emitter");
        }
    }

    /// <summary>Where a burst of particles starts (ParticleEmitter.Emit(int, ParticleBurst)).</summary>
    public struct ParticleBurst
    {
        /// <summary>Where they start (world).</summary>
        public Vector3 Position;
        /// <summary>The middle of the cone they start in (world; the emitter's Cone Angle around it).</summary>
        public Vector3 Direction;
        /// <summary>Meters a second (0: the emitter's speed range).</summary>
        public float Speed;
        /// <summary>Seconds they live (0: the emitter's lifetime range).</summary>
        public float Lifetime;

        /// <summary>A burst from position around direction; speed and lifetime 0: the emitter's own.</summary>
        public ParticleBurst(Vector3 position, Vector3 direction, float speed = 0.0f, float lifetime = 0.0f)
        {
            Position = position;
            Direction = direction;
            Speed = speed;
            Lifetime = lifetime;
        }
    }

    /// <summary>
    /// What PointLight and SpotLight share (the Inspector's lights; Shooter E8). A change shows from the next frame on. The
    /// intensity is the engine's artistic unit: a point light's 3 lights a surface 1.5 m away about fully.
    /// </summary>
    public abstract unsafe class Light : Component
    {
        private protected abstract int LightKind { get; }

        string What => LightKind == 0 ? "point light" : "spot light";

        NativeLightSettings Settings
        {
            get
            {
                NativeLightSettings settings;
                if (Interop.Game->Light_GetSettings(Entity.Id, LightKind, &settings) == 0) throw Missing(What);
                return settings;
            }
            set
            {
                if (Interop.Game->Light_SetSettings(Entity.Id, LightKind, &value) == 0) throw Missing(What);
            }
        }

        /// <summary>On or off (a lamp switched, a light shot out).</summary>
        public bool Enabled { get => Settings.Enabled != 0; set { var s = Settings; s.Enabled = value ? 1 : 0; Settings = s; } }

        /// <summary>Its color (red, green, blue: 1 is full).</summary>
        public Vector3 Color { get => Settings.Color; set { var s = Settings; s.Color = value; Settings = s; } }

        /// <summary>How bright.</summary>
        public float Intensity { get => Settings.Intensity; set { var s = Settings; s.Intensity = value; Settings = s; } }

        /// <summary>Meters: the light reaches exactly nothing here.</summary>
        public float Range { get => Settings.Range; set { var s = Settings; s.Range = value; Settings = s; } }

        /// <summary>Casts shadows (a few lights at once can: the others light without them).</summary>
        public bool CastShadows { get => Settings.CastShadows != 0; set { var s = Settings; s.CastShadows = value ? 1 : 0; Settings = s; } }

        private protected float InnerAngleValue { get => Settings.InnerAngle; set { var s = Settings; s.InnerAngle = value; Settings = s; } }
        private protected float OuterAngleValue { get => Settings.OuterAngle; set { var s = Settings; s.OuterAngle = value; Settings = s; } }
    }

    /// <summary>A light shining every way from the entity (the Inspector's Point Light).</summary>
    public sealed class PointLight : Light
    {
        private protected override int LightKind => 0;
    }

    /// <summary>A light shining in a cone along the entity's -Y (the Inspector's Spot Light).</summary>
    public sealed class SpotLight : Light
    {
        private protected override int LightKind => 1;

        /// <summary>The cone's half angle in degrees within which it shines fully.</summary>
        public float InnerAngle { get => InnerAngleValue; set => InnerAngleValue = value; }

        /// <summary>The cone's half angle in degrees where it fades to nothing (under 90).</summary>
        public float OuterAngle { get => OuterAngleValue; set => OuterAngleValue = value; }
    }

    /// <summary>
    /// A decal placed in the scene (the Inspector's Decal; Shooter E6): its texture projected along the entity's -Y onto the
    /// surfaces inside its box. Bullet holes and such are spawned without entities: <see cref="Decals"/>.Spawn.
    /// </summary>
    public sealed unsafe class Decal : Component
    {
        /// <summary>Times the texture's color, and how much it covers (w).</summary>
        public Vector4 Color
        {
            get
            {
                Vector4 color;
                if (Interop.Game->Decal_GetColor(Entity.Id, &color) == 0) throw Missing("decal");
                return color;
            }
            set
            {
                if (Interop.Game->Decal_SetColor(Entity.Id, &value) == 0) throw Missing("decal");
            }
        }
    }

    /// <summary>A model (an entity with a model file, Inspector: Models and Meshes).</summary>
    public sealed unsafe class Model : Component
    {
        /// <summary>
        /// Not drawn in the main view (the game's camera): a first-person player's own body. Its shadow stays, and the water's
        /// reflection, the render textures' cameras and the ray-traced passes still see it.
        /// </summary>
        public bool HiddenInMainView
        {
            get
            {
                int hidden;
                if (Interop.Game->Model_GetHiddenInMainView(Entity.Id, &hidden) == 0) throw Missing("model");
                return hidden != 0;
            }
            set
            {
                if (Interop.Game->Model_SetHiddenInMainView(Entity.Id, value ? 1 : 0) == 0) throw Missing("model");
            }
        }

        /// <summary>
        /// Drawn at all (a weapon put away: false). Off, nothing sees it: not the main view, no shadow, not the rays, the
        /// probes, the water's reflection or the render textures. Its entity, transform and animation stay.
        /// </summary>
        public bool Visible
        {
            get
            {
                int visible;
                if (Interop.Game->Model_GetVisible(Entity.Id, &visible) == 0) throw Missing("model");
                return visible != 0;
            }
            set
            {
                if (Interop.Game->Model_SetVisible(Entity.Id, value ? 1 : 0) == 0) throw Missing("model");
            }
        }
    }

    /// <summary>
    /// Marks the entity leaves behind as it moves: skid marks (the Inspector's Trail). A ribbon on the surface under the
    /// entity, across its up: put the entity where the wheel touches the road and turn its up to the road's. A change here
    /// shows from the next frame on; the marks already left keep their color and width.
    /// </summary>
    public sealed unsafe class Trail : Component
    {
        NativeTrailSettings Settings
        {
            get
            {
                NativeTrailSettings settings;
                if (Interop.Game->Trail_GetSettings(Entity.Id, &settings) == 0) throw Missing("trail");
                return settings;
            }
            set
            {
                if (Interop.Game->Trail_SetSettings(Entity.Id, &value) == 0) throw Missing("trail");
            }
        }

        /// <summary>Leaves marks as it moves (false: the ribbon ends; true again starts a new one).</summary>
        public bool Emitting { get => Settings.Emitting != 0; set { var s = Settings; s.Emitting = value ? 1 : 0; Settings = s; } }

        /// <summary>Meters across the ribbon.</summary>
        public float Width { get => Settings.Width; set { var s = Settings; s.Width = value; Settings = s; } }

        /// <summary>Meters between two points of the ribbon (smaller: smoother curves, more points).</summary>
        public float MinDistance { get => Settings.MinDistance; set { var s = Settings; s.MinDistance = value; Settings = s; } }

        /// <summary>Seconds a mark stays, fading over the second half (0: until the oldest marks make room for new ones).</summary>
        public float Lifetime { get => Settings.Lifetime; set { var s = Settings; s.Lifetime = value; Settings = s; } }

        /// <summary>The mark's color (x, y, z) and how much it covers (w): it darkens the surface under it. A darker one for a harder slide.</summary>
        public Vector4 Color { get => Settings.Color; set { var s = Settings; s.Color = value; Settings = s; } }

        /// <summary>Its marks go (a restart).</summary>
        public void Clear()
        {
            if (Interop.Game->Trail_Clear(Entity.Id) == 0) throw Missing("trail");
        }
    }
}
