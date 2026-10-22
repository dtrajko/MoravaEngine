# Script host spike (Phase 4.1)

A console program that proves the risky parts of C# scripting before any engine code, as a shipped game will run them:

1. find hostfxr in a private .NET folder next to the executable (`dotnet/`), never an installed .NET;
2. start the runtime from `SpikeCore.runtimeconfig.json`, load `SpikeCore.dll` into the default context;
3. exchange two function tables (`NativeApi`: C# calls C++; `ManagedApi`: C++ calls C#, `[UnmanagedCallersOnly]`);
4. load a game assembly from bytes into a collectible `AssemblyLoadContext`, run its scripts, catch a script's exception
   at the boundary;
5. overwrite the loaded game's file with a new build, unload, check the context was collected, load the new build;
6. see a leak: a game object kept in an engine list keeps the context alive, and unloading reports it.

Each step prints PASS or FAIL; the exit code is 0 when all passed. CI runs it on Windows, Linux and macOS
(`.github/workflows/script-host-spike.yml`).

## Build and run

Needs the .NET 10 SDK (its host pack gives `nethost` and the hosting headers) and CMake.

```
cmake -S Spikes/ScriptHost -B Spikes/ScriptHost/build -DCMAKE_BUILD_TYPE=Release
cmake --build Spikes/ScriptHost/build --config Release
Spikes/ScriptHost/build/out/ScriptHostSpike
```

`build/out` is laid out as a shipped game: the executable, `nethost.dll` (Windows), `SpikeCore.dll` and its
runtimeconfig, `game/` (the loaded game), `game_v1/` and `game_v2/` (two builds of `SpikeGame`), and `dotnet/` with
`host/fxr/<version>` and `shared/Microsoft.NETCore.App/<version>` copied from the installed SDK (77 MB).
