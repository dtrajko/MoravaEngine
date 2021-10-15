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

	ApplicationSpecification applicationSpecification{};
	applicationSpecification.Name = "Morava Engine";
	applicationSpecification.WindowWidth = 1280;
	applicationSpecification.WindowHeight = 720;

	Application::Create(applicationSpecification, "");

	Application::Get()->Run();

	return 0;
}
