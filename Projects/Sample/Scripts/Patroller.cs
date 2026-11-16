using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A character that walks a round of waypoints (docs/games/Shooter_Plan.html, E1.5): its Nav Agent takes it along the
    /// navmesh to Point1, Point2, ... and back to Point1, waiting WaitSeconds at each. Its paths take the Nav Links (up a
    /// ladder, a jump down) and go around Costly areas; a closed door stops it before the door (a partial path): it waits
    /// there and tries again. On a link it faces along the link and tells its animator Speed 0 on a ladder (it is carried
    /// up and down), its walking speed in a jump. It needs a Character, a Nav Agent and an Animator, and a baked Nav Mesh.
    /// </summary>
    public class Patroller : Entity
    {
        /// <summary>The waypoints, in order (the empty ones are skipped).</summary>
        public Entity? Point1, Point2, Point3, Point4;

        /// <summary>Seconds it stands at each waypoint.</summary>
        public float WaitSeconds = 1.0f;

        /// <summary>Seconds between tries when it can't reach the waypoint (a closed door).</summary>
        public float RetrySeconds = 1.0f;

        /// <summary>Degrees per second it turns towards where it walks.</summary>
        public float TurnSpeed = 540.0f;

        NavAgent agent = null!;
        Animator animator = null!;
        Entity?[] points = Array.Empty<Entity?>();
        int next;
        float waiting;
        float speed; // smoothed, for the animator
        bool wasOnLink;
        bool started, stopped;

        /// <summary>The waypoint it walks to.</summary>
        public int NextPoint => next;

        protected override void OnCreate()
        {
            agent = GetComponent<NavAgent>() ??
                throw new InvalidOperationException($"{Name} needs a Nav Agent (Inspector: Physics, beside Character) for Patroller");
            animator = GetComponent<Animator>() ??
                throw new InvalidOperationException($"{Name} needs an Animator for Patroller");
            points = Array.FindAll(new[] { Point1, Point2, Point3, Point4 }, p => p != null);
            if (!Navigation.IsReady)
                Log.Warn($"{Name}: the scene's Nav Mesh isn't baked: it stands");
        }

        protected override void OnUpdate(float dt)
        {
            if (points.Length == 0 || !Navigation.IsReady)
                return;

            // Arrived: wait, then the next waypoint. As near as a closed door lets it (a partial path): wait, then the same
            // one again
            if (!agent.HasPath)
            {
                if (!stopped)
                {
                    stopped = true;
                    bool reached = !agent.IsPathPartial;
                    if (started && reached)
                        next = (next + 1) % points.Length;
                    waiting = !started ? 0.0f : reached ? WaitSeconds : RetrySeconds;
                }
                waiting -= dt;
                if (waiting <= 0.0f)
                {
                    agent.SetDestination(points[next]!.Translation);
                    started = true;
                    stopped = false;
                }
            }

            bool onLink = agent.IsOnLink;
            if (onLink != wasOnLink)
            {
                Log.Info($"{Name}: {(onLink ? $"on a {agent.LinkType} link ({agent.Link?.Name})" : "off the link")}");
                wasOnLink = onLink;
            }
            Vector3 velocity = agent.Velocity;

            // Face where it walks (on a ladder: the way across it climbs, towards the ladder), at most TurnSpeed per second
            Vector2 across = new Vector2(velocity.X, velocity.Z);
            if (across.LengthSquared() > 0.04f)
            {
                float target = MathF.Atan2(velocity.X, velocity.Z) * 180.0f / MathF.PI;
                Vector3 angles = EulerAngles;
                float turn = ((target - angles.Y) % 360.0f + 540.0f) % 360.0f - 180.0f;
                float step = TurnSpeed * dt;
                EulerAngles = new Vector3(0, angles.Y + Math.Clamp(turn, -step, step), 0);
            }

            float walking = onLink && agent.LinkType == NavLinkType.Ladder ? 0.0f : across.Length();
            speed += (walking - speed) * Math.Min(1.0f, 10.0f * dt);
            animator.SetFloat("Speed", speed);
        }
    }
}
