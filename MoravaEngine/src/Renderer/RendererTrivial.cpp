#include "RendererTrivial.h"

#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"

#include "Platform/DX11/DX11Renderer.h"


RendererTrivial::RendererTrivial()
{
}

RendererTrivial::~RendererTrivial()
{
}

void RendererTrivial::Init(Scene* scene)
{
	SetShaders();
}

void RendererTrivial::SetShaders()
{
}

void RendererTrivial::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererTrivial::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererTrivial::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	RendererBasic::Clear();

	// ...

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());
}

void RendererTrivial::BeginFrame()
{
}

void RendererTrivial::WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	// In Hazel: s_Data.m_CommandQueue.Execute()

	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPassMain(mainWindow, scene, projectionMatrix);
}
