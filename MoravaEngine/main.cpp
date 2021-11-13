#include "main.h"


void SelectRendererAPI()
{
#if defined(SCENE_HAZEL_VULKAN) 
	Hazel::RendererAPIHazelLegacy::SetAPI(Hazel::RendererAPITypeHazelLegacy::Vulkan);
#elif defined(SCENE_DX11) 
	Hazel::RendererAPIHazelLegacy::SetAPI(Hazel::RendererAPITypeHazelLegacy::DX11);
#else:
	Hazel::RendererAPIHazelLegacy::SetAPI(Hazel::RendererAPITypeHazelLegacy::OpenGL);
#endif;
}


int main()
{
	Log::Init();

	SelectRendererAPI();

	Application::Get()->InitWindow(WindowProps("Morava Engine", 1280, 720));

	Application::Get()->Run();

	return 0;
}
