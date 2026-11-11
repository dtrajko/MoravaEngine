cmake_minimum_required(VERSION 3.15)

project(MoravaEngine VERSION 0.0.1 LANGUAGES C CXX)

# macOS: GLFW's and nfd's Cocoa code is Objective-C (.m). Enabled here, for every folder: GLFW enables it only in its
# own, and the files of the other folders then have no compile rule
if(APPLE)
    enable_language(OBJC)
endif()

# =========================================================
# GLOBAL SETTINGS
# =========================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# =========================================================
# SCENE AND GRAPHICS APIS
#   The graphics APIs (Vulkan, OpenGL, DirectX 11) are switched on independently, any combination builds; only their
#   classes are compiled, and all the scenes of each of them. The scene that runs is chosen in src/config.h, among the
#   scenes of the compiled APIs, unless MORAVA_SCENE chooses it here (its API is then always compiled).
#
#     cmake -DMORAVA_OPENGL=ON -DMORAVA_VULKAN=OFF -DMORAVA_DIRECTX11=OFF <build folder>   OpenGL, scene in config.h
#     cmake -DMORAVA_VULKAN=ON <build folder>                                               Vulkan, scene in config.h
#     cmake -DMORAVA_SCENE=<name> <build folder>       this scene, config.h's choice is ignored (e.g. -DMORAVA_SCENE=EDITOR)
#     cmake -DMORAVA_SCENE= <build folder>             back to the scene in config.h
#
#   With no API switched on and no MORAVA_SCENE, Vulkan is compiled.
# =========================================================

set(MORAVA_SCENES_VULKAN ENV_MAP_VULKAN HAZEL_VULKAN)
set(MORAVA_SCENES_DIRECTX11 DX11)
set(MORAVA_SCENES_OPENGL
    COTTAGE EIFFEL SPONZA TERRAIN PBR LEARN_OPENGL JOLT INSTANCED ASTEROIDS NANOSUIT FRAMEBUFFERS CUBEMAPS PARTICLES
    OMNI_SHADOWS VOXEL_TERRAIN PROCEDURAL_LANDMASS VOXEL_TERRAIN_SL MARCHING_CUBES SSAO BLOOM DEFERRED DEFERRED_OGL
    ANIM_PBR EDITOR EDITOR_IMGUIZMO HAZEL_ENV_MAP)

set(MORAVA_SCENE "" CACHE STRING "The scene the engine is built for, its graphics API always compiled; empty: the scene chosen in src/config.h")
set_property(CACHE MORAVA_SCENE PROPERTY STRINGS "" ${MORAVA_SCENES_VULKAN} ${MORAVA_SCENES_OPENGL} ${MORAVA_SCENES_DIRECTX11})

option(MORAVA_VULKAN "Compile the Vulkan classes and scenes (always, when MORAVA_SCENE is a Vulkan scene)" OFF)
option(MORAVA_OPENGL "Compile the OpenGL classes and scenes (always, when MORAVA_SCENE is an OpenGL scene)" OFF)
option(MORAVA_DIRECTX11 "Compile the DirectX 11 classes and scene (always, when MORAVA_SCENE is the DirectX 11 scene); Windows only" OFF)

if(MORAVA_SCENE STREQUAL "")
    set(MORAVA_SCENE_GRAPHICS_API "")
elseif(MORAVA_SCENE IN_LIST MORAVA_SCENES_VULKAN)
    set(MORAVA_SCENE_GRAPHICS_API VULKAN)
elseif(MORAVA_SCENE IN_LIST MORAVA_SCENES_OPENGL)
    set(MORAVA_SCENE_GRAPHICS_API OPENGL)
elseif(MORAVA_SCENE IN_LIST MORAVA_SCENES_DIRECTX11)
    set(MORAVA_SCENE_GRAPHICS_API DIRECTX11)
else()
    message(FATAL_ERROR "MORAVA_SCENE=${MORAVA_SCENE} is not a scene. The scenes: ${MORAVA_SCENES_VULKAN} ${MORAVA_SCENES_OPENGL} ${MORAVA_SCENES_DIRECTX11}")
endif()

# C# scripting of games on .NET 10 (src/Scripting, ScriptCore/): needs the .NET 10 SDK
option(MORAVA_SCRIPTING "C# scripting on .NET (needs the .NET 10 SDK)" ON)

# The older C# demo through mono (src/Mono, used by SceneEnvMapVulkan). The repository has mono's Windows libraries only.
if(WIN32)
    option(MORAVA_MONO "C# scripting through mono" ON)
else()
    option(MORAVA_MONO "C# scripting through mono" OFF)
endif()

# The graphics APIs of this build
set(MORAVA_GRAPHICS_APIS "")
foreach(api VULKAN OPENGL DIRECTX11)
    if(MORAVA_${api} OR api STREQUAL MORAVA_SCENE_GRAPHICS_API)
        list(APPEND MORAVA_GRAPHICS_APIS ${api})
    endif()
endforeach()
if(NOT MORAVA_GRAPHICS_APIS)
    set(MORAVA_GRAPHICS_APIS VULKAN)
endif()
if("DIRECTX11" IN_LIST MORAVA_GRAPHICS_APIS AND NOT WIN32)
    message(FATAL_ERROR "DirectX 11 exists on Windows only (MORAVA_SCENE=${MORAVA_SCENE}, MORAVA_DIRECTX11=${MORAVA_DIRECTX11})")
endif()
string(REPLACE ";" ", " MORAVA_GRAPHICS_APIS_TEXT "${MORAVA_GRAPHICS_APIS}")
if(MORAVA_SCENE STREQUAL "")
    message(STATUS "Scene: chosen in src/config.h; graphics APIs compiled: ${MORAVA_GRAPHICS_APIS_TEXT}")
else()
    message(STATUS "Scene: ${MORAVA_SCENE} (${MORAVA_SCENE_GRAPHICS_API}); graphics APIs compiled: ${MORAVA_GRAPHICS_APIS_TEXT}")
endif()

# =========================================================
# MSVC RUNTIME LIBRARY (one switch for the engine and every vendor target)
#   ON  = static runtime:  /MTd (Debug), /MT (Release)
#   OFF = DLL runtime:     /MDd (Debug), /MD (Release)
# =========================================================

option(MORAVA_MSVC_STATIC_RUNTIME "Use the static MSVC runtime (/MT, /MTd) for all targets; OFF uses the DLL runtime (/MD, /MDd)" ON)

# Compiler warnings in the engine's own sources fail the build (-Werror, /WX); the libraries in vendor/ are not affected.
# Off by default so that a warning doesn't stop work in progress; CI switches it on (.github/workflows/build.yml).
option(MORAVA_WARNINGS_AS_ERRORS "Treat compiler warnings in the engine's sources as errors" OFF)

if(MORAVA_MSVC_STATIC_RUNTIME)
    set(MORAVA_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(MORAVA_SHARED_CRT OFF)
else()
    set(MORAVA_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set(MORAVA_SHARED_CRT ON)
endif()

# Vendors with an old cmake_minimum_required would otherwise ignore CMAKE_MSVC_RUNTIME_LIBRARY
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY ${MORAVA_MSVC_RUNTIME})

# Each vendor's own runtime switch, set to match
set(USE_MSVC_RUNTIME_LIBRARY_DLL ${MORAVA_SHARED_CRT} CACHE BOOL "" FORCE)    # glfw
set(YAML_MSVC_SHARED_RT ${MORAVA_SHARED_CRT} CACHE BOOL "" FORCE)             # yaml-cpp
set(SHADERC_ENABLE_SHARED_CRT ${MORAVA_SHARED_CRT} CACHE BOOL "" FORCE)       # shaderc, glslang, spirv-tools
set(USE_STATIC_CRT ${MORAVA_MSVC_STATIC_RUNTIME} CACHE BOOL "" FORCE)         # assimp

# Engine-only defines (platform, HZ_DEBUG/HZ_RELEASE) are set on the engine's targets below,
# so they don't leak into the vendor projects

# =========================================================
# PATHS
# =========================================================

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
set(VENDOR_CROSS_PLATFORM_DIR ${VENDOR_DIR}/cross-platform)

# =========================================================
# THIRD-PARTY CONFIG
# =========================================================

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# ---- Assimp ----
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)  # the engine only imports models

# ---- Vulkan ----
# Headers and vulkan-1.lib come from the installed Vulkan SDK (found through the VULKAN_SDK
# environment variable), so they match the SDK's validation layers
find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
message(STATUS "Vulkan SDK: ${Vulkan_VERSION} (${Vulkan_INCLUDE_DIRS})")

# ---- .NET (C# scripting) ----
if(MORAVA_SCRIPTING)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DotNet.cmake)
endif()

# ---- Jolt Physics ----
# The library alone: no samples, tests or viewer, and none of its GPU compute (DirectX 12, Vulkan, Metal shaders)
foreach(option TARGET_UNIT_TESTS TARGET_HELLO_WORLD TARGET_PERFORMANCE_TEST TARGET_SAMPLES TARGET_VIEWER ENABLE_INSTALL
        JPH_USE_DX12 JPH_USE_VK JPH_USE_MTL JPH_USE_CPU_COMPUTE)
    set(${option} OFF CACHE BOOL "" FORCE)
endforeach()
set(USE_STATIC_MSVC_RUNTIME_LIBRARY ${MORAVA_MSVC_STATIC_RUNTIME} CACHE BOOL "" FORCE)
# Compiled with the engine's flags (Debug: debug info, no optimization), not Jolt's own, and without link-time optimization
# (its objects would need the same compiler's linker plugin, and ccache can't keep them)
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
set(GENERATE_DEBUG_SYMBOLS OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
# A vendor's warnings don't fail the build (MORAVA_WARNINGS_AS_ERRORS is for the engine's sources)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE ON CACHE BOOL "" FORCE)    # the physics debug view (shapes as lines)
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(ENABLE_OBJECT_STREAM OFF CACHE BOOL "" FORCE)                  # Jolt's own file format: the scene saves the components
set(CROSS_PLATFORM_DETERMINISTIC OFF CACHE BOOL "" FORCE)
# The CPU a game needs (x64; Apple Silicon and ARM use NEON, no option). Jolt's default is AVX2 with FMA, F16C, LZCNT and
# BMI (Haswell, 2013): a game built with it crashes at startup on a CPU without them (Pentium and Celeron sold until ~2020
# have no AVX). SSE4.2 runs on every x64 CPU of the last 15 years; the instructions after it make physics a few percent
# faster at most. The flags are Jolt's PUBLIC ones: the engine's own code is compiled for the same CPU
set(USE_SSE4_1 ON CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)
foreach(option USE_AVX USE_AVX2 USE_AVX512 USE_LZCNT USE_TZCNT USE_F16C USE_FMADD)
    set(${option} OFF CACHE BOOL "" FORCE)
endforeach()

# ---- Shaderc ----
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
# The engine compiles GLSL only (DirectX 11 compiles its HLSL with D3DCompile): without glslang's HLSL front-end,
# which glslang has deprecated (a warning at every configure)
set(SHADERC_ENABLE_HLSL OFF CACHE BOOL "" FORCE)

# ---- Native File Dialog Extended (the file dialogs of Windows, Linux and macOS) ----
set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NFD_INSTALL OFF CACHE BOOL "" FORCE)
# Windows: a dialog given a folder starts there, not in the folder a dialog was last closed in
set(NFD_OVERRIDE_RECENT_WITH_DEFAULT ON CACHE BOOL "" FORCE)

# ---- FreeType (reads the fonts the game UI's atlases are made from, Phase 7) ----
# Only TrueType and OpenType outlines: none of its optional libraries (compressed fonts, color emoji, shaping)
foreach(option FT_DISABLE_ZLIB FT_DISABLE_BZIP2 FT_DISABLE_PNG FT_DISABLE_HARFBUZZ FT_DISABLE_BROTLI SKIP_INSTALL_ALL)
    set(${option} ON CACHE BOOL "" FORCE)
endforeach()
# Its own copy of zlib (gzip module, always built) doesn't compile on macOS: the SDK defines TARGET_OS_MAC, and FreeType
# 2.11's ftzconf.h then leaves out the Byte type. The system's zlib there (the SDK always has it)
if(APPLE)
    set(FT_DISABLE_ZLIB OFF CACHE BOOL "" FORCE)
    set(FT_REQUIRE_ZLIB ON CACHE BOOL "" FORCE)
endif()

# =========================================================
# OPTIONAL SUBDIRECTORY HELPER
# =========================================================

# Vendors are added with EXCLUDE_FROM_ALL: the solution contains, and "Build Solution" builds, only the
# libraries the engine links (and what they depend on). Vendor tools, examples and tests are left out;
# they can still be built from each vendor's own solution in the build folder (e.g. build/shaderc).
# (The INSTALL project is not built either; vendor install rules are left as they are, since
# shaderc's SHADERC_SKIP_INSTALL breaks glslang's.)
function(add_optional dir out)
    if(EXISTS ${dir}/CMakeLists.txt)
        add_subdirectory(${dir} ${CMAKE_BINARY_DIR}/${out} EXCLUDE_FROM_ALL)
    endif()
endfunction()

# =========================================================
# VENDORS (ORDER MATTERS)
# =========================================================

add_optional(${VENDOR_CROSS_PLATFORM_DIR}/glfw glfw)
add_optional(${VENDOR_DIR}/box2d box2d)
# Jolt's CMakeLists.txt is in its Build folder
add_optional(${VENDOR_CROSS_PLATFORM_DIR}/JoltPhysics/Build JoltPhysics)
if(NOT TARGET Jolt)
    message(FATAL_ERROR "Jolt Physics not found: git submodule update --init vendor/cross-platform/JoltPhysics")
endif()
add_optional(${VENDOR_CROSS_PLATFORM_DIR}/yaml-cpp yaml-cpp)
add_optional(${VENDOR_CROSS_PLATFORM_DIR}/SPIRV-Cross spirv-cross)
add_optional(${VENDOR_CROSS_PLATFORM_DIR}/shaderc shaderc)
if("DIRECTX11" IN_LIST MORAVA_GRAPHICS_APIS)
    add_optional(${VENDOR_DIR}/DirectXTex directxtex) # loads the DirectX 11 textures
endif()
add_optional(${VENDOR_CROSS_PLATFORM_DIR}/assimp assimp)
# assimp turns on Clang's -Wunsafe-buffer-usage for its own sources (~9000 warnings about raw pointer indexing in every
# Clang build); off again for its target, whose options come after the folder's
if(TARGET assimp AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(assimp PRIVATE -Wno-unsafe-buffer-usage -Wno-unsafe-buffer-usage-in-libc-call)
endif()
add_optional(${VENDOR_CROSS_PLATFORM_DIR}/nativefiledialog-extended nfd)
# FreeType 2.11 asks for CMake 2.8.12, which CMake 4 no longer accepts (and warns about below 3.10): taken as 3.10 for
# these folders alone
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
add_optional(${VENDOR_DIR}/bc7enc bc7enc) # cooked textures: the engine's own build file, in vendor/bc7enc
add_optional(${VENDOR_DIR}/lz4 lz4)       # the pack file: the engine's own build file, in vendor/lz4
add_optional(${VENDOR_DIR}/msdf-atlas-gen/msdfgen/freetype freetype)
unset(CMAKE_POLICY_VERSION_MINIMUM)
if(NOT TARGET freetype)
    message(FATAL_ERROR "FreeType not found: git submodule update --init --recursive vendor/msdf-atlas-gen")
endif()

# =========================================================
# SOURCES
# =========================================================

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS RELATIVE ${SRC_DIR}
    ${SRC_DIR}/*.cpp
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS RELATIVE ${SRC_DIR}
    ${SRC_DIR}/*.h
    ${SRC_DIR}/*.hpp
)

# Only the classes of the build's graphics APIs are compiled. A source file belongs to an API by its name:
#   Vulkan      "Vulkan" anywhere in its path        (H2M/Platform/Vulkan/, EnvMapVulkan/, Scene/SceneEnvMapVulkan.cpp ...)
#   OpenGL      a folder or file name that starts with "OpenGL"   (Platform/OpenGL/, .../OpenGLTextureH2M.cpp ...)
#   DirectX 11  "DX11" anywhere in its path          (Platform/DX11/, Scene/SceneDX11.cpp)
# Every other file is shared. Shared code that names a class of one API does so inside #if defined(GRAPHICS_API_...).
set(MORAVA_SOURCES_OF_VULKAN "Vulkan")
set(MORAVA_SOURCES_OF_OPENGL "(^|/)OpenGL")
set(MORAVA_SOURCES_OF_DIRECTX11 "DX11")
foreach(api VULKAN OPENGL DIRECTX11)
    if(NOT api IN_LIST MORAVA_GRAPHICS_APIS)
        list(FILTER SRC EXCLUDE REGEX "${MORAVA_SOURCES_OF_${api}}")
        list(FILTER HEADERS EXCLUDE REGEX "${MORAVA_SOURCES_OF_${api}}")
    endif()
endforeach()

# The editor (src/Editor) edits the scene of SceneEnvMapVulkan (EnvMapVulkanScene), and the player (src/Player) runs it:
# so far only in Vulkan builds
if(NOT "VULKAN" IN_LIST MORAVA_GRAPHICS_APIS)
    list(FILTER SRC EXCLUDE REGEX "^(Editor|Player)/|^Assets/(ShaderCompile|PackBuilder|GameBuilder|GameArchive)\\.")
    list(FILTER HEADERS EXCLUDE REGEX "^(Editor|Player)/|^Assets/(ShaderCompile|PackBuilder|GameBuilder|GameArchive)\\.")
endif()

# And the shared classes only the OpenGL scenes use (the older Morava scenes and what they are made of)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenGLSceneSources.cmake)
if(NOT "OPENGL" IN_LIST MORAVA_GRAPHICS_APIS)
    foreach(entry ${MORAVA_SOURCES_OF_OPENGL_SCENES})
        string(REPLACE "." "\\." entry_pattern "${entry}")
        list(FILTER SRC EXCLUDE REGEX "^${entry_pattern}")
        list(FILTER HEADERS EXCLUDE REGEX "^${entry_pattern}")
    endforeach()
endif()

# C# scripting
if(NOT MORAVA_MONO)
    list(FILTER SRC EXCLUDE REGEX "^Mono/")
    list(FILTER HEADERS EXCLUDE REGEX "^Mono/")
endif()

# =========================================================
# TARGETS
#   MoravaCore    static library: what a game runs on. The platform layer (window, input, files), the H2M classes, the
#                 renderers, the scene and its files
#   MoravaEngine  executable: the editor (src/Editor), the older scenes and their editors, and everything else that draws
#                 with ImGui, on MoravaCore
#   MoravaPlayer  executable: runs a game on MoravaCore alone, without the editor and without ImGui (src/Player; in builds
#                 with Vulkan)
#   A source belongs to the core unless a rule below gives it to one of the executables. An executable links only the
#   core's object files it calls, so the player's link line shows what the core still needs from the editor.
# =========================================================

set(MORAVA_PLAYER_SOURCES "^Player/")
set(MORAVA_EDITOR_SOURCES
    "^main\\.cpp$"
    "^EditorApplication\\."
    "^Editor/"
    "^Editor_Legacy/"
    "^Assets/(ModelImport|ShaderCompile|PackBuilder|GameBuilder|GameArchive)\\."  # what makes cooked files and games: a player only reads them
    "^EnvMap/EnvMap(SceneRenderer|SharedData)\\." # the OpenGL editor of SCENE_HAZEL_ENV_MAP (its materials are the core's)
    "^HazelVulkan/"
    "^ImGui/"
    "^Mono/"
    "^Scene/"                                     # the older scenes, with ImGui windows in their base class
    "^Camera/CameraController\\."                 # the older scenes' camera control (asks ImGui whether the viewport has the mouse)
    "^H2M/Editor/"
    "^H2M/ImGui/"
    "ImGuiLayer"                                  # the ImGui layers of every graphics API
    "TestLayer"                                   # the test scenes of Hazel Vulkan and DirectX 11
    "EditorLayer"
    "^H2M/Platform/Vulkan/VulkanRendererH2M\\."   # the renderer of SCENE_HAZEL_VULKAN, with its ImGui windows
    "^Platform/DX11/DX11Renderer\\."              # the renderer of SCENE_DX11, with its ImGui windows
    "_ImGui\\.cpp$"                               # the ImGui windows of a core class (SceneRendererH2M_ImGui.cpp)
)
foreach(entry ${MORAVA_SOURCES_OF_OPENGL_SCENES})
    string(REPLACE "." "\\." entry_pattern "${entry}")
    list(APPEND MORAVA_EDITOR_SOURCES "^${entry_pattern}")
endforeach()
string(JOIN "|" MORAVA_EDITOR_SOURCES_REGEX ${MORAVA_EDITOR_SOURCES})

set(MORAVA_ALL_SOURCES ${SRC} ${HEADERS})
set(PLAYER_SRC ${MORAVA_ALL_SOURCES})
list(FILTER PLAYER_SRC INCLUDE REGEX "${MORAVA_PLAYER_SOURCES}")
set(EDITOR_SRC ${MORAVA_ALL_SOURCES})
list(FILTER EDITOR_SRC INCLUDE REGEX "${MORAVA_EDITOR_SOURCES_REGEX}")
set(CORE_SRC ${MORAVA_ALL_SOURCES})
list(FILTER CORE_SRC EXCLUDE REGEX "${MORAVA_PLAYER_SOURCES}|${MORAVA_EDITOR_SOURCES_REGEX}")

foreach(part CORE EDITOR PLAYER)
    set(part_sources ${${part}_SRC})
    list(FILTER part_sources INCLUDE REGEX "\\.cpp$")
    list(LENGTH part_sources MORAVA_${part}_SOURCE_COUNT)
endforeach()
message(STATUS "Sources compiled for ${MORAVA_GRAPHICS_APIS_TEXT}: core ${MORAVA_CORE_SOURCE_COUNT}, editor ${MORAVA_EDITOR_SOURCE_COUNT}, player ${MORAVA_PLAYER_SOURCE_COUNT}")

list(TRANSFORM CORE_SRC PREPEND ${SRC_DIR}/)
list(TRANSFORM EDITOR_SRC PREPEND ${SRC_DIR}/)
list(TRANSFORM PLAYER_SRC PREPEND ${SRC_DIR}/)

add_library(MoravaCore STATIC ${CORE_SRC})
add_executable(${PROJECT_NAME} ${EDITOR_SRC})
if("VULKAN" IN_LIST MORAVA_GRAPHICS_APIS)
    add_executable(MoravaPlayer ${PLAYER_SRC})
endif()
set(MORAVA_ENGINE_TARGETS MoravaCore ${PROJECT_NAME})
if(TARGET MoravaPlayer)
    list(APPEND MORAVA_ENGINE_TARGETS MoravaPlayer)
endif()

# The settings of the engine's own sources, the same in each of its targets
foreach(tgt ${MORAVA_ENGINE_TARGETS})
    if(MSVC)
        target_compile_options(${tgt} PRIVATE /utf-8) # the sources are UTF-8 (GCC and Clang take that as given)
    endif()

    if(MORAVA_WARNINGS_AS_ERRORS)
        target_compile_options(${tgt} PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/WX,-Werror>)
    endif()

    # The scene (when MORAVA_SCENE sets it; otherwise src/config.h does) and the graphics APIs of the build
    if(NOT MORAVA_SCENE STREQUAL "")
        target_compile_definitions(${tgt} PRIVATE SCENE_${MORAVA_SCENE} MORAVA_SCENE_FROM_CMAKE)
    endif()
    foreach(api ${MORAVA_GRAPHICS_APIS})
        target_compile_definitions(${tgt} PRIVATE GRAPHICS_API_${api})
    endforeach()
    if(MORAVA_SCRIPTING)
        target_compile_definitions(${tgt} PRIVATE MORAVA_SCRIPTING)
    endif()
    if(MORAVA_MONO)
        target_compile_definitions(${tgt} PRIVATE MORAVA_MONO)
    endif()

    if(WIN32)
        target_compile_definitions(${tgt} PRIVATE HZ_PLATFORM_WINDOWS NOMINMAX WIN32_LEAN_AND_MEAN)
    elseif(APPLE)
        target_compile_definitions(${tgt} PRIVATE HZ_PLATFORM_MACOS)
    elseif(UNIX)
        target_compile_definitions(${tgt} PRIVATE HZ_PLATFORM_LINUX)
    endif()

    # Debug: asserts and debug-only code; Release/RelWithDebInfo/MinSizeRel: HZ_RELEASE
    target_compile_definitions(${tgt} PRIVATE
        $<$<CONFIG:Debug>:HZ_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:HZ_RELEASE>
    )
    # The configuration's name, for the window title (Application::GetBuildConfig): per configuration, as Visual Studio
    # and Xcode choose it when building, not when configuring
    target_compile_definitions(${tgt} PRIVATE "MORAVA_BUILD_CONFIG=\"$<CONFIG>\"")

    # Many sources #define this at their top; the precompiled header now comes first, so define it for the whole target.
    # Defined empty ("=") to match those #defines exactly, otherwise MSVC warns about a redefinition (C4005).
    target_compile_definitions(${tgt} PRIVATE _CRT_SECURE_NO_WARNINGS=)

    target_include_directories(${tgt} PRIVATE
        ${SRC_DIR}
        ${VENDOR_DIR}
        ${VENDOR_CROSS_PLATFORM_DIR}/SPIRV-Cross
    )

    # Precompiled header: stable third-party and standard headers, included first in every source file
    # (src/PrecompiledHeader.h; the older src/pch.h is an ordinary header some engine headers include)
    target_precompile_headers(${tgt} PRIVATE ${SRC_DIR}/PrecompiledHeader.h)
endforeach()

# The editor's sources draw with ImGui
target_precompile_headers(${PROJECT_NAME} PRIVATE <imgui.h>)

# =========================================================
# INTERFACE LIBS
# =========================================================

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${VENDOR_DIR}/GLM)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ${VENDOR_DIR}/tinyobjloader)

add_library(entt INTERFACE)
target_include_directories(entt INTERFACE ${VENDOR_DIR}/entt/include)

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${VENDOR_DIR}/spdlog/include)

add_library(mono INTERFACE)
target_include_directories(mono INTERFACE ${VENDOR_DIR}/mono/include)

add_library(NvidiaAftermath INTERFACE)
target_include_directories(NvidiaAftermath INTERFACE ${VENDOR_DIR}/NvidiaAftermath)

add_library(Optick INTERFACE)
target_include_directories(Optick INTERFACE ${VENDOR_DIR}/Optick/src)

add_library(farbot INTERFACE)
target_include_directories(farbot INTERFACE ${VENDOR_DIR}/farbot)

# =========================================================
# GLEW
# =========================================================

# Built from the release sources (src/glew.c); defines the static target 'glew'
add_subdirectory(${VENDOR_DIR}/GLEW ${CMAKE_BINARY_DIR}/glew)

# =========================================================
# VULKAN MEMORY ALLOCATOR
# =========================================================

add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE
    ${VENDOR_DIR}/VulkanMemoryAllocator
)

# =========================================================
# IMGUI
# =========================================================

set(IMGUI_DIR ${VENDOR_DIR}/ImGui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${VENDOR_CROSS_PLATFORM_DIR}/glfw/include
)

# imgui_impl_vulkan.cpp needs the Vulkan headers (from the Vulkan SDK)
target_link_libraries(imgui PUBLIC Vulkan::Headers)

# The backends: GLFW (the window of the Vulkan and OpenGL scenes) and one per graphics API of the build
target_sources(imgui PRIVATE ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp)
if("VULKAN" IN_LIST MORAVA_GRAPHICS_APIS)
    target_sources(imgui PRIVATE ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
endif()
if("OPENGL" IN_LIST MORAVA_GRAPHICS_APIS)
    target_sources(imgui PRIVATE ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
endif()
if("DIRECTX11" IN_LIST MORAVA_GRAPHICS_APIS)
    target_sources(imgui PRIVATE
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    )
endif()

# =========================================================
# IMGUIZMO
# =========================================================

file(GLOB IMGUIZMO_SRC
    ${VENDOR_CROSS_PLATFORM_DIR}/ImGuizmo/src/*.cpp
)

add_library(ImGuizmo STATIC ${IMGUIZMO_SRC})
target_include_directories(ImGuizmo PUBLIC
    ${VENDOR_CROSS_PLATFORM_DIR}/ImGuizmo/src
)
target_link_libraries(ImGuizmo PUBLIC imgui)

# =========================================================
# MINIAUDIO (audio, Phase 5)
# =========================================================

# One C file: miniaudio's implementation with stb_vorbis (src/Audio/MiniaudioLibrary.c). Not miniaudio's own CMakeLists,
# which looks for libvorbis and libopus. A vendor's warnings are off
add_library(miniaudio STATIC ${SRC_DIR}/Audio/MiniaudioLibrary.c)
target_include_directories(miniaudio PUBLIC ${VENDOR_CROSS_PLATFORM_DIR}/miniaudio)
if(MSVC)
    target_compile_options(miniaudio PRIVATE /w)
else()
    target_compile_options(miniaudio PRIVATE -w)
endif()
if(APPLE)
    # Linked to Core Audio, not loading it at runtime (miniaudio's default, which Apple's notarization can refuse)
    target_compile_definitions(miniaudio PUBLIC MA_NO_RUNTIME_LINKING)
    target_link_libraries(miniaudio PUBLIC "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
elseif(UNIX)
    # ALSA, PulseAudio and JACK are loaded at runtime (a computer without them plays through the null device)
    find_package(Threads REQUIRED)
    target_link_libraries(miniaudio PUBLIC Threads::Threads m ${CMAKE_DL_LIBS})
endif()

# =========================================================
# MSDF FONT ATLASES (the game UI's text, Phase 7)
# =========================================================

# msdfgen (a glyph's outline to a multi-channel signed distance field) and msdf-atlas-gen (a font's glyphs packed into one
# atlas) as one library: their own build files are premake (msdfgen, TheCherno's fork) or build the command-line tool.
# Without the tool's main, SVG import or Skia. A vendor's warnings are off
set(MSDF_DIR ${VENDOR_DIR}/msdf-atlas-gen)
file(GLOB MSDF_SRC
    ${MSDF_DIR}/msdfgen/core/*.cpp
    ${MSDF_DIR}/msdf-atlas-gen/*.cpp
)
list(FILTER MSDF_SRC EXCLUDE REGEX "/msdf-atlas-gen/main\\.cpp$")
list(APPEND MSDF_SRC
    ${MSDF_DIR}/msdfgen/ext/import-font.cpp
    ${MSDF_DIR}/msdfgen/ext/resolve-shape-geometry.cpp
    ${MSDF_DIR}/msdfgen/ext/save-png.cpp
    ${MSDF_DIR}/msdfgen/lib/lodepng.cpp
)
add_library(msdf STATIC ${MSDF_SRC})
# SYSTEM: the engine's sources that include its headers (UIFont.cpp) aren't failed by their warnings (-Werror)
target_include_directories(msdf SYSTEM PUBLIC ${MSDF_DIR} ${MSDF_DIR}/msdfgen ${MSDF_DIR}/msdfgen/include)
# MSDFGEN_CMAKE_BUILD: no #pragma comment(lib) for FreeType on Windows (CMake links it)
target_compile_definitions(msdf PUBLIC MSDFGEN_USE_CPP11 MSDFGEN_CMAKE_BUILD)
if(MSVC)
    target_compile_definitions(msdf PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(msdf PRIVATE /w)
else()
    target_compile_options(msdf PRIVATE -w)
endif()
find_package(Threads REQUIRED)
target_link_libraries(msdf PUBLIC freetype Threads::Threads)

# =========================================================
# LINKING
# =========================================================

set(MONO_DIR ${VENDOR_DIR}/mono)

# The core's libraries: the executables get them through the core
target_link_libraries(MoravaCore PUBLIC
    glm entt spdlog tinyobjloader

    glfw
    Vulkan::Vulkan
    glew
    nfd

    VulkanMemoryAllocator
    NvidiaAftermath
    Optick
    farbot

    # OpenGL itself, in every build: the older Morava classes that all scenes share call it (through GLEW)
    OpenGL::GL

    # PUBLIC: Jolt checks at startup that the program was compiled with its JPH_* defines (and for its CPU)
    Jolt

    miniaudio
    msdf
    bc7enc
    lz4
)

if(TARGET spirv-cross-core)
    target_link_libraries(MoravaCore PUBLIC spirv-cross-core spirv-cross-glsl)
endif()

foreach(lib box2d yaml-cpp DirectXTex)
    if(TARGET ${lib})
        target_link_libraries(MoravaCore PUBLIC ${lib})
    endif()
endforeach()

# assimp and shaderc: the editor's (a player reads what a pack holds, 10.3), linked below. The core still sees their headers:
# Core/Math.h and the older classes of the other graphics APIs
if(TARGET assimp)
    set(MORAVA_ASSIMP_TARGET assimp)
elseif(TARGET assimp::assimp)
    set(MORAVA_ASSIMP_TARGET assimp::assimp)
else()
    message(FATAL_ERROR "Assimp target not found")
endif()
target_include_directories(MoravaCore SYSTEM PUBLIC $<TARGET_PROPERTY:${MORAVA_ASSIMP_TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
add_dependencies(MoravaCore ${MORAVA_ASSIMP_TARGET})
set(MORAVA_EDITOR_ONLY_LIBRARIES ${MORAVA_ASSIMP_TARGET})
if(TARGET shaderc)
    target_include_directories(MoravaCore SYSTEM PRIVATE $<TARGET_PROPERTY:shaderc,INTERFACE_INCLUDE_DIRECTORIES>)
    list(APPEND MORAVA_EDITOR_ONLY_LIBRARIES shaderc shaderc_util)
endif()

if("DIRECTX11" IN_LIST MORAVA_GRAPHICS_APIS)
    target_link_libraries(MoravaCore PUBLIC d3d11 d3dcompiler)
endif()

# nethost finds hostfxr in the private .NET folder (src/Scripting/ScriptHost.cpp). Windows: nethost.dll through its import
# library, not the static libnethost.lib, which is built with the static release runtime (/MT) only and clashes with
# Debug builds and with /MD
if(MORAVA_SCRIPTING)
    target_include_directories(MoravaCore PRIVATE "${MORAVA_NETHOST_DIR}")
    if(WIN32)
        target_link_libraries(MoravaCore PUBLIC "${MORAVA_NETHOST_DIR}/nethost.lib")
    else()
        target_link_libraries(MoravaCore PUBLIC "${MORAVA_NETHOST_DIR}/libnethost.a" ${CMAKE_DL_LIBS})
    endif()
endif()

# Some of the core's headers still include ImGui's: the core sees them, but doesn't link ImGui (the player has none)
target_include_directories(MoravaCore PRIVATE
    $<TARGET_PROPERTY:imgui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ImGuizmo,INTERFACE_INCLUDE_DIRECTORIES>
)

# The editor: ImGui, and what only the older scenes use (mono: C# in SceneEnvMapVulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE
    MoravaCore
    ${MORAVA_EDITOR_ONLY_LIBRARIES}
    imgui
    ImGuizmo
)

# zlib: Build Game's .tar.gz of a Linux game (Assets/GameArchive). assimp's own copy on Windows (zconf.h is made in its build
# folder), the system's elsewhere (Linux packages, the macOS SDK)
if(TARGET ${PROJECT_NAME} AND "VULKAN" IN_LIST MORAVA_GRAPHICS_APIS)
    if(TARGET zlibstatic)
        get_target_property(MORAVA_ZLIB_SOURCE_DIR zlibstatic SOURCE_DIR)
        get_target_property(MORAVA_ZLIB_BINARY_DIR zlibstatic BINARY_DIR)
        target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${MORAVA_ZLIB_SOURCE_DIR}" "${MORAVA_ZLIB_BINARY_DIR}")
        target_link_libraries(${PROJECT_NAME} PRIVATE zlibstatic)
    else()
        find_package(ZLIB REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
    endif()
endif()

if(MORAVA_MONO)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        mono
        debug ${MONO_DIR}/lib/Debug/mono-2.0-sgen.lib
        optimized ${MONO_DIR}/lib/Release/mono-2.0-sgen.lib
    )
endif()

if(TARGET MoravaPlayer)
    target_link_libraries(MoravaPlayer PRIVATE MoravaCore)
    # Vulkan's loader comes with the graphics driver. Delay-loaded, so a machine without it gets the player's own message
    # (VulkanSupport::Check) instead of Windows' about a missing DLL. DbgHelp writes the crash dumps
    if(MSVC)
        target_link_libraries(MoravaPlayer PRIVATE delayimp dbghelp)
        target_link_options(MoravaPlayer PRIVATE /DELAYLOAD:vulkan-1.dll)
        # Release keeps its symbols (MoravaPlayer.pdb, not shipped: CI keeps it beside the template), so a player's crash dump
        # can be read. /Z7: in the object files, which ccache can keep. /DEBUG turns the linker's removal of unused code off,
        # hence /OPT again
        foreach(tgt MoravaCore MoravaPlayer)
            target_compile_options(${tgt} PRIVATE $<$<CONFIG:Release>:/Z7>)
        endforeach()
        target_link_options(MoravaPlayer PRIVATE $<$<CONFIG:Release>:/DEBUG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
    endif()
    # Linux: a game's libraries beside its executable are found there (11.6). Today it has none of its own; the system gives
    # libvulkan and the rest
    if(UNIX AND NOT APPLE)
        set_property(TARGET MoravaPlayer APPEND PROPERTY BUILD_RPATH "\$ORIGIN")
    endif()
    # A build with another graphics API as well: the core's shaders and models of that API call them, and the graphics API's
    # choice in the shared classes links those into the player. A Vulkan build's player has neither
    if(NOT MORAVA_GRAPHICS_APIS STREQUAL "VULKAN")
        target_link_libraries(MoravaPlayer PRIVATE ${MORAVA_EDITOR_ONLY_LIBRARIES})
    endif()
endif()

# macOS: Vulkan layers name their library by file name only (Homebrew's validation layer: libVkLayer_khronos_validation.dylib),
# and the loader opens it from the executable's search paths, which don't include Homebrew's folder. The layers sit next to
# the Vulkan loader (Homebrew's lib folder, or the Vulkan SDK's), so that folder joins the search paths
if(APPLE AND Vulkan_LIBRARY)
    get_filename_component(MORAVA_VULKAN_LIBRARY_DIR "${Vulkan_LIBRARY}" DIRECTORY)
    # The player first looks in a macOS app's Frameworks (11.6): a game carries its Vulkan loader
    if(TARGET MoravaPlayer)
        set_property(TARGET MoravaPlayer APPEND PROPERTY BUILD_RPATH "@executable_path/../Frameworks")
    endif()
    foreach(tgt ${PROJECT_NAME} MoravaPlayer)
        if(TARGET ${tgt})
            set_property(TARGET ${tgt} APPEND PROPERTY BUILD_RPATH "${MORAVA_VULKAN_LIBRARY_DIR}")
        endif()
    endforeach()
endif()

# macOS player template (11.6): a game is an app that brings Vulkan with it. Beside the player: Frameworks/ with the Vulkan
# loader and MoltenVK, and vulkan/icd.d/MoltenVK_icd.json naming MoltenVK from an app's Resources/vulkan/icd.d, where the loader
# looks. The player names the loader @rpath/libvulkan.1.dylib (Homebrew's names its own file), so the app's copy is found first.
# install_name_tool breaks a signature: the changed files are signed again, ad hoc, which Apple Silicon needs to run them
if(APPLE AND TARGET MoravaPlayer AND Vulkan_LIBRARY)
    get_filename_component(MORAVA_VULKAN_LOADER_FILE "${Vulkan_LIBRARY}" REALPATH)
    execute_process(COMMAND otool -D "${MORAVA_VULKAN_LOADER_FILE}" OUTPUT_VARIABLE MORAVA_VULKAN_LOADER_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "^[^\n]*\n" "" MORAVA_VULKAN_LOADER_ID "${MORAVA_VULKAN_LOADER_ID}")
    find_library(MORAVA_MOLTENVK_LIBRARY MoltenVK HINTS "${MORAVA_VULKAN_LIBRARY_DIR}" /opt/homebrew/lib /usr/local/lib)
    if(MORAVA_MOLTENVK_LIBRARY)
        get_filename_component(MORAVA_MOLTENVK_FILE "${MORAVA_MOLTENVK_LIBRARY}" REALPATH)
        get_filename_component(MORAVA_MOLTENVK_DIR "${MORAVA_MOLTENVK_LIBRARY}" DIRECTORY)
        find_file(MORAVA_MOLTENVK_ICD MoltenVK_icd.json HINTS "${MORAVA_MOLTENVK_DIR}/../etc/vulkan/icd.d" "${MORAVA_MOLTENVK_DIR}/../share/vulkan/icd.d" NO_DEFAULT_PATH)
    endif()
    if(NOT MORAVA_MOLTENVK_LIBRARY OR NOT MORAVA_MOLTENVK_ICD)
        message(WARNING "MoltenVK or its MoltenVK_icd.json not found: the player template can't make a macOS app (brew install molten-vk)")
    else()
        file(READ "${MORAVA_MOLTENVK_ICD}" MORAVA_MOLTENVK_ICD_TEXT)
        string(REGEX REPLACE "\"library_path\"[ \t]*:[ \t]*\"[^\"]*\"" "\"library_path\": \"../../../Frameworks/libMoltenVK.dylib\""
            MORAVA_MOLTENVK_ICD_TEXT "${MORAVA_MOLTENVK_ICD_TEXT}")
        file(WRITE "${CMAKE_BINARY_DIR}/MoltenVK_icd.json" "${MORAVA_MOLTENVK_ICD_TEXT}")
        set(MORAVA_FRAMEWORKS "$<TARGET_FILE_DIR:MoravaPlayer>/Frameworks")
        add_custom_command(TARGET MoravaPlayer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MORAVA_FRAMEWORKS}" "$<TARGET_FILE_DIR:MoravaPlayer>/vulkan/icd.d"
            COMMAND ${CMAKE_COMMAND} -E copy "${MORAVA_VULKAN_LOADER_FILE}" "${MORAVA_FRAMEWORKS}/libvulkan.1.dylib"
            COMMAND ${CMAKE_COMMAND} -E copy "${MORAVA_MOLTENVK_FILE}" "${MORAVA_FRAMEWORKS}/libMoltenVK.dylib"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/MoltenVK_icd.json" "$<TARGET_FILE_DIR:MoravaPlayer>/vulkan/icd.d/MoltenVK_icd.json"
            COMMAND chmod u+w "${MORAVA_FRAMEWORKS}/libvulkan.1.dylib" "${MORAVA_FRAMEWORKS}/libMoltenVK.dylib"
            COMMAND install_name_tool -id @rpath/libvulkan.1.dylib "${MORAVA_FRAMEWORKS}/libvulkan.1.dylib"
            COMMAND install_name_tool -id @rpath/libMoltenVK.dylib "${MORAVA_FRAMEWORKS}/libMoltenVK.dylib"
            COMMAND install_name_tool -change "${MORAVA_VULKAN_LOADER_ID}" @rpath/libvulkan.1.dylib "$<TARGET_FILE:MoravaPlayer>"
            COMMAND codesign --force --sign - "${MORAVA_FRAMEWORKS}/libvulkan.1.dylib" "${MORAVA_FRAMEWORKS}/libMoltenVK.dylib" "$<TARGET_FILE:MoravaPlayer>"
            COMMENT "MoravaPlayer: the Vulkan loader and MoltenVK beside it, for a macOS app"
            VERBATIM
        )
    endif()

    # Their license notices go with the game: the files Homebrew keeps with each (11.5)
    foreach(entry "Vulkan Loader|${MORAVA_VULKAN_LOADER_FILE}" "MoltenVK|${MORAVA_MOLTENVK_FILE}")
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(GET parts 1 file)
        get_filename_component(keg "${file}" DIRECTORY)
        file(GLOB licenses "${keg}/../LICENSE*" "${keg}/../share/doc/*/LICENSE*")
        if(licenses)
            list(GET licenses 0 license)
            list(APPEND MORAVA_EXTRA_NOTICES "${name}|${license}")
        else()
            message(WARNING "License notices: ${name}: no LICENSE file beside '${file}': the macOS game would ship without its notice")
        endif()
    endforeach()
endif()

# =========================================================
# POST BUILD
# =========================================================

# Models, Shaders, Textures etc. are not copied: the app runs with the source folder as its
# working directory (VS_DEBUGGER_WORKING_DIRECTORY below) and loads them from there.

# mono ships Debug and Release builds only; RelWithDebInfo and MinSizeRel use the Release one
if(MORAVA_MONO)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MONO_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/mono-2.0-sgen.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# The license notices a game ships with, beside the player: Build Game copies them with it (11.5). The extra ones (macOS:
# Vulkan's loader and MoltenVK) by absolute path, joined with ^^ (a ; would split the command's argument)
string(REPLACE ";" "^^" MORAVA_EXTRA_NOTICES_TEXT "${MORAVA_EXTRA_NOTICES}")
if(TARGET MoravaPlayer AND MORAVA_SCRIPTING)
    add_custom_command(TARGET MoravaPlayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}" "-DDOTNET_DIR=${MORAVA_DOTNET_INSTALL}"
            "-DEXTRA_NOTICES=${MORAVA_EXTRA_NOTICES_TEXT}"
            "-DOUTPUT=$<TARGET_FILE_DIR:MoravaPlayer>/THIRD_PARTY_NOTICES.txt" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ThirdPartyNotices.cmake"
        VERBATIM
    )
endif()

# C# scripting: next to the executables (the folder they share: per configuration in Visual Studio, the build folder with
# Ninja) as in a shipped game. Morava.ScriptCore.dll with its runtimeconfig.json, built from ScriptCore/ when a source
# changed (dotnet's obj/ and bin/ in the build folder, not in the sources); the private .NET (dotnet/: hostfxr and the base
# runtime, about 75 MB, copied once per version); nethost.dll on Windows
if(MORAVA_SCRIPTING)
    if(CMAKE_CONFIGURATION_TYPES)
        set(MORAVA_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
    else()
        set(MORAVA_OUTPUT_DIR "${CMAKE_BINARY_DIR}")
    endif()

    file(GLOB MORAVA_SCRIPT_CORE_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/ScriptCore/*.cs
        ${CMAKE_CURRENT_SOURCE_DIR}/ScriptCore/*.csproj
    )
    if(MORAVA_WARNINGS_AS_ERRORS)
        set(MORAVA_DOTNET_WARNINGS_AS_ERRORS true)
    else()
        set(MORAVA_DOTNET_WARNINGS_AS_ERRORS false)
    endif()
    # touch: dotnet leaves an unchanged DLL's time as it was, and the command would run again on every build
    add_custom_command(
        OUTPUT "${MORAVA_OUTPUT_DIR}/Morava.ScriptCore.dll"
        COMMAND "${MORAVA_DOTNET_EXE}" build "${CMAKE_CURRENT_SOURCE_DIR}/ScriptCore/Morava.ScriptCore.csproj"
            -c $<IF:$<CONFIG:Debug>,Debug,Release> --nologo -v q
            --artifacts-path "${CMAKE_BINARY_DIR}/ScriptCore" -o "${MORAVA_OUTPUT_DIR}"
            -p:TreatWarningsAsErrors=${MORAVA_DOTNET_WARNINGS_AS_ERRORS}
        COMMAND ${CMAKE_COMMAND} -E touch "${MORAVA_OUTPUT_DIR}/Morava.ScriptCore.dll"
        DEPENDS ${MORAVA_SCRIPT_CORE_SOURCES}
        COMMENT "Building Morava.ScriptCore (C#)"
        VERBATIM
    )

    set(MORAVA_PRIVATE_DOTNET_STAMP "${MORAVA_OUTPUT_DIR}/dotnet/.copied-${MORAVA_DOTNET_RUNTIME_VERSION}-${MORAVA_HOSTFXR_VERSION}")
    add_custom_command(
        OUTPUT "${MORAVA_PRIVATE_DOTNET_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${MORAVA_HOSTFXR_DIR}"
            "${MORAVA_OUTPUT_DIR}/dotnet/host/fxr/${MORAVA_HOSTFXR_VERSION}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${MORAVA_DOTNET_RUNTIME_DIR}"
            "${MORAVA_OUTPUT_DIR}/dotnet/shared/Microsoft.NETCore.App/${MORAVA_DOTNET_RUNTIME_VERSION}"
        COMMAND ${CMAKE_COMMAND} -E touch "${MORAVA_PRIVATE_DOTNET_STAMP}"
        COMMENT "Copying the private .NET ${MORAVA_DOTNET_RUNTIME_VERSION} next to the executables"
        VERBATIM
    )

    set(MORAVA_SCRIPTING_OUTPUTS "${MORAVA_OUTPUT_DIR}/Morava.ScriptCore.dll" "${MORAVA_PRIVATE_DOTNET_STAMP}")
    if(WIN32)
        add_custom_command(
            OUTPUT "${MORAVA_OUTPUT_DIR}/nethost.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MORAVA_NETHOST_DIR}/nethost.dll" "${MORAVA_OUTPUT_DIR}"
            DEPENDS "${MORAVA_NETHOST_DIR}/nethost.dll"
            VERBATIM
        )
        list(APPEND MORAVA_SCRIPTING_OUTPUTS "${MORAVA_OUTPUT_DIR}/nethost.dll")
    endif()

    add_custom_target(MoravaScriptCore DEPENDS ${MORAVA_SCRIPTING_OUTPUTS} SOURCES ${MORAVA_SCRIPT_CORE_SOURCES})
    # The editor builds a project's scripts with the dotnet on the PATH, or in DOTNET_ROOT, else with this one (4.7)
    if(TARGET ${PROJECT_NAME})
        target_compile_definitions(${PROJECT_NAME} PRIVATE "MORAVA_DOTNET_SDK_EXE=\"${MORAVA_DOTNET_EXE}\"")
    endif()
    foreach(tgt ${PROJECT_NAME} MoravaPlayer)
        if(TARGET ${tgt})
            add_dependencies(${tgt} MoravaScriptCore)
        endif()
    endforeach()

    # The projects' scripts (Projects/<Name>/Scripts/<Name>.csproj: Sample, Racing), which their scenes use: built after
    # ScriptCore into the project's Scripts/bin (where <Name>.mproject's ScriptAssembly points) when a .cs or the .csproj
    # changed. Target Morava<Name>Scripts. The smoke tests play the sample with them
    foreach(MORAVA_SCRIPT_PROJECT Sample Racing)
        set(MORAVA_PROJECT_SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Projects/${MORAVA_SCRIPT_PROJECT}/Scripts")
        if(NOT EXISTS "${MORAVA_PROJECT_SCRIPTS_DIR}/${MORAVA_SCRIPT_PROJECT}.csproj")
            continue()
        endif()
        set(MORAVA_PROJECT_DLL "${MORAVA_PROJECT_SCRIPTS_DIR}/bin/${MORAVA_SCRIPT_PROJECT}.dll")
        file(GLOB MORAVA_PROJECT_SCRIPT_SOURCES CONFIGURE_DEPENDS
            ${MORAVA_PROJECT_SCRIPTS_DIR}/*.cs
            ${MORAVA_PROJECT_SCRIPTS_DIR}/*.csproj
        )
        add_custom_command(
            OUTPUT "${MORAVA_PROJECT_DLL}"
            COMMAND "${MORAVA_DOTNET_EXE}" build "${MORAVA_PROJECT_SCRIPTS_DIR}/${MORAVA_SCRIPT_PROJECT}.csproj"
                -c $<IF:$<CONFIG:Debug>,Debug,Release> --nologo -v q
                "-p:MoravaScriptCoreDir=${MORAVA_OUTPUT_DIR}/"
                -p:TreatWarningsAsErrors=${MORAVA_DOTNET_WARNINGS_AS_ERRORS}
            COMMAND ${CMAKE_COMMAND} -E touch "${MORAVA_PROJECT_DLL}"
            DEPENDS ${MORAVA_PROJECT_SCRIPT_SOURCES} "${MORAVA_OUTPUT_DIR}/Morava.ScriptCore.dll"
            COMMENT "Building the ${MORAVA_SCRIPT_PROJECT} project's scripts (C#)"
            VERBATIM
        )
        add_custom_target(Morava${MORAVA_SCRIPT_PROJECT}Scripts DEPENDS "${MORAVA_PROJECT_DLL}" SOURCES ${MORAVA_PROJECT_SCRIPT_SOURCES})
        add_dependencies(Morava${MORAVA_SCRIPT_PROJECT}Scripts MoravaScriptCore)
        foreach(tgt ${PROJECT_NAME} MoravaPlayer)
            if(TARGET ${tgt})
                add_dependencies(${tgt} Morava${MORAVA_SCRIPT_PROJECT}Scripts)
            endif()
        endforeach()
    endforeach()
endif()

# =========================================================
# APPLY MSVC RUNTIME TO EVERY TARGET (explicit, overrides vendor defaults)
# =========================================================

function(morava_collect_targets dir out)
    get_property(targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirs})
        morava_collect_targets(${subdir} sub_targets)
        list(APPEND targets ${sub_targets})
    endforeach()
    set(${out} ${targets} PARENT_SCOPE)
endfunction()

if(MSVC)
    morava_collect_targets(${CMAKE_CURRENT_SOURCE_DIR} MORAVA_ALL_TARGETS)
    foreach(tgt ${MORAVA_ALL_TARGETS})
        get_target_property(tgt_type ${tgt} TYPE)
        if(tgt_type MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY|EXECUTABLE)$")
            set_target_properties(${tgt} PROPERTIES MSVC_RUNTIME_LIBRARY ${MORAVA_MSVC_RUNTIME})

            # Compile the files within each project in parallel (MSBuild --parallel only runs projects in parallel)
            target_compile_options(${tgt} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/MP>)
        endif()
    endforeach()
endif()

message(STATUS "MSVC runtime for all targets: ${MORAVA_MSVC_RUNTIME}")

# =========================================================
# STARTUP PROJECT
# =========================================================

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)

# Both executables run with the source folder as their working directory (Models, Shaders, Textures...)
foreach(tgt ${PROJECT_NAME} MoravaPlayer)
    if(TARGET ${tgt})
        set_target_properties(${tgt} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    endif()
endforeach()

message(STATUS "MoravaEngine configured successfully")
