using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>The body's numbers: how fast it turns, and the speed each moving clip covers at its own pace.</summary>
    public sealed class PlayerBodySettings
    {
        /// <summary>Degrees per second the body turns towards where it moves (or where it looks, standing).</summary>
        public float TurnSpeed = 600.0f;

        /// <summary>Standing, the body turns to the look once the look is this many degrees away from it.</summary>
        public float StandingTurnAngle = 80.0f;

        /// <summary>
        /// Meters per second each clip covers at its own pace (measured from the UAL1 clips' feet; the runs have a flight
        /// phase the measure misses, so they are set a little higher). The play rate is the speed over this.
        /// </summary>
        public float WalkClipSpeed = 0.95f, RunClipSpeed = 3.0f, SprintClipSpeed = 4.2f, CrouchClipSpeed = 0.7f;

        /// <summary>The play rate's limits: faster than MaxRate the clip looks hurried, so the feet slide a little instead.</summary>
        public float MinRate = 0.6f, MaxRate = 2.0f;

        /// <summary>Seconds in the air before the body falls (shorter hops down a step keep it walking).</summary>
        public float FallAfter = 0.2f;

        /// <summary>Landing faster than this (meters per second) plays the landing; slower lands straight into the walk or run.</summary>
        public float LandSpeed = 5.0f;

        /// <summary>Seconds the landing is played before the body walks or runs again.</summary>
        public float LandSeconds = 0.35f;
    }

    /// <summary>
    /// The player's body (docs/games/Shooter_Plan.html, Phase 1.2): the model and its animator, the same in both views (the
    /// guards see and hit the same body whichever view the player chose). It shows what the motor does: idle, walk, run,
    /// sprint, crouched idle and walk, falling and landing, played at the rate the speed asks for; it turns to where it
    /// moves, and standing, to where the player looks once that is far enough round. Only presentation: it never changes
    /// the movement.
    /// </summary>
    public sealed class PlayerBody
    {
        readonly Entity body;
        readonly Animator? animator;
        readonly Model? model;
        readonly PlayerBodySettings settings;
        string state = "";
        float speed;        // horizontal, smoothed
        float airTime;      // seconds off the ground
        float landing;      // seconds of the landing left
        bool jumped;        // since the jump (or a fall) until the landing
        bool turningToLook; // standing: turning on the spot until it faces the look

        public PlayerBody(Entity body, PlayerBodySettings settings)
        {
            this.body = body;
            this.settings = settings;
            animator = body.GetComponent<Animator>();
            model = body.GetComponent<Model>();
            if (animator == null)
                Log.Warn($"{body.Name}: no Animator, the body won't move its limbs");
        }

        /// <summary>The body entity.</summary>
        public Entity Entity => body;

        /// <summary>The state the body plays (an Animator state of Player.manim).</summary>
        public string State => state;

        /// <summary>The body's heading in degrees (0: it faces +Z, the model's front).</summary>
        public float Heading => body.EulerAngles.Y;

        /// <summary>Hidden from the main view (first person: only its shadow is seen). False without a Model.</summary>
        public bool HiddenInMainView
        {
            get => model != null && model.HiddenInMainView;
            set
            {
                if (model != null && model.HiddenInMainView != value)
                    model.HiddenInMainView = value;
            }
        }

        /// <summary>Standing, turn to face the look (a switch to third person: the camera then sees its back, not its face).</summary>
        public void FaceLook() => turningToLook = true;

        /// <summary>The motor jumped (from the fixed step): the body leaves the ground at once.</summary>
        public void OnJumped() => jumped = true;

        /// <summary>The motor landed, this fast (meters per second).</summary>
        public void OnLanded(float fallSpeed)
        {
            // (jumped also holds the fall until here: false now)
            if (fallSpeed >= settings.LandSpeed)
                landing = settings.LandSeconds;
            airTime = 0.0f;
            jumped = false;
        }

        /// <summary>Each frame: velocity (meters per second, the world's), lookYaw (the command's, degrees).</summary>
        public void Update(float dt, Vector3 velocity, bool grounded, bool crouched, bool walking, bool sprinting, float lookYaw)
        {
            float horizontal = new Vector2(velocity.X, velocity.Z).Length();
            speed += (horizontal - speed) * MathF.Min(1.0f, 12.0f * dt);
            airTime = grounded ? 0.0f : airTime + dt;
            if (airTime >= settings.FallAfter)
                jumped = true; // in the air long enough to fall: the landing is waited for, as after a jump
            landing = MathF.Max(0.0f, landing - dt);

            Turn(dt, velocity, horizontal, lookYaw);

            // The state: from the motor's, the speed it really has deciding between standing and moving (a wall stops it)
            string wanted;
            float clipSpeed = 0.0f;
            if ((!grounded && (jumped || airTime >= settings.FallAfter)) || (state == "Fall" && jumped))
                wanted = "Fall"; // (down, it falls until the motor's landing, in the next fixed step: no frame of standing between)
            else if (landing > 0.0f)
                wanted = "Land";
            else if (crouched)
            {
                wanted = speed > 0.3f ? "CrouchWalk" : "CrouchIdle";
                clipSpeed = settings.CrouchClipSpeed;
            }
            else if (speed < 0.3f)
                wanted = "Idle";
            else if (walking || speed < 2.0f)
            {
                wanted = "Walk";
                clipSpeed = settings.WalkClipSpeed;
            }
            else if (sprinting && speed > 5.0f)
            {
                wanted = "Sprint";
                clipSpeed = settings.SprintClipSpeed;
            }
            else
            {
                wanted = "Run";
                clipSpeed = settings.RunClipSpeed;
            }

            if (animator == null)
            {
                state = wanted;
                return;
            }
            if (clipSpeed > 0.0f)
                animator.SetFloat("Rate", Math.Clamp(speed / clipSpeed, settings.MinRate, settings.MaxRate));
            if (wanted != state)
            {
                float fade = wanted == "Fall" ? 0.15f : state == "Fall" || wanted == "Land" ? 0.1f : 0.2f;
                animator.Play(wanted, state.Length == 0 ? 0.0f : fade);
                state = wanted;
            }
        }

        // Moving: towards where it goes. Standing: towards the look once it is StandingTurnAngle away (a turn on the spot)
        void Turn(float dt, Vector3 velocity, float horizontal, float lookYaw)
        {
            float current = body.EulerAngles.Y;
            float target;
            if (horizontal > 0.5f)
            {
                target = MathF.Atan2(velocity.X, velocity.Z) * 180.0f / MathF.PI; // the model faces +Z at heading 0
                turningToLook = false;
            }
            else
            {
                target = lookYaw + 180.0f; // the look's yaw faces -Z at 0
                float away = MathF.Abs(DeltaAngle(current, target));
                if (away >= settings.StandingTurnAngle)
                    turningToLook = true;
                else if (away < 1.0f)
                    turningToLook = false;
                if (!turningToLook)
                    return;
            }
            float delta = DeltaAngle(current, target);
            float step = settings.TurnSpeed * dt;
            float heading = current + Math.Clamp(delta, -step, step);
            heading = (heading % 360.0f + 540.0f) % 360.0f - 180.0f;
            body.EulerAngles = new Vector3(0.0f, heading, 0.0f);
        }

        /// <summary>From a to b the short way round, -180 to 180 degrees.</summary>
        public static float DeltaAngle(float a, float b) => ((b - a) % 360.0f + 540.0f) % 360.0f - 180.0f;
    }
}
