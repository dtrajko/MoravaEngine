#include "RendererAsteroids.h"

#include "SceneInstanced.h"
#include "ShaderInstanced.h"
#include "VertexInstanced.h"



RendererAsteroids::RendererAsteroids()
{
}

void RendererAsteroids::Init()
{
	SetUniforms();
	SetShaders();

	unsigned int amount = 1000;
	glm::mat4* modelMatrices;
	modelMatrices = new glm::mat4[amount];
	srand((unsigned int)glfwGetTime()); // initialize random seed	
	float radius = 50.0;
	float offset = 2.5f;
	for (unsigned int i = 0; i < amount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		// 1. translation: displace along circle with 'radius' in range [-offset, offset]
		float angle = (float)i / (float)amount * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y, z));

		// 2. scale: scale between 0.05 and 0.25f
		float scale = (float)(rand() % 20) / 100.0f + 0.05f;
		model = glm::scale(model, glm::vec3(scale));

		// 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
		float rotAngle = (float)(rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		// 4. now add to list of matrices
		modelMatrices[i] = model;
	}
}

void RendererAsteroids::SetUniforms()
{
}

void RendererAsteroids::SetShaders()
{
	static const char* vertShader = "Shaders/instanced.vert";
	static const char* fragShader = "Shaders/instanced.frag";
	ShaderInstanced* shaderInstanced = new ShaderInstanced();
	shaderInstanced->CreateFromFiles(vertShader, fragShader);
	shaders.insert(std::make_pair("instanced", shaderInstanced));
	printf("Renderer: Main shader compiled [programID=%d]\n", shaderInstanced->GetProgramID());
}

void RendererAsteroids::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererAsteroids::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderInstanced* shaderInstanced = static_cast<ShaderInstanced*>(shaders["instanced"]);
	shaderInstanced->Bind();

	// draw planet
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
	model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));

	shaderInstanced->setMat4("projection", projectionMatrix);
	shaderInstanced->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
	shaderInstanced->setMat4("model", model);

	// planet.Draw(shader);

	// draw meteorites
	// for (unsigned int i = 0; i < amount; i++)
	// {
	// 	shader.setMat4("model", modelMatrices[i]);
	// 	rock.Draw(shader);
	// }

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererAsteroids::~RendererAsteroids()
{
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);
}
