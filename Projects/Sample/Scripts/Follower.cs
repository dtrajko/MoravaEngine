using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A character that follows another one around (docs/games/Shooter_Plan.html, E1.4): its Nav Agent walks it to the
    /// Target along the navmesh, around walls and the other characters, and stops FollowDistance from it. Farther behind
    /// than RunDistance it hurries (RunSpeed). It turns to face where it walks and tells its animator how fast it goes (the
    /// Speed parameter, meters per second), as AnimatedCharacter does. Stuck near the Target (the other followers stand
    /// where it would go) is near enough: it waits there until the Target moves on. It needs a Character, a Nav Agent and
    /// an Animator, and the scene a baked Nav Mesh.
    /// </summary>
    public class Follower : Entity
    {
        /// <summary>Whom it follows (none: it stands).</summary>
        public Entity? Target;

        /// <summary>Meters from the Target where it stops (followers of the same target: each its own, so they don't crowd).</summary>
        public float FollowDistance = 1.5f;

        /// <summary>Meters per second while it is near.</summary>
        public float WalkSpeed = 1.65f;

        /// <summary>Meters per second while it is farther than RunDistance from the Target.</summary>
        public float RunSpeed = 3.5f;

        /// <summary>Meters: farther behind than this, it hurries.</summary>
        public float RunDistance = 6.0f;

        /// <summary>Meters the Target moves before it is sent there again (a new path a few times a second at most).</summary>
        public float RepathDistance = 0.5f;

        /// <summary>Degrees per second it turns towards where it walks.</summary>
        public float TurnSpeed = 540.0f;

        NavAgent agent = null!;
        Animator animator = null!;
        Vector3 sentTo;
        bool sent;
        float speed; // smoothed, for the animator

        protected override void OnCreate()
        {
            agent = GetComponent<NavAgent>() ??
                throw new InvalidOperationException($"{Name} needs a Nav Agent (Inspector: Physics, beside Character) for Follower");
            animator = GetComponent<Animator>() ??
                throw new InvalidOperationException($"{Name} needs an Animator for Follower");
            if (!Navigation.IsReady)
                Log.Warn($"{Name}: the scene's Nav Mesh isn't baked: it stands");
        }

        protected override void OnUpdate(float dt)
        {
            if (Target != null && Navigation.IsReady)
                Follow();

            Vector3 velocity = Navigation.IsReady ? agent.Velocity : Vector3.Zero;

            // Face where it walks: the shorter way, at most TurnSpeed degrees per second (the model looks along +Z)
            if (velocity.LengthSquared() > 0.04f)
            {
                float target = MathF.Atan2(velocity.X, velocity.Z) * 180.0f / MathF.PI;
                Vector3 angles = EulerAngles;
                float turn = ((target - angles.Y) % 360.0f + 540.0f) % 360.0f - 180.0f;
                float step = TurnSpeed * dt;
                EulerAngles = new Vector3(0, angles.Y + Math.Clamp(turn, -step, step), 0);
            }

            float horizontal = new Vector2(velocity.X, velocity.Z).Length();
            speed += (horizontal - speed) * Math.Min(1.0f, 10.0f * dt);
            animator.SetFloat("Speed", speed);
        }

        void Follow()
        {
            Vector3 target = Target!.Translation;
            float distance = new Vector2(target.X - Translation.X, target.Z - Translation.Z).Length();

            // Far behind: hurry; near: walk, and stop FollowDistance from it
            agent.Speed = distance > RunDistance ? RunSpeed : WalkSpeed;
            agent.StoppingDistance = FollowDistance;

            // Sent again when the target has moved on, or when it arrived and the target walked away since
            bool moved = !sent || Vector3.Distance(target, sentTo) > RepathDistance;
            bool left = agent.HasArrived && distance > FollowDistance + RepathDistance;

            // Stuck near it, the others in the way: near enough, it waits (sent again once the target moves on)
            if (agent.IsStuck && distance < FollowDistance + 1.5f)
            {
                agent.Stop();
                return;
            }
            if ((moved || left) && distance > FollowDistance)
            {
                sent = agent.SetDestination(target);
                sentTo = target;
            }
        }
    }
}
