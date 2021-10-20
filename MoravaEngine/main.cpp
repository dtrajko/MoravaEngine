#include "main.h"


void SelectRendererAPI()
{
#if defined(SCENE_HAZEL_VULKAN) 
	Hazel::RendererAPI::SetAPI(Hazel::RendererAPIType::Vulkan);
#elif defined(SCENE_DX11) 
	Hazel::RendererAPI::SetAPI(Hazel::RendererAPIType::DX11);
#else:
	Hazel::RendererAPI::SetAPI(Hazel::RendererAPIType::OpenGL);
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
