cmake_minimum_required(VERSION 3.15)

project(MoravaEngine VERSION 0.0.1 LANGUAGES C CXX)

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
    COTTAGE EIFFEL SPONZA TERRAIN PBR LEARN_OPENGL BULLET INSTANCED ASTEROIDS NANOSUIT FRAMEBUFFERS CUBEMAPS PARTICLES
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

# C# scripting (mono). The repository has mono's Windows libraries only.
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

# Vendors with an old cmake_minimum_required (e.g. bullet3: 3.5) would otherwise
# ignore CMAKE_MSVC_RUNTIME_LIBRARY
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY ${MORAVA_MSVC_RUNTIME})

# Each vendor's own runtime switch, set to match
set(USE_MSVC_RUNTIME_LIBRARY_DLL ${MORAVA_SHARED_CRT} CACHE BOOL "" FORCE)    # glfw, bullet3
set(USE_MSVC_RELEASE_RUNTIME_ALWAYS OFF CACHE BOOL "" FORCE)                  # bullet3
set(YAML_MSVC_SHARED_RT ${MORAVA_SHARED_CRT} CACHE BOOL "" FORCE)             # yaml-cpp
set(SHADERC_ENABLE_SHARED_CRT ${MORAVA_SHARED_CRT} CACHE BOOL "" FORCE)       # shaderc, glslang, spirv-tools
set(USE_STATIC_CRT ${MORAVA_MSVC_STATIC_RUNTIME} CACHE BOOL "" FORCE)         # assimp

# Engine-only defines (platform, HZ_DEBUG/HZ_RELEASE) are set on the MoravaEngine target below,
# so they don't leak into the vendor projects

# =========================================================
# ROOT PATH
# =========================================================

set(ROOT_PATH_ABS ${CMAKE_CURRENT_SOURCE_DIR}/..)
message(STATUS "ROOT_PATH_ABS = ${ROOT_PATH_ABS}")

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

# ---- Bullet ----
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)  # the engine only uses Bullet 2 (BulletDynamics, BulletCollision, LinearMath)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

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

add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/glfw glfw)
add_optional(${ROOT_PATH_ABS}/vendor/box2d box2d)
# bullet3 declares cmake_minimum_required(3.5); raise its policy version to avoid CMake's deprecation warning
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/bullet3 bullet3)
unset(CMAKE_POLICY_VERSION_MINIMUM)
add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp yaml-cpp)
add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross spirv-cross)
add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/shaderc shaderc)
if("DIRECTX11" IN_LIST MORAVA_GRAPHICS_APIS)
    add_optional(${ROOT_PATH_ABS}/vendor/DirectXTex directxtex) # loads the DirectX 11 textures
endif()
add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/assimp assimp)
add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/nativefiledialog-extended nfd)

# =========================================================
# MAIN TARGET
# =========================================================

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
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

list(LENGTH SRC MORAVA_SOURCE_COUNT)
message(STATUS "Sources compiled for ${MORAVA_GRAPHICS_APIS_TEXT}: ${MORAVA_SOURCE_COUNT}")

list(TRANSFORM SRC PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)
list(TRANSFORM HEADERS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_executable(${PROJECT_NAME} ${SRC} ${HEADERS})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8) # the sources are UTF-8 (GCC and Clang take that as given)
endif()

if(MORAVA_WARNINGS_AS_ERRORS)
    target_compile_options(${PROJECT_NAME} PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/WX,-Werror>)
endif()

# The scene (when MORAVA_SCENE sets it; otherwise src/config.h does) and the graphics APIs of the build
if(NOT MORAVA_SCENE STREQUAL "")
    target_compile_definitions(${PROJECT_NAME} PRIVATE SCENE_${MORAVA_SCENE} MORAVA_SCENE_FROM_CMAKE)
endif()
foreach(api ${MORAVA_GRAPHICS_APIS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE GRAPHICS_API_${api})
endforeach()
if(MORAVA_MONO)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MORAVA_MONO)
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HZ_PLATFORM_WINDOWS NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HZ_PLATFORM_LINUX)
endif()

# Debug: asserts and debug-only code; Release/RelWithDebInfo/MinSizeRel: HZ_RELEASE
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:HZ_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:HZ_RELEASE>
)

# Precompiled header: stable third-party and standard headers, included first in every source file
# (src/PrecompiledHeader.h; the older src/pch.h is an ordinary header some engine headers include)
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/PrecompiledHeader.h)

# Many sources #define this at their top; the precompiled header now comes first, so define it for the whole target.
# Defined empty ("=") to match those #defines exactly, otherwise MSVC warns about a redefinition (C4005).
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS=)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ROOT_PATH_ABS}/vendor
    ${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
)

# =========================================================
# INTERFACE LIBS
# =========================================================

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${ROOT_PATH_ABS}/vendor/GLM)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ${ROOT_PATH_ABS}/vendor/tinyobjloader)

add_library(entt INTERFACE)
target_include_directories(entt INTERFACE ${ROOT_PATH_ABS}/vendor/entt/include)

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${ROOT_PATH_ABS}/vendor/spdlog/include)

add_library(mono INTERFACE)
target_include_directories(mono INTERFACE ${ROOT_PATH_ABS}/vendor/mono/include)

add_library(NvidiaAftermath INTERFACE)
target_include_directories(NvidiaAftermath INTERFACE ${ROOT_PATH_ABS}/vendor/NvidiaAftermath)

add_library(Optick INTERFACE)
target_include_directories(Optick INTERFACE ${ROOT_PATH_ABS}/vendor/Optick/src)

add_library(farbot INTERFACE)
target_include_directories(farbot INTERFACE ${ROOT_PATH_ABS}/vendor/farbot)

# =========================================================
# GLEW
# =========================================================

# Built from the release sources (src/glew.c); defines the static target 'glew'
add_subdirectory(${ROOT_PATH_ABS}/vendor/GLEW ${CMAKE_BINARY_DIR}/glew)

# =========================================================
# VULKAN MEMORY ALLOCATOR
# =========================================================

add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE
    ${ROOT_PATH_ABS}/vendor/VulkanMemoryAllocator
)

# =========================================================
# IMGUI
# =========================================================

set(IMGUI_DIR ${ROOT_PATH_ABS}/vendor/ImGui)

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
    ${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include
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
    ${ROOT_PATH_ABS}/vendor/cross-platform/ImGuizmo/src/*.cpp
)

add_library(ImGuizmo STATIC ${IMGUIZMO_SRC})
target_include_directories(ImGuizmo PUBLIC
    ${ROOT_PATH_ABS}/vendor/cross-platform/ImGuizmo/src
)
target_link_libraries(ImGuizmo PUBLIC imgui)

# =========================================================
# SPIRV-CROSS (FIXED ORDER)
# =========================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    ${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross
)

if(TARGET spirv-cross-core)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        spirv-cross-core
        spirv-cross-glsl
    )
endif()

# =========================================================
# CORE LINKING
# =========================================================

set(MONO_DIR ${ROOT_PATH_ABS}/vendor/mono)

target_link_libraries(${PROJECT_NAME} PRIVATE

    imgui
    ImGuizmo

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
)

if("DIRECTX11" IN_LIST MORAVA_GRAPHICS_APIS)
    target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 d3dcompiler)
endif()

if(MORAVA_MONO)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        mono
        debug ${MONO_DIR}/lib/Debug/mono-2.0-sgen.lib
        optimized ${MONO_DIR}/lib/Release/mono-2.0-sgen.lib
    )
endif()

# =========================================================
# OPTIONAL LIBS
# =========================================================

foreach(lib box2d yaml-cpp DirectXTex)
    if(TARGET ${lib})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${lib})
    endif()
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE
    BulletDynamics
    BulletCollision
    LinearMath
)

if(TARGET assimp)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
elseif(TARGET assimp::assimp)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
else()
    message(FATAL_ERROR "Assimp target not found")
endif()

if(TARGET shaderc)
    target_link_libraries(${PROJECT_NAME} PRIVATE shaderc shaderc_util)
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

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

message(STATUS "MoravaEngine configured successfully")
