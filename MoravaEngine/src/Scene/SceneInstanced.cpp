#include "Scene/SceneInstanced.h"

#include "Mesh/VertexInstanced.h"


SceneInstanced::SceneInstanced()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 0.0f, 2.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);

	SetupShaders();
	SetupGeometry();
}

void SceneInstanced::SetupShaders()
{
	m_ShaderInstanced = new MoravaShader("Shaders/instanced.vert", "Shaders/instanced.frag");
	printf("SceneInstanced: m_ShaderInstanced compiled [programID=%d]\n", m_ShaderInstanced->GetProgramID());
}

void SceneInstanced::SetupGeometry()
{
	glm::vec2 translations[100];
	int index = 0;
	float offset = 0.1f;
	for (int y = -10; y < 10; y += 2)
	{
		for (int x = -10; x < 10; x += 2)
		{
			glm::vec2 translation;
			translation.x = (float)x / 10.0f + offset;
			translation.y = (float)y / 10.0f + offset;
			translations[index++] = translation;
		}
	}

	unsigned int instanceVBO;
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 100, &translations[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	float quadVertices[] = {
		// positions       // colors
		 0.05f, -0.05f,    0.0f, 1.0f, 0.0f,
		-0.05f,  0.05f,    1.0f, 0.0f, 0.0f,
		-0.05f, -0.05f,    0.0f, 0.0f, 1.0f,

		-0.05f,  0.05f,    1.0f, 0.0f, 0.0f,
		 0.05f, -0.05f,    0.0f, 1.0f, 0.0f,
		 0.05f,  0.05f,    0.0f, 1.0f, 1.0f,
	};

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);

	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// VertexInstanced.Position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexInstanced), (const void*)offsetof(VertexInstanced, Position));

	// VertexInstanced.Color
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexInstanced), (const void*)offsetof(VertexInstanced, Color));

	// also set instance data - VertexInstanced.Offset
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); // this attribute comes from a different vertex buffer
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.

	m_ShaderInstanced->Bind();
	for (unsigned int i = 0; i < 100; i++)
	{
		m_ShaderInstanced->setVec2("offsets[" + std::to_string(i) + "]", translations[i]);
	}
}

void SceneInstanced::Update(float timestep, Window* mainWindow)
{
	Scene::Update(timestep, mainWindow);
}

void SceneInstanced::UpdateImGui(float timestep, Window* mainWindow)
{
}

void SceneInstanced::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Hazel::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
	m_ShaderInstanced->Bind();

	m_ShaderInstanced->SetMat4("projection", projectionMatrix);
	m_ShaderInstanced->SetMat4("view", m_Camera->GetViewMatrix());
	m_ShaderInstanced->SetMat4("model", glm::mat4(1.0f));

	glBindVertexArray(quadVAO);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100); // 100 triangles of 6 vertices each
	glBindVertexArray(0);

}

SceneInstanced::~SceneInstanced()
{
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);
}
