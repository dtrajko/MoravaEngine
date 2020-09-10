#include "RendererSSAO.h"
#include "SceneSSAO.h"
#include "Log.h"
#include "Application.h"


RendererSSAO::RendererSSAO()
{
}

void RendererSSAO::Init(Scene* scene)
{
	SetShaders();
	SetupSSAO();

	SceneSSAO* sceneSSAO = (SceneSSAO*)scene;
	sceneSSAO->m_SSAO = m_SSAO;
}

void RendererSSAO::SetShaders()
{
}

void RendererSSAO::SetupSSAO()
{
	m_SSAO = new SSAO();
	m_SSAO->Init();
}

void RendererSSAO::RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererSSAO::RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererSSAO::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());
	
	// Clear the window
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SceneSSAO* sceneSSAO = (SceneSSAO*)scene;
	m_SSAO->Render(projectionMatrix, scene->GetCameraController()->CalculateViewMatrix(), scene->GetMeshes(), &sceneSSAO->modelsSSAO);

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererSSAO::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderOmniShadows(mainWindow, scene, projectionMatrix);
	RenderPass(mainWindow, scene, projectionMatrix);
}

RendererSSAO::~RendererSSAO()
{
	delete m_SSAO;
}
