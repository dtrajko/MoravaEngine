#include "RendererTrivial.h"
#include "SceneAnimPBR.h"
#include "Log.h"
#include "Application.h"


RendererTrivial::RendererTrivial()
{
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
	glViewport(0, 0, (GLsizei)mainWindow.GetWidth(), (GLsizei)mainWindow.GetHeight());
	
	// Clear the window
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// ...

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererTrivial::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPass(mainWindow, scene, projectionMatrix);
}

RendererTrivial::~RendererTrivial()
{
}
