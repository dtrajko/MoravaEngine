# Testing tools

Scripts for checking the engine without sitting in front of it: run the editor or the player for a while, capture its
window, read what its log says, and compile files with GCC as the Linux CI build does. Logs and screenshots go to
`Tools/Testing/Output/`, which Git ignores.

| Script | What it does | Runs in |
|---|---|---|
| `Run-Engine.ps1` | Starts the editor or the player with a project or scene, closes it after some seconds, captures its window, summarizes the log | PowerShell |
| `Capture-Window.ps1` | Saves a running window as a PNG (all of it, or a part enlarged) | PowerShell |
| `gcc-check.sh` | Compiles source files with GCC for errors only | WSL (Ubuntu with `g++`) |

## Before you start

- **Build first.** The scripts run what is in `MoravaEngine/build/Debug` (`-Config Release` for a Release build):

  ```powershell
  cmake --build D:\Workspace\MoravaEngine\MoravaEngine\build --config Debug --parallel -- /nologo /v:minimal
  ```

  Close an open editor before building: Windows locks its executable and the link fails (`LNK1168`).
- **Use a windowed project.** The sample project (`Projects/Sample`) starts fullscreen and takes over the screen. Copy it
  to a test folder (for example `D:\Workspace\Temp\Sample`), set `Fullscreen: false` (and `VSync: false` to measure
  without waiting for the display) in its `.mproject`, and run that copy.
- **PowerShell scripts** are not signed. If PowerShell refuses to run them, start them with
  `powershell -ExecutionPolicy Bypass -File .\Run-Engine.ps1 ...`.

## Run-Engine.ps1

Starts the player (or with `-Editor` the editor), lets it run for `-Seconds` (the engine's `--exit-after`), waits until
it closes, and prints a summary:

- how many **Vulkan validation messages** the log has (lines with `VulkanDebugCallback`),
- **problems**: script errors and exceptions, failed self-tests, files that couldn't be opened or loaded,
- the game loop's counters, and in the editor whether Play's Stop gave the edited scene back unchanged.

The exit code is 0 when there were neither validation messages nor problems, 1 otherwise, so it can be used in other
scripts. The whole log is in `Output/MoravaPlayer.log` (or `MoravaEngine.log`).

| Parameter | Meaning |
|---|---|
| `-Editor` | The editor (`MoravaEngine.exe`); without it, the player (`MoravaPlayer.exe`) |
| `-Project <file>` | A `.mproject`: the player plays its start scene, the editor opens it |
| `-Scene <file>` | An `.mscene`: this scene instead of the project's start scene |
| `-Seconds <n>` | How long it runs after starting up (default 20) |
| `-Play`, `-Simulate` | The editor starts Play or Simulate once the scene is open |
| `-DebugOverlay` | Starts with the debug overlay (F3) shown |
| `-Maximize` | Maximizes the window when it appears |
| `-Capture <name.png>` | A screenshot of the window, in `Output/` (or at the path given) |
| `-CaptureAfter <s>` | When to take it, in seconds after the window appeared (default: 3 s before it closes) |
| `-Crop x,y,w,h`, `-Zoom <n>` | Keep only that part of the screenshot, enlarged (pixels stay sharp) |
| `-Config <name>` | `Debug` (default) or `Release` |
| `-Log <file>` | Where the log goes (default `Output/<exe>.log`) |
| `-Extra <options>` | More engine options, as written on its command line (see below) |

Relative paths are resolved from the folder you run the script in.

### Examples

From `Tools/Testing`:

```powershell
# The player on the sample copy's start scene, with the debug overlay, and a screenshot
.\Run-Engine.ps1 -Project D:\Workspace\Temp\Sample\Sample.mproject -DebugOverlay -Capture player.png

# Another scene of the project
.\Run-Engine.ps1 -Project D:\Workspace\Temp\Sample\Sample.mproject -Scene D:\Workspace\Temp\Sample\Scenes\Erika.mscene

# The editor maximized, the game running in Play for 40 seconds, then stopped: the summary says whether the edited
# scene came back unchanged
.\Run-Engine.ps1 -Editor -Project ..\..\Projects\Sample\Sample.mproject -Play -Maximize -Seconds 40 -Capture editor.png

# Only the debug overlay's top rows and graph, twice as big (the player in a 1280x720 window)
.\Run-Engine.ps1 -Project D:\Workspace\Temp\Sample\Sample.mproject -DebugOverlay -Capture graph.png -Crop 960,40,320,100 -Zoom 2

# As on a GPU without ray tracing, with a debug view (8: Probe Irradiance Only)
.\Run-Engine.ps1 -Editor -Scene ..\..\MoravaEngine\Scenes\Old_Stove_Corner.mscene -Extra "--no-ray-tracing", "--debug-view", "8"
```

### The engine's own options

What `-Extra` can pass (both executables read them; `src/Core/Application.cpp` has the full list):

| Option | Meaning |
|---|---|
| `--debug-view <n>` | Starts with entry n of the Environment panel's Debug View list (0 Off, 7 Diffuse Surface Shading, 8 Probe Irradiance Only, 10 Ambient Occlusion, 18 Path Traced) |
| `--no-ray-tracing` | Vulkan without the ray tracing extensions, as on a GPU that has none |
| `--select <name>` | Selects the entity of that name (to capture the selection outline or the gizmo) |
| `--save-scene` | The editor saves the open scene when it closes (a scene opened and saved unattended) |

## Capture-Window.ps1

Saves the window of a running process, even when other windows cover it. `Run-Engine.ps1` uses it; on its own it is
useful while you run the engine yourself:

```powershell
.\Capture-Window.ps1 -Name MoravaEngine -Out D:\Workspace\Temp\editor.png
.\Capture-Window.ps1 -Id 12345 -Out graph.png -Crop 972,86,302,46 -Zoom 4
```

`-Name` takes the newest process of that name that has a window; `-Id` a process by its ID. Coordinates for `-Crop`
are pixels of the whole window, title bar included: take one full screenshot first to find them.

## gcc-check.sh

The Windows build uses MSVC; Linux CI uses GCC, which rejects a few things MSVC accepts (comparing two different enum
types, a missing `#include` that MSVC's headers happened to bring in). This compiles files with GCC for errors only,
without linking, in a few seconds per file. Run it in WSL from the repository root, with paths relative to it:

```bash
wsl bash Tools/Testing/gcc-check.sh MoravaEngine/src/UI/DebugOverlay.cpp MoravaEngine/src/Player/PlayerLayer.cpp
```

Files are compiled as the Vulkan build compiles them. Code shared by every graphics API (`src/Core` and the like) is
also built for OpenGL and DirectX 11, where the Vulkan-only sources (`EnvMapVulkan/` and every path with "Vulkan", as
`CMakeLists.txt` sorts them) are left out. Such a file still compiles when it includes one of their headers, but the
build fails when it links: the functions behind the header aren't there. Guard those includes and calls with
`#if defined(GRAPHICS_API_VULKAN)`. With `--api opengl` (or `directx11`) the check compiles the files for that API and
also lists any Vulkan-only header they still include:

```bash
wsl bash Tools/Testing/gcc-check.sh --api opengl MoravaEngine/src/Core/ResourceManager.cpp
```

```
== src/Core/ResourceManager.cpp
Vulkan-only header included (not linked without Vulkan): src/EnvMapVulkan/EnvMapVulkanRenderTexture.h
```

From Git Bash, put `MSYS_NO_PATHCONV=1` in front (otherwise Git Bash rewrites the paths). Each file is printed, then
its errors, if any; the exit code is 1 when a file had errors. Warnings are left out. WSL needs `g++` (in Ubuntu:
`sudo apt install g++`); the headers come from the repository's `vendor` folder.

## What can't be automated

- **Keys and the mouse.** Windows lets a program send keys to another window only while it is in front, and refuses to
  bring it to the front while you use the computer. Test input yourself, or let a C# script in a test copy of the project
  drive the game (call the game's methods, check the results, `Log` a PASS or FAIL line): `Run-Engine.ps1` then shows
  any exception it throws, and its lines are in the log.
- **How something looks.** The scripts capture the screenshots; you look at them.
