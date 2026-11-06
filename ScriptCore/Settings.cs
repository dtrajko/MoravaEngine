namespace Morava
{
    /// <summary>
    /// What the game remembers between its runs of the player's choices that the engine doesn't keep itself (the mouse's
    /// sensitivity): values by key ("MouseSensitivity"). The engine keeps its own: Application.Fullscreen,
    /// Application.Resolution, Application.VSync, Quality.Level, the volumes (Audio.SetBusVolume) and the bindings
    /// (Input.Rebind). They are kept while scenes load and saved when the game ends (or at once: Save). The player keeps them
    /// in the user's folder (Windows: %APPDATA%\&lt;Company&gt;\&lt;Product&gt;\Settings.yaml), the editor's Play in the
    /// project's Library folder, so playing in the editor leaves an installed game's settings alone. The game's progress
    /// goes in SaveData.
    /// </summary>
    public static unsafe class Settings
    {
        /// <summary>The value stored for key, or fallback when there is none (or it isn't a number).</summary>
        public static float GetFloat(string key, float fallback = 0.0f) =>
            TryGet(key, out string text) && StoredValues.TryGetFloat(text, out float value) ? value : fallback;

        /// <summary>Stores a value for key (in the file at the next save).</summary>
        public static void SetFloat(string key, float value) => SetString(key, StoredValues.FromFloat(value));

        /// <summary>The value stored for key, or fallback when there is none (or it isn't a whole number).</summary>
        public static int GetInt(string key, int fallback = 0) =>
            TryGet(key, out string text) && StoredValues.TryGetInt(text, out int value) ? value : fallback;

        /// <summary>Stores a whole number for key.</summary>
        public static void SetInt(string key, int value) => SetString(key, StoredValues.FromInt(value));

        /// <summary>The value stored for key, or fallback when there is none (or it isn't true or false).</summary>
        public static bool GetBool(string key, bool fallback = false) =>
            TryGet(key, out string text) && bool.TryParse(text, out bool value) ? value : fallback;

        /// <summary>Stores true or false for key.</summary>
        public static void SetBool(string key, bool value) => SetString(key, value ? "true" : "false");

        /// <summary>The text stored for key, or fallback when there is none.</summary>
        public static string GetString(string key, string fallback = "") => TryGet(key, out string text) ? text : fallback;

        /// <summary>Stores a text for key.</summary>
        public static void SetString(string key, string value) => StoredValues.Set(Interop.Game->Settings_Set, key, value);

        /// <summary>A value is stored for key.</summary>
        public static bool Has(string key) => TryGet(key, out _);

        /// <summary>Forgets key's value: false when there was none.</summary>
        public static bool Remove(string key) => StoredValues.Remove(Interop.Game->Settings_Remove, key);

        /// <summary>
        /// Writes the values (and the engine's: the player's choices) to the file now (they are also written when the game
        /// ends): false when it can't (logged).
        /// </summary>
        public static bool Save() => Interop.Game->Settings_Save() != 0;

        /// <summary>The file the values are kept in (empty: for this run only, the user's folder isn't known).</summary>
        public static string File => StoredValues.GetFile(Interop.Game->Settings_GetFile);

        static bool TryGet(string key, out string value) => StoredValues.TryGet(Interop.Game->Settings_Get, key, out value);
    }
}
