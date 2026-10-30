#!/bin/bash
# Compiles source files with GCC for errors only (-fsyntax-only), as the Linux CI build would: GCC rejects some code
# MSVC accepts (comparing two different enum types, for one). Run in WSL from the repository root:
#
#   wsl bash Tools/Testing/gcc-check.sh MoravaEngine/src/UI/DebugOverlay.cpp MoravaEngine/src/Player/PlayerLayer.cpp
#
# (from Git Bash: MSYS_NO_PATHCONV=1 wsl bash ...). Paths are relative to the repository root. Prints each file and its
# errors; exit code 1 when any file has one. Warnings are left out: the old headers have many.

cd "$(dirname "$0")/../.." || exit 2
repo=$(pwd)
cd MoravaEngine || exit 2

v=../vendor
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

failed=0
for file in "$@"; do
	file=${file#MoravaEngine/}
	echo "== $file"
	errors=$(g++ -std=c++17 -fsyntax-only -w -DGRAPHICS_API_VULKAN -DMORAVA_SCRIPTING "${includes[@]}" "$repo/MoravaEngine/$file" 2>&1 | grep -E "error" | head -10)
	if [ -n "$errors" ]; then
		echo "$errors"
		failed=1
	fi
done
exit $failed
