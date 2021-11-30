#include "main.h"


void SelectRendererAPI()
{
#if defined(SCENE_HAZEL_VULKAN) 
	H2M::RendererAPIH2M::SetAPI(H2M::RendererAPIH2MType::Vulkan);
#elif defined(SCENE_DX11) 
	H2M::RendererAPIH2M::SetAPI(H2M::RendererAPIH2MType::DX11);
#else:
	H2M::RendererAPIH2M::SetAPI(H2M::RendererAPIH2MType::OpenGL);
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
