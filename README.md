# MoravaEngine

[![Build](https://github.com/dtrajko/MoravaEngine/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/dtrajko/MoravaEngine/actions/workflows/build.yml)

## About
2D/3D graphics engine written in C++.

It supports three graphics APIs:

- Vulkan 1.4 (ray tracing is used when the GPU has it)
- OpenGL
- DirectX 11 (Windows only)

It runs on Windows 10 and 11, and on Linux with Vulkan and OpenGL (built with GCC and Clang, tested on Ubuntu in WSL2). macOS support is in progress: the window, input and file dialogs are already platform independent.

The main scene is a Vulkan scene editor (`ENV_MAP_VULKAN`):

* Scene Hierarchy, Inspector, Content Browser, Material Library and Material Editor panels
* Scenes and materials saved to files (`.mscene`, `.mmat`)
* Physically based rendering (PBR) and image based lighting (IBL)
* Sun, point and spot lights with shadows
* Global illumination with a probe volume
* Ray-traced probe updates, ambient occlusion and reflections, with raster fallbacks
* A path tracer as a reference view
* Screen-space ambient occlusion (GTAO)
* Glass, water and cutout materials (alpha-tested: leaves, eyelashes)
* Bloom, auto exposure and tonemapping
* Skeletal animation
* Debug views for the passes above

The older scenes (OpenGL, and one DirectX 11 scene) experiment with single techniques:

* Phong lighting model
* Shadows (directional light, omni-directional shadows)
* Screen-space ambient occlusion (SSAO)
* Particle systems
* Instanced rendering
* Surface reflection and refraction
* Framebuffers and post-processing
* Level of detail
* Perlin noise generator
* Procedural landmass generation
* Voxel based volumes
* Marching cubes algorithm
* Physics simulations

The rendering design of the Vulkan editor is described in [docs/rendering/SSAO_GI_RayTracing_Guide.html](docs/rendering/SSAO_GI_RayTracing_Guide.html).

## Credits
* Yan Chernikov a.k.a. <a href="https://twitter.com/thecherno" target="_blank">TheCherno</a> [ <a href="https://github.com/TheCherno/Hazel" target="_blank">Hazel</a> ]
* <a href="https://twitter.com/ThinMatrix/" target="_blank">ThinMatrix</a> [ <a href="https://www.youtube.com/user/ThinMatrix" target="_blank">ThinMatrix YouTube channel</a> ]
* <a href="https://www.udemy.com/user/ben-cook-19/" target="_blank">Ben Cook</a> [ Vulkan and OpenGL courses on Udemy ]
* <a href="https://twitter.com/JoeyDeVriez" target="_blank">Joey de Vries</a> [ <a href="https://learnopengl.com/" target="_blank">learnopengl.com</a> ]
* <a href="https://twitter.com/SaschaWillems2" target="_blank">Sascha Willems</a> [ <a href="https://github.com/SaschaWillems/Vulkan" target="_blank">Vulkan C++ examples and demos</a> ]
* <a href="https://twitter.com/sebastianlague" target="_blank">Sebastian Lague</a> [ <a href="https://www.youtube.com/c/SebastianLague" target="_blank">Sebastian Lague YouTube channel</a> ]
* <a href="https://www.cgtrader.com/ilya73" target="_blank">ilya73</a> [ <a href="https://www.cgtrader.com/free-3d-models/character/man/gladiator-7ac625db-88c2-46ed-a2f1-182fd1f79739" target="_blank">Gladiator free VR</a> ]

## Building

The build is one CMake project, `CMakeLists.txt` in the repository's root. It builds the engine and the libraries it uses (GLFW, assimp, shaderc, SPIRV-Cross, Jolt Physics, miniaudio, yaml-cpp, ImGui and others) from source.

### Requirements

* Windows 10 or 11 with Visual Studio 2026 (C++17)
* <a href="https://cmake.org/download/" target="_blank">CMake</a> 3.15 or newer
* <a href="https://vulkan.lunarg.com/" target="_blank">Vulkan SDK</a> (CMake finds it through the `VULKAN_SDK` environment variable)
* <a href="https://dotnet.microsoft.com/download" target="_blank">.NET 10 SDK</a> (C# scripting: CMake finds it through `DOTNET_ROOT` or the `dotnet` command; `-DMORAVA_SCRIPTING=OFF` builds without it)
* Python 3 (used by the shaderc build)
* Git

On Linux: see "Building on Linux" below.

### Get the sources

```
git clone --recursive https://github.com/dtrajko/MoravaEngine.git
cd MoravaEngine
python vendor/cross-platform/shaderc/utils/git-sync-deps
```

The last command downloads the libraries shaderc itself depends on (glslang, SPIRV-Tools, SPIRV-Headers).

### Configure and build

From the repository's root folder:

```
cmake -S . -B build
cmake --build build --config Debug --parallel
```

This builds the default: the scene SceneEnvMapVulkan (the Vulkan editor) with the Vulkan API. Other scenes and graphics APIs: see "Select the scene and the graphics API" below.

The first build takes a while, because it also builds the libraries. `--config Release` builds the optimized version.

To work in Visual Studio instead, run only the first command and open `build/MoravaEngine.sln`. MoravaEngine is the startup project.

### Run

The engine loads its shaders, models and textures relative to the repository's root folder, so start it from there:

```
build\Debug\MoravaEngine.exe
```

Visual Studio is already set up to start it in that folder.

### Run a project with MoravaPlayer

The Vulkan build also makes `MoravaPlayer`, the game player: it plays a project's start scene in the project's window, without the editor. It is built next to the editor (`build\Debug\MoravaPlayer.exe`; `build-linux/MoravaPlayer` on Linux) and, like the editor, has to be started from the repository's root folder, where the engine's shaders are. The project's path is then relative to that folder:

```
build\Debug\MoravaPlayer.exe --project ..\Projects\Sample\Sample.mproject
```

In the player, Alt+Enter and F11 switch between fullscreen and a maximized window, Escape leaves fullscreen (unless the game uses Escape itself), and Alt+F4 closes the game.

Started from another folder, it stops at once with "Resources/Shaders not found in the working directory".

A project is a folder of its own, anywhere (`Projects/Sample` is the sample): a `.mproject` file names the project, its start scene, its asset folder and the game's window (size, full screen, VSync, title). Its scenes and materials are found in the project's folder first, then in the engine's (its models and environment maps). In the editor, File > New Project makes one in a chosen folder (with a first scene as its start scene), File > Open Project opens one, and File > Project Settings edits it; its *Scenes* list opens the project's scenes (double-click, or *Open*). `--scene <file>` instead of `--project` plays a single scene.

The game reads *actions*, not keys: a project's `Settings/Input.minput` names them and their bindings (keys, the mouse, gamepad buttons and sticks). Any connected gamepad works; GLFW knows the layout of most controllers (Xbox, PlayStation and many more). The sample's actions are Move (WASD, the arrow keys, the left stick), Look (the mouse, the right stick) and Jump (Space, A): its orange cube, Player, moves with them. In the editor, View > Input shows each action's live value and the connected gamepads; its *Try* option checks the bindings without Play. During Play the game has the input while the Viewport has the focus (Play gives it, a click on the view gives it back); Escape gives the mouse back to the editor.

Models collide and move with physics (Jolt) while the game runs. In the Inspector of a model, *Collider* gives it a shape (a box, sphere or capsule fitted around it, or its meshes: Convex Mesh for moving models, Triangle Mesh for static ones); a collider alone makes it static, like a floor or a wall. *Rigid Body* makes it move: Dynamic (gravity and collisions move it, with its mass, friction and bounciness) or Kinematic (the game moves it, and it pushes the dynamic ones). *Character* makes it a walking figure, as a player: a standing capsule the game moves by a velocity, which stops at walls and slides along them, walks up slopes and steps, slides down slopes too steep for it, stands on things and pushes dynamic models. The sample's second scene, `Scenes/Physics.mscene`, is the same room with a pyramid and a wall of 100 boxes, ramps, stairs and falling balls; its Player is a capsule character that walks with the Move action and jumps with Jump. Open it in the editor (File > Open Scene) and press Play, or play it with the player:

```
build\Debug\MoravaPlayer.exe --project ..\Projects\Sample\Sample.mproject --scene ..\Projects\Sample\Scenes\Physics.mscene
```

A collider marked *Trigger* is a zone things pass through: the log says what enters and leaves it (the Physics scene has a glass Trigger Zone). View > Physics opens the Physics panel: the bodies, the step time and the last collision and trigger events while the game runs, debug views that draw every collider (colored by state: static, awake, sleeping, trigger, character) and the contact points into the viewport, a query test that casts a ray or a sphere from the middle of the view, and the project's collision layers (which kinds of bodies collide, saved in the `.mproject` file).

Simulate (the button after Play, CTRL + SHIFT + P) runs physics alone, without the game's logic and input, on a copy of the scene like Play: the view stays the editor's camera and models can be selected and moved while the bodies fall, roll and settle. Stop brings the scene back as it was.

Games are scripted in C# on .NET 10 (in progress). The build puts `Morava.ScriptCore.dll` (the engine's C# side) and a private copy of the .NET runtime (`dotnet/`, about 75 MB) next to the executables, and the engine starts .NET from there at startup, as a shipped game will: the log says `C# scripting: .NET 10.0.x from ...`. The C# API scripts use is in `ScriptCore` (namespace `Morava`): `Entity` (transform, name, tags, `Destroy`, components, collision and trigger methods), `Rigidbody`, `CharacterController`, `Scene`, `Input`, `Physics` queries and `Time`. Entities get tags for the scripts in the Inspector, under the name (*Tags, separated by commas*).

A project's scripts are a C# project of its own, built into a DLL that the project's settings name (*Script Assembly*); the sample's are in `Projects/Sample/Scripts`, and the build makes them. A script is a class deriving from `Morava.Entity`: in the Inspector, an entity's *Script* section picks the class and shows its public fields (numbers, text, vectors, other entities, enums) to set for that entity. While the game runs, each script gets `OnCreate`, `OnUpdate`, `OnFixedUpdate` and its collision and trigger calls; a script that throws an exception pauses Play, and the log has the file and line. The editor builds the scripts itself when one is saved (*Build on Save*, or *Scripts > Build Scripts*, CTRL + B) and reloads them after each build (a build during Play: at Stop), keeping the values set in the scenes. Compiler errors and a script's exceptions appear in *View > Console*: double-click one to open the file at that line in the code editor (Visual Studio, VS Code or Rider, chosen in the Project Settings). *Scripts > New Script* makes a script from a template, and the Inspector's *Edit Script* opens an entity's. To debug the scripts, start the game from the code editor: in Visual Studio (or Rider) F5 with the *Morava Player* launch profile, in VS Code the *Morava Player* configuration in *Run and Debug* (the *Morava Editor* ones start the editor instead, to debug during Play). Both build the scripts first and open the project. Or attach the .NET debugger to an editor already running (Visual Studio: *Debug > Attach to Process*, `MoravaEngine.exe`, code type *Managed (.NET Core, .NET 5+)*; VS Code: *Attach to the Morava editor*). Breakpoints in the scripts hit during Play. The sample's start scene, `Scenes/RollABall.mscene`, is a small game made this way: roll the ball with Move (WASD, arrows, the left stick) to collect the 12 pickups. C (the gamepad's Y) switches the camera: above the playground, behind the ball, or the ball's eyes; Q, E, R and F (the right stick) turn the view behind and in front of it, and while the ball rolls the view turns behind its direction by itself. The game starts behind the ball.

A prefab is an entity with everything under it, saved as a file (`.mprefab`) to place again: right-click an entity in the Scene Hierarchy and *Save as Prefab...*, or drag it onto the Content Browser. Drag a prefab from the Content Browser onto the viewport or the Scene Hierarchy (or Add > Prefab...) to place an instance of it; an instance's name, position and rotation are its own, the rest is the prefab's. In the Inspector, *Apply to Prefab* writes an instance to its prefab, and the other instances that weren't changed take it (also when a scene opens); *Revert* brings an instance back to its prefab, *Unpack* makes it ordinary entities. The Content Browser shows the project's files (or the engine's): double-click opens scenes and scripts and places prefabs. Roll a Ball's pickups are instances of `Assets/Prefabs/Pickup.mprefab`.

Scripts place prefabs while the game runs: a public `Prefab` field shows in the Inspector (pick one, or drag it from the Content Browser), and `Scene.Instantiate(prefab, position)` places it at once, its scripts' `OnCreate` run and its bodies and sounds working, returning its root (its script when it has one). `Scene.Load("Scenes/Level2.mscene")` goes on in another scene at the end of the frame (`Scene.Reload()`: this one again, as it started); `Scene.Name` and `Scene.Path` say which one runs. In the editor, Stop brings back the scene that was played. In Roll a Ball a collected pickup bursts into sparkles (`Assets/Prefabs/Sparkle.mprefab`), and the game starts again after the win or a fall.

The renderer draws only what the camera can see: before the G-buffer prepass and the scene pass, each mesh's bounding box is tested against the camera's view (frustum culling), and each shadow map's pass tests the models against its own view. Skinned meshes are always drawn. The Renderer Stats panel shows how many meshes were drawn and culled, and has the debug toggles: *Frustum Culling* (off draws everything, to compare), *Freeze* (keeps the view culled against, so the camera can move out and look at it) and *Show Culled* (the culled meshes' boxes in red, the frozen view in yellow).

An animated (skinned) model is played by an animator while the game runs: in the Inspector, its *Animator* section takes a controller (a `.manim` file; *New* makes one with a state for each of the model's clips), edited in *View > Animator*. A controller has states (a clip, from the model's own file or another file of the same rig, its speed, looped or not, *In Place* for a walk made with root motion), parameters the game sets, and transitions between the states (from one state or from any, when their conditions hold, crossfading for so many seconds). A script sets the parameters, `GetComponent<Animator>().SetFloat("Speed", speed)` or `SetTrigger("Use")`, and the animator crossfades on its own. A state can have events at moments of its clip (a foot down: `Footstep`), which a script hears in `OnAnimationEvent`; while the game runs, the Inspector shows the state, the crossfade and the parameters. The sample's `Scenes/Character.mscene` is a character walking where Move says (Shift runs, Enter or the gamepad's X uses), with clips from three Mixamo files of one character (`Models/PirateVoid`). `Scenes/Erika.mscene` is another one that also runs with its own clip and jumps (Space), Mixamo's Erika Archer (`Models/Erika_Archer`); her steps, jumps and landings are heard.

The game is seen through a game camera: an entity with a *Camera* (Scene Hierarchy: Add > Camera), placed and aimed by its transform, with its field of view and near and far clip planes. With several, the *Primary* one shows the game, and a script switches with `GetComponent<Camera>().Primary = true`. The editor's view is its own: Edit and Simulate show it, Play shows the primary camera and Stop goes back to the editor's view, which is saved with the scene. In the Inspector, *Move to View* puts a camera where the editor looks from, and *View from Camera* looks through it; View > Editor Field of View is the editor's lens.

Entities play sounds with miniaudio (WAV, FLAC, MP3, Ogg Vorbis). In the Inspector, *Audio Source* gives an entity a sound: its clip, its bus (Music, Effects or UI), volume, pitch, Loop, Play on Start, Stream (read while it plays, for music) and Spatial (heard from where the entity is, louder within its Min and Max distances); *Preview* plays the clip in the editor. *Audio Listener* (usually on the camera; without one the primary camera listens) is where the game is heard from. A sound that belongs to no model is a *Sound* entity (Scene Hierarchy: Add > Sound), with a speaker icon in the viewport. Scripts play sounds with `GetComponent<AudioSource>().Play()` and `Audio.PlayOneShot("Assets/Audio/Pickup.wav", position)`. View > Audio has the mixer (the project's bus volumes, saved with it) and, while the game runs, its sounds. Roll a Ball's pickups hum, collecting them chimes, and its music streams.

### Building on Linux

Tested on Ubuntu with GCC 15 and Clang, in WSL2. The Vulkan and OpenGL scenes build (GCC; Clang tested with Vulkan); the Vulkan editor and the OpenGL scene SceneSSAO were run. DirectX 11 is Windows only.

Install the compilers, CMake and the system libraries (Vulkan loader and headers, OpenGL, X11 and Wayland for GLFW, GTK 3 for the file dialogs):

```
sudo apt update
sudo apt install build-essential clang cmake ninja-build git pkg-config python3 \
    libvulkan-dev vulkan-tools libgl1-mesa-dev libglu1-mesa-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxkbcommon-dev \
    libwayland-dev wayland-protocols libgtk-3-dev zlib1g-dev
```

Install the .NET 10 SDK for C# scripting (Microsoft's `dotnet-sdk-10.0` package, or the `dotnet-install.sh` script; or configure with `-DMORAVA_SCRIPTING=OFF`).

Get the sources as above (`git clone --recursive`, then `python3 vendor/cross-platform/shaderc/utils/git-sync-deps`). In a clone made without `--recursive`, run `git submodule update --init --recursive`: the file dialog library needs its own submodule (`wayland-protocols`) on Linux.

Configure and build from the repository's root folder. Ninja builds one configuration, chosen when configuring:

```
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-linux
```

For Clang, add `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++` to the first command. The `-D` settings for the graphics APIs and the scene (below) work the same way.

Run it from the same folder:

```
./build-linux/MoravaEngine
```

In WSL2 the window opens on the Windows desktop (WSLg). There the engine uses X11 (through XWayland) rather than Wayland, because WSLg gives X11 windows a full frame (minimize, maximize, close, resizable edges). The only Vulkan device there is Mesa's llvmpipe, which renders on the CPU: the editor works, but at a few frames per second. Recent Mesa versions also emulate ray tracing there, so the ray-traced features run too, even slower; `--no-ray-tracing` switches them off. For full speed, run on native Linux with the GPU's Vulkan driver. Building in WSL2 is also much faster when the build folder is on the Linux file system, e.g. `-B ~/morava-build` instead of `-B build-linux`.

### Select the graphics APIs and the scene

The graphics APIs are selected when CMake configures the project, with `-D` settings. The scene is selected in `src/config.h`, among the scenes of the compiled APIs.

* `MORAVA_VULKAN`, `MORAVA_OPENGL` and `MORAVA_DIRECTX11` select which graphics APIs are compiled. They are independent: any combination works. All the scenes of a compiled API are compiled.
* In `src/config.h`, the block `#if !defined(MORAVA_SCENE_FROM_CMAKE)` has one line per API with the scene it runs, by default the first scene of that API in the list above it (`SCENE_ENV_MAP_VULKAN`, `SCENE_COTTAGE`, `SCENE_DX11`). Change the line to run another scene. When several APIs are compiled, the first in the order Vulkan, OpenGL, DirectX 11 runs its scene.
* `MORAVA_SCENE` (optional) selects the scene in CMake instead, and then `src/config.h`'s choice is ignored. The graphics API of that scene is compiled automatically.

**Example: the OpenGL classes and all OpenGL scenes.** From the repository's root folder:

```
cmake -S . -B build -DMORAVA_OPENGL=ON -DMORAVA_VULKAN=OFF -DMORAVA_DIRECTX11=OFF
cmake --build build --config Debug --parallel
```

SceneCottage runs; change `#define SCENE_COTTAGE` in `src/config.h` to e.g. `#define SCENE_EDITOR` for another OpenGL scene.

**Example: the Vulkan classes and all Vulkan scenes.**

```
cmake -S . -B build -DMORAVA_VULKAN=ON -DMORAVA_OPENGL=OFF -DMORAVA_DIRECTX11=OFF
cmake --build build --config Debug --parallel
```

SceneEnvMapVulkan runs (`#define SCENE_ENV_MAP_VULKAN`). A plain `cmake -S . -B build` gives the same: with no API switched on and no `MORAVA_SCENE`, Vulkan is compiled.

**Example: SceneDX11 with the DirectX 11 API** (Windows only).

```
cmake -S . -B build -DMORAVA_DIRECTX11=ON -DMORAVA_VULKAN=OFF -DMORAVA_OPENGL=OFF
cmake --build build --config Debug --parallel
```

**Example: the scene selected in CMake.**

```
cmake -S . -B build -DMORAVA_SCENE=COTTAGE
cmake --build build --config Debug --parallel
```

Good to know:

* The settings stay in the build folder until they are changed, so only the ones that change have to be given, e.g. `cmake -DMORAVA_OPENGL=ON build`.
* `cmake -DMORAVA_SCENE= build` (empty) clears a `MORAVA_SCENE` set earlier, so `src/config.h` selects the scene again.
* A scene in `src/config.h` whose API is not compiled stops the build with an `#error` that names the missing setting.
* After changing a setting, build again. In Visual Studio, accept "Reload All" first.
* From a terminal that is already in the build folder (as the terminal of Visual Studio), use a dot for the folder: `cmake -DMORAVA_OPENGL=ON .`
* `cmake-gui` shows the same settings: `MORAVA_SCENE` as a list, the others as checkboxes.

All settings:

| Setting | Values | Meaning |
|---|---|---|
| `MORAVA_SCENE` | a name from the table below, default empty | The scene to build, instead of the one selected in `src/config.h`. Its graphics API is always compiled. |
| `MORAVA_VULKAN` | `ON` / `OFF`, default `OFF` | Compile the Vulkan classes and scenes. |
| `MORAVA_OPENGL` | `ON` / `OFF`, default `OFF` | Compile the OpenGL classes and scenes. |
| `MORAVA_DIRECTX11` | `ON` / `OFF`, default `OFF` | Compile the DirectX 11 classes and scene (Windows only). |
| `MORAVA_MONO` | `ON` / `OFF`, default `ON` on Windows | C# scripting through mono. |
| `MORAVA_MSVC_STATIC_RUNTIME` | `ON` / `OFF`, default `ON` | Static (`/MT`) or DLL (`/MD`) MSVC runtime, for the engine and every library. |

The scenes:

| `MORAVA_SCENE` (`SCENE_...` in `config.h`) | Scene class | Graphics API |
|---|---|---|
| `ENV_MAP_VULKAN` | SceneEnvMapVulkan (the Vulkan editor) | Vulkan |
| `HAZEL_VULKAN` | SceneHazelVulkan | Vulkan |
| `DX11` | SceneDX11 | DirectX 11 |
| `COTTAGE` | SceneCottage | OpenGL |
| `EIFFEL` | SceneEiffel | OpenGL |
| `SPONZA` | SceneSponza | OpenGL |
| `TERRAIN` | SceneTerrain | OpenGL |
| `PBR` | ScenePBR | OpenGL |
| `LEARN_OPENGL` | SceneJoey | OpenGL |
| `JOLT` | SceneJolt (Jolt Physics: a tower of planks, fire spheres with the left mouse button) | OpenGL |
| `INSTANCED` | SceneInstanced | OpenGL |
| `ASTEROIDS` | SceneAsteroids | OpenGL |
| `NANOSUIT` | SceneNanosuit | OpenGL |
| `FRAMEBUFFERS` | SceneFramebuffers | OpenGL |
| `CUBEMAPS` | SceneCubemaps | OpenGL |
| `PARTICLES` | SceneParticles | OpenGL |
| `OMNI_SHADOWS` | SceneOmniShadows | OpenGL |
| `VOXEL_TERRAIN` | SceneVoxelTerrain | OpenGL |
| `PROCEDURAL_LANDMASS` | SceneProceduralLandmass | OpenGL |
| `VOXEL_TERRAIN_SL` | SceneVoxelTerrainSL | OpenGL |
| `MARCHING_CUBES` | SceneMarchingCubes | OpenGL |
| `SSAO` | SceneSSAO | OpenGL |
| `BLOOM` | SceneBloom | OpenGL |
| `DEFERRED` | SceneDeferred | OpenGL |
| `DEFERRED_OGL` | SceneDeferredOGL | OpenGL |
| `ANIM_PBR` | SceneAnimPBR | OpenGL |
| `EDITOR` | SceneEditor | OpenGL |
| `EDITOR_IMGUIZMO` | SceneEditorImGuizmo | OpenGL |
| `HAZEL_ENV_MAP` | SceneHazelEnvMap | OpenGL |

### Command line options

For the Vulkan editor (`ENV_MAP_VULKAN`):

| Option | Meaning |
|---|---|
| `--project <file>` | Open this project and its start scene at startup, e.g. `--project Projects/Sample/Sample.mproject` (also MoravaPlayer: plays it) |
| `--scene <file>` | Open this scene at startup, e.g. `--scene Scenes/Old_Stove_Corner.mscene` (also MoravaPlayer: plays it) |
| `--select <name>` | Select the entity of this name in that scene |
| `--debug-view <number>` | Start with this entry of the Environment panel's Debug View list (0 = off) |
| `--no-ray-tracing` | Run without the ray tracing extensions, as on a GPU that has none |
| `--play` | Start the game (Play) once the scene is open |
| `--simulate` | Start Simulate (physics only, seen through the editor's camera) once the scene is open |
| `--save-scene` | The editor saves the open scene to its file when it closes (with `--exit-after`: a scene opened and saved unattended) |
| `--exit-after <seconds>` | Close the application after this time |

## Keyboard and Mouse shortcuts

Vulkan editor (`ENV_MAP_VULKAN`):

* Camera Rotation: Right Mouse Button
* Camera Movement: W) forward, A) left, S) back, D) right, Q) down, E) up
* Fast Movement: Left SHIFT + W|A|S|D|Q|E
* Select: Left Mouse Button; add to or remove from the selection: CTRL or SHIFT + Left Mouse Button
* Gizmo Modes: 1) Translate, 2) Rotate, 3) Scale, 4) Disable
* Gizmo Snapping: Left CTRL while dragging
* New Scene: CTRL + N
* Open Scene: CTRL + O
* Save Scene: CTRL + S
* Save Scene As: CTRL + SHIFT + S
* Copy and Paste: CTRL + C, CTRL + V
* Rename: F2
* Delete: DELETE
* Play and Stop: CTRL + P
* Simulate and Stop: CTRL + SHIFT + P
* While the game runs: Escape gives the input (and a captured mouse) back to the editor, a click on the view gives it to the game

Older editor scenes (`EDITOR`, `EDITOR_IMGUIZMO`):

* Add Mesh or Model: Left CTRL + Left Mouse Button
* Camera Rotation: Right Mouse Button
* Camera Movement: W) forward, A) left, S) back, D) right, Q) down, E) up
* Fast Movement: Left SHIFT + W|A|S|D|Q|E
* Enable Gizmo: TAB + Left Mouse Button
* Toggle Gizmo Modes: 1) Translate, 2) Scale, 3) Rotate, 4) Disable
* Scene Save: Left CTRL + S
* Scene Reset: Left CTRL + R
* Scene Load: Left CTRL + L
* Toggle Wireframe Mode: R
* Copy scene object: Left CTRL + C

## Images

The Vulkan editor on Linux (Ubuntu in WSL2, rendered by Mesa's llvmpipe on the CPU):

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2026-10-17_03-56-20.png)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2021-09-15_04-36-54.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2021-03-23_07-35-56.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2020-10-14_16-14-02.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2020-10-14_16-23-01.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2020-09-21_03-46-55.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2020-03-19-2106.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2020-03-18-0101.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2020-08-12_16-53-48.png)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/Screenshots/2020-07-08_0158.jpg)
