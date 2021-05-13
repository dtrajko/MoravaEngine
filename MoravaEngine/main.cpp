#include "main.h"


int main()
{
	Log::Init();

	Hazel::RendererAPI::SetAPI(Hazel::RendererAPIType::OpenGL);

	Application::Get()->InitWindow(WindowProps(WINDOW_TITLE, WIDTH, HEIGHT));

	SceneProperties sceneProperties = SetSceneProperties();

	Application::OnInit(sceneProperties);

	Application::Run();

	Application::OnShutdown();

	return 0;
}
