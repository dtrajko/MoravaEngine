#include "main.h"


void SelectRendererAPI()
{
#if defined(SCENE_HAZEL_VULKAN) ||\
	defined(SCENE_ENV_MAP_VULKAN)
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::Vulkan);
#elif defined(SCENE_DX11) 
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::DX11);
#else:
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::OpenGL);
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
