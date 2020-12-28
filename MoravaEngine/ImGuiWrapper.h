#pragma once

#define _CRT_NONSTDC_NO_DEPRECATE

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include "CommonValues.h"
#include "Window.h"


enum class PropertyFlag
{
	None = 0, ColorProperty = 1, DragProperty = 2, SliderProperty = 4
};

class ImGuiWrapper
{
public:
	static void Init(Window* window);
	static void Begin();
	static void End();
	static void Cleanup();

	static bool ImGuiWrapper::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

	// ImGui UI helpers
	static bool Property(const std::string& name, bool& value);
	static bool Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	static bool Property(const std::string& name, glm::vec2& value, PropertyFlag flags);
	static bool Property(const std::string& name, glm::vec2& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	static bool Property(const std::string& name, glm::vec3& value, PropertyFlag flags);
	static bool Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	static bool Property(const std::string& name, glm::vec4& value, PropertyFlag flags);
	static bool Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);

	static bool Property(const char* label, const char* value);
	static bool Property(const char* label, int& value);
	static bool Property(const char* label, float& value, float delta = 0.1f);
	static bool Property(const char* label, glm::vec2& value, float delta = 0.1f);

	static void PushID();
	static void PopID();
	static void BeginPropertyGrid();

private:
	static Window* m_Window;
	static float m_Time;

};
