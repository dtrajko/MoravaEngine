# The installed .NET SDK, for C# scripting (docs/engine/Game_Engine_Runtime_Guide.html, Phase 4), as the Phase 4.1 spike
# (Spikes/ScriptHost) proved it on Windows, Linux and macOS. Sets:
#   MORAVA_DOTNET_EXE           the dotnet executable (builds ScriptCore and the games' scripts)
#   MORAVA_DOTNET_INSTALL       the installed .NET: DOTNET_ROOT if set (actions/setup-dotnet sets it), else the dotnet
#                               executable's folder
#   MORAVA_HOSTFXR_DIR          host/fxr/<version> and shared/Microsoft.NETCore.App/<version> of the newest 10.x: what
#   MORAVA_DOTNET_RUNTIME_DIR   the private dotnet/ folder next to the executables is copied from
#   MORAVA_NETHOST_DIR          nethost and the hosting headers (nethost.h, hostfxr.h, coreclr_delegates.h), from the
#                               host pack of this platform

set(MORAVA_DOTNET_MAJOR 10)

if(DEFINED ENV{DOTNET_ROOT} AND EXISTS "$ENV{DOTNET_ROOT}/shared/Microsoft.NETCore.App")
    file(TO_CMAKE_PATH "$ENV{DOTNET_ROOT}" MORAVA_DOTNET_INSTALL)
else()
    find_program(MORAVA_DOTNET_FOUND_EXE dotnet)
    if(NOT MORAVA_DOTNET_FOUND_EXE)
        message(FATAL_ERROR "C# scripting needs the .NET ${MORAVA_DOTNET_MAJOR} SDK (https://dotnet.microsoft.com/download), "
            "or -DMORAVA_SCRIPTING=OFF for a build without it")
    endif()
    file(REAL_PATH "${MORAVA_DOTNET_FOUND_EXE}" MORAVA_DOTNET_EXE_REAL)
    get_filename_component(MORAVA_DOTNET_INSTALL "${MORAVA_DOTNET_EXE_REAL}" DIRECTORY)
endif()
find_program(MORAVA_DOTNET_EXE dotnet HINTS "${MORAVA_DOTNET_INSTALL}" NO_DEFAULT_PATH)
if(NOT MORAVA_DOTNET_EXE)
    message(FATAL_ERROR "No dotnet executable in ${MORAVA_DOTNET_INSTALL}")
endif()

# The newest 10.x of a folder of versions (host/fxr, shared/Microsoft.NETCore.App, the host pack)
function(morava_newest_dotnet_version folder out)
    file(GLOB versions RELATIVE "${folder}" "${folder}/${MORAVA_DOTNET_MAJOR}.*")
    if(NOT versions)
        message(FATAL_ERROR "C# scripting needs the .NET ${MORAVA_DOTNET_MAJOR} SDK: no ${MORAVA_DOTNET_MAJOR}.x in ${folder} "
            "(https://dotnet.microsoft.com/download, or -DMORAVA_SCRIPTING=OFF for a build without it)")
    endif()
    list(SORT versions COMPARE NATURAL ORDER DESCENDING)
    list(GET versions 0 newest)
    set(${out} "${newest}" PARENT_SCOPE)
endfunction()

if(WIN32)
    set(MORAVA_DOTNET_RID win-x64)
elseif(APPLE)
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(MORAVA_DOTNET_RID osx-arm64)
    else()
        set(MORAVA_DOTNET_RID osx-x64)
    endif()
else()
    set(MORAVA_DOTNET_RID linux-x64)
endif()

set(MORAVA_HOST_PACK "${MORAVA_DOTNET_INSTALL}/packs/Microsoft.NETCore.App.Host.${MORAVA_DOTNET_RID}")
morava_newest_dotnet_version("${MORAVA_HOST_PACK}" MORAVA_HOST_PACK_VERSION)
set(MORAVA_NETHOST_DIR "${MORAVA_HOST_PACK}/${MORAVA_HOST_PACK_VERSION}/runtimes/${MORAVA_DOTNET_RID}/native")
morava_newest_dotnet_version("${MORAVA_DOTNET_INSTALL}/host/fxr" MORAVA_HOSTFXR_VERSION)
set(MORAVA_HOSTFXR_DIR "${MORAVA_DOTNET_INSTALL}/host/fxr/${MORAVA_HOSTFXR_VERSION}")
morava_newest_dotnet_version("${MORAVA_DOTNET_INSTALL}/shared/Microsoft.NETCore.App" MORAVA_DOTNET_RUNTIME_VERSION)
set(MORAVA_DOTNET_RUNTIME_DIR "${MORAVA_DOTNET_INSTALL}/shared/Microsoft.NETCore.App/${MORAVA_DOTNET_RUNTIME_VERSION}")

message(STATUS "C# scripting: .NET ${MORAVA_DOTNET_RUNTIME_VERSION} (hostfxr ${MORAVA_HOSTFXR_VERSION}) from ${MORAVA_DOTNET_INSTALL}")
