# A player template (docs/engine/Game_Engine_Runtime_Guide.html, 11.2): what Build Game copies into a game, taken out of a
# Release build folder. CI uploads one per platform; Tools/Get-PlayerTemplates.ps1 puts them in PlayerTemplates/<platform>
# beside an editor, so it builds games for that platform too:
#   cmake -DBUILD_DIR=<folder with MoravaPlayer> -DOUTPUT=<template folder> -P cmake/PlayerTemplate.cmake
#
# The player, ScriptCore and the .NET host (Windows: nethost.dll), the license notices, the private .NET (without the
# build's stamp file) and on macOS the Vulkan loader, MoltenVK and its driver file. Not the symbols: MoravaPlayer.pdb is kept
# apart, to read a crash dump with.

if(NOT BUILD_DIR OR NOT OUTPUT)
    message(FATAL_ERROR "PlayerTemplate.cmake: -DBUILD_DIR=<build folder> -DOUTPUT=<template folder>")
endif()

file(REMOVE_RECURSE "${OUTPUT}")
file(MAKE_DIRECTORY "${OUTPUT}")

set(found FALSE)
foreach(player MoravaPlayer MoravaPlayer.exe)
    if(EXISTS "${BUILD_DIR}/${player}")
        file(COPY "${BUILD_DIR}/${player}" DESTINATION "${OUTPUT}")
        set(found TRUE)
    endif()
endforeach()
if(NOT found)
    message(FATAL_ERROR "PlayerTemplate.cmake: no MoravaPlayer in ${BUILD_DIR}")
endif()

foreach(file Morava.ScriptCore.dll Morava.ScriptCore.deps.json Morava.ScriptCore.runtimeconfig.json THIRD_PARTY_NOTICES.txt nethost.dll)
    if(EXISTS "${BUILD_DIR}/${file}")
        file(COPY "${BUILD_DIR}/${file}" DESTINATION "${OUTPUT}")
    elseif(NOT file STREQUAL "nethost.dll")
        message(FATAL_ERROR "PlayerTemplate.cmake: ${BUILD_DIR}/${file} is missing")
    endif()
endforeach()

foreach(folder dotnet Frameworks vulkan)
    if(IS_DIRECTORY "${BUILD_DIR}/${folder}")
        file(COPY "${BUILD_DIR}/${folder}" DESTINATION "${OUTPUT}" PATTERN ".copied-*" EXCLUDE)
    elseif(folder STREQUAL "dotnet")
        message(FATAL_ERROR "PlayerTemplate.cmake: ${BUILD_DIR}/dotnet (the private .NET) is missing")
    endif()
endforeach()

message(STATUS "Player template: ${OUTPUT}")
