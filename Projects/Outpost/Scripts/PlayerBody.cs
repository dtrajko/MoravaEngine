using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>The body's numbers: how fast it turns, and the speed each moving clip covers at its own pace.</summary>
    public sealed class PlayerBodySettings
    {
        /// <summary>Degrees per second the body turns to face the look (it holds the rifle: it walks every way, aiming).</summary>
        public float TurnSpeed = 720.0f;

        /// <summary>
        /// Meters per second each clip covers at its own pace (measured from the UAL1 clips' feet; the runs have a flight
        /// phase the measure misses, so they are set a little higher; the walks every way are our own, made for 3 m/s
        /// forward and back and 1.15 m/s to the side by Tools/CharacterPoses/make_fast_walk.py). The play rate is the
        /// speed over this.
        /// </summary>
        public float MoveClipSpeed = 3.0f, RunClipSpeed = 3.0f, SprintClipSpeed = 4.2f;
        public float CrouchClipSpeed = 0.7f;

        /// <summary>
        /// Move's side walks (a step-together: 1.15 m/s) and the seconds of a cycle, forward and to the side: Move blends
        /// them by MoveAngle, its length between theirs, so its speed between is the meters over the seconds, both blended.
        /// </summary>
        public float MoveSideClipSpeed = 1.15f, MoveClipSeconds = 20.0f / 30.0f, MoveSideClipSeconds = 19.0f / 30.0f;

        /// <summary>
        /// Meters per second between the states: faster than RunFrom the jog (only between the fast walk and the sprint,
        /// speeding up or slowing down), faster than SprintFrom the sprint (while sprinting).
        /// </summary>
        public float RunFrom = 4.0f, SprintFrom = 5.0f;

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
    /// The player's body (docs/games/Shooter_Plan.html, Phases 1.2 and 2.1): the model and its animator, the same in both
    /// views (the guards see and hit the same body whichever view the player chose), and the rifle in its right hand (a
    /// child on the hand's bone: E3). It shows what the motor does: idle, walking every way (the fast walk and walking
    /// quietly: Move, blended by the move's direction), the jog (between the fast walk and the sprint), sprint, crouched
    /// idle and walk, falling and landing, played at the rate the speed asks for. Holding a weapon it faces the look, and
    /// its upper body (Player.manim's UpperBody layer: E5) aims it where the camera looks, lowered while sprinting: the weapon in
    /// the hands chooses the states (Phase 2.4: the rifle's, the pistol's, the knife's) and the model hidden with the body.
    /// Only presentation: it never changes the movement.
    /// </summary>
    public sealed class PlayerBody
    {
        readonly Entity body;
        readonly Animator? animator;
        readonly Model? model;
        Model? gun;
        string aimState = "Aim", readyState = "Ready", reloadState = "Reload"; // the weapon's (SetWeapon)
        readonly PlayerBodySettings settings;
        string state = "";
        float speed;        // horizontal, smoothed
        float airTime;      // seconds off the ground
        float landing;      // seconds of the landing left
        bool jumped;        // since the jump (or a fall) until the landing
        bool sprinting;     // the UpperBody layer's: the rifle lowered
        bool dead;          // lying dead (Die): Update leaves the clip alone

        /// <summary>gun: the rifle in the body's hand (a child on its bone), hidden with the body.</summary>
        public PlayerBody(Entity body, PlayerBodySettings settings, Entity? gun = null)
        {
            this.body = body;
            this.settings = settings;
            animator = body.GetComponent<Animator>();
            model = body.GetComponent<Model>();
            this.gun = gun?.GetComponent<Model>();
            if (animator == null)
                Log.Warn($"{body.Name}: no Animator, the body won't move its limbs");
        }

        /// <summary>The body entity.</summary>
        public Entity Entity => body;

        /// <summary>The state the body plays (an Animator state of Player.manim).</summary>
        public string State => state;

        /// <summary>The body's heading in degrees (0: it faces +Z, the model's front).</summary>
        public float Heading => body.EulerAngles.Y;

        /// <summary>Hidden from the main view, with the gun in its hand (first person: only their shadows are seen). False without a Model.</summary>
        public bool HiddenInMainView
        {
            get => model != null && model.HiddenInMainView;
            set
            {
                if (model != null && model.HiddenInMainView != value)
                    model.HiddenInMainView = value;
                if (gun != null && gun.HiddenInMainView != value)
                    gun.HiddenInMainView = value;
            }
        }

        /// <summary>The move's direction from where the body faces, degrees (0 ahead, 90 right, ±180 back): MoveAngle.</summary>
        public float MoveAngle { get; private set; }

        /// <summary>
        /// The meters per second Move covers at its own pace with the move this many degrees from the body's facing:
        /// the side walks' weight in its blend is how far the angle is to the side (0 ahead or back, 1 at ±90).
        /// </summary>
        public float MoveClipSpeedAt(float moveAngle)
        {
            float angle = MathF.Abs(moveAngle);
            float side = (angle <= 90.0f ? angle : 180.0f - angle) / 90.0f;
            float seconds = settings.MoveClipSeconds + (settings.MoveSideClipSeconds - settings.MoveClipSeconds) * side;
            float meters = settings.MoveClipSpeed * settings.MoveClipSeconds
                + (settings.MoveSideClipSpeed * settings.MoveSideClipSeconds - settings.MoveClipSpeed * settings.MoveClipSeconds) * side;
            return meters / seconds;
        }

        /// <summary>The state the upper body (the UpperBody layer) plays: Aim, Ready, Reload (the rifle's), PistolAim, KnifeIdle...</summary>
        public string UpperState => animator?.GetLayerState("UpperBody") ?? "";

        /// <summary>
        /// The weapon in the hands (Phase 2.4): its model on the hand (hidden from the main view with the body), its upper
        /// body states (Player.manim's UpperBody: aiming, lowered while sprinting, reloading). The upper body takes its aim
        /// (or the ready while sprinting) at once.
        /// </summary>
        public void SetWeapon(Entity? weapon, string aim, string ready, string reload)
        {
            Model? held = weapon?.GetComponent<Model>();
            if (held != null && model != null && held.HiddenInMainView != model.HiddenInMainView)
                held.HiddenInMainView = model.HiddenInMainView;
            gun = held;
            aimState = aim;
            readyState = ready;
            reloadState = reload;
            animator?.Play(sprinting ? readyState : aimState, 0.2f, "UpperBody");
        }

        /// <summary>The weapon's reload started (true) or finished: the upper body plays it, then aims again (or the ready while sprinting).</summary>
        public void Reloading(bool on)
        {
            if (animator == null)
                return;
            // (into the reload in 0.1 s, as the first person's arms: the sniper's gun switches hands by the arms' clip time)
            animator.Play(on ? reloadState : sprinting ? readyState : aimState, on ? 0.1f : 0.2f, "UpperBody");
        }

        /// <summary>A strike (the knife's slash: the state, on the upper body), or back to holding the weapon when it ended (null).</summary>
        public void Attack(string? state)
        {
            if (animator == null)
                return;
            animator.Play(state ?? (sprinting ? readyState : aimState), state != null ? 0.05f : 0.15f, "UpperBody");
        }

        /// <summary>
        /// Dead (Phase 3.1): the body falls (Death, Player.manim's Death_Back: the dummies' fall, not in place: it lies where
        /// the clip takes it), the upper body with it (the UpperBody layer plays the same clip, so nothing of the aim stays);
        /// Update leaves it lying until Revive.
        /// </summary>
        public void Die()
        {
            dead = true;
            animator?.Play("Death", 0.12f);
            animator?.Play("Death", 0.12f, "UpperBody");
            state = "Death";
        }

        /// <summary>Alive again (the respawn): standing at once, facing heading degrees (the model's: the look's + 180), the weapon's aim.</summary>
        public void Revive(float heading)
        {
            dead = false;
            jumped = false;
            airTime = landing = 0.0f;
            speed = 0.0f;
            sprinting = false;
            animator?.SetBool("Sprinting", false);
            body.Translation = Vector3.Zero; // (the death's clip moves only the bones; kept at the feet anyway)
            body.EulerAngles = new Vector3(0.0f, (heading % 360.0f + 540.0f) % 360.0f - 180.0f, 0.0f);
            animator?.Play("Idle", 0.0f);
            animator?.Play(aimState, 0.0f, "UpperBody");
            state = "Idle";
        }

        /// <summary>The body lies dead (Die, until Revive).</summary>
        public bool IsDead => dead;

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

        /// <summary>Each frame: velocity (meters per second, the world's), the look (the command's yaw and pitch, degrees).</summary>
        public void Update(float dt, Vector3 velocity, bool grounded, bool crouched, bool walking, bool sprinting, float lookYaw, float lookPitch)
        {
            if (dead)
                return;
            float horizontal = new Vector2(velocity.X, velocity.Z).Length();
            speed += (horizontal - speed) * MathF.Min(1.0f, 12.0f * dt);
            airTime = grounded ? 0.0f : airTime + dt;
            if (airTime >= settings.FallAfter)
                jumped = true; // in the air long enough to fall: the landing is waited for, as after a jump
            landing = MathF.Max(0.0f, landing - dt);

            Turn(dt, lookYaw);
            // Where it moves, from where it faces (the model faces +Z at heading 0; right of it is -X turned by the heading)
            if (horizontal > 0.05f)
            {
                float heading = body.EulerAngles.Y * MathF.PI / 180.0f;
                Vector3 forward = new Vector3(MathF.Sin(heading), 0.0f, MathF.Cos(heading));
                Vector3 right = new Vector3(-MathF.Cos(heading), 0.0f, MathF.Sin(heading));
                MoveAngle = MathF.Atan2(Vector3.Dot(velocity, right), Vector3.Dot(velocity, forward)) * 180.0f / MathF.PI;
            }

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
            else if (sprinting && speed > settings.SprintFrom)
            {
                wanted = "Sprint";
                clipSpeed = settings.SprintClipSpeed;
            }
            else if (!walking && speed > settings.RunFrom)
            {
                wanted = "Run";
                clipSpeed = settings.RunClipSpeed;
            }
            else
            {
                wanted = "Move"; // the fast walk or walking quietly, every way
                clipSpeed = MoveClipSpeedAt(MoveAngle);
            }

            if (animator == null)
            {
                state = wanted;
                return;
            }
            if (clipSpeed > 0.0f)
                animator.SetFloat("Rate", Math.Clamp(speed / clipSpeed, settings.MinRate, settings.MaxRate));
            animator.SetFloat("MoveAngle", MoveAngle);
            animator.SetFloat("AimPitch", lookPitch);
            bool lowered = wanted == "Sprint" || wanted == "Run";
            if (lowered != this.sprinting)
            {
                this.sprinting = lowered;
                animator.SetBool("Sprinting", lowered);
            }
            if (wanted != state)
            {
                float fade = wanted == "Fall" ? 0.15f : state == "Fall" || wanted == "Land" ? 0.1f : 0.2f;
                animator.Play(wanted, state.Length == 0 ? 0.0f : fade);
                state = wanted;
            }
        }

        // Towards the look, at TurnSpeed (holding the rifle it faces where it aims, moving any way)
        void Turn(float dt, float lookYaw)
        {
            float current = body.EulerAngles.Y;
            float target = lookYaw + 180.0f; // the look's yaw faces -Z at 0, the model +Z
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
