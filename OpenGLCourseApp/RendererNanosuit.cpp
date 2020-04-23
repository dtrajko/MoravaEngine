#include "RendererNanosuit.h"

#include "SceneNanosuit.h"


RendererNanosuit::RendererNanosuit()
{
	Shader* shaderNanosuit = new Shader("Shaders/nanosuit.vs", "Shaders/nanosuit.fs");
	shaders.insert(std::make_pair("nanosuit", shaderNanosuit));
	printf("RendererNanosuit: shaderNanosuit compiled [programID=%d]\n", shaderNanosuit->GetProgramID());
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

	glm::mat4 view = scene->GetCamera()->CalculateViewMatrix();

	// configure transformation matrices
	shaders["nanosuit"]->Bind();
	shaders["nanosuit"]->setMat4("projection", projectionMatrix);
	shaders["nanosuit"]->setMat4("view", view);

	// Draw the Nanosuit model
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	shaders["nanosuit"]->setMat4("model", model);
	models["nanosuit"]->Draw(shaders["nanosuit"]);

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererNanosuit::~RendererNanosuit()
{
}
