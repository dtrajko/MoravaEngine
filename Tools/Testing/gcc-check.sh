#!/bin/bash
# Compiles source files with GCC for errors only (-fsyntax-only), as the Linux CI build would: GCC rejects some code
# MSVC accepts (comparing two different enum types, for one). Run in WSL from the repository root:
#
#   wsl bash Tools/Testing/gcc-check.sh src/UI/DebugOverlay.cpp src/Player/PlayerLayer.cpp
#
# As the Vulkan build compiles them; --api opengl (or directx11) first: as that build does, for code shared by all the
# graphics APIs (src/Core and the like): it must compile, and include no header of the Vulkan-only sources (EnvMapVulkan/
# and every path with "Vulkan"): their functions aren't linked in that build.
#
# (from Git Bash: MSYS_NO_PATHCONV=1 wsl bash ...). Paths are relative to the repository root. Prints each file and its
# errors; exit code 1 when any file has one. Warnings are left out: the old headers have many.

cd "$(dirname "$0")/../.." || exit 2
repo=$(pwd)

v=vendor
includes=(
	-Isrc
	-I$v -I$v/GLM -I$v/entt/include -I$v/spdlog/include -I$v/ImGui -I$v/box2d/include -I$v/GLEW/include
	-I$v/VulkanMemoryAllocator -I$v/Optick/src -I$v/farbot -I$v/choc
	-I$v/cross-platform/glfw/include -I$v/cross-platform/miniaudio -I$v/cross-platform/JoltPhysics
	-I$v/cross-platform/yaml-cpp/include -I$v/cross-platform/assimp/include -Ibuild/assimp/include
	-I$v/cross-platform/ImGuizmo/src
)
# nethost.h (C# scripting): from the Windows .NET SDK's app host pack, whichever version is installed
nethost=$(ls -d "/mnt/c/Program Files/dotnet/packs/Microsoft.NETCore.App.Host.win-x64/"*/runtimes/win-x64/native 2>/dev/null | tail -1)
[ -n "$nethost" ] && includes+=("-I$nethost")

api=VULKAN
if [ "$1" = "--api" ]; then
	api=$(echo "$2" | tr '[:lower:]' '[:upper:]')
	shift 2
fi

failed=0
for file in "$@"; do
	echo "== $file"
	errors=$(g++ -std=c++17 -fsyntax-only -w -DGRAPHICS_API_$api -DMORAVA_SCRIPTING "${includes[@]}" "$repo/$file" 2>&1 | grep -E "error" | head -10)
	# Built for another API, the Vulkan-only sources (every path with "Vulkan", as CMakeLists.txt sorts them) aren't
	# compiled: a header of theirs included here compiles, but its functions are missing when the build links
	if [ "$api" != "VULKAN" ] && [ -z "$errors" ]; then
		vulkanOnly=$(g++ -std=c++17 -MM -w -DGRAPHICS_API_$api -DMORAVA_SCRIPTING "${includes[@]}" "$repo/$file" 2>/dev/null |
			tr ' \\' '\n\n' | grep -E '^src/.*Vulkan' | sort -u)
		if [ -n "$vulkanOnly" ]; then
			errors=$(echo "$vulkanOnly" | sed 's/^/Vulkan-only header included (not linked without Vulkan): /')
		fi
	fi
	if [ -n "$errors" ]; then
		echo "$errors"
		failed=1
	fi
done
exit $failed
