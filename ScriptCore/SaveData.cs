namespace Morava
{
    /// <summary>
    /// The game's progress, kept between its runs: values by key (SaveData.SetInt("Level", 2), the best time, what was
    /// unlocked). As Settings, in a file of its own: the player keeps it in the user's folder (Windows:
    /// %APPDATA%\&lt;Company&gt;\&lt;Product&gt;\SaveData.yaml), the editor's Play in the project's Library folder. Saved when
    /// the game ends, or at once (Save: at a checkpoint, the end of a level). A save never leaves half a file: the old one
    /// stays until the new one is complete.
    /// </summary>
    public static unsafe class SaveData
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
        public static void SetString(string key, string value) => StoredValues.Set(Interop.Game->SaveData_Set, key, value);

        /// <summary>A value is stored for key.</summary>
        public static bool Has(string key) => TryGet(key, out _);

        /// <summary>Forgets key's value: false when there was none.</summary>
        public static bool Remove(string key) => StoredValues.Remove(Interop.Game->SaveData_Remove, key);

        /// <summary>Forgets every value: a new game (in the file at the next save).</summary>
        public static void DeleteAll() => Interop.Game->SaveData_RemoveAll();

        /// <summary>Writes the values to the file now (they are also written when the game ends): false when it can't (logged).</summary>
        public static bool Save() => Interop.Game->SaveData_Save() != 0;

        /// <summary>The file the values are kept in (empty: for this run only, the user's folder isn't known).</summary>
        public static string File => StoredValues.GetFile(Interop.Game->SaveData_GetFile);

        static bool TryGet(string key, out string value) => StoredValues.TryGet(Interop.Game->SaveData_Get, key, out value);
    }
}
