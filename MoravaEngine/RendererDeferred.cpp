#include "RendererDeferred.h"
#include "SceneAnimPBR.h"
#include "Log.h"
#include "Application.h"


RendererDeferred::RendererDeferred()
{
}

void RendererDeferred::Init(Scene* scene)
{
	SetShaders();
}

void RendererDeferred::SetShaders()
{
}

void RendererDeferred::RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererDeferred::RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererDeferred::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());
	
	// Clear the window
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// ...

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererDeferred::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPass(mainWindow, scene, projectionMatrix);
}

RendererDeferred::~RendererDeferred()
{
}
