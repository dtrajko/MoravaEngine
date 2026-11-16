using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A door that slides up to open (docs/games/Shooter_Plan.html, E1.5): the player opens and closes it with Use when
    /// near, and its Nav Area (a Door) opens and closes with it, so the characters walking the navmesh go through it or find
    /// another way (or wait before it). CycleSeconds above 0 opens and closes it by itself. The entity is the door's slab:
    /// a Kinematic rigid body, so the navmesh's bake doesn't take it for a wall.
    /// </summary>
    public class Door : Entity
    {
        /// <summary>The Nav Area (Type: Door) in the doorway.</summary>
        public Entity? Area;

        /// <summary>Who opens it with Use (none: only CycleSeconds does).</summary>
        public Entity? Player;

        /// <summary>Meters from the door the player can reach it from.</summary>
        public float Reach = 3.0f;

        /// <summary>Meters it slides up when open.</summary>
        public float OpenHeight = 2.1f;

        /// <summary>Meters per second it slides.</summary>
        public float Speed = 3.0f;

        /// <summary>Seconds between opening and closing by itself (0: only the player).</summary>
        public float CycleSeconds = 0.0f;

        NavArea? navArea;
        Vector3 closed;
        bool open;
        float cycle;

        protected override void OnCreate()
        {
            navArea = Area?.GetComponent<NavArea>();
            if (navArea == null)
                Log.Warn($"{Name}: its Area has no Nav Area: the characters don't know when it is closed");
            closed = Translation;
            open = navArea?.IsOpen ?? true;
            if (open)
                Translation = closed + new Vector3(0, OpenHeight, 0);
        }

        /// <summary>Opens or closes it (and its Nav Area at once).</summary>
        public void SetOpen(bool value)
        {
            open = value;
            if (navArea != null)
                navArea.IsOpen = value;
            Log.Info($"{Name}: {(value ? "open" : "closed")}");
        }

        protected override void OnUpdate(float dt)
        {
            if (Player != null && Input.WasPressed("Use") && Vector3.Distance(Player.Translation, closed) < Reach)
                SetOpen(!open);
            if (CycleSeconds > 0.0f && (cycle += dt) >= CycleSeconds)
            {
                cycle = 0.0f;
                SetOpen(!open);
            }

            // The slab towards where it should be
            Vector3 target = closed + new Vector3(0, open ? OpenHeight : 0, 0);
            Vector3 position = Translation;
            float step = Speed * dt;
            position.Y += Math.Clamp(target.Y - position.Y, -step, step);
            Translation = position;
        }
    }
}
