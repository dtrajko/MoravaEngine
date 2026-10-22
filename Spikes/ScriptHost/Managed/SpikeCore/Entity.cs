using System.Numerics;

namespace Morava.Spike
{
    // The base class of game scripts: the engine's entity it belongs to, and its transform through C++
    public abstract unsafe class Entity
    {
        public ulong Id { get; internal set; }

        public Vector3 Translation
        {
            get { Vector3 value; Interop.Native.GetTranslation(Id, &value); return value; }
            set => Interop.Native.SetTranslation(Id, &value);
        }

        protected internal virtual void OnCreate() { }
        protected internal virtual void OnUpdate(float dt) { }
    }

    // Something of the engine's that lives on: a game object kept here holds the game's context alive (the leak test)
    public static class Registry
    {
        public static readonly System.Collections.Generic.List<object> Kept = new();
    }

    public static unsafe class Log
    {
        public static void Info(string text) => Write(0, text);
        public static void Error(string text) => Write(2, text);

        static void Write(int level, string text)
        {
            fixed (char* chars = text)
                Interop.Native.Log(level, chars, text.Length);
        }
    }
}
