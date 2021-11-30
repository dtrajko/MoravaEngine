#define _CRT_SECURE_NO_WARNINGS

#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifndef  NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif // !_WIN32


#include "config.h"

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/Events/EventH2M.h"

#include "Camera/CameraController.h"
#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/MousePicker.h"
#include "Core/Profiler.h"
#include "Core/Timer.h"
#include "ImGui/ImGuiWrapper.h"
#include "Light/LightManager.h"
#include "Platform/Windows/WindowsWindow.h"

#include <GL/glew.h>

#include <memory>
