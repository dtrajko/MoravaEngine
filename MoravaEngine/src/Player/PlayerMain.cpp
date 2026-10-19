// MoravaPlayer: runs a game on MoravaCore alone, without the editor and without ImGui
// (docs/engine/Game_Engine_Runtime_Guide.html, Phase 1). So far the window and the renderer; the runtime layer that opens
// the game's scene and runs it comes next (1.3).

#include "Core/Application.h"
#include "Core/Log.h"

#include "H2M/Core/LogH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "EnvMapVulkan/EnvMapVulkanRenderer.h"

#include <memory>


// The player on the application: the EnvMapVulkan renderer, no older scene class, no ImGui
class PlayerApplication : public ApplicationClient
{
public:
	virtual SceneProperties CreateScene() override
	{
		return { nullptr, nullptr, SceneName::ENV_MAP_VULKAN };
	}

	virtual H2M::RendererAPI_H2M* CreateRendererAPI() override
	{
		return new EnvMapVulkanRenderer();
	}

	virtual void Draw() override
	{
		EnvMapVulkanRenderer::Draw(nullptr);
	}
};

int main(int argc, char** argv)
{
	Log::Init();
	H2M::LogH2M::Init(); // H2M_ERROR / H2M_CORE_ERROR and failed H2M asserts log through these loggers

	if (!Application::IsWorkingDirectoryValid())
	{
		return 1;
	}

	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::Vulkan);

	Application::Get()->SetClient(std::make_unique<PlayerApplication>());
	Application::Get()->SetCommandLine(ApplicationCommandLine::Parse(argc, argv)); // the options: see ApplicationCommandLine::Parse
	if (Application::Get()->GetCommandLine().NoRayTracing)
	{
		H2M::VulkanPhysicalDeviceH2M::DisableRayTracing(); // before the window creates the Vulkan device
	}

	Application::Get()->InitWindow(WindowSpecification("Morava Player", 1280, 720));

	Application::Get()->Run();

	return 0;
}
