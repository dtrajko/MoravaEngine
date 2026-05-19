cmake_minimum_required(VERSION 3.15)

project(MoravaEngine VERSION 0.0.1 LANGUAGES C CXX)

# =========================================================
# GLOBAL SETTINGS
# =========================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# =========================================================
# PLATFORM DEFINITIONS
# =========================================================

if(WIN32)
    add_compile_definitions(HZ_PLATFORM_WINDOWS NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(UNIX)
    add_compile_definitions(HZ_PLATFORM_LINUX)
endif()

add_compile_definitions(HZ_DEBUG)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/std:c++17)
endif()

# =========================================================
# ROOT
# =========================================================

get_filename_component(ROOT_PATH_ABS ".." ABSOLUTE)
message(STATUS "ROOT_PATH_ABS = ${ROOT_PATH_ABS}")

# =========================================================
# THIRD PARTY SUBDIRECTORIES
# =========================================================

macro(add_optional_subdir path out)
    if(EXISTS ${path}/CMakeLists.txt)
        add_subdirectory(${path} ${CMAKE_BINARY_DIR}/${out})
    endif()
endmacro()

add_optional_subdir(${ROOT_PATH_ABS}/vendor/cross-platform/glfw glfw)
add_optional_subdir(${ROOT_PATH_ABS}/vendor/box2d box2d)
add_optional_subdir(${ROOT_PATH_ABS}/vendor/cross-platform/bullet3 bullet3)
add_optional_subdir(${ROOT_PATH_ABS}/vendor/cross-platform/assimp assimp)
add_optional_subdir(${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp yaml-cpp)
add_optional_subdir(${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross spirv-cross)
add_optional_subdir(${ROOT_PATH_ABS}/vendor/cross-platform/shaderc shaderc)
add_optional_subdir(${ROOT_PATH_ABS}/vendor/DirectXTex directxtex)

# =========================================================
# INTERFACE LIBRARIES
# =========================================================

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${ROOT_PATH_ABS}/vendor/GLM)

add_library(entt INTERFACE)
target_include_directories(entt INTERFACE ${ROOT_PATH_ABS}/vendor/entt/include)

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${ROOT_PATH_ABS}/vendor/spdlog/include)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ${ROOT_PATH_ABS}/vendor/tinyobjloader)

add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE
    ${ROOT_PATH_ABS}/vendor/VulkanMemoryAllocator
)

add_library(mono INTERFACE)
target_include_directories(mono INTERFACE ${ROOT_PATH_ABS}/vendor/mono/include)

add_library(PhysX INTERFACE)
target_include_directories(PhysX INTERFACE ${ROOT_PATH_ABS}/vendor/PhysX/include)

add_library(NvidiaAftermath INTERFACE)
target_include_directories(NvidiaAftermath INTERFACE ${ROOT_PATH_ABS}/vendor/NvidiaAftermath)

add_library(Optick INTERFACE)
target_include_directories(Optick INTERFACE ${ROOT_PATH_ABS}/vendor/Optick/src)

add_library(farbot INTERFACE)
target_include_directories(farbot INTERFACE ${ROOT_PATH_ABS}/vendor/farbot)

# =========================================================
# GLEW
# =========================================================

set(GLEW_DIR ${ROOT_PATH_ABS}/vendor/GLEW)

add_library(glew INTERFACE)
target_include_directories(glew INTERFACE ${GLEW_DIR}/include)

target_link_libraries(glew INTERFACE
    ${GLEW_DIR}/lib/glew32s.lib
)

target_compile_definitions(glew INTERFACE GLEW_STATIC)

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

    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
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
# MAIN ENGINE SOURCES
# =========================================================

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

add_executable(${PROJECT_NAME} ${SRC} ${HEADERS})

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
# ENGINE INCLUDE PATHS
# =========================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ROOT_PATH_ABS}/vendor
    ${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1/Include
    ${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
)

# =========================================================
# VULKAN
# =========================================================

set(VULKAN_DIR ${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1)

target_link_directories(${PROJECT_NAME} PRIVATE ${VULKAN_DIR}/Lib)

# =========================================================
# MONO (Debug/Release split FIXED)
# =========================================================

set(MONO_DIR ${ROOT_PATH_ABS}/vendor/mono)

# =========================================================
# SINGLE LINK BLOCK (IMPORTANT FIX)
# =========================================================

target_link_libraries(${PROJECT_NAME} PRIVATE

    # Core engine libs
    imgui
    ImGuizmo

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

    # Graphics / runtime
    glfw
    vulkan-1

    # System
    opengl32
    d3d11
    d3dcompiler

    # Config-specific Mono
    $<$<CONFIG:Debug>:${MONO_DIR}/lib/Debug/mono-2.0-sgen.lib>
    $<$<CONFIG:Release>:${MONO_DIR}/lib/Release/mono-2.0-sgen.lib>
)

# =========================================================
# OPTIONAL LIBS
# =========================================================

if(TARGET box2d)
    target_link_libraries(${PROJECT_NAME} PRIVATE box2d)
endif()

if(TARGET BulletDynamics)
    target_link_libraries(${PROJECT_NAME} PRIVATE BulletDynamics)
endif()

if(TARGET BulletCollision)
    target_link_libraries(${PROJECT_NAME} PRIVATE BulletCollision)
endif()

if(TARGET LinearMath)
    target_link_libraries(${PROJECT_NAME} PRIVATE LinearMath)
endif()

if(TARGET assimp)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
endif()

if(TARGET yaml-cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)
endif()

if(TARGET DirectXTex)
    target_link_libraries(${PROJECT_NAME} PRIVATE DirectXTex)
endif()

if(TARGET spirv-cross-core)
    target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-core spirv-cross-cpp spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl spirv-cross-reflect spirv-cross-util)
endif()

if(TARGET shaderc)
    target_link_libraries(${PROJECT_NAME} PRIVATE shaderc shaderc_util)
endif()

if(TARGET glfw)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()

# =========================================================
# POST BUILD (MONO DLL)
# =========================================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MONO_DIR}/bin/$<CONFIG>/mono-2.0-sgen.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# =========================================================
# STARTUP PROJECT
# =========================================================

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)

message(STATUS "Configured MoravaEngine successfully")
