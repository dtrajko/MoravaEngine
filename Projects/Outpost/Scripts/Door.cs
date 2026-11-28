using System;
using System.Collections.Generic;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// A door's leaf on its hinge (docs/games/Shooter_Plan.html, Phase 4.1), made by the level builder from an opening with
    /// a Leaf (Tools/LevelBuilder): the entity is at the hinge, the leaf along its +X, Width long. Use opens it, away from
    /// whoever opens it, and closes it; a double door's two leaves (Partner) go together. A Kinematic body, so the
    /// navmesh's bake leaves it out and it pushes what it swings into.
    ///
    /// The navmesh: its Area (a Nav Area, Door) is closed only while the door is Locked. A closed door that isn't locked
    /// is on the paths: whoever walks one opens it on arriving (OpenFor: the guards, Phase 5), so nobody behind a shut door
    /// is stuck there. A locked door's paths go round by another way; Unlock (the mission, Phase 7) opens them.
    /// </summary>
    public class Door : Entity, IUsable
    {
        /// <summary>The Nav Area (Type: Door) in the doorway (a double door's leaves share it).</summary>
        public Entity? Area;

        /// <summary>A double door's other leaf: it opens and closes with this one.</summary>
        public Entity? Partner;

        /// <summary>Locked: Use only rattles it, the navmesh's paths go round.</summary>
        public bool Locked;

        /// <summary>Meters from the hinge to the leaf's free end.</summary>
        public float Width = 1.0f;

        /// <summary>Degrees it swings open; degrees per second it swings.</summary>
        public float OpenAngle = 100.0f, Speed = 160.0f;

        /// <summary>Meters from the eyes to the doorway it is used from.</summary>
        public float Reach = 2.2f;

        public string OpenSound = "Assets/Audio/Door_Open.wav";
        public string CloseSound = "Assets/Audio/Door_Close.wav";
        public string LockedSound = "Assets/Audio/Door_Locked.wav";
        public float Volume = 0.7f;

        static readonly List<Door> all = new();

        NavArea? navArea;
        Door? partner;
        float closedYaw;
        float angle;      // degrees from closed, now
        float target;     // degrees from closed it swings to (0: closed)
        bool closeSound;  // swinging shut, the leaf that was used: the close sound when it gets there

        /// <summary>The doors in the scene.</summary>
        public static IReadOnlyList<Door> All
        {
            get
            {
                all.RemoveAll(d => !d.Exists);
                return all;
            }
        }

        /// <summary>Open (or opening); false closed or closing.</summary>
        public bool IsOpen => target != 0.0f;

        /// <summary>Degrees it is open now (0 closed; positive or negative by the way it swung).</summary>
        public float Angle => angle;

        /// <summary>Where the hinge is, and the leaf's direction closed (along the doorway).</summary>
        public Vector3 Hinge => WorldPosition;
        public Vector3 Along => new Vector3(MathF.Cos(closedYaw * MathF.PI / 180.0f), 0.0f, -MathF.Sin(closedYaw * MathF.PI / 180.0f));

        /// <summary>The middle of the doorway (a double door: of its own half), at the floor.</summary>
        public Vector3 Middle => Hinge + Along * (Width * 0.5f);

        protected override void OnCreate()
        {
            closedYaw = EulerAngles.Y;
            navArea = Area?.GetComponent<NavArea>();
            if (navArea == null)
                Log.Warn($"{Name}: its Area has no Nav Area: the navmesh doesn't know when it is locked");
            partner = Partner as Door;
            if (Partner != null && partner == null)
                Log.Warn($"{Name}: its Partner has no Door script");
            all.RemoveAll(d => !d.Exists);
            all.Add(this);
            Usables.Add(this);
            UpdateArea();
        }

        void UpdateArea()
        {
            if (navArea != null && navArea.IsOpen == Locked)
                navArea.IsOpen = !Locked;
        }

        // --- IUsable

        public float UseReach => Reach;

        public string UseText => Locked ? "LOCKED" : IsOpen ? "CLOSE" : "OPEN";

        public bool CanUse => !Locked;

        /// <summary>The nearest point of the doorway (the leaf's line closed) at the height of a hand.</summary>
        public Vector3 UsePoint(Vector3 eyes)
        {
            Vector3 start = Hinge + new Vector3(0.0f, 1.0f, 0.0f);
            float along = Math.Clamp(Vector3.Dot(eyes - start, Along), 0.0f, Width);
            return start + Along * along;
        }

        public void Use(Player player)
        {
            if (Locked)
            {
                Audio.PlayOneShot(LockedSound, Middle + new Vector3(0, 1, 0), Volume);
                Log.Info($"{Name}: locked");
                return;
            }
            SetOpen(!IsOpen, player.WorldPosition);
        }

        // ---

        /// <summary>
        /// Opens it for someone at a place (a guard on its path, Phase 5), swinging away from them; false when it is
        /// locked (it stays shut).
        /// </summary>
        public bool OpenFor(Vector3 from)
        {
            if (Locked)
                return false;
            if (!IsOpen)
                SetOpen(true, from);
            return true;
        }

        /// <summary>
        /// For whoever walks a path on the navmesh (a guard, Phase 5): opens the shut doors its next meters (ahead) go
        /// through, swinging away from it (position: where it is; the path from its feet, NavAgent.Path). Doors it only
        /// passes by stay as they are. True when one of them is locked (it can't go on: the path was made before the lock).
        /// </summary>
        public static bool OpenAlong(Vector3 position, NavPath? path, float ahead = 2.5f)
        {
            if (path == null || path.Corners.Length < 2)
                return false;
            bool locked = false;
            float left = ahead;
            for (int i = 1; i < path.Corners.Length && left > 0.0f; i++)
            {
                Vector3 a = path.Corners[i - 1], b = path.Corners[i];
                float length = Vector3.Distance(a, b);
                if (length > left)
                    b = a + (b - a) * (left / length);
                left -= length;
                foreach (Door door in All)
                    if (!door.IsOpen && door.Crosses(a, b))
                    {
                        if (door.Locked)
                            locked = true;
                        else
                            door.SetOpen(true, position);
                    }
            }
            return locked;
        }

        /// <summary>A walk from a to b (on the floor) goes through this leaf's doorway: the lines cross, seen from above.</summary>
        public bool Crosses(Vector3 a, Vector3 b)
        {
            if (MathF.Abs(a.Y - Hinge.Y) > 1.5f && MathF.Abs(b.Y - Hinge.Y) > 1.5f)
                return false; // another floor
            Vector2 p = new(a.X, a.Z), r = new(b.X - a.X, b.Z - a.Z);
            Vector2 q = new(Hinge.X, Hinge.Z), s = new(Along.X * Width, Along.Z * Width);
            float denominator = r.X * s.Y - r.Y * s.X;
            if (MathF.Abs(denominator) < 1e-6f)
                return false; // along the wall
            Vector2 qp = q - p;
            float t = (qp.X * s.Y - qp.Y * s.X) / denominator, u = (qp.X * r.Y - qp.Y * r.X) / denominator;
            return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
        }

        /// <summary>Opens (away from the place: the one opening it is there) or closes it, and its partner.</summary>
        public void SetOpen(bool open, Vector3 from)
        {
            if (Locked && open)
                return;
            SwingTo(open, from, sound: true);
            partner?.SwingTo(open, from, sound: false);
            Log.Info($"{Name}: {(open ? "open" : "closed")}");
        }

        void SwingTo(bool open, Vector3 from, bool sound)
        {
            if (open == IsOpen)
                return;
            if (open)
            {
                // Turning the leaf by a positive angle (around Y) moves its free end along this normal: the way away from them
                float yaw = closedYaw * MathF.PI / 180.0f;
                Vector3 normal = new Vector3(-MathF.Sin(yaw), 0.0f, -MathF.Cos(yaw));
                target = Vector3.Dot(from - Hinge, normal) > 0.0f ? -OpenAngle : OpenAngle;
                closeSound = false;
                if (sound)
                    Audio.PlayOneShot(OpenSound, Middle + new Vector3(0, 1, 0), Volume);
            }
            else
            {
                target = 0.0f;
                closeSound = sound;
            }
        }

        /// <summary>Locks or unlocks it (a locked door shuts first); the navmesh's paths follow at once.</summary>
        public void SetLocked(bool locked)
        {
            if (locked && IsOpen)
                SetOpen(false, Hinge);
            Locked = locked;
            UpdateArea();
            if (partner != null)
            {
                partner.Locked = locked;
                partner.UpdateArea();
            }
        }

        protected override void OnFixedUpdate(float dt)
        {
            if (angle == target)
                return;
            float step = Speed * dt;
            angle += Math.Clamp(target - angle, -step, step);
            EulerAngles = new Vector3(0.0f, closedYaw + angle, 0.0f);
            if (angle == target && closeSound)
            {
                closeSound = false;
                Audio.PlayOneShot(CloseSound, Middle + new Vector3(0, 1, 0), Volume);
            }
        }
    }
}
