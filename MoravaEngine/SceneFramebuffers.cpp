#include "SceneFramebuffers.h"

#include "ImGuiWrapper.h"
#include "GeometryFactory.h"


SceneFramebuffers::SceneFramebuffers()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 1.0f, 5.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);

	m_EffectFrame_0 = 0; // default colors (diffuse)
	m_EffectFrame_1 = 1; // inverse colors
	m_EffectFrame_2 = 3; // nightvision
	m_EffectFrame_3 = 4; // kernel sharpen

	SetCamera();
	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();
}

void SceneFramebuffers::SetSkybox()
{
}

void SceneFramebuffers::SetupTextures()
{
	textures.insert(std::make_pair("floor_metal", TextureLoader::Get()->GetTexture("Textures/metal.png", false, false)));
	textures.insert(std::make_pair("cube_marble", TextureLoader::Get()->GetTexture("Textures/marble.jpg", false, false)));
	textures.insert(std::make_pair("cube_wood",   TextureLoader::Get()->GetTexture("Textures/container/container2.png", false, false)));
}

void SceneFramebuffers::SetupMeshes()
{
}

void SceneFramebuffers::SetupModels()
{
}

void SceneFramebuffers::SetGeometry()
{
	GeometryFactory::CubeTexCoords::Create();
	GeometryFactory::Plane::Create();
	GeometryFactory::Quad::Create();
}

void SceneFramebuffers::Update(float timestep, Window& mainWindow)
{
}

void SceneFramebuffers::UpdateImGui(float timestep, Window& mainWindow)
{
	ImGui::Begin("Effects");
	ImGui::SliderInt("Effect Frame 0", &m_EffectFrame_0, 0, 5);
	ImGui::SliderInt("Effect Frame 1", &m_EffectFrame_1, 0, 5);
	ImGui::SliderInt("Effect Frame 2", &m_EffectFrame_2, 0, 5);
	ImGui::SliderInt("Effect Frame 3", &m_EffectFrame_3, 0, 5);
	ImGui::End();
}

void SceneFramebuffers::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
	shaders["framebuffers_scene"]->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	shaders["framebuffers_scene"]->setMat4("view", m_CameraController->CalculateViewMatrix());
	shaders["framebuffers_scene"]->setMat4("projection", projectionMatrix);

	// -- cubes
	glBindVertexArray(GeometryFactory::CubeTexCoords::GetVAO());
	glActiveTexture(GL_TEXTURE0);

	textures["cube_wood"]->Bind(0);

	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
	shaders["framebuffers_scene"]->setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
	shaders["framebuffers_scene"]->setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	// -- floor
	glBindVertexArray(GeometryFactory::Plane::GetVAO());

	textures["floor_metal"]->Bind(0);

	shaders["framebuffers_scene"]->setMat4("model", glm::mat4(1.0f));
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void SceneFramebuffers::CleanupGeometry()
{
	// Geometry cleanup
	GeometryFactory::CubeTexCoords::Destroy();
	GeometryFactory::Plane::Destroy();
	GeometryFactory::Quad::Destroy();
}

int SceneFramebuffers::GetEffectForFrame(int frameID)
{
	switch (frameID)
	{
	case 0:
		return m_EffectFrame_0;
	case 1:
		return m_EffectFrame_1;
	case 2:
		return m_EffectFrame_2;
	case 3:
		return m_EffectFrame_3;
	default:
		return 0;
	}
}

SceneFramebuffers::~SceneFramebuffers()
{
	CleanupGeometry();
}
