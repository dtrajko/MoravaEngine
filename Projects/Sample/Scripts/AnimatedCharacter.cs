using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A character with an animated model: Move walks it (forward being away from the camera; Run held: faster), it turns
    /// to face where it walks, and its animator is told how fast it goes (the Speed parameter, meters per second) and when
    /// to use what is in front of it (Use: the trigger, on the Use action). With a JumpSpeed, Jump jumps: the Jump trigger
    /// at once, the push up JumpDelay later (when the clip leaves the ground), and Grounded each step. With a RunJumpSpeed,
    /// a jump faster than it is a running jump: the RunJump trigger, the push up RunJumpDelay later. The animator's
    /// controller picks the states and crossfades: Idle, Walk (and Run) by Speed, Use or Jump from any state. It needs a
    /// Character (Inspector: Physics) and an Animator.
    /// With a FootstepSound, the controller's Footstep events (each foot down in the walk and run clips) are heard, louder
    /// the faster it goes; a JumpSound at the push up and a LandSound when it comes down, louder the faster it falls.
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

        /// <summary>Meters per second up at the jump (0: no jumping; the controller then needs no Jump and Grounded).</summary>
        public float JumpSpeed = 0.0f;

        /// <summary>Seconds from the Jump press to the push up: the jump clip's crouch.</summary>
        public float JumpDelay = 0.0f;

        /// <summary>Meters per second above which a jump is a running jump (0: none; the controller then needs no RunJump).</summary>
        public float RunJumpSpeed = 0.0f;

        /// <summary>Seconds from the Jump press to the push up in a running jump: its clip's take-off.</summary>
        public float RunJumpDelay = 0.0f;

        /// <summary>Forward is where this entity looks (none: along -Z).</summary>
        public Entity? Camera;

        /// <summary>The sound of a step, at each Footstep event of the animator; {0} in it: one of 1 to FootstepVariants, at random.</summary>
        public string FootstepSound = "";

        /// <summary>How many sounds {0} in FootstepSound picks from (a step never sounds like the one before).</summary>
        public int FootstepVariants = 1;

        /// <summary>A step's volume at Speed; at RunSpeed and faster: RunFootstepVolume.</summary>
        public float FootstepVolume = 0.45f;

        public float RunFootstepVolume = 0.8f;

        /// <summary>At the push up of a jump.</summary>
        public string JumpSound = "";

        /// <summary>When it comes down from a jump or a fall (after LandAfter seconds in the air).</summary>
        public string LandSound = "";

        public float LandAfter = 0.2f;

        CharacterController character = null!;
        Animator animator = null!;
        Vector3 walk;
        float speed; // the horizontal speed the character really has (a wall stops it), smoothed for the animator
        float jumpIn = -1.0f; // seconds to the push up of a jump pressed (negative: none)
        bool rising;          // pushed up and still going up: not on the ground, even where it still touches it
        float airTime;        // seconds off the ground
        float fallSpeed;      // meters per second down, the last step in the air
        int lastFootstep;
        readonly Random random = new Random();

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

            if (JumpSpeed > 0)
            {
                if (Input.WasPressed("Jump") && character.IsGrounded && jumpIn < 0 && !rising)
                {
                    bool running = RunJumpSpeed > 0 && speed > RunJumpSpeed;
                    animator.SetTrigger(running ? "RunJump" : "Jump");
                    jumpIn = running ? RunJumpDelay : JumpDelay;
                }
                animator.SetBool("Grounded", character.IsGrounded && !rising);
            }

            // Down after a while in the air: the landing, louder the faster it fell (8 m/s: at full volume)
            bool grounded = character.IsGrounded && !rising;
            if (grounded && airTime > LandAfter && LandSound.Length > 0)
                Audio.PlayOneShot(LandSound, WorldPosition, Math.Clamp(fallSpeed / 8.0f, 0.3f, 1.0f), AudioBus.Effects, Pitch());
            if (grounded)
            {
                airTime = 0;
            }
            else
            {
                airTime += dt;
                fallSpeed = Math.Max(0, -character.Velocity.Y);
            }

            float up;
            if (jumpIn >= 0 && (jumpIn -= dt) < 0)
            {
                up = JumpSpeed;
                rising = true;
                if (JumpSound.Length > 0)
                    Audio.PlayOneShot(JumpSound, WorldPosition, 0.7f, AudioBus.Effects, Pitch());
            }
            else
            {
                rising &= character.Velocity.Y > 0;
                up = character.IsGrounded && !rising ? 0 : character.Velocity.Y - Gravity * dt;
            }
            character.Move(new Vector3(walk.X, up, walk.Z));

            if (Input.WasPressed("Use"))
                animator.SetTrigger("Use");
        }

        protected override void OnAnimationEvent(AnimationEvent animationEvent)
        {
            if (animationEvent.Name != "Footstep" || FootstepSound.Length == 0 || !character.IsGrounded)
                return;
            // Another sound than the last step's
            int variant = 1;
            if (FootstepVariants > 1)
            {
                variant = random.Next(1, lastFootstep == 0 ? FootstepVariants + 1 : FootstepVariants);
                if (lastFootstep != 0 && variant >= lastFootstep)
                    variant++;
            }
            lastFootstep = variant;
            float fast = RunSpeed > Speed ? Math.Clamp((speed - Speed) / (RunSpeed - Speed), 0.0f, 1.0f) : 0.0f;
            float volume = FootstepVolume + (RunFootstepVolume - FootstepVolume) * fast;
            Audio.PlayOneShot(string.Format(FootstepSound, variant), WorldPosition, volume, AudioBus.Effects, Pitch());
        }

        // A little higher or lower each time, so no two sound the same
        float Pitch() => 0.94f + 0.12f * (float)random.NextDouble();

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
