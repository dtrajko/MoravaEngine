#include "PlayerApplication.h"

#include "PlayerLayer.h"

#include "EnvMapVulkan/EnvMapVulkanRenderer.h"


SceneProperties PlayerApplication::CreateScene()
{
	return { nullptr, nullptr, SceneName::ENV_MAP_VULKAN };
}

H2M::RendererAPI_H2M* PlayerApplication::CreateRendererAPI()
{
	return new EnvMapVulkanRenderer();
}

void PlayerApplication::PushLayers(Application& application)
{
	m_PlayerLayer = new PlayerLayer();
	application.PushLayer(m_PlayerLayer);
}

void PlayerApplication::Draw()
{
	m_PlayerLayer->Draw();
}
