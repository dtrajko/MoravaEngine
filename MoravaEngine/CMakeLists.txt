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
# MSVC RUNTIME LIBRARY (one switch for the engine and every vendor target)
#   ON  = static runtime:  /MTd (Debug), /MT (Release)
#   OFF = DLL runtime:     /MDd (Debug), /MD (Release)
# =========================================================

option(MORAVA_MSVC_STATIC_RUNTIME "Use the static MSVC runtime (/MT, /MTd) for all targets; OFF uses the DLL runtime (/MD, /MDd)" ON)

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
add_optional(${ROOT_PATH_ABS}/vendor/DirectXTex directxtex)
add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/assimp assimp)

# =========================================================
# MAIN TARGET
# =========================================================

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

add_executable(${PROJECT_NAME} ${SRC} ${HEADERS})

target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)

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

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ROOT_PATH_ABS}/vendor
    ${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1/Include
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
    ${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1/Include
)

target_sources(imgui PRIVATE
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

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
    vulkan-1
    glew

    VulkanMemoryAllocator
    mono
    NvidiaAftermath
    Optick
    farbot

    opengl32
    d3d11
    d3dcompiler

    debug ${MONO_DIR}/lib/Debug/mono-2.0-sgen.lib
    optimized ${MONO_DIR}/lib/Release/mono-2.0-sgen.lib
)

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
# VULKAN LIBS
# =========================================================

set(VULKAN_DIR ${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1)
target_link_directories(${PROJECT_NAME} PRIVATE ${VULKAN_DIR}/Lib)

# =========================================================
# POST BUILD
# =========================================================

# Models, Shaders, Textures etc. are not copied: the app runs with the source folder as its
# working directory (VS_DEBUGGER_WORKING_DIRECTORY below) and loads them from there.

# mono ships Debug and Release builds only; RelWithDebInfo and MinSizeRel use the Release one
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MONO_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/mono-2.0-sgen.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

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
