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
        internal enum Kind { Rigidbody = 0, CharacterController = 1, Collider = 2, AudioSource = 3, AudioListener = 4, Camera = 5 }

        internal static Kind KindOf(Type type)
        {
            if (type == typeof(Rigidbody)) return Kind.Rigidbody;
            if (type == typeof(CharacterController)) return Kind.CharacterController;
            if (type == typeof(Collider)) return Kind.Collider;
            if (type == typeof(AudioSource)) return Kind.AudioSource;
            if (type == typeof(AudioListener)) return Kind.AudioListener;
            if (type == typeof(Camera)) return Kind.Camera;
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
    /// (Translation, EulerAngles: X down, Y to the left). Setting Primary switches the game's view to it.
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
    }
}
