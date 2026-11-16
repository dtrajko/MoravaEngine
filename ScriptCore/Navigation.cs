using System;
using System.Numerics;

namespace Morava
{
    /// <summary>A way along the navmesh (Navigation.FindPath): its corners, the first at the start and the last at the end.</summary>
    public sealed class NavPath
    {
        internal NavPath(Vector3[] corners, bool complete)
        {
            Corners = corners;
            IsComplete = complete;
            float length = 0.0f;
            for (int i = 1; i < corners.Length; i++)
                length += Vector3.Distance(corners[i - 1], corners[i]);
            Length = length;
        }

        /// <summary>The points where the way turns: walk straight from one to the next.</summary>
        public Vector3[] Corners { get; }

        /// <summary>
        /// Whether it reaches the end. A partial path (false) stops at the reachable point nearest the end: on another
        /// floor with no stairs to it, on a roof, behind a locked door.
        /// </summary>
        public bool IsComplete { get; }

        /// <summary>How long the way is, in meters.</summary>
        public float Length { get; }

        /// <summary>Where it ends: the end given, or the reachable point nearest it.</summary>
        public Vector3 End => Corners[Corners.Length - 1];
    }

    /// <summary>
    /// Questions to the scene's navmesh (docs/games/Shooter_Plan.html, E1): where a character can walk. The navmesh is the
    /// Nav Mesh's, baked in the editor (Bake) from the static colliders; without one every query says no.
    /// </summary>
    public static unsafe class Navigation
    {
        /// <summary>Whether the scene has a baked navmesh.</summary>
        public static bool IsReady => Interop.Game->Navigation_IsReady() != 0;

        /// <summary>
        /// The way from one point to another, walking. Both are taken to the navmesh's nearest point (within an agent's
        /// width across, its height up and down). False when either isn't near the navmesh or nothing joins them; a way
        /// that only gets near the end is returned (IsComplete false).
        /// </summary>
        public static bool FindPath(Vector3 from, Vector3 to, out NavPath path)
        {
            const int Room = 64;
            Vector3* corners = stackalloc Vector3[Room];
            int complete;
            int count = Interop.Game->Navigation_FindPath(&from, &to, corners, Room, &complete);
            if (count <= 0)
            {
                path = null!;
                return false;
            }
            Vector3[] result = new Vector3[count];
            if (count <= Room)
            {
                for (int i = 0; i < count; i++)
                    result[i] = corners[i];
            }
            else
            {
                fixed (Vector3* buffer = result)
                    count = Interop.Game->Navigation_FindPath(&from, &to, buffer, result.Length, &complete);
                if (count <= 0)
                {
                    path = null!;
                    return false;
                }
                if (count < result.Length)
                    Array.Resize(ref result, count);
            }
            path = new NavPath(result, complete != 0);
            return true;
        }

        /// <summary>The way from one point to another (Corners), or null when there is none; see FindPath(from, to, out path).</summary>
        public static NavPath? FindPath(Vector3 from, Vector3 to) => FindPath(from, to, out NavPath path) ? path : null;

        /// <summary>
        /// Whether walking straight from one point to another stays on the navmesh (no wall, ledge or gap between them,
        /// looked at across: a ramp up is fine). hit: where the walk leaves the navmesh (to when it doesn't).
        /// </summary>
        public static bool Raycast(Vector3 from, Vector3 to, out Vector3 hit)
        {
            Vector3 result;
            bool clear = Interop.Game->Navigation_Raycast(&from, &to, &result) != 0;
            hit = result;
            return clear;
        }

        /// <summary>Whether walking straight from one point to another stays on the navmesh.</summary>
        public static bool Raycast(Vector3 from, Vector3 to) => Raycast(from, to, out _);

        /// <summary>The navmesh's nearest point to a position, within maxDistance meters across (and an agent's height up and down).</summary>
        public static bool NearestPoint(Vector3 position, float maxDistance, out Vector3 point)
        {
            Vector3 result;
            bool found = Interop.Game->Navigation_NearestPoint(&position, maxDistance, &result) != 0;
            point = found ? result : position;
            return found;
        }

        /// <summary>
        /// A random point on the navmesh within radius meters of center (across), which one can walk to from there:
        /// patrols, searching. False when center isn't near the navmesh.
        /// </summary>
        public static bool RandomPointAround(Vector3 center, float radius, out Vector3 point)
        {
            Vector3 result;
            bool found = Interop.Game->Navigation_RandomPointAround(&center, radius, &result) != 0;
            point = found ? result : center;
            return found;
        }
    }

    /// <summary>
    /// A character that walks by itself (Inspector: Nav Agent, beside Character): to where SetDestination sends it, along
    /// the scene's baked navmesh and around the other characters. Each fixed step it gives its character the velocity to
    /// walk with; physics decides where it gets. It doesn't turn the entity: a script turns it to Velocity, and gives an
    /// animator its speed. Needs a baked navmesh (Navigation.IsReady).
    /// </summary>
    public sealed unsafe class NavAgent : Component
    {
        NativeNavAgentState State
        {
            get
            {
                NativeNavAgentState state;
                if (Interop.Game->NavAgent_GetState(Entity.Id, &state) == 0) throw Missing("nav agent on a baked navmesh");
                return state;
            }
        }

        /// <summary>
        /// Walks to the navmesh's point nearest destination (within a few meters); one it can't reach: as near as the
        /// navmesh gets (IsPathPartial). False when nothing on the navmesh is near it: it goes on as before. Calling it
        /// again with a moving destination (someone it follows) is fine, a few times a second.
        /// </summary>
        public bool SetDestination(Vector3 destination)
        {
            int result = Interop.Game->NavAgent_SetDestination(Entity.Id, &destination);
            if (result == 0) throw Missing("nav agent on a baked navmesh");
            return result > 0;
        }

        /// <summary>Stops where it is (slowing down as fast as its Acceleration lets it), without a destination.</summary>
        public void Stop()
        {
            if (Interop.Game->NavAgent_Stop(Entity.Id) == 0) throw Missing("nav agent on a baked navmesh");
        }

        /// <summary>Where it walks to (on the navmesh), or where it last walked to.</summary>
        public Vector3 Destination => State.Destination;

        /// <summary>It walks somewhere: its path found, or being found (false: no destination, stopped, or arrived).</summary>
        public bool HasPath => State.HasDestination != 0;

        /// <summary>Its path is still being found (many agents asking at once: a few steps).</summary>
        public bool IsPathPending => State.PathPending != 0;

        /// <summary>The destination can't be reached: it walks (or has walked) to the navmesh's point nearest it.</summary>
        public bool IsPathPartial => State.PathPartial != 0;

        /// <summary>It got to its destination, within its StoppingDistance (or to a partial path's end), and stands there.</summary>
        public bool HasArrived => State.Arrived != 0;

        /// <summary>
        /// It should walk, but hasn't got on for a second: something the navmesh doesn't know is in the way (a closed
        /// door, a box), or the others are.
        /// </summary>
        public bool IsStuck => State.Stuck != 0;

        /// <summary>It stands on the navmesh (false: pushed or put off it, or falling).</summary>
        public bool IsOnNavMesh => State.OnNavMesh != 0;

        /// <summary>How far it still has to walk along its path, in meters (0: arrived, or no destination).</summary>
        public float RemainingDistance => State.RemainingDistance;

        /// <summary>
        /// The velocity it asks its character for, across (meters per second in the world; zero when it stands): turn the
        /// model to it, give its length to the animator.
        /// </summary>
        public Vector3 Velocity => State.Velocity;

        /// <summary>The velocity along its path, before it steers around the others.</summary>
        public Vector3 DesiredVelocity => State.DesiredVelocity;

        /// <summary>
        /// The engine carries it along a Nav Link of its path (up or down a ladder, in a jump, across a gap): play the
        /// animation of LinkType. Velocity is how it is carried then (up a ladder: up).
        /// </summary>
        public bool IsOnLink => State.OnLink != 0;

        /// <summary>The kind of the Nav Link it is on (IsOnLink), or null.</summary>
        public NavLinkType? LinkType { get { NativeNavAgentState s = State; return s.OnLink != 0 ? (NavLinkType)s.LinkType : null; } }

        /// <summary>The Nav Link's entity it is on (IsOnLink), or null.</summary>
        public Entity? Link { get { NativeNavAgentState s = State; return s.OnLink != 0 && s.LinkEntity != 0 ? new Entity(s.LinkEntity) : null; } }

        /// <summary>Meters per second at most (Inspector: Speed): a walk, a run.</summary>
        public float Speed
        {
            get => State.Speed;
            set { NativeNavAgentState s = State; Set(value, s.Acceleration, s.StoppingDistance); }
        }

        /// <summary>Meters per second squared: how fast it gets to its speed, stops and turns.</summary>
        public float Acceleration
        {
            get => State.Acceleration;
            set { NativeNavAgentState s = State; Set(s.Speed, value, s.StoppingDistance); }
        }

        /// <summary>Meters: it has arrived this near its destination.</summary>
        public float StoppingDistance
        {
            get => State.StoppingDistance;
            set { NativeNavAgentState s = State; Set(s.Speed, s.Acceleration, value); }
        }

        void Set(float speed, float acceleration, float stoppingDistance)
        {
            if (Interop.Game->NavAgent_SetSettings(Entity.Id, speed, acceleration, stoppingDistance) == 0) throw Missing("nav agent on a baked navmesh");
        }

        /// <summary>The path it walks, from its feet to the destination; null when it has none (arrived, stopped).</summary>
        public NavPath? Path
        {
            get
            {
                const int Room = 64;
                Vector3* corners = stackalloc Vector3[Room];
                int complete;
                int count = Interop.Game->NavAgent_GetPath(Entity.Id, corners, Room, &complete);
                if (count < 0) throw Missing("nav agent on a baked navmesh");
                if (count == 0)
                    return null;
                Vector3[] result = new Vector3[count];
                if (count <= Room)
                {
                    for (int i = 0; i < count; i++)
                        result[i] = corners[i];
                }
                else
                {
                    fixed (Vector3* buffer = result)
                        count = Interop.Game->NavAgent_GetPath(Entity.Id, buffer, result.Length, &complete);
                    if (count <= 0)
                        return null;
                    if (count < result.Length)
                        Array.Resize(ref result, count);
                }
                return new NavPath(result, complete != 0);
            }
        }
    }

    /// <summary>A Nav Link's kind: how characters are carried along it.</summary>
    public enum NavLinkType
    {
        /// <summary>Straight up and onto the top, or over the edge and straight down.</summary>
        Ladder = 0,
        /// <summary>In an arc from one end to the other.</summary>
        Jump = 1,
        /// <summary>In a line (a gap to step over).</summary>
        Walk = 2
    }

    /// <summary>A Nav Area's kind.</summary>
    public enum NavAreaType
    {
        /// <summary>Its polygons close and open while the game runs (NavArea.IsOpen).</summary>
        Door = 0,
        /// <summary>Paths go around it when that costs less (NavArea.Cost).</summary>
        Costly = 1,
        /// <summary>Not on the navmesh at all.</summary>
        Blocked = 2
    }

    /// <summary>
    /// A box of the navmesh with rules of its own (Inspector: Nav Area): a Door whose polygons close (a locked door), a
    /// Costly area paths go around when they can, a Blocked one off the navmesh. A change goes into the navmesh at once,
    /// no new bake: Navigation's queries after it see it, and agents walking through a closed door find another way.
    /// </summary>
    public sealed unsafe class NavArea : Component
    {
        void Get(out int type, out int open, out float cost)
        {
            int t, o;
            float c;
            if (Interop.Game->NavArea_Get(Entity.Id, &t, &o, &c) == 0) throw Missing("nav area");
            type = t;
            open = o;
            cost = c;
        }

        /// <summary>Door, Costly or Blocked (the bake's: a Blocked area has no polygons to open).</summary>
        public NavAreaType Type { get { Get(out int type, out _, out _); return (NavAreaType)type; } }

        /// <summary>A Door: walkable (false: closed, paths go around it or stop before it).</summary>
        public bool IsOpen
        {
            get { Get(out _, out int open, out _); return open != 0; }
            set { Get(out _, out _, out float cost); if (Interop.Game->NavArea_Set(Entity.Id, value ? 1 : 0, cost) == 0) throw Missing("nav area"); }
        }

        /// <summary>Walking a meter in it costs this many meters of ground (1: as the ground).</summary>
        public float Cost
        {
            get { Get(out _, out _, out float cost); return cost; }
            set { Get(out _, out int open, out _); if (Interop.Game->NavArea_Set(Entity.Id, open, value) == 0) throw Missing("nav area"); }
        }
    }

    /// <summary>
    /// A way between two places on the navmesh the ground doesn't give (Inspector: Nav Link): a ladder, a jump, a gap.
    /// Agents take it as part of their paths (NavAgent.IsOnLink while carried along it). Enabled off: paths don't take it
    /// (a raised ladder), at once, no new bake.
    /// </summary>
    public sealed unsafe class NavLink : Component
    {
        /// <summary>Ladder, Jump or Walk.</summary>
        public NavLinkType Type
        {
            get { int type, enabled; if (Interop.Game->NavLink_Get(Entity.Id, &type, &enabled) == 0) throw Missing("nav link"); return (NavLinkType)type; }
        }

        /// <summary>Paths may take it.</summary>
        public bool Enabled
        {
            get { int type, enabled; if (Interop.Game->NavLink_Get(Entity.Id, &type, &enabled) == 0) throw Missing("nav link"); return enabled != 0; }
            set { if (Interop.Game->NavLink_SetEnabled(Entity.Id, value ? 1 : 0) == 0) throw Missing("nav link"); }
        }
    }
}
