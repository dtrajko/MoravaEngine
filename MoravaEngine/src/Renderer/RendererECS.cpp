#include "Renderer/RendererECS.h"

#include "Core/Application.h"


RendererECS::RendererECS()
{
}

void RendererECS::Init(Scene* scene)
{
	SetShaders();
}

void RendererECS::SetShaders()
{
}

void RendererECS::RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    std::string passType = "shadow";
    scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererECS::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererECS::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererECS::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	RendererBasic::Clear(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);

	// ...

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererECS::Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPassShadow(mainWindow, scene, projectionMatrix);

	RenderPassMain(mainWindow, scene, projectionMatrix);
}

RendererECS::~RendererECS()
{
}
