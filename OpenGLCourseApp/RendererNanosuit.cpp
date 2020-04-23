#include "RendererNanosuit.h"

#include "SceneNanosuit.h"


RendererNanosuit::RendererNanosuit()
{
}

void RendererNanosuit::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

	models = ((SceneNanosuit*)scene)->GetModels();
}

void RendererNanosuit::SetUniforms()
{
}

void RendererNanosuit::SetShaders()
{
	Shader* shaderNanosuit = new Shader("Shaders/nanosuit.vs", "Shaders/nanosuit.fs");

	shaderNanosuit->Bind();
	shaderNanosuit->setInt("material.diffuse", 0);
	shaderNanosuit->setInt("material.specular", 1);

	shaders.insert(std::make_pair("nanosuit", shaderNanosuit));
	printf("RendererNanosuit: shaderNanosuit compiled [programID=%d]\n", shaderNanosuit->GetProgramID());
}

void RendererNanosuit::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	modelRotationY += 10.0f * deltaTime;

	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererNanosuit::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Shader* shaderNanosuit = shaders["nanosuit"];

	// be sure to activate shader when setting uniforms/drawing objects
	shaderNanosuit->Bind();
	shaderNanosuit->setVec3("light.position", scene->GetCamera()->GetPosition());
	shaderNanosuit->setVec3("light.direction", scene->GetCamera()->GetFront());
	shaderNanosuit->setFloat("light.cutOff", glm::cos(glm::radians(12.5f)));
	shaderNanosuit->setFloat("light.outerCutOff", glm::cos(glm::radians(17.5f)));
	shaderNanosuit->setVec3("viewPos", scene->GetCamera()->GetPosition());

	// light properties
	shaderNanosuit->setVec3("light.ambient", 2.0f, 2.0f, 2.0f);
	// we configure the diffuse intensity slightly higher; the right lighting conditions differ with each lighting method and environment.
	// each environment and lighting type requires some tweaking to get the best out of your environment.
	shaderNanosuit->setVec3("light.diffuse", 3.0f, 3.0f, 3.0f);
	shaderNanosuit->setVec3("light.specular", 2.0f, 2.0f, 2.0f);
	shaderNanosuit->setFloat("light.constant", 1.0f);
	shaderNanosuit->setFloat("light.linear", 0.09f);
	shaderNanosuit->setFloat("light.quadratic", 0.032f);

	// material properties
	shaderNanosuit->setFloat("material.shininess", 32.0f);

	glm::mat4 view = scene->GetCamera()->CalculateViewMatrix();

	// configure transformation matrices
	shaderNanosuit->setMat4("projection", projectionMatrix);
	shaderNanosuit->setMat4("view", view);

	// Draw the Nanosuit model
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	shaderNanosuit->setMat4("model", model);
	models["nanosuit"]->Draw(shaders["nanosuit"]);

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererNanosuit::~RendererNanosuit()
{
}
