#include "RendererImGuizmo.h"
#include "SceneImGuizmo.h"
#include "Log.h"
#include "Application.h"


RendererImGuizmo::RendererImGuizmo()
{
	bgColor = glm::vec4(204 / 255.0f, 255 / 255.0f, 204 / 255.0f, 1.0f);
}

void RendererImGuizmo::Init(Scene* scene)
{
	SetShaders();
}

void RendererImGuizmo::SetShaders()
{
	Shader* shaderMain = new Shader("Shaders/shader.vert", "Shaders/shader.frag");
	shaders.insert(std::make_pair("main", shaderMain));
	Log::GetLogger()->info("RendererImGuizmo: shaderMain compiled [programID={0}]", shaderMain->GetProgramID());
}

void RendererImGuizmo::RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererImGuizmo::RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererImGuizmo::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());
	
	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Shader* shaderMain = (Shader*)shaders["main"];
	shaderMain->Bind();

	shaderMain->setMat4("model", glm::mat4(1.0f));
	shaderMain->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());
	shaderMain->setMat4("projection", projectionMatrix);
	shaderMain->setVec3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderMain->setInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderMain->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderMain->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderMain->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderMain->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderMain->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	shaderMain->setInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->setInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->setInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000.0f));
	shaderMain->setFloat("tilingFactor", 1.0f);
	shaderMain->setVec4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderMain->Validate();

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);

	shaderMain->Unbind();
}

void RendererImGuizmo::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPass(mainWindow, scene, projectionMatrix);
}

RendererImGuizmo::~RendererImGuizmo()
{
}
