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
        internal enum Kind { Rigidbody = 0, CharacterController = 1, Collider = 2 }

        internal static Kind KindOf(Type type)
        {
            if (type == typeof(Rigidbody)) return Kind.Rigidbody;
            if (type == typeof(CharacterController)) return Kind.CharacterController;
            if (type == typeof(Collider)) return Kind.Collider;
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

    /// <summary>A dynamic physics body (Properties panel: Rigid Body, Dynamic): gravity and collisions move it.</summary>
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
    /// A walking figure (Properties panel: Character): moved by the velocity the script asks for, stopped by walls, up
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

    /// <summary>A physics shape (Properties panel: Collider): alone it is a static body, like a floor or a wall.</summary>
    public sealed class Collider : Component
    {
    }
}
