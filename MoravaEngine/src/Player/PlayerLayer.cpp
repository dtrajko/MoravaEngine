#include "PlayerLayer.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"

#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"

#include <glm/glm.hpp>

#include <string>


PlayerLayer::PlayerLayer()
	: H2M::LayerH2M("PlayerLayer")
{
}

void PlayerLayer::OnAttach()
{
	const std::string& scenePath = Application::Get()->GetCommandLine().ScenePath;
	if (scenePath.empty())
	{
		Log::GetLogger()->error("No scene to play: start the player with --scene <file.mscene>");
		Application::Get()->Close(1);
		return;
	}

	std::string name, error;
	if (!EnvMapVulkanSceneLoader::Open(scenePath, m_Scene, name, m_RenderSettings, error))
	{
		Log::GetLogger()->error("Scene '{0}' could not be opened: {1}", scenePath, error);
		Application::Get()->Close(1);
		return;
	}
	if (m_Scene.FindFirst<CameraComponent>() == NoEntity)
	{
		Log::GetLogger()->warn("Scene '{0}' has no camera: seen from the origin, looking along -Z", name);
	}
	Log::GetLogger()->info("Playing scene '{0}' from '{1}': {2} entities", name, scenePath, m_Scene.GetEntityCount());

	// The renderer draws it with the scene's render settings and a game's view (no grid, no selection, no debug view)
	EnvMapVulkanRenderer::SetScene(&m_Scene, &m_RenderSettings, nullptr);
}

void PlayerLayer::OnDetach()
{
	// The application waited for the device: no frame in flight draws the scene's models any more
	EnvMapVulkanRenderer::SetScene(nullptr, nullptr, nullptr);
	m_Scene.Clear();
}

void PlayerLayer::OnUpdate(H2M::TimestepH2M ts)
{
	// The view fills the window: the camera's aspect ratio follows it (the swapchain's size, in pixels)
	H2M::VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();
	EnvMapVulkanRenderer::SetViewportSize(swapChain.GetWidth(), swapChain.GetHeight());

	// The camera where the scene's camera entity is, with its lens
	const EnvMapVulkanEntityID cameraEntity = m_Scene.FindFirst<CameraComponent>();
	if (cameraEntity != NoEntity)
	{
		EnvMapVulkanSceneData::PlaceCamera(m_Camera, m_Scene.Get<TransformComponent>(cameraEntity));
		const float fieldOfView = m_Scene.Get<CameraComponent>(cameraEntity).FieldOfView;
		m_Camera.SetPerspectiveVerticalFOV(glm::clamp(fieldOfView, CameraComponent::MinFieldOfView, CameraComponent::MaxFieldOfView));
	}
}

void PlayerLayer::Draw()
{
	EnvMapVulkanRenderer::Draw(&m_Camera);
}
