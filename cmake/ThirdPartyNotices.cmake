# The license notices a shipped game carries (docs/engine/Game_Engine_Runtime_Guide.html, 11.5): one entry for each library
# in the player. The editor's own libraries (ImGui, assimp, shaderc) don't count. Run after the player links, so its template
# has the file:
#   cmake -DSOURCE_DIR=<repository> -DDOTNET_DIR=<.NET install> -DOUTPUT=<file> -P cmake/ThirdPartyNotices.cmake
#
# An entry: name; the file with its license, relative to the repository; optionally, for a library that has it in a comment of
# its header, the text the license begins with and the text after it (default: the comment's end). A missing file fails the
# build: a game must not ship without its notice.

set(MORAVA_NOTICES
    "MoravaEngine|LICENSE"
    "Jolt Physics|vendor/cross-platform/JoltPhysics/LICENSE"
    "miniaudio|vendor/cross-platform/miniaudio/LICENSE"
    "GLFW|vendor/cross-platform/glfw/LICENSE.md"
    "GLEW|vendor/GLEW/LICENSE.txt"
    "GLM|vendor/cross-platform/glm/copying.txt"
    "EnTT|vendor/entt/LICENSE"
    "yaml-cpp|vendor/cross-platform/yaml-cpp/LICENSE"
    "spdlog|vendor/spdlog/LICENSE"
    "{fmt}|vendor/spdlog/include/spdlog/fmt/bundled/LICENSE.rst"
    "Vulkan Memory Allocator|vendor/VulkanMemoryAllocator/vk_mem_alloc.h|Copyright (c)|#ifndef AMD_VULKAN_MEMORY_ALLOCATOR_H"
    "SPIRV-Cross|vendor/cross-platform/SPIRV-Cross/LICENSE"
    "Native File Dialog Extended|vendor/cross-platform/nativefiledialog-extended/LICENSE"
    "Box2D|vendor/box2d/LICENSE"
    "msdf-atlas-gen|vendor/msdf-atlas-gen/LICENSE.txt"
    "msdfgen|vendor/msdf-atlas-gen/msdfgen/LICENSE.txt"
    "FreeType|vendor/msdf-atlas-gen/msdfgen/freetype/docs/FTL.TXT"
    "LodePNG|vendor/msdf-atlas-gen/msdfgen/lib/lodepng.cpp|Copyright (c)"
    "stb_image|src/stb_image.h|This software is available under 2 licenses"
    "bc7enc|vendor/bc7enc/LICENSE"
    "LZ4|vendor/lz4/LICENSE"
    "SMAA|vendor/smaa/LICENSE.txt"
)

set(text "Third-party software in this game\n=================================\n\nParts of the MoravaEngine's renderer are based on Hazel (https://github.com/TheCherno/Hazel), under the Apache License 2.0, whose text is the first below.\n")

foreach(entry ${MORAVA_NOTICES})
    string(REPLACE "|" ";" parts "${entry}")
    list(LENGTH parts count)
    list(GET parts 0 name)
    list(GET parts 1 file)
    set(path "${SOURCE_DIR}/${file}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "License notices: ${name}: ${file} doesn't exist (submodules checked out?)")
    endif()
    file(READ "${path}" license)
    if(count GREATER 2)
        list(GET parts 2 marker)
        string(FIND "${license}" "${marker}" begin)
        if(begin EQUAL -1)
            message(FATAL_ERROR "License notices: ${name}: '${marker}' isn't in ${file}")
        endif()
        string(SUBSTRING "${license}" ${begin} -1 license)
        set(after "*/")
        if(count GREATER 3)
            list(GET parts 3 after)
        endif()
        string(FIND "${license}" "${after}" end)
        if(NOT end EQUAL -1)
            string(SUBSTRING "${license}" 0 ${end} license)
        endif()
    endif()
    string(APPEND text "\n\n--------------------------------------------------------------------------------\n${name}\n--------------------------------------------------------------------------------\n\n${license}")
endforeach()

# .NET: the runtime the game carries, with its own list of what is in it
foreach(file LICENSE.txt ThirdPartyNotices.txt)
    set(path "${DOTNET_DIR}/${file}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "License notices: .NET: ${path} doesn't exist")
    endif()
    file(READ "${path}" license)
    string(APPEND text "\n\n--------------------------------------------------------------------------------\n.NET (${file})\n--------------------------------------------------------------------------------\n\n${license}")
endforeach()

file(WRITE "${OUTPUT}" "${text}")
