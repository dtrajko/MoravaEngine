using System;
using System.Numerics;

namespace Morava
{
    /// <summary>The kinds of bodies a physics query finds; combine them with |.</summary>
    [Flags]
    public enum PhysicsLayers : uint
    {
        /// <summary>Bodies that don't move: floors, walls.</summary>
        Static = 1,
        /// <summary>Bodies that move: dynamic and kinematic ones.</summary>
        Dynamic = 2,
        /// <summary>Character controllers.</summary>
        Character = 4,
        /// <summary>Triggers.</summary>
        Trigger = 8,
        /// <summary>Everything but triggers.</summary>
        Solid = Static | Dynamic | Character,
        /// <summary>Everything.</summary>
        All = Solid | Trigger,
    }

    /// <summary>What a ray or a moving sphere hit (Physics.Raycast, Physics.SphereCast).</summary>
    public readonly struct RaycastHit
    {
        internal RaycastHit(Entity entity, Vector3 point, Vector3 normal, float distance)
        {
            Entity = entity;
            Point = point;
            Normal = normal;
            Distance = distance;
        }

        /// <summary>The entity hit.</summary>
        public Entity Entity { get; }

        /// <summary>Where, in the world.</summary>
        public Vector3 Point { get; }

        /// <summary>The surface's direction there, towards where the ray came from, of length 1.</summary>
        public Vector3 Normal { get; }

        /// <summary>How far along the ray (a sphere: how far its center went), in meters.</summary>
        public float Distance { get; }
    }

    /// <summary>Questions to the running game's physics: what is where, after the last fixed step.</summary>
    public static unsafe class Physics
    {
        /// <summary>
        /// The first body a ray from origin along direction hits within maxDistance meters. ignore: an entity the ray goes
        /// through (the one asking, so a ray from inside it doesn't hit it).
        /// </summary>
        public static bool Raycast(Vector3 origin, Vector3 direction, float maxDistance, out RaycastHit hit,
            PhysicsLayers layers = PhysicsLayers.Solid, Entity? ignore = null)
        {
            NativeRaycastHit result;
            bool found = Interop.Game->Physics_Raycast(&origin, &direction, maxDistance, (uint)layers, ignore?.Id ?? 0, &result) != 0;
            hit = found ? ToHit(result) : default;
            return found;
        }

        /// <summary>The first body a sphere of this radius touches when it moves from origin along direction.</summary>
        public static bool SphereCast(Vector3 origin, float radius, Vector3 direction, float maxDistance, out RaycastHit hit,
            PhysicsLayers layers = PhysicsLayers.Solid, Entity? ignore = null)
        {
            NativeRaycastHit result;
            bool found = Interop.Game->Physics_SphereCast(&origin, radius, &direction, maxDistance, (uint)layers, ignore?.Id ?? 0, &result) != 0;
            hit = found ? ToHit(result) : default;
            return found;
        }

        /// <summary>The entities whose bodies are (partly) inside the sphere, each once.</summary>
        public static Entity[] OverlapSphere(Vector3 center, float radius, PhysicsLayers layers = PhysicsLayers.Solid)
        {
            ulong* ids = stackalloc ulong[64];
            int count = Interop.Game->Physics_OverlapSphere(&center, radius, (uint)layers, ids, 64);
            if (count <= 64)
                return Scene.ToEntities(ids, count);
            ulong[] large = new ulong[count];
            fixed (ulong* buffer = large)
                return Scene.ToEntities(buffer, Math.Min(count, Interop.Game->Physics_OverlapSphere(&center, radius, (uint)layers, buffer, large.Length)));
        }

        /// <summary>The entities whose bodies are (partly) inside the box (half its size along each axis, turned by rotation).</summary>
        public static Entity[] OverlapBox(Vector3 center, Vector3 halfExtents, Quaternion rotation, PhysicsLayers layers = PhysicsLayers.Solid)
        {
            ulong* ids = stackalloc ulong[64];
            int count = Interop.Game->Physics_OverlapBox(&center, &halfExtents, &rotation, (uint)layers, ids, 64);
            if (count <= 64)
                return Scene.ToEntities(ids, count);
            ulong[] large = new ulong[count];
            fixed (ulong* buffer = large)
                return Scene.ToEntities(buffer, Math.Min(count, Interop.Game->Physics_OverlapBox(&center, &halfExtents, &rotation, (uint)layers, buffer, large.Length)));
        }

        static RaycastHit ToHit(NativeRaycastHit hit) => new RaycastHit(new Entity(hit.Entity), hit.Point, hit.Normal, hit.Distance);
    }
}
