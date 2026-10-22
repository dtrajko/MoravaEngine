using System;
using System.Collections.Generic;
using System.IO;
using System.Numerics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Morava.Spike
{
    // What C# may call in C++: plain C functions, blittable arguments only. Same order as NativeApi in ScriptHostSpike.cpp.
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct NativeApi
    {
        public delegate* unmanaged<int, char*, int, void> Log;
        public delegate* unmanaged<ulong, Vector3*, void> GetTranslation;
        public delegate* unmanaged<ulong, Vector3*, void> SetTranslation;
    }

    // What C++ calls in C#; Initialize fills it in. Same order as ManagedApi in ScriptHostSpike.cpp.
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ManagedApi
    {
        public delegate* unmanaged<char*, int, int> GetRuntimeDirectory;
        public delegate* unmanaged<char*, int, int> LoadGame;
        public delegate* unmanaged<int> UnloadGame;
        public delegate* unmanaged<char*, int, ulong, IntPtr> CreateInstance;
        public delegate* unmanaged<IntPtr, float, void> InvokeUpdate;
        public delegate* unmanaged<IntPtr, void> DestroyInstance;
    }

    // The game's assembly in a context of its own that can be unloaded. SpikeCore is the one already loaded, so the game's
    // classes derive from the same Entity type the engine knows; the rest falls back to the default context (null).
    sealed class GameLoadContext : AssemblyLoadContext
    {
        public GameLoadContext() : base("Game", isCollectible: true) { }

        protected override Assembly? Load(AssemblyName name)
        {
            Assembly core = typeof(Entity).Assembly;
            return name.Name == core.GetName().Name ? core : null;
        }
    }

    public static unsafe class Interop
    {
        internal static NativeApi Native;

        static GameLoadContext? s_Context;
        static Assembly? s_Game;
        static readonly HashSet<IntPtr> s_Instances = new();

        [UnmanagedCallersOnly]
        public static int Initialize(NativeApi* native, ManagedApi* managed)
        {
            Native = *native;
            managed->GetRuntimeDirectory = &GetRuntimeDirectory;
            managed->LoadGame = &LoadGame;
            managed->UnloadGame = &UnloadGame;
            managed->CreateInstance = &CreateInstance;
            managed->InvokeUpdate = &InvokeUpdate;
            managed->DestroyInstance = &DestroyInstance;
            Log.Info($".NET {Environment.Version}, SpikeCore loaded into '{AssemblyLoadContext.GetLoadContext(typeof(Interop).Assembly)?.Name}'");
            return 0;
        }

        // Where the base library came from: the host checks it is the private .NET folder, not an installed one
        [UnmanagedCallersOnly]
        static int GetRuntimeDirectory(char* buffer, int capacity)
        {
            string directory = Path.GetDirectoryName(typeof(object).Assembly.Location) ?? "";
            if (directory.Length > capacity) return -1;
            directory.AsSpan().CopyTo(new Span<char>(buffer, capacity));
            return directory.Length;
        }

        // From bytes, so the file isn't locked and a build can overwrite it while the game is loaded
        [UnmanagedCallersOnly]
        static int LoadGame(char* path, int length)
        {
            try
            {
                if (s_Context != null) { Log.Error("LoadGame: a game is already loaded"); return 1; }
                string file = new string(path, 0, length);
                string pdb = Path.ChangeExtension(file, ".pdb");
                s_Context = new GameLoadContext();
                using var assembly = new MemoryStream(File.ReadAllBytes(file));
                using var symbols = File.Exists(pdb) ? new MemoryStream(File.ReadAllBytes(pdb)) : null;
                s_Game = s_Context.LoadFromStream(assembly, symbols);
                Log.Info($"Game loaded: {s_Game.GetName().Name} from {file}");
                return 0;
            }
            catch (Exception e) { Log.Error($"LoadGame: {e}"); return 1; }
        }

        // 0: unloaded and collected; 1: still alive after the GC cycles (something holds the game); -1: refused
        [UnmanagedCallersOnly]
        static int UnloadGame()
        {
            try
            {
                if (s_Context == null) return 0;
                if (s_Instances.Count > 0) { Log.Error($"UnloadGame: {s_Instances.Count} instances still alive"); return -1; }
                WeakReference context = StartUnload();
                for (int i = 0; i < 10 && context.IsAlive; i++)
                {
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                }
                if (context.IsAlive) { Log.Error("UnloadGame: the game's context is still alive (a reference holds it)"); return 1; }
                Log.Info("Game unloaded and collected");
                return 0;
            }
            catch (Exception e) { Log.Error($"UnloadGame: {e}"); return 1; }
        }

        // Not inlined: no local of the caller may keep the context alive
        [MethodImpl(MethodImplOptions.NoInlining)]
        static WeakReference StartUnload()
        {
            var context = new WeakReference(s_Context);
            s_Context!.Unload();
            s_Context = null;
            s_Game = null;
            return context;
        }

        // Returns a GCHandle (IntPtr.Zero on failure); the type is looked up each time, never cached (it would hold the game)
        [UnmanagedCallersOnly]
        static IntPtr CreateInstance(char* className, int length, ulong entityId)
        {
            try
            {
                string name = new string(className, 0, length);
                Type? type = s_Game?.GetType(name);
                if (type == null || !type.IsSubclassOf(typeof(Entity))) { Log.Error($"CreateInstance: no Entity class '{name}'"); return IntPtr.Zero; }
                var entity = (Entity)Activator.CreateInstance(type)!;
                entity.Id = entityId;
                IntPtr handle = GCHandle.ToIntPtr(GCHandle.Alloc(entity));
                s_Instances.Add(handle);
                entity.OnCreate();
                return handle;
            }
            catch (Exception e) { Log.Error($"CreateInstance: {e}"); return IntPtr.Zero; }
        }

        // An exception must never leave an [UnmanagedCallersOnly] method: it would end the process
        [UnmanagedCallersOnly]
        static void InvokeUpdate(IntPtr instance, float dt)
        {
            var entity = (Entity)GCHandle.FromIntPtr(instance).Target!;
            try { entity.OnUpdate(dt); }
            catch (Exception e) { Log.Error($"{entity.GetType().Name}.OnUpdate: {e.GetType().Name}: {e.Message}"); }
        }

        [UnmanagedCallersOnly]
        static void DestroyInstance(IntPtr instance)
        {
            if (!s_Instances.Remove(instance)) return;
            GCHandle.FromIntPtr(instance).Free();
        }
    }
}
