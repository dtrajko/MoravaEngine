using System;

namespace Morava
{
    /// <summary>The running game's scene: its entities.</summary>
    public static unsafe class Scene
    {
        /// <summary>The first entity with this name (as the Scene Hierarchy shows it), null when there is none.</summary>
        public static Entity? FindEntityByName(string name)
        {
            ulong id;
            fixed (char* chars = name) id = Interop.Game->Scene_FindEntityByName(chars, name.Length);
            return Interop.EntityFor(id);
        }

        /// <summary>All entities with this tag (Inspector: Tags); none: an empty array.</summary>
        public static Entity[] FindEntitiesWithTag(string tag)
        {
            ulong* ids = stackalloc ulong[64];
            int count;
            fixed (char* chars = tag) count = Interop.Game->Scene_FindEntitiesWithTag(chars, tag.Length, ids, 64);
            if (count <= 64)
                return ToEntities(ids, count);
            ulong[] large = new ulong[count];
            fixed (char* chars = tag)
            fixed (ulong* buffer = large)
            {
                count = Math.Min(count, Interop.Game->Scene_FindEntitiesWithTag(chars, tag.Length, buffer, large.Length));
                return ToEntities(buffer, count);
            }
        }

        internal static Entity[] ToEntities(ulong* ids, int count)
        {
            var entities = new Entity[count];
            for (int i = 0; i < count; i++)
                entities[i] = Interop.EntityFor(ids[i])!;
            return entities;
        }
    }
}
