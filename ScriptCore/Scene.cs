using System;
using System.Numerics;

namespace Morava
{
    /// <summary>The running game's scene: its entities, placing prefabs, and loading another scene.</summary>
    public static unsafe class Scene
    {
        /// <summary>The scene's name, as its file has it.</summary>
        public static string Name => GetText(Interop.Game->Scene_GetName);

        /// <summary>The scene's file, relative to the project ("Scenes/RollABall.mscene"; empty: a scene the editor never saved).</summary>
        public static string Path => GetText(Interop.Game->Scene_GetPath);

        /// <summary>
        /// Places a prefab at a position (in its parent's space; without one, the world's), turned as the prefab is. The
        /// entities are there at once: their scripts' OnCreate has run, their Rigidbodies and sounds work, and the result is
        /// the root (its script, when it has one: <c>(Sparkle)Scene.Instantiate(...)</c>). Their models are loaded from their
        /// files, which takes a moment: place what a game needs often before it is needed, or not too many at once.
        /// </summary>
        public static Entity Instantiate(Prefab prefab, Vector3 position) => Instantiate(prefab, position, Quaternion.Identity, null);

        /// <summary>Places a prefab at a position, turned by rotation.</summary>
        public static Entity Instantiate(Prefab prefab, Vector3 position, Quaternion rotation) => Instantiate(prefab, position, rotation, null);

        /// <summary>Places a prefab as a child of parent (null: at the top), at a position and rotation in the parent's space.</summary>
        public static Entity Instantiate(Prefab prefab, Vector3 position, Quaternion rotation, Entity? parent)
        {
            if (prefab is null) throw new ArgumentNullException(nameof(prefab), "Scene.Instantiate needs a prefab (is the script's Prefab field set in the Inspector?)");
            ulong id;
            fixed (char* chars = prefab.Path)
                id = Interop.Game->Scene_Instantiate(chars, prefab.Path.Length, &position, &rotation, parent?.Id ?? 0);
            if (id == 0)
                throw new ArgumentException($"The prefab '{prefab.Path}' could not be placed (the log says why)", nameof(prefab));
            return Interop.EntityFor(id)!;
        }

        /// <summary>
        /// Loads another scene, at the end of this frame: the game goes on in it. Every entity of this scene is gone then,
        /// with its script, its body and its sound; the new scene's scripts start as at the game's start (OnCreate), and
        /// Time goes on. scene is the file, relative to the project ("Scenes/Level2.mscene"; the extension may be left out).
        /// In the editor, Stop brings back the scene that was played.
        /// </summary>
        public static void Load(string scene)
        {
            if (string.IsNullOrEmpty(scene)) throw new ArgumentException("Scene.Load needs the scene's file", nameof(scene));
            int result;
            fixed (char* chars = scene) result = Interop.Game->Scene_Load(chars, scene.Length);
            if (result == 0)
                throw new ArgumentException($"There is no scene '{scene}' in the project (a file relative to the project folder, " +
                    "such as \"Scenes/Level2.mscene\")", nameof(scene));
        }

        /// <summary>Starts this scene again, at the end of this frame, as it was when the game started (see Load).</summary>
        public static void Reload() => Interop.Game->Scene_Load(null, 0);

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

        static string GetText(delegate* unmanaged<char*, int, int> get)
        {
            char* buffer = stackalloc char[512];
            int length = get(buffer, 512);
            if (length >= 0) return new string(buffer, 0, length);
            char[] large = new char[8192];
            fixed (char* chars = large) length = get(chars, large.Length);
            return length >= 0 ? new string(large, 0, length) : "";
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
