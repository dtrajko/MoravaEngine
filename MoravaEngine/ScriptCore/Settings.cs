using System;
using System.Globalization;

namespace Morava
{
    /// <summary>
    /// What the game remembers between its runs: the player's choices in a Settings menu (volumes, the mouse's
    /// sensitivity), values by key ("MusicVolume"). They are kept while scenes load and saved when the game ends (or at
    /// once: Save). The player keeps them in the user's folder (Windows: %APPDATA%\&lt;project&gt;\Settings.yaml), the
    /// editor's Play in the project's Library folder, so playing in the editor leaves an installed game's settings
    /// alone. Not for save games (a later phase): a few small values.
    /// </summary>
    public static unsafe class Settings
    {
        /// <summary>The value stored for key, or fallback when there is none (or it isn't a number).</summary>
        public static float GetFloat(string key, float fallback = 0.0f) =>
            TryGet(key, out string text) && float.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out float value) ? value : fallback;

        /// <summary>Stores a value for key (in the file at the next save).</summary>
        public static void SetFloat(string key, float value) => SetString(key, value.ToString("R", CultureInfo.InvariantCulture));

        /// <summary>The value stored for key, or fallback when there is none (or it isn't a whole number).</summary>
        public static int GetInt(string key, int fallback = 0) =>
            TryGet(key, out string text) && int.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out int value) ? value : fallback;

        /// <summary>Stores a whole number for key.</summary>
        public static void SetInt(string key, int value) => SetString(key, value.ToString(CultureInfo.InvariantCulture));

        /// <summary>The value stored for key, or fallback when there is none (or it isn't true or false).</summary>
        public static bool GetBool(string key, bool fallback = false) =>
            TryGet(key, out string text) && bool.TryParse(text, out bool value) ? value : fallback;

        /// <summary>Stores true or false for key.</summary>
        public static void SetBool(string key, bool value) => SetString(key, value ? "true" : "false");

        /// <summary>The text stored for key, or fallback when there is none.</summary>
        public static string GetString(string key, string fallback = "") => TryGet(key, out string text) ? text : fallback;

        /// <summary>Stores a text for key.</summary>
        public static void SetString(string key, string value)
        {
            CheckKey(key);
            value ??= "";
            fixed (char* k = key)
            fixed (char* v = value)
                Interop.Game->Settings_Set(k, key.Length, v, value.Length);
        }

        /// <summary>A value is stored for key.</summary>
        public static bool Has(string key) => TryGet(key, out _);

        /// <summary>Forgets key's value: false when there was none.</summary>
        public static bool Remove(string key)
        {
            CheckKey(key);
            fixed (char* k = key)
                return Interop.Game->Settings_Remove(k, key.Length) != 0;
        }

        /// <summary>Writes the values to the file now (they are also written when the game ends): false when it can't (logged).</summary>
        public static bool Save() => Interop.Game->Settings_Save() != 0;

        /// <summary>The file the values are kept in (empty: for this run only, the user's folder isn't known).</summary>
        public static string File
        {
            get
            {
                char* buffer = stackalloc char[1024];
                int length = Interop.Game->Settings_GetFile(buffer, 1024);
                return length >= 0 ? new string(buffer, 0, length) : "";
            }
        }

        static bool TryGet(string key, out string value)
        {
            CheckKey(key);
            char* small = stackalloc char[256];
            int length;
            fixed (char* k = key)
            {
                length = Interop.Game->Settings_Get(k, key.Length, small, 256);
                for (int capacity = 4096; length == -1 && capacity <= 1 << 24; capacity *= 8)
                {
                    char[] large = new char[capacity];
                    fixed (char* chars = large)
                        length = Interop.Game->Settings_Get(k, key.Length, chars, capacity);
                    if (length >= 0)
                    {
                        value = new string(large, 0, length);
                        return true;
                    }
                }
            }
            value = length >= 0 ? new string(small, 0, length) : "";
            return length >= 0;
        }

        static void CheckKey(string key)
        {
            if (string.IsNullOrEmpty(key)) throw new ArgumentException("A setting needs a key (\"MusicVolume\")", nameof(key));
        }
    }
}
