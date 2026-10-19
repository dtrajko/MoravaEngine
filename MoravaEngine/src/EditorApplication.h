#pragma once

#include "Core/Application.h"


// The editor's executable (MoravaEngine) on the application: the scene chosen in src/config.h (or by CMake's MORAVA_SCENE),
// its renderer, its layers (SceneEnvMapVulkan: the editor, src/Editor) and ImGui
class EditorApplication : public ApplicationClient
{
public:
	virtual SceneProperties CreateScene() override;
	virtual H2M::RendererAPI_H2M* CreateRendererAPI() override;
	virtual H2M::ImGuiLayerH2M* CreateImGuiLayer() override;
	virtual void PushLayers(Application& application) override;
	virtual void Draw() override;
	virtual void OnWindowResize(uint32_t width, uint32_t height) override;
};
