#pragma once

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "CommonValues.h"
#include "Window.h"


enum class PropertyFlag
{
	None = 0, ColorProperty = 1
};

class ImGuiWrapper
{
public:
	static void Init(Window* window);
	static void Begin();
	static void End();
	static void Cleanup();

	// ImGui UI helpers
	static bool Property(const std::string& name, bool& value);
	static void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	static void Property(const std::string& name, glm::vec2& value, PropertyFlag flags);
	static void Property(const std::string& name, glm::vec2& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	static void Property(const std::string& name, glm::vec3& value, PropertyFlag flags);
	static void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	static void Property(const std::string& name, glm::vec4& value, PropertyFlag flags);
	static void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);

private:
	static Window* m_Window;
	static float m_Time;
};
