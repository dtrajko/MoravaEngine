#include "RendererCubemaps.h"

#include "SceneCubemaps.h"

#include <stdexcept>


RendererCubemaps::RendererCubemaps()
{
}

void RendererCubemaps::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();
}

void RendererCubemaps::SetUniforms()
{
}

void RendererCubemaps::SetShaders()
{
	Shader* shaderCubemaps = new Shader("Shaders/learnopengl/6.2.cubemaps.vs", "Shaders/learnopengl/6.2.cubemaps.fs");
	shaders.insert(std::make_pair("cubemaps", shaderCubemaps));
	printf("RendererCubemaps: shaderCubemaps compiled [programID=%d]\n", shaderCubemaps->GetProgramID());

	Shader* shaderSkybox = new Shader("Shaders/learnopengl/6.2.skybox.vs", "Shaders/learnopengl/6.2.skybox.fs");
	shaders.insert(std::make_pair("skybox", shaderSkybox));
	printf("RendererCubemaps: shaderSkybox compiled [programID=%d]\n", shaderSkybox->GetProgramID());

	// shader configuration
	shaders["cubemaps"]->Bind();
	shaders["cubemaps"]->setInt("skybox", 0);

	shaders["skybox"]->Bind();
	shaders["skybox"]->setInt("skybox", 0);
}

void RendererCubemaps::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererCubemaps::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SceneCubemaps* sceneCubemaps = (SceneCubemaps*)scene;

    shaders["cubemaps"]->Bind();
    shaders["cubemaps"]->setMat4("model", glm::mat4(1.0f));
    shaders["cubemaps"]->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
    shaders["cubemaps"]->setMat4("projection", projectionMatrix);
    shaders["cubemaps"]->setVec3("cameraPos", scene->GetCamera()->GetPosition());

    // cubes
    glBindVertexArray(sceneCubemaps->GetCubeVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTexture());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // draw skybox as last
    glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
    shaders["skybox"]->Bind();
    glm::mat4 view = glm::mat4(glm::mat3(scene->GetCamera()->CalculateViewMatrix())); // remove translation from the view matrix
    shaders["skybox"]->setMat4("view", view);
    shaders["skybox"]->setMat4("projection", projectionMatrix);

    // skybox cube
    glBindVertexArray(sceneCubemaps->GetSkyboxVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTexture());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default

    // printf("cubeVAO=%i skyboxVAO=%i cubemapTexture=%i\n", sceneCubemaps->GetCubeVAO(), sceneCubemaps->GetSkyboxVAO(), sceneCubemaps->GetCubemapTexture());

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererCubemaps::~RendererCubemaps()
{
}
