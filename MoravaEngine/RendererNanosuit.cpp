#include "RendererNanosuit.h"

#include "SceneNanosuit.h"


RendererNanosuit::RendererNanosuit()
{
}

void RendererNanosuit::Init(Scene* scene)
{
	models = ((SceneNanosuit*)scene)->GetModels();

	SetUniforms();
	SetShaders();
}

void RendererNanosuit::SetUniforms()
{
}

void RendererNanosuit::SetShaders()
{
	Shader* shaderNanosuit = new Shader("Shaders/nanosuit.vs", "Shaders/nanosuit.fs");
	s_Shaders.insert(std::make_pair("nanosuit", shaderNanosuit));
	printf("RendererNanosuit: shaderNanosuit compiled [programID=%d]\n", shaderNanosuit->GetProgramID());
}

void RendererNanosuit::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	SceneNanosuit* sceneNanosuit = (SceneNanosuit*)scene;
	m_ModelRotationY = sceneNanosuit->m_IsRotating ? m_ModelRotationY + sceneNanosuit->m_RotationSpeed * deltaTime : 0.0f;

	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererNanosuit::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	Shader* shaderNanosuit = s_Shaders["nanosuit"];
	SceneNanosuit* sceneNanosuit = (SceneNanosuit*)scene;

	// Clear the window
	glClearColor(sceneNanosuit->m_BgColor.r, sceneNanosuit->m_BgColor.g, sceneNanosuit->m_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	EnableTransparency();

	// be sure to activate shader when setting uniforms/drawing objects
	shaderNanosuit->Bind();

	if (sceneNanosuit->m_DefaultNanosuitUniforms)
		sceneNanosuit->InitNanosuitUniforms();

	nanosuitUniforms = sceneNanosuit->GetNanosuitUniforms();

	shaderNanosuit->setVec3("viewPos", scene->GetCamera()->GetPosition());
	shaderNanosuit->setBool("enableNormalMap", nanosuitUniforms->enableNormalMap);

	// light properties
	shaderNanosuit->setVec3("light.position",      sceneNanosuit->m_LightOnCamera ? scene->GetCamera()->GetPosition() : nanosuitUniforms->light.position);
	shaderNanosuit->setVec3("light.direction",     sceneNanosuit->m_LightOnCamera ? scene->GetCamera()->GetFront() : nanosuitUniforms->light.direction);
	shaderNanosuit->setFloat("light.cutOff",       nanosuitUniforms->light.cutOff);
	shaderNanosuit->setFloat("light.outerCutOff",  nanosuitUniforms->light.outerCutOff);

	shaderNanosuit->setVec3("light.ambient",       nanosuitUniforms->light.ambient);
	// we configure the diffuse intensity slightly higher; the right lighting conditions differ with each lighting method and environment.
	// each environment and lighting type requires some tweaking to get the best out of your environment.
	shaderNanosuit->setVec3("light.diffuse",       nanosuitUniforms->light.diffuse);
	shaderNanosuit->setVec3("light.specular",      nanosuitUniforms->light.specular);
	shaderNanosuit->setFloat("light.constant",     nanosuitUniforms->light.constant);
	shaderNanosuit->setFloat("light.linear",       nanosuitUniforms->light.linear);
	shaderNanosuit->setFloat("light.quadratic",    nanosuitUniforms->light.quadratic);

	// material properties
	shaderNanosuit->setInt("material.diffuse",     nanosuitUniforms->material.m_AlbedoMap);
	shaderNanosuit->setInt("material.specular",    nanosuitUniforms->material.m_SpecularMap);
	shaderNanosuit->setInt("material.normalMap",   nanosuitUniforms->material.m_NormalMap);
	shaderNanosuit->setFloat("material.shininess", nanosuitUniforms->material.m_Shininess);

	glm::mat4 view = scene->GetCameraController()->CalculateViewMatrix();

	// configure transformation matrices
	shaderNanosuit->setMat4("projection", projectionMatrix);
	shaderNanosuit->setMat4("view", view);

	// Draw the Nanosuit model
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(m_ModelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	shaderNanosuit->setMat4("model", model);
	models["nanosuit"]->Draw(shaderNanosuit);

	// Render Sphere (Light source)
	model = glm::mat4(1.0f);
	model = glm::translate(model, sceneNanosuit->m_LightOnCamera ? scene->GetCamera()->GetPosition() : nanosuitUniforms->light.position);
	model = glm::scale(model, glm::vec3(0.25f));
	shaderNanosuit->setMat4("model", model);
	scene->GetTextures()["plain"]->Bind(0);
	scene->GetTextures()["plain"]->Bind(1);
	scene->GetTextures()["plain"]->Bind(2);
	if (!sceneNanosuit->m_LightOnCamera && sceneNanosuit->m_LightSourceVisible)
		scene->GetMeshes()["sphere"]->Render();

	// Render Cube (with Diffuse/Specular maps)
	model = glm::mat4(1.0f);
	model = glm::translate(model, { 15.0f, 4.0f, 0.0f });
	model = glm::scale(model, glm::vec3(8.0f));
	shaderNanosuit->setMat4("model", model);
	sceneNanosuit->GetMeshesJoey()["cube"]->Draw(shaderNanosuit);

	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

RendererNanosuit::~RendererNanosuit()
{
}
