#include "SceneInstanced.h"

#include "ImGuiWrapper.h"

#include "ShaderMain.h"
#include "RendererInstanced.h"
#include "VertexInstanced.h"


SceneInstanced::SceneInstanced()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 16.0f, 28.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 4.0f;
	sceneSettings.ambientIntensity = 0.4f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);

	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
}

void SceneInstanced::SetSkybox()
{
}

void SceneInstanced::SetTextures()
{
}

void SceneInstanced::SetupMeshes()
{
	float quadVertices[] = {
		// positions       // colors
		-0.05f,  0.05f,    1.0f, 0.0f, 0.0f,
		 0.05f, -0.05f,    0.0f, 1.0f, 0.0f,
		-0.05f, -0.05f,    0.0f, 0.0f, 1.0f,

		-0.05f,  0.05f,    1.0f, 0.0f, 0.0f,
		 0.05f, -0.05f,    0.0f, 1.0f, 0.0f,
		 0.05f,  0.05f,    0.0f, 1.0f, 1.0f
	};

	unsigned int vertexCount = 6 * 5;

	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	unsigned int quadVBO;

	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices[0]) * vertexCount, quadVertices, GL_STATIC_DRAW);

	// VertexInstanced.Position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexInstanced), (const void*)offsetof(VertexInstanced, Position));
	// VertexInstanced.Color
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexInstanced), (const void*)offsetof(VertexInstanced, Color));

	glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind VBO
	glBindVertexArray(0);             // Unbind VAO
}

void SceneInstanced::SetupModels()
{ 
}

void SceneInstanced::Update(float timestep, Window& mainWindow)
{
}

void SceneInstanced::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
}

void SceneInstanced::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];

	glm::mat4 model = glm::mat4(1.0f);


}

SceneInstanced::~SceneInstanced()
{
}
