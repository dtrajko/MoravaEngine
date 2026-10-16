#pragma once

// Precompiled header for the MoravaEngine target.
// CMake (target_precompile_headers) includes it automatically at the top of every source file.
//
// Keep it to stable third-party and standard library headers:
// - no engine headers: editing one would rebuild the whole engine
//   (pch.h is a different, older file: some engine headers include it to pull in engine classes)
// - no headers that source files configure with #defines before including them
//   (GLFW with GLFW_INCLUDE_VULKAN, windows.h, stb, VMA, tinyobjloader, glm/gtx with GLM_ENABLE_EXPERIMENTAL)

// OpenGL loader first: GLEW must be included before any other OpenGL header
#include <GL/glew.h>

// C++ standard library
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// GLM (core and gtc extensions)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Logging (included by Core/Log.h). Not in GCC builds: fmt switches on #pragma GCC optimize("Og") for its own
// code, and a GCC precompiled header holding it leaves __OPTIMIZE__ defined in every source file of a debug build
// (the SSE intrinsics then take their optimized forms, which stb_image can't use without optimization).
#if !defined(__GNUC__) || defined(__clang__)
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#endif

// Dear ImGui
#include "imgui.h"
