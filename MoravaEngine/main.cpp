#include "main.h"



// Window properties
const char* WINDOW_TITLE = "Morava Engine";
const int WIDTH = 1280;
const int HEIGHT = 720;


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

	Application::Get()->InitWindow(WindowProps(WINDOW_TITLE, WIDTH, HEIGHT));

	Application::Get()->Run();

	return 0;
}
