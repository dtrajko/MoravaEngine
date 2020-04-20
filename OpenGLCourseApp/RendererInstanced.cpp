#include "RendererInstanced.h"

#include "SceneInstanced.h"
#include "ShaderInstanced.h"



RendererInstanced::RendererInstanced()
{
}

void RendererInstanced::Init()
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

	unsigned int quadVAO = static_cast<SceneInstanced*>(scene)->GetQuadVAO();
	glBindVertexArray(quadVAO);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);

}

RendererInstanced::~RendererInstanced()
{
}
