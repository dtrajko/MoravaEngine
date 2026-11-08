using System;

namespace Morava
{
    /// <summary>
    /// The project's files, read by the game (the racing game, Phase 3): a data file the game reads itself, such as a track's
    /// center line. The same in the editor and in a built game: from the project's folder, or from the game's pack. The
    /// pack holds the project's Assets folder (and its scenes): keep such files there.
    /// </summary>
    public static unsafe class Assets
    {
        /// <summary>
        /// A text file (UTF-8) of the project, by its path in the project (as the project's files write them, with forward
        /// slashes: "Assets/Tracks/TestTrack.centerline.json"). Null when there is no such file.
        /// </summary>
        public static string? ReadText(string path)
        {
            if (string.IsNullOrEmpty(path))
                return null;
            fixed (char* name = path)
            {
                char* small = stackalloc char[1024];
                int length = Interop.Game->Asset_ReadText(name, path.Length, small, 1024);
                if (length == -2)
                    return null;
                if (length >= 0)
                    return new string(small, 0, length);
                for (int capacity = 1 << 16; capacity <= 1 << 28; capacity *= 8)
                {
                    char[] large = new char[capacity];
                    fixed (char* chars = large)
                        length = Interop.Game->Asset_ReadText(name, path.Length, chars, capacity);
                    if (length == -2)
                        return null;
                    if (length >= 0)
                        return new string(large, 0, length);
                }
                throw new InvalidOperationException($"The file '{path}' is too large to read as text");
            }
        }
    }
}
