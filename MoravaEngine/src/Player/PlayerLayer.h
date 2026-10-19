#pragma once

#include "H2M/Core/LayerH2M.h"

#include "Camera/Camera.h"

#include "EnvMapVulkan/EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Game/GameLoop.h"


/**
 * The game the player runs (docs/engine/Game_Engine_Runtime_Guide.html, 1.3): the scene given with --scene, seen through
 * its camera entity, over the whole window, run by the game loop. Opening the scene fails: the player closes with exit
 * code 1. The project (1.6) will name the start scene instead.
 */
class PlayerLayer : public H2M::LayerH2M
{
public:
	PlayerLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnUpdate(H2M::TimestepH2M ts) override;
	virtual void OnEvent(H2M::EventH2M& event) override {}
	virtual void OnImGuiRender() override {}

	// The frame, after the update (ApplicationClient::Draw)
	void Draw();

private:
	EnvMapVulkanScene m_Scene;
	EnvMapVulkanRenderSettings m_RenderSettings;
	Camera m_Camera;
	GameLoop m_GameLoop;
	bool m_Playing = false; // the scene is open
};
