#include "Renderer/RendererHazelVulkan.h"

#include "Core/Application.h"


RendererHazelVulkan::RendererHazelVulkan()
{
}

void RendererHazelVulkan::Init(Scene* scene)
{
	SetShaders();
}

void RendererHazelVulkan::SetShaders()
{
}

void RendererHazelVulkan::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererHazelVulkan::RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    std::string passType = "shadow";
    scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererHazelVulkan::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	std::string passType = "shadow_omni";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererHazelVulkan::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererHazelVulkan::PostProcessing(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	std::string passType = "post_processing";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererHazelVulkan::Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPassShadow(mainWindow, scene, projectionMatrix);

	RenderOmniShadows(mainWindow, scene, projectionMatrix);

	RenderPassMain(mainWindow, scene, projectionMatrix);

	PostProcessing(mainWindow, scene, projectionMatrix);
}

RendererHazelVulkan::~RendererHazelVulkan()
{
}
