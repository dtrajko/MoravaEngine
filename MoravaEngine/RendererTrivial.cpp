#include "RendererTrivial.h"
#include "SceneAnimPBR.h"
#include "Log.h"
#include "Application.h"


RendererTrivial::RendererTrivial()
{
	bgColor = glm::vec4(204 / 255.0f, 255 / 255.0f, 204 / 255.0f, 1.0f);
}

void RendererTrivial::Init(Scene* scene)
{
	SetShaders();
}

void RendererTrivial::SetShaders()
{
}

void RendererTrivial::RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererTrivial::RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererTrivial::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());
	
	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ...

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

void RendererTrivial::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPass(mainWindow, scene, projectionMatrix);
}

RendererTrivial::~RendererTrivial()
{
}
