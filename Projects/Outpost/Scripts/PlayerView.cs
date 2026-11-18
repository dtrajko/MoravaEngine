using System;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>First person or third person (docs/games/Shooter_Plan.html, "First and third person").</summary>
    public enum ViewMode
    {
        /// <summary>The camera at the eyes; the body is hidden from it (its shadow stays).</summary>
        FirstPerson,
        /// <summary>The camera over the shoulder, behind the body.</summary>
        ThirdPerson,
    }

    /// <summary>The view's numbers.</summary>
    public sealed class PlayerViewSettings
    {
        /// <summary>Third person: meters behind the shoulder, to the side of the eyes, and above them.</summary>
        public float Distance = 2.5f, ShoulderOffset = 0.5f, Height = 0.15f;

        /// <summary>The camera's sphere (meters) kept out of walls, and how fast it moves back out once one is passed (higher: faster).</summary>
        public float Radius = 0.2f, ReturnSmoothing = 6.0f;

        /// <summary>Seconds a switch (V, the shoulder) glides from one place to the other.</summary>
        public float SwitchSeconds = 0.25f;

        /// <summary>
        /// While a switch glides (V), the body is hidden as long as the camera is this close to the eyes: the camera would
        /// pass through the head and close by the face. In first person it is always hidden.
        /// </summary>
        public float HideBodyWithin = 1.0f;

        /// <summary>
        /// In third person the body is always shown, however close a wall or the ground (looking up) pulls the camera in,
        /// unless the camera is this close to the eyes: inside the head, where only the head's inside would be seen.
        /// </summary>
        public float InsideHeadWithin = 0.3f;
    }

    /// <summary>
    /// Where the camera is: at the eyes in first person, over the shoulder in third person (the right one, or the left after
    /// SwapShoulder), pulled in by a sphere cast so it never goes into a wall, and gliding out again once the wall is passed.
    /// Both views look the same way (the player's look), so the crosshair at the middle of the screen aims the same in both:
    /// only the camera's place changes. A switch glides there. Only presentation: the rules (movement, later the shots) don't
    /// know the view.
    /// </summary>
    public sealed class PlayerView
    {
        readonly PlayerViewSettings settings;
        ViewMode mode;
        float thirdWeight;          // 0 first person, 1 third person (gliding between)
        float side = 1.0f;          // 1 the right shoulder, -1 the left (gliding between)
        bool leftShoulder;
        float distance = -1.0f;     // behind the shoulder now (a wall: closer)
        float shoulder = -1.0f;     // to the side now (a wall beside the head: less)

        public PlayerView(PlayerViewSettings settings, ViewMode mode, bool leftShoulder)
        {
            this.settings = settings;
            this.mode = mode;
            this.leftShoulder = leftShoulder;
            thirdWeight = mode == ViewMode.ThirdPerson ? 1.0f : 0.0f;
            side = leftShoulder ? -1.0f : 1.0f;
        }

        public ViewMode Mode
        {
            get => mode;
            set => mode = value;
        }

        public bool LeftShoulder
        {
            get => leftShoulder;
            set => leftShoulder = value;
        }

        /// <summary>The camera's place after the last Update.</summary>
        public Vector3 Position { get; private set; }

        /// <summary>
        /// The body is hidden: first person, a switch's glide close by the head (HideBodyWithin), or in third person the
        /// camera inside the head (InsideHeadWithin).
        /// </summary>
        public bool HidesBody { get; private set; }

        /// <summary>
        /// Each frame: the eyes (the world's), the look (degrees, as PlayerCommand's) and the entity the casts ignore (the
        /// player's character). Returns the camera's place.
        /// </summary>
        public Vector3 Update(float dt, Vector3 eyes, float yaw, float pitch, Entity? ignore)
        {
            float glide = settings.SwitchSeconds > 0.0f ? dt / settings.SwitchSeconds : 1.0f;
            thirdWeight = MoveTowards(thirdWeight, mode == ViewMode.ThirdPerson ? 1.0f : 0.0f, glide);
            side = MoveTowards(side, leftShoulder ? -1.0f : 1.0f, 2.0f * glide);

            Vector3 position = eyes;
            if (thirdWeight > 0.0f)
            {
                float t = thirdWeight * thirdWeight * (3.0f - 2.0f * thirdWeight); // eased in and out
                position = ThirdPerson(dt, eyes, yaw, pitch, ignore, t);
            }
            else
            {
                distance = shoulder = -1.0f; // the next third person starts from where the walls allow, not from here
            }
            Position = position;
            // Third person, the switch done: shown unless the camera is inside the head. First person or gliding: hidden
            // while close (the glide passes the head)
            bool settled = mode == ViewMode.ThirdPerson && thirdWeight >= 1.0f;
            HidesBody = Vector3.Distance(position, eyes) < (settled ? settings.InsideHeadWithin : settings.HideBodyWithin);
            return position;
        }

        // Over the shoulder, as far as no wall is in the way: first sideways from the eyes, then back from there. Closer at
        // once, back out smoothly (no jumps past a pillar). weight: how far the switch has come (it scales what the walls
        // leave, so a switch glides at its own pace)
        Vector3 ThirdPerson(float dt, Vector3 eyes, float yaw, float pitch, Entity? ignore, float weight)
        {
            float yawRadians = yaw * MathF.PI / 180.0f, pitchRadians = pitch * MathF.PI / 180.0f;
            Vector3 right = new Vector3(MathF.Cos(yawRadians), 0.0f, -MathF.Sin(yawRadians));
            Vector3 look = new Vector3(-MathF.Sin(yawRadians) * MathF.Cos(pitchRadians), -MathF.Sin(pitchRadians), -MathF.Cos(yawRadians) * MathF.Cos(pitchRadians));
            float ease = 1.0f - MathF.Exp(-settings.ReturnSmoothing * dt);

            // Sideways (and a little up) from the eyes
            Vector3 start = eyes + Vector3.UnitY * settings.Height * weight;
            Vector3 sideways = right * (side < 0.0f ? -1.0f : 1.0f);
            float freeShoulder = settings.ShoulderOffset;
            if (freeShoulder > 0.01f && Physics.SphereCast(start, settings.Radius, sideways, freeShoulder, out RaycastHit wall, PhysicsLayers.Solid, ignore))
                freeShoulder = MathF.Max(0.0f, wall.Distance - 0.05f);
            shoulder = shoulder < 0.0f || freeShoulder <= shoulder ? freeShoulder : shoulder + (freeShoulder - shoulder) * ease;
            Vector3 pivot = start + sideways * MathF.Min(shoulder, settings.ShoulderOffset * MathF.Abs(side)) * weight;

            // Back along the look
            float freeDistance = settings.Distance;
            if (freeDistance > 0.01f && Physics.SphereCast(pivot, settings.Radius, -look, freeDistance, out RaycastHit hit, PhysicsLayers.Solid, ignore))
                freeDistance = MathF.Max(0.0f, hit.Distance - 0.05f);
            distance = distance < 0.0f || freeDistance <= distance ? freeDistance : distance + (freeDistance - distance) * ease;
            return pivot - look * distance * weight;
        }

        static float MoveTowards(float from, float to, float step) =>
            MathF.Abs(to - from) <= step ? to : from + MathF.Sign(to - from) * step;
    }
}
