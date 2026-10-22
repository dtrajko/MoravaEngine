# Phase 4.1 spike: a console program that hosts .NET the way a shipped game will. It starts the runtime from a private
# .NET folder next to the executable, calls C# and is called back, and loads, unloads and reloads a game assembly.
# Not part of the engine: built and run on its own (see README.md), on Windows, Linux and macOS in CI.
cmake_minimum_required(VERSION 3.20)
project(ScriptHostSpike CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The installed .NET: DOTNET_ROOT if set (actions/setup-dotnet sets it), else the folder of the dotnet executable
if(DEFINED ENV{DOTNET_ROOT} AND EXISTS "$ENV{DOTNET_ROOT}/shared/Microsoft.NETCore.App")
    file(TO_CMAKE_PATH "$ENV{DOTNET_ROOT}" DOTNET_INSTALL)
else()
    find_program(DOTNET_EXE dotnet REQUIRED)
    file(REAL_PATH "${DOTNET_EXE}" DOTNET_EXE_REAL)
    get_filename_component(DOTNET_INSTALL "${DOTNET_EXE_REAL}" DIRECTORY)
endif()
find_program(DOTNET_EXE dotnet HINTS "${DOTNET_INSTALL}" REQUIRED)

# The newest 10.x of a folder of versions (host/fxr, shared/Microsoft.NETCore.App, the host pack)
set(SPIKE_DOTNET_MAJOR 10)
function(newest_version folder out)
    file(GLOB versions RELATIVE "${folder}" "${folder}/${SPIKE_DOTNET_MAJOR}.*")
    if(NOT versions)
        message(FATAL_ERROR "No .NET ${SPIKE_DOTNET_MAJOR} in ${folder}")
    endif()
    list(SORT versions COMPARE NATURAL ORDER DESCENDING)
    list(GET versions 0 newest)
    set(${out} "${newest}" PARENT_SCOPE)
endfunction()

if(WIN32)
    set(DOTNET_RID win-x64)
elseif(APPLE)
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(DOTNET_RID osx-arm64)
    else()
        set(DOTNET_RID osx-x64)
    endif()
else()
    set(DOTNET_RID linux-x64)
endif()

set(HOST_PACK "${DOTNET_INSTALL}/packs/Microsoft.NETCore.App.Host.${DOTNET_RID}")
newest_version("${HOST_PACK}" HOST_PACK_VERSION)
set(NETHOST_DIR "${HOST_PACK}/${HOST_PACK_VERSION}/runtimes/${DOTNET_RID}/native")
newest_version("${DOTNET_INSTALL}/host/fxr" HOSTFXR_VERSION)
newest_version("${DOTNET_INSTALL}/shared/Microsoft.NETCore.App" RUNTIME_VERSION)
message(STATUS ".NET: ${DOTNET_INSTALL}, runtime ${RUNTIME_VERSION}, hostfxr ${HOSTFXR_VERSION}, nethost ${NETHOST_DIR}")

# Everything lands in one folder, as a shipped game: the executable, SpikeCore.dll, game/, dotnet/
set(SPIKE_OUT "${CMAKE_BINARY_DIR}/out")

add_executable(ScriptHostSpike ScriptHostSpike.cpp)
set_target_properties(ScriptHostSpike PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${SPIKE_OUT}>")
target_include_directories(ScriptHostSpike PRIVATE "${NETHOST_DIR}")
# Windows: nethost.dll, not the static libnethost.lib, which is built with the static release CRT (/MT) only and clashes
# with the engine's /MD and every Debug build
if(WIN32)
    target_link_libraries(ScriptHostSpike PRIVATE "${NETHOST_DIR}/nethost.lib")
    add_custom_command(TARGET ScriptHostSpike POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NETHOST_DIR}/nethost.dll" "${SPIKE_OUT}" VERBATIM)
else()
    target_link_libraries(ScriptHostSpike PRIVATE "${NETHOST_DIR}/libnethost.a" ${CMAKE_DL_LIBS})
endif()
if(MSVC)
    target_compile_options(ScriptHostSpike PRIVATE /W4 /utf-8)
else()
    target_compile_options(ScriptHostSpike PRIVATE -Wall -Wextra)
endif()

# The private .NET: only what running needs (hostfxr and the base runtime), copied from the installed one
add_custom_target(SpikePrivateDotnet
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${DOTNET_INSTALL}/host/fxr/${HOSTFXR_VERSION}"
            "${SPIKE_OUT}/dotnet/host/fxr/${HOSTFXR_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${DOTNET_INSTALL}/shared/Microsoft.NETCore.App/${RUNTIME_VERSION}"
            "${SPIKE_OUT}/dotnet/shared/Microsoft.NETCore.App/${RUNTIME_VERSION}"
    VERBATIM)

# SpikeCore next to the executable; the game twice (version 1 and 2), for the reload test
set(MANAGED "${CMAKE_CURRENT_SOURCE_DIR}/Managed")
add_custom_target(SpikeManaged
    COMMAND "${DOTNET_EXE}" build "${MANAGED}/SpikeCore/SpikeCore.csproj" -c Release --nologo -v q -o "${SPIKE_OUT}"
    COMMAND "${DOTNET_EXE}" build "${MANAGED}/SpikeGame/SpikeGame.csproj" -c Release --nologo -v q
            -p:SpikeGameVersion=1 -p:SpikeCoreDir=${SPIKE_OUT}/ -o "${SPIKE_OUT}/game_v1"
    COMMAND "${DOTNET_EXE}" build "${MANAGED}/SpikeGame/SpikeGame.csproj" -c Release --nologo -v q
            -p:SpikeGameVersion=2 -p:SpikeCoreDir=${SPIKE_OUT}/ -o "${SPIKE_OUT}/game_v2"
    VERBATIM)

add_dependencies(ScriptHostSpike SpikePrivateDotnet SpikeManaged)
