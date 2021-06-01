#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Scene/Components.h"

#include "Core/CommonValues.h"
#include "Core/Window.h"
#include "EnvMap/EnvMapMaterial.h"

#define _CRT_NONSTDC_NO_DEPRECATE

// ImGui includes
#if !defined(IMGUI_IMPL_API)
#define IMGUI_IMPL_API
#endif
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_vulkan_with_textures.h"


enum class PropertyFlag
{
	None = 0, ColorProperty = 1, DragProperty = 2, SliderProperty = 4
};

class ImGuiWrapper
{
public:
	// Moved to ImGuiLayer
	// static void Create();
	// static void OnAttach();
	// static void OnDetach();
	// static void Begin();
	// static void End();
	// static void OnImGuiRender();
	// static void Destroy();

	static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
	static void DrawMaterialUI(Hazel::Ref<EnvMapMaterial> material, Hazel::Ref<Hazel::HazelTexture2D> checkerboardTexture);

	// ImGui UI helpers
	static bool Property(const std::string& name, bool& value);
	static bool Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	static bool Property(const std::string& name, float& value, float speed, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
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

	static inline void SetViewportEnabled(bool enabled) { s_ViewportEnabled = enabled; }
	static inline void SetViewportHovered(bool viewportHovered) { s_ViewportHovered = viewportHovered; }
	static inline void SetViewportFocused(bool viewportFocused) { s_ViewportFocused = viewportFocused; }

	static inline bool const GetViewportHovered() { return s_ViewportHovered; }
	static inline bool const GetViewportFocused() { return s_ViewportFocused; }
	static bool const CanViewportReceiveEvents();

private:
	static bool s_ViewportEnabled;
	static bool s_ViewportHovered;
	static bool s_ViewportFocused;
	static bool s_CanViewportReceiveEvents;

};
