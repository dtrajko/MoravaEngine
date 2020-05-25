#include "RendererInstanced.h"

#include "SceneInstanced.h"
#include "ShaderInstanced.h"
#include "VertexInstanced.h"



RendererInstanced::RendererInstanced()
{
}

void RendererInstanced::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

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

	ShaderInstanced* shaderInstanced = static_cast<ShaderInstanced*>(shaders["instanced"]);
	shaderInstanced->Bind();
	for (unsigned int i = 0; i < 100; i++)
	{
		shaderInstanced->setVec2("offsets[" + std::to_string(i) + "]", translations[i]);
	}
}

void RendererInstanced::SetUniforms()
{
}

void RendererInstanced::SetShaders()
{
	static const char* vertShader = "Shaders/instanced.vert";
	static const char* fragShader = "Shaders/instanced.frag";
	ShaderInstanced* shaderInstanced = new ShaderInstanced();
	shaderInstanced->CreateFromFiles(vertShader, fragShader);
	shaders.insert(std::make_pair("instanced", shaderInstanced));
	printf("Renderer: Main shader compiled [programID=%d]\n", shaderInstanced->GetProgramID());
}

void RendererInstanced::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererInstanced::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderInstanced* shaderInstanced = static_cast<ShaderInstanced*>(shaders["instanced"]);
	shaderInstanced->Bind();

	shaderInstanced->setMat4("projection", projectionMatrix);
	shaderInstanced->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
	shaderInstanced->setMat4("model", glm::mat4(1.0f));

	glBindVertexArray(quadVAO);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100); // 100 triangles of 6 vertices each
	glBindVertexArray(0);

	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

RendererInstanced::~RendererInstanced()
{
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);
}
