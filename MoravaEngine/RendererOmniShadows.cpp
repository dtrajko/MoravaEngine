#include "RendererOmniShadows.h"


RendererOmniShadows::RendererOmniShadows()
{
}

void RendererOmniShadows::Init(Scene* scene)
{
}

void RendererOmniShadows::SetUniforms()
{
}

void RendererOmniShadows::SetShaders()
{
}

void RendererOmniShadows::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererOmniShadows::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render here

	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

RendererOmniShadows::~RendererOmniShadows()
{
}
