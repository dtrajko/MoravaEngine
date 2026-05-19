# =========================================================
# OPTION A - ADD_SUBDIRECTORY DEPENDENCY MODEL
# =========================================================

cmake_minimum_required(VERSION 3.15)

project(MoravaEngine VERSION 0.0.1 LANGUAGES C CXX)

# =========================================================
# PLATFORM
# =========================================================

if(WIN32)
    add_compile_definitions(HZ_PLATFORM_WINDOWS)
elseif(UNIX)
    add_compile_definitions(HZ_PLATFORM_LINUX)
endif()

if(WIN32)
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

add_compile_definitions(HZ_DEBUG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(
        /std:c++17
    )
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# =========================================================
# ROOT
# =========================================================

get_filename_component(ROOT_PATH_ABS ".." ABSOLUTE)

message(STATUS "ROOT_PATH_ABS = ${ROOT_PATH_ABS}")

# =========================================================
# THIRD PARTY OPTIONS
# =========================================================

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# =========================================================
# GLFW
# =========================================================

if(EXISTS ${ROOT_PATH_ABS}/vendor/cross-platform/glfw/CMakeLists.txt)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        ${ROOT_PATH_ABS}/vendor/cross-platform/glfw
        ${CMAKE_BINARY_DIR}/vendor/glfw
    )

endif()

# =========================================================
# BOX2D
# =========================================================

if(EXISTS ${ROOT_PATH_ABS}/vendor/box2d/CMakeLists.txt)

    set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
    set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        ${ROOT_PATH_ABS}/vendor/box2d
        ${CMAKE_BINARY_DIR}/vendor/box2d
    )

endif()

# =========================================================
# BULLET
# =========================================================

if(EXISTS ${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/CMakeLists.txt)

    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        ${ROOT_PATH_ABS}/vendor/cross-platform/bullet3
        ${CMAKE_BINARY_DIR}/vendor/bullet3
    )

endif()

# =========================================================
# ASSIMP
# =========================================================

if(EXISTS ${ROOT_PATH_ABS}/vendor/cross-platform/assimp/CMakeLists.txt)

    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        ${ROOT_PATH_ABS}/vendor/cross-platform/assimp
        ${CMAKE_BINARY_DIR}/vendor/assimp
    )

endif()

# =========================================================
# YAML-CPP
# =========================================================

if(EXISTS ${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/CMakeLists.txt)

    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        ${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp
        ${CMAKE_BINARY_DIR}/vendor/yaml-cpp
    )

endif()

# =========================================================
# SPIRV-CROSS
# =========================================================

if(EXISTS ${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/CMakeLists.txt)

    set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)

    add_subdirectory(
        ${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross
        ${CMAKE_BINARY_DIR}/vendor/SPIRV-Cross
    )

endif()

# =========================================================
# SHADERC
# =========================================================

if(EXISTS ${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/CMakeLists.txt)

    set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
    set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)

    add_subdirectory(
        ${ROOT_PATH_ABS}/vendor/cross-platform/shaderc
        ${CMAKE_BINARY_DIR}/vendor/shaderc
    )

endif()

# =========================================================
# DIRECTXTEX
# =========================================================

if(EXISTS ${ROOT_PATH_ABS}/vendor/DirectXTex/CMakeLists.txt)

    add_subdirectory(
        ${ROOT_PATH_ABS}/vendor/DirectXTex
        ${CMAKE_BINARY_DIR}/vendor/DirectXTex
    )

endif()

# =========================================================
# HEADER ONLY LIBS
# =========================================================

# ---------------- GLM ----------------

add_library(glm INTERFACE)

target_include_directories(glm INTERFACE
    ${ROOT_PATH_ABS}/vendor/GLM
)

# ---------------- ENTT ----------------

add_library(entt INTERFACE)

target_include_directories(entt INTERFACE
    ${ROOT_PATH_ABS}/vendor/entt/include
)

# ---------------- SPDLOG ----------------

add_library(spdlog INTERFACE)

target_include_directories(spdlog INTERFACE
    ${ROOT_PATH_ABS}/vendor/spdlog/include
)

# ---------------- TINYOBJLOADER ----------------

add_library(tinyobjloader INTERFACE)

target_include_directories(tinyobjloader INTERFACE
    ${ROOT_PATH_ABS}/vendor/tinyobjloader
)

# ---------------- GLEW ----------------

set(GLEW_DIR ${ROOT_PATH_ABS}/vendor/GLEW)

add_library(glew INTERFACE)

target_include_directories(glew INTERFACE
    ${GLEW_DIR}/include
)

target_link_libraries(glew INTERFACE
    ${GLEW_DIR}/lib/glew32s.lib
)

target_compile_definitions(glew INTERFACE GLEW_STATIC)

# ---------------- VMA ----------------

add_library(VulkanMemoryAllocator INTERFACE)

target_include_directories(VulkanMemoryAllocator INTERFACE
    ${ROOT_PATH_ABS}/vendor/VulkanMemoryAllocator/include
)

# ---------------- MONO ----------------

add_library(mono INTERFACE)

target_include_directories(mono INTERFACE
    ${ROOT_PATH_ABS}/vendor/mono/include
)

# ---------------- PHYSX ----------------

add_library(PhysX INTERFACE)

target_include_directories(PhysX INTERFACE
    ${ROOT_PATH_ABS}/vendor/PhysX/include
)

# ---------------- NVIDIA AFTERMATH ----------------

add_library(NvidiaAftermath INTERFACE)

target_include_directories(NvidiaAftermath INTERFACE
    ${ROOT_PATH_ABS}/vendor/NvidiaAftermath
)

# ---------------- OPTICK ----------------

add_library(Optick INTERFACE)

target_include_directories(Optick INTERFACE
    ${ROOT_PATH_ABS}/vendor/Optick/src
)

# ---------------- FARBOT ----------------

add_library(farbot INTERFACE)

target_include_directories(farbot INTERFACE
    ${ROOT_PATH_ABS}/vendor/farbot
)

# =========================================================
# IMGUI (core)
# =========================================================

set(IMGUI_DIR ${ROOT_PATH_ABS}/vendor/ImGui)

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
)

set(IMGUI_BACKENDS
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_library(imgui STATIC
    ${IMGUI_SRC}
    ${IMGUI_BACKENDS}
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
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
# MINIAUDIO (HEADER-ONLY IN THIS PROJECT)
# =========================================================

add_library(miniaudio INTERFACE)

target_include_directories(miniaudio INTERFACE
    ${ROOT_PATH_ABS}/vendor/miniaudio/include
)

# =========================================================
# MAIN ENGINE
# =========================================================

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

add_executable(${PROJECT_NAME}
	${SRC}
    ${HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PREFIX "Source Files"
    FILES ${SRC}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PREFIX "Header Files"
    FILES ${HEADERS}
)

target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)

# =========================================================
# ENGINE INCLUDES
# =========================================================

target_include_directories(${PROJECT_NAME} PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/src

    ${ROOT_PATH_ABS}/vendor

    ${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1/Include

    ${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src

)

# =========================================================
# ENGINE LINKS
# =========================================================

target_link_libraries(${PROJECT_NAME}

    PRIVATE

    # UI
    imgui
    ImGuizmo

    # Audio
    miniaudio

    # Header-only
    glm
    entt
    spdlog
    tinyobjloader
    glew
    VulkanMemoryAllocator
    mono
    PhysX
    NvidiaAftermath
    Optick
    farbot

)

# =========================================================
# GLFW
# =========================================================

# Engine uses ONLY this:
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# =========================================================
# GLEW
# =========================================================

# Engine uses ONLY this:
target_link_libraries(${PROJECT_NAME} PRIVATE glew)

# =========================================================
# VULKAN
# =========================================================

set(VULKAN_DIR ${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${VULKAN_DIR}/Include
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${VULKAN_DIR}/Lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    vulkan-1
)

# =========================================================
# BOX2D
# =========================================================

if(TARGET box2d)
    target_link_libraries(${PROJECT_NAME} PRIVATE box2d)
endif()

# =========================================================
# BULLET
# =========================================================

if(TARGET BulletDynamics)
    target_link_libraries(${PROJECT_NAME} PRIVATE BulletDynamics)
endif()

if(TARGET BulletCollision)
    target_link_libraries(${PROJECT_NAME} PRIVATE BulletCollision)
endif()

if(TARGET LinearMath)
    target_link_libraries(${PROJECT_NAME} PRIVATE LinearMath)
endif()

# =========================================================
# ASSIMP
# =========================================================

if(TARGET assimp)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
endif()

# =========================================================
# YAML
# =========================================================

if(TARGET yaml-cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)
endif()

# =========================================================
# SPIRV CROSS
# =========================================================

if(TARGET spirv-cross-core)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-core)
endif()

if(TARGET spirv-cross-cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-cpp)
endif()

if(TARGET spirv-cross-glsl)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-glsl)
endif()

if(TARGET spirv-cross-hlsl)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-hlsl)
endif()

if(TARGET spirv-cross-msl)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-msl)
endif()

if(TARGET spirv-cross-reflect)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-reflect)
endif()

if(TARGET spirv-cross-util)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-util)
endif()

# =========================================================
# SHADERC
# =========================================================

if(TARGET shaderc)
    target_link_libraries(${PROJECT_NAME} PRIVATE shaderc)
endif()

if(TARGET shaderc_util)
    target_link_libraries(${PROJECT_NAME} PRIVATE shaderc_util)
endif()

# =========================================================
# DIRECTXTEX
# =========================================================

if(TARGET DirectXTex)
    target_link_libraries(${PROJECT_NAME} PRIVATE DirectXTex)
endif()

# =========================================================
# VULKAN
# =========================================================

set(VULKAN_DIR ${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${VULKAN_DIR}/Include
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${VULKAN_DIR}/Lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    vulkan-1
)

# =========================================================
# WINDOWS SYSTEM LIBS
# =========================================================

if(WIN32)

    target_link_libraries(${PROJECT_NAME}

        PRIVATE

        opengl32
        d3d11
        d3dcompiler

    )

endif()

# =========================================================
# STARTUP PROJECT
# =========================================================

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)

# =========================================================
# STATUS
# =========================================================

message(STATUS "Configured MoravaEngine successfully")
