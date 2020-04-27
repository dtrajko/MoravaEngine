#include "RendererFramebuffers.h"

#include "SceneNanosuit.h"


RendererFramebuffers::RendererFramebuffers()
{
}

void RendererFramebuffers::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();
}

void RendererFramebuffers::SetUniforms()
{
}

void RendererFramebuffers::SetShaders()
{
}

void RendererFramebuffers::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererFramebuffers::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render code here

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererFramebuffers::~RendererFramebuffers()
{
}
