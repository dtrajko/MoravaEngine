#include "SceneNanosuit.h"

#include "CubeNanosuit.h"
#include "ImGuiWrapper.h"
#include "LearnOpenGL/ModelJoey.h"
#include "Sphere.h"
#include "Timer.h"


SceneNanosuit::SceneNanosuit()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 10.5f, 10.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);

	SetCamera();
	SetSkybox();
	SetupShaders();
	SetupTextures();
	SetupModels();

	m_LightOnCamera = true;
	m_IsRotating = true;
	m_RotationSpeed = 10.0f;
	m_LightSourceVisible = false;
	m_BgColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	InitNanosuitUniforms();
}

void SceneNanosuit::SetupShaders()
{
	m_ShaderNanosuit = new Shader("Shaders/nanosuit.vs", "Shaders/nanosuit.fs");
	printf("SceneNanosuit: m_ShaderNanosuit compiled [programID=%d]\n", m_ShaderNanosuit->GetProgramID());
}

void SceneNanosuit::SetupTextures()
{
	textures.insert(std::make_pair("plain", TextureLoader::Get()->GetTexture("Textures/plain.png", false, false)));
}

void SceneNanosuit::SetupModels()
{
	ModelJoey* nanosuit = new ModelJoey("Models/nanosuit.obj", "Textures/nanosuit");
	models.insert(std::make_pair("nanosuit", nanosuit));

	Sphere* sphere = new Sphere();
	meshes.insert(std::make_pair("sphere", sphere));

	CubeNanosuit* cubeNanosuit = new CubeNanosuit("Textures/container");
	MeshJoey* cube = cubeNanosuit->GetMesh();
	meshesJoey.insert(std::make_pair("cube", cube));
}

void SceneNanosuit::InitNanosuitUniforms()
{
	if (nanosuitUniforms == nullptr)
		nanosuitUniforms = new NanosuitUniforms;

	nanosuitUniforms->viewPos = m_Camera->GetPosition();
	nanosuitUniforms->enableNormalMap = true;

	int txSlotDiffuse =  0; // sampler2D / Texture slot
	int txSlotSpecular = 1; // sampler2D / Texture slot
	int txSlotNormal =   2; // sampler2D / Texture slot
	float shininess = 32.0f;
	Material material(txSlotDiffuse, txSlotSpecular, txSlotNormal, shininess);

	nanosuitUniforms->material = material;

	nanosuitUniforms->light.position = m_Camera->GetPosition();
	nanosuitUniforms->light.direction = m_Camera->GetFront();
	nanosuitUniforms->light.cutOff = glm::cos(glm::radians(12.5f));
	nanosuitUniforms->light.outerCutOff = glm::cos(glm::radians(20.0f));

	nanosuitUniforms->light.ambient = glm::vec3(2.0f, 2.0f, 2.0f);
	nanosuitUniforms->light.diffuse = glm::vec3(3.0f, 3.0f, 3.0f);
	nanosuitUniforms->light.specular = glm::vec3(2.0f, 2.0f, 2.0f);

	nanosuitUniforms->light.constant = 1.0f;
	nanosuitUniforms->light.linear = 0.09f;
	nanosuitUniforms->light.quadratic = 0.032f;
}

void SceneNanosuit::Update(float timestep, Window* mainWindow)
{
	m_Camera->OnUpdate(timestep);

	float deltaTime = Timer::Get()->GetDeltaTime();
	m_ModelRotationY = m_IsRotating ? m_ModelRotationY + m_RotationSpeed * deltaTime : 0.0f;
}

void SceneNanosuit::UpdateImGui(float timestep, Window* mainWindow)
{
	ImGui::Begin("Nanosuit Shader Parameters:");

	if (ImGui::Button("Set Defaults")) {
		InitNanosuitUniforms();
	}

	ImGui::Checkbox("Enable Normal Map", &nanosuitUniforms->enableNormalMap);
	ImGui::Checkbox("Light on Camera", &m_LightOnCamera);
	ImGui::Checkbox("Light Source Visible", &m_LightSourceVisible);
	ImGui::Checkbox("Is Rotating", &m_IsRotating);
	ImGui::SliderFloat("Rotating Speed", &m_RotationSpeed, -500.0f, 500.0f);
	ImGui::ColorEdit3("Background Color", glm::value_ptr(m_BgColor));
	ImGui::Separator();
	ImGui::Separator();
	ImGui::SliderInt("Material.diffuse",      &nanosuitUniforms->material.m_AlbedoMap,   0, 3);
	ImGui::SliderInt("Material.specular",     &nanosuitUniforms->material.m_SpecularMap, 0, 3);
	ImGui::SliderInt("Material.normalMap",    &nanosuitUniforms->material.m_NormalMap,   0, 3);
	ImGui::SliderFloat("Material.shininess",  &nanosuitUniforms->material.m_Shininess,   0, 512);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::SliderFloat3("Light.position",   glm::value_ptr(nanosuitUniforms->light.position),  -50.0f, 50.0f);
	ImGui::SliderFloat3("Light.direction",  glm::value_ptr(nanosuitUniforms->light.direction), -1.0f, 1.0f);
	ImGui::SliderFloat("Light.cutOff",      &nanosuitUniforms->light.cutOff,      -1.0f, 1.0f);
	ImGui::SliderFloat("Light.outerCutOff", &nanosuitUniforms->light.outerCutOff, -1.0f, 1.0f);
	ImGui::Separator();
	ImGui::ColorEdit3("Light.ambient", glm::value_ptr(nanosuitUniforms->light.ambient));
	ImGui::ColorEdit3("Light.diffuse", glm::value_ptr(nanosuitUniforms->light.diffuse));
	ImGui::ColorEdit3("Light.specular", glm::value_ptr(nanosuitUniforms->light.specular));
	ImGui::Separator();
	ImGui::SliderFloat("Light.constant",  &nanosuitUniforms->light.constant,  0.8f, 1.2f);
	ImGui::SliderFloat("Light.linear",    &nanosuitUniforms->light.linear,    0.0f, 0.6f);
	ImGui::SliderFloat("Light.quadratic", &nanosuitUniforms->light.quadratic, 0.0f, 0.6f);

	ImGui::End();
}

void SceneNanosuit::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
	// Clear the window
	glClearColor(m_BgColor.r, m_BgColor.g, m_BgColor.b, m_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RendererBasic::EnableTransparency();

	// be sure to activate shader when setting uniforms/drawing objects
	m_ShaderNanosuit->Bind();

	nanosuitUniforms = GetNanosuitUniforms();

	m_ShaderNanosuit->setVec3("viewPos", m_Camera->GetPosition());
	m_ShaderNanosuit->setBool("enableNormalMap", nanosuitUniforms->enableNormalMap);

	// light properties
	m_ShaderNanosuit->setVec3("light.position", m_LightOnCamera ? m_Camera->GetPosition() : nanosuitUniforms->light.position);
	m_ShaderNanosuit->setVec3("light.direction", m_LightOnCamera ? m_Camera->GetFront() : nanosuitUniforms->light.direction);
	m_ShaderNanosuit->setFloat("light.cutOff", nanosuitUniforms->light.cutOff);
	m_ShaderNanosuit->setFloat("light.outerCutOff", nanosuitUniforms->light.outerCutOff);

	m_ShaderNanosuit->setVec3("light.ambient", nanosuitUniforms->light.ambient);
	// we configure the diffuse intensity slightly higher; the right lighting conditions differ with each lighting method and environment.
	// each environment and lighting type requires some tweaking to get the best out of your environment.
	m_ShaderNanosuit->setVec3("light.diffuse", nanosuitUniforms->light.diffuse);
	m_ShaderNanosuit->setVec3("light.specular", nanosuitUniforms->light.specular);
	m_ShaderNanosuit->setFloat("light.constant", nanosuitUniforms->light.constant);
	m_ShaderNanosuit->setFloat("light.linear", nanosuitUniforms->light.linear);
	m_ShaderNanosuit->setFloat("light.quadratic", nanosuitUniforms->light.quadratic);

	// material properties
	m_ShaderNanosuit->setInt("material.diffuse", nanosuitUniforms->material.m_AlbedoMap);
	m_ShaderNanosuit->setInt("material.specular", nanosuitUniforms->material.m_SpecularMap);
	m_ShaderNanosuit->setInt("material.normalMap", nanosuitUniforms->material.m_NormalMap);
	m_ShaderNanosuit->setFloat("material.shininess", nanosuitUniforms->material.m_Shininess);

	glm::mat4 view = m_Camera->GetViewMatrix();

	// configure transformation matrices
	m_ShaderNanosuit->setMat4("projection", projectionMatrix);
	m_ShaderNanosuit->setMat4("view", view);

	// Draw the Nanosuit model
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(m_ModelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	m_ShaderNanosuit->setMat4("model", model);
	models["nanosuit"]->Draw(m_ShaderNanosuit);

	// Render Sphere (Light source)
	model = glm::mat4(1.0f);
	model = glm::translate(model, m_LightOnCamera ? m_Camera->GetPosition() : nanosuitUniforms->light.position);
	model = glm::scale(model, glm::vec3(0.25f));
	m_ShaderNanosuit->setMat4("model", model);
	GetTextures()["plain"]->Bind(0);
	GetTextures()["plain"]->Bind(1);
	GetTextures()["plain"]->Bind(2);
	if (!m_LightOnCamera && m_LightSourceVisible)
		GetMeshes()["sphere"]->Render();

	// Render Cube (with Diffuse/Specular maps)
	model = glm::mat4(1.0f);
	model = glm::translate(model, { 15.0f, 4.0f, 0.0f });
	model = glm::scale(model, glm::vec3(8.0f));
	m_ShaderNanosuit->setMat4("model", model);
	meshesJoey["cube"]->Draw(m_ShaderNanosuit);
}

SceneNanosuit::~SceneNanosuit()
{
	for (auto& model : models)
		delete model.second;

	models.clear();

	delete nanosuitUniforms;
}
