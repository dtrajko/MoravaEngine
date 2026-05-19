cmake_minimum_required(VERSION 3.15)

project(MoravaEngine VERSION 0.0.1 LANGUAGES C CXX)

# =========================================================
# GLOBAL SETTINGS
# =========================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/std:c++17)
endif()

# =========================================================
# PLATFORM DEFINES
# =========================================================

if(WIN32)
    add_compile_definitions(HZ_PLATFORM_WINDOWS NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(UNIX)
    add_compile_definitions(HZ_PLATFORM_LINUX)
endif()

add_compile_definitions(HZ_DEBUG)

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

# ---- Bullet ----
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)

# ---- Shaderc ----
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)

# =========================================================
# OPTIONAL SUBDIRECTORY HELPER
# =========================================================

function(add_optional dir out)
    if(EXISTS ${dir}/CMakeLists.txt)
        add_subdirectory(${dir} ${CMAKE_BINARY_DIR}/${out})
    endif()
endfunction()

# =========================================================
# VENDORS (ORDER MATTERS)
# =========================================================

add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/glfw glfw)
add_optional(${ROOT_PATH_ABS}/vendor/box2d box2d)
add_optional(${ROOT_PATH_ABS}/vendor/cross-platform/bullet3 bullet3)
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

# AFTER ALL subdirectories
if(TARGET Bullet3Dynamics AND NOT TARGET BulletDynamics)
    add_library(BulletDynamics ALIAS Bullet3Dynamics)
endif()

if(TARGET Bullet3Collision AND NOT TARGET BulletCollision)
    add_library(BulletCollision ALIAS Bullet3Collision)
endif()

add_library(entt INTERFACE)
target_include_directories(entt INTERFACE ${ROOT_PATH_ABS}/vendor/entt/include)

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${ROOT_PATH_ABS}/vendor/spdlog/include)

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
    PhysX
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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Models"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Models"
)

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

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

message(STATUS "MoravaEngine configured successfully")
