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

void RendererSSAO::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	uint32_t width = Application::Get()->GetWindow()->GetBufferWidth();
	uint32_t height = Application::Get()->GetWindow()->GetBufferHeight();

	scene->GetCameraController()->OnResize((float)width, (float)height);

	// Override the Projection matrix
	float aspectRatio = scene->GetCameraController()->GetAspectRatio();
	projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio,
		scene->GetSettings().nearPlane, scene->GetSettings().farPlane);

	RendererBasic::SetProjectionMatrix(projectionMatrix);

	RenderOmniShadows(mainWindow, scene, projectionMatrix);
	RenderPass(mainWindow, scene, projectionMatrix);
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
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SceneSSAO* sceneSSAO = (SceneSSAO*)scene;
	m_SSAO->Render(projectionMatrix, scene->GetCameraController()->CalculateViewMatrix(), scene->GetMeshes(), &sceneSSAO->modelsSSAO);

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

RendererSSAO::~RendererSSAO()
{
	delete m_SSAO;
}
