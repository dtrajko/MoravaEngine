using System;
using System.Globalization;

namespace Morava
{
    // What Settings and SaveData share: text values by key in a file of the engine's (GameSettings), and the typed values as
    // text (invariant culture: a file written on one machine reads the same on another)
    static unsafe class StoredValues
    {
        public static bool TryGetFloat(string text, out float value) =>
            float.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out value);

        public static string FromFloat(float value) => value.ToString("R", CultureInfo.InvariantCulture);

        public static bool TryGetInt(string text, out int value) =>
            int.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out value);

        public static string FromInt(int value) => value.ToString(CultureInfo.InvariantCulture);

        // The value stored for key: get is Settings_Get or SaveData_Get (the length, -1: doesn't fit, -2: no such key)
        public static bool TryGet(delegate* unmanaged<char*, int, char*, int, int> get, string key, out string value)
        {
            CheckKey(key);
            char* small = stackalloc char[256];
            int length;
            fixed (char* k = key)
            {
                length = get(k, key.Length, small, 256);
                for (int capacity = 4096; length == -1 && capacity <= 1 << 24; capacity *= 8)
                {
                    char[] large = new char[capacity];
                    fixed (char* chars = large)
                        length = get(k, key.Length, chars, capacity);
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

        public static void Set(delegate* unmanaged<char*, int, char*, int, void> set, string key, string value)
        {
            CheckKey(key);
            value ??= "";
            fixed (char* k = key)
            fixed (char* v = value)
                set(k, key.Length, v, value.Length);
        }

        public static bool Remove(delegate* unmanaged<char*, int, int> remove, string key)
        {
            CheckKey(key);
            fixed (char* k = key)
                return remove(k, key.Length) != 0;
        }

        public static string GetFile(delegate* unmanaged<char*, int, int> getFile)
        {
            char* buffer = stackalloc char[1024];
            int length = getFile(buffer, 1024);
            return length >= 0 ? new string(buffer, 0, length) : "";
        }

        static void CheckKey(string key)
        {
            if (string.IsNullOrEmpty(key)) throw new ArgumentException("A value needs a key (\"MusicVolume\")", nameof(key));
        }
    }
}
