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
}
