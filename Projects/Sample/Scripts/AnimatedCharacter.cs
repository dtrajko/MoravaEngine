using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A character with an animated model: Move walks it (forward being away from the camera; Run held: faster), it turns
    /// to face where it walks, and its animator is told how fast it goes (the Speed parameter, meters per second) and when
    /// to use what is in front of it (Use: the trigger, on the Use action). The animator's controller picks the states and
    /// crossfades: Idle and Walk by Speed, Use from any state. It needs a Character (Inspector: Physics) and an Animator.
    /// </summary>
    public class AnimatedCharacter : Entity
    {
        /// <summary>Meters per second (the walk clip of Models/PirateVoid covers 1.65 m/s at its own speed).</summary>
        public float Speed = 1.65f;

        /// <summary>Meters per second while Run is held (Shift, the left stick pressed).</summary>
        public float RunSpeed = 4.0f;

        /// <summary>Degrees per second it turns towards where it walks.</summary>
        public float TurnSpeed = 540.0f;

        /// <summary>Meters per second squared.</summary>
        public float Gravity = 20.0f;

        /// <summary>Forward is where this entity looks (none: along -Z).</summary>
        public Entity? Camera;

        CharacterController character = null!;
        Animator animator = null!;
        Vector3 walk;
        float speed; // the horizontal speed the character really has (a wall stops it), smoothed for the animator

        protected override void OnCreate()
        {
            character = GetComponent<CharacterController>() ??
                throw new InvalidOperationException($"{Name} needs a Character (Inspector: Physics) for AnimatedCharacter");
            animator = GetComponent<Animator>() ??
                throw new InvalidOperationException($"{Name} needs an Animator for AnimatedCharacter");
        }

        protected override void OnFixedUpdate(float dt)
        {
            Vector2 move = Input.GetAxis2D("Move");
            Vector3 forward = Camera != null ? Camera.Forward : -Vector3.UnitZ;
            forward = Vector3.Normalize(new Vector3(forward.X, 0, forward.Z));
            Vector3 right = Vector3.Cross(forward, Vector3.UnitY);
            walk = (right * move.X + forward * move.Y) * (Input.IsHeld("Run") ? RunSpeed : Speed);

            float up = character.IsGrounded ? 0 : character.Velocity.Y - Gravity * dt;
            character.Move(new Vector3(walk.X, up, walk.Z));

            if (Input.WasPressed("Use"))
                animator.SetTrigger("Use");
        }

        protected override void OnUpdate(float dt)
        {
            // Face where it walks: a turn around Y, the shorter way, at most TurnSpeed degrees per second. The model looks
            // along +Z, so a heading of 0 walks along +Z
            if (walk.LengthSquared() > 0.01f)
            {
                float target = MathF.Atan2(walk.X, walk.Z) * 180.0f / MathF.PI;
                Vector3 angles = EulerAngles;
                float turn = ((target - angles.Y) % 360.0f + 540.0f) % 360.0f - 180.0f;
                float step = TurnSpeed * dt;
                angles.Y += Math.Clamp(turn, -step, step);
                EulerAngles = new Vector3(0, angles.Y, 0);
            }

            Vector3 velocity = character.Velocity;
            float horizontal = new Vector2(velocity.X, velocity.Z).Length();
            speed += (horizontal - speed) * Math.Min(1.0f, 10.0f * dt);
            animator.SetFloat("Speed", speed);
        }
    }
}
