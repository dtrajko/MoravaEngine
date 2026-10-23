namespace Morava
{
    /// <summary>Messages in the engine's log (the editor's console and the log file).</summary>
    public static unsafe class Log
    {
        /// <summary>A message.</summary>
        public static void Info(string text) => Write(0, text);

        /// <summary>Something that may be wrong.</summary>
        public static void Warn(string text) => Write(1, text);

        /// <summary>Something that went wrong.</summary>
        public static void Error(string text) => Write(2, text);

        static void Write(int level, string text)
        {
            fixed (char* chars = text)
                Interop.Native.Log(level, chars, text.Length);
        }
    }
}
