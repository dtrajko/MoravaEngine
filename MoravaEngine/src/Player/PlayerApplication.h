#pragma once

#include "Core/Application.h"


class PlayerLayer;

// The player on the application: the EnvMapVulkan renderer and the PlayerLayer; no older scene class, no ImGui
class PlayerApplication : public ApplicationClient
{
public:
	virtual SceneProperties CreateScene() override;
	virtual H2M::RendererAPI_H2M* CreateRendererAPI() override;
	virtual void PushLayers(Application& application) override;
	virtual void Draw() override;

private:
	PlayerLayer* m_PlayerLayer = nullptr; // the application's layer stack owns it
};
