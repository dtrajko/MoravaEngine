using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// The player: Move walks it, forward being away from the camera, and Jump jumps. It needs a Character (Inspector:
    /// Physics): the character stops at walls and climbs steps, the script says where it wants to go, adds gravity
    /// and jumps. The C# version of the engine's Input Test.
    /// </summary>
    public class PlayerController : Entity
    {
        /// <summary>Meters per second.</summary>
        public float Speed = 4.0f;

        /// <summary>Meters per second, up, when it jumps.</summary>
        public float JumpSpeed = 6.0f;

        /// <summary>Meters per second squared.</summary>
        public float Gravity = 20.0f;

        /// <summary>Forward is where this entity looks (none: along -Z, away from the scene's start view).</summary>
        public Entity? Camera;

        CharacterController character = null!;

        protected override void OnCreate()
        {
            character = GetComponent<CharacterController>() ??
                throw new InvalidOperationException($"{Name} needs a Character (Inspector: Physics) for PlayerController");
        }

        protected override void OnFixedUpdate(float dt)
        {
            Vector2 move = Input.GetAxis2D("Move");
            Vector3 forward = Camera != null ? Camera.Forward : -Vector3.UnitZ;
            forward = Vector3.Normalize(new Vector3(forward.X, 0, forward.Z));
            Vector3 right = Vector3.Cross(forward, Vector3.UnitY);
            Vector3 walk = (right * move.X + forward * move.Y) * Speed;

            // On the ground: stand, or jump. In the air (or sliding down a steep slope): fall faster each step
            float up = character.Velocity.Y;
            if (character.IsGrounded)
                up = Input.WasPressed("Jump") ? JumpSpeed : 0;
            else
                up -= Gravity * dt;

            character.Move(new Vector3(walk.X, up, walk.Z));
        }
    }
}
