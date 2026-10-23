using System;
using System.Collections.Generic;
using System.IO;
using System.Numerics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Morava
{
    // What C++ calls in C#; Interop.Initialize fills it in. Same order as ScriptManagedApi in src/Scripting/ScriptApi.h.
    [StructLayout(LayoutKind.Sequential)]
    unsafe struct ManagedApi
    {
        public delegate* unmanaged<char*, int, int> GetRuntimeVersion;
        public delegate* unmanaged<char*, int, int> GetRuntimeDirectory;
        public delegate* unmanaged<char*, int, int> LoadGameAssembly;
        public delegate* unmanaged<int> UnloadGameAssembly;
        public delegate* unmanaged<char*, int, ulong, IntPtr> CreateInstance;
        public delegate* unmanaged<IntPtr, void> DestroyInstance;
        public delegate* unmanaged<IntPtr, int> InvokeCreate;
        public delegate* unmanaged<IntPtr, float, int> InvokeUpdate;
        public delegate* unmanaged<IntPtr, float, int> InvokeFixedUpdate;
        public delegate* unmanaged<IntPtr, int, ulong, Vector3*, Vector3*, int> InvokeCollision;
    }

    // The game's assembly in a context of its own that can be unloaded (hot reload). ScriptCore is the one already
    // loaded, so the game's classes derive from the same Entity type the engine knows; the rest comes from the default
    // context (null).
    sealed class GameLoadContext : AssemblyLoadContext
    {
        public GameLoadContext() : base("Game", isCollectible: true) { }

        protected override Assembly? Load(AssemblyName name)
        {
            Assembly core = typeof(Entity).Assembly;
            return name.Name == core.GetName().Name ? core : null;
        }
    }

    /// <summary>The engine's entry points into C#. Called by the engine only, not by scripts.</summary>
    public static unsafe class Interop
    {
        // C++'s table, read through the pointer: the game's functions are set into it when a game starts
        internal static NativeApi* Native;

        // The table for the functions of a running game: an exception that says so outside one (a script's static
        // constructor, a thread of its own) instead of a call through a null pointer
        internal static NativeApi* Game
        {
            get
            {
                if (Native == null || Native->Time_Get == null)
                    throw new InvalidOperationException("The engine's API works only while the game runs (in a script's methods)");
                return Native;
            }
        }

        static GameLoadContext? s_Context;
        static Assembly? s_Game;
        static readonly HashSet<IntPtr> s_Instances = new();

        // The first call, from ScriptHost::Initialize: takes the C++ functions, gives the C# ones. 0 when done.
        [UnmanagedCallersOnly]
        static int Initialize(NativeApi* native, ManagedApi* managed)
        {
            try
            {
                Native = native;
                managed->GetRuntimeVersion = &GetRuntimeVersion;
                managed->GetRuntimeDirectory = &GetRuntimeDirectory;
                managed->LoadGameAssembly = &LoadGameAssembly;
                managed->UnloadGameAssembly = &UnloadGameAssembly;
                managed->CreateInstance = &CreateInstance;
                managed->DestroyInstance = &DestroyInstance;
                managed->InvokeCreate = &InvokeCreate;
                managed->InvokeUpdate = &InvokeUpdate;
                managed->InvokeFixedUpdate = &InvokeFixedUpdate;
                managed->InvokeCollision = &InvokeCollision;
                return 0;
            }
            catch (Exception) { return 1; }
        }

        // A string into a C++ buffer: its length, -1 when it doesn't fit
        static int CopyString(string text, char* buffer, int capacity)
        {
            if (text.Length > capacity) return -1;
            text.AsSpan().CopyTo(new Span<char>(buffer, capacity));
            return text.Length;
        }

        [UnmanagedCallersOnly]
        static int GetRuntimeVersion(char* buffer, int capacity) => CopyString(Environment.Version.ToString(), buffer, capacity);

        // Where the base library came from: the host checks it is the private .NET folder, not an installed one
        [UnmanagedCallersOnly]
        static int GetRuntimeDirectory(char* buffer, int capacity) =>
            CopyString(Path.GetDirectoryName(typeof(object).Assembly.Location) ?? "", buffer, capacity);

        // From bytes, so the file isn't locked and a build can overwrite it while the game is loaded. 0 when loaded.
        [UnmanagedCallersOnly]
        static int LoadGameAssembly(char* path, int length)
        {
            try
            {
                if (s_Context != null) { Log.Error("LoadGameAssembly: a game assembly is already loaded"); return 1; }
                string file = new string(path, 0, length);
                string pdb = Path.ChangeExtension(file, ".pdb");
                s_Context = new GameLoadContext();
                using var assembly = new MemoryStream(File.ReadAllBytes(file));
                using var symbols = File.Exists(pdb) ? new MemoryStream(File.ReadAllBytes(pdb)) : null;
                s_Game = s_Context.LoadFromStream(assembly, symbols);
                return 0;
            }
            catch (Exception e)
            {
                Log.Error($"LoadGameAssembly: {e.Message}");
                s_Context?.Unload();
                s_Context = null;
                s_Game = null;
                return 1;
            }
        }

        // 0: unloaded and collected; 1: still alive after the GC cycles (something holds the game: a leak); -1: refused,
        // instances still exist
        [UnmanagedCallersOnly]
        static int UnloadGameAssembly()
        {
            try
            {
                if (s_Context == null) return 0;
                if (s_Instances.Count > 0) { Log.Error($"UnloadGameAssembly: {s_Instances.Count} script instances still exist"); return -1; }
                WeakReference context = StartUnload();
                for (int i = 0; i < 10 && context.IsAlive; i++)
                {
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                }
                return context.IsAlive ? 1 : 0;
            }
            catch (Exception e) { Log.Error($"UnloadGameAssembly: {e}"); return 1; }
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

        // An instance of a script class of the game, for an entity: a GCHandle, IntPtr.Zero when there is no such class or
        // its constructor threw. OnCreate comes later (InvokeCreate): the engine sets the script's fields in between. The
        // type is looked up each time, never cached (it would hold the game's context alive).
        [UnmanagedCallersOnly]
        static IntPtr CreateInstance(char* className, int length, ulong entityId)
        {
            string name = new string(className, 0, length);
            try
            {
                Type? type = s_Game?.GetType(name);
                if (type == null || !type.IsSubclassOf(typeof(Entity)) || type.IsAbstract)
                {
                    Log.Error($"CreateInstance: the game has no script class '{name}' (a non-abstract class deriving from Entity)");
                    return IntPtr.Zero;
                }
                var entity = (Entity)Activator.CreateInstance(type)!;
                entity.Id = entityId;
                IntPtr handle = GCHandle.ToIntPtr(GCHandle.Alloc(entity));
                s_Instances.Add(handle);
                return handle;
            }
            catch (Exception e) { Log.Error($"{name}: {e.InnerException ?? e}"); return IntPtr.Zero; }
        }

        [UnmanagedCallersOnly]
        static void DestroyInstance(IntPtr instance)
        {
            if (!s_Instances.Remove(instance)) return;
            GCHandle.FromIntPtr(instance).Free();
        }

        // The script's methods; the collision kinds are ScriptCollisionKind's values (2 + kind)
        enum Method { OnCreate, OnUpdate, OnFixedUpdate, OnCollisionEnter, OnCollisionExit, OnTriggerEnter, OnTriggerExit }

        // An exception must never leave an [UnmanagedCallersOnly] method (it would end the process): it is logged here with
        // the script's class and where it was thrown, and the engine gets 1 instead of 0
        static int Invoke(IntPtr instance, Method method, float dt, ulong other = 0, Vector3 point = default, Vector3 normal = default)
        {
            Entity? entity = null;
            try
            {
                if (!s_Instances.Contains(instance)) { Log.Error($"{method}: not a script instance"); return 1; }
                entity = (Entity)GCHandle.FromIntPtr(instance).Target!;
                switch (method)
                {
                    case Method.OnCreate: entity.OnCreate(); break;
                    case Method.OnUpdate: entity.OnUpdate(dt); break;
                    case Method.OnFixedUpdate: entity.OnFixedUpdate(dt); break;
                    case Method.OnCollisionEnter: entity.OnCollisionEnter(new Collision(new Entity(other), point, normal)); break;
                    case Method.OnCollisionExit: entity.OnCollisionExit(new Entity(other)); break;
                    case Method.OnTriggerEnter: entity.OnTriggerEnter(new Entity(other)); break;
                    case Method.OnTriggerExit: entity.OnTriggerExit(new Entity(other)); break;
                }
                return 0;
            }
            catch (Exception e)
            {
                Log.Error($"{entity?.GetType().Name}.{method}: {e}");
                return 1;
            }
        }

        [UnmanagedCallersOnly]
        static int InvokeCreate(IntPtr instance) => Invoke(instance, Method.OnCreate, 0);

        [UnmanagedCallersOnly]
        static int InvokeUpdate(IntPtr instance, float dt) => Invoke(instance, Method.OnUpdate, dt);

        [UnmanagedCallersOnly]
        static int InvokeFixedUpdate(IntPtr instance, float dt) => Invoke(instance, Method.OnFixedUpdate, dt);

        [UnmanagedCallersOnly]
        static int InvokeCollision(IntPtr instance, int kind, ulong other, Vector3* point, Vector3* normal)
        {
            if (kind < 0 || kind > 3) { Log.Error($"InvokeCollision: no collision kind {kind}"); return 1; }
            return Invoke(instance, Method.OnCollisionEnter + kind, 0, other, *point, *normal);
        }
    }
}
