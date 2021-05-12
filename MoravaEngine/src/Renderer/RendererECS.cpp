#include "Renderer/RendererECS.h"

#include "Core/Application.h"


RendererECS::RendererECS()
{
}

RendererECS::~RendererECS()
{
}

void RendererECS::Init(Scene* scene)
{
	SetShaders();
}

void RendererECS::SetShaders()
{
}

void RendererECS::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererECS::RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    std::string passType = "shadow";
    scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());
}

void RendererECS::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	std::string passType = "shadow_omni";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());
}

void RendererECS::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());
}

void RendererECS::PostProcessing(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	std::string passType = "post_processing";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());
}

void RendererECS::BeginFrame()
{
}

void RendererECS::WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPassShadow(mainWindow, scene, projectionMatrix);

	RenderOmniShadows(mainWindow, scene, projectionMatrix);

	RenderPassMain(mainWindow, scene, projectionMatrix);

	PostProcessing(mainWindow, scene, projectionMatrix);
}
