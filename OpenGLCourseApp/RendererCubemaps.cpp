#include "RendererCubemaps.h"

#include "SceneCubemaps.h"
#include "MousePicker.h"

#include <stdexcept>


RendererCubemaps::RendererCubemaps()
{
}

void RendererCubemaps::Init(Scene* scene)
{
    models = ((SceneCubemaps*)scene)->GetModels();

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

    Shader* shaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    shaders.insert(std::make_pair("basic", shaderBasic));
    printf("RendererCubemaps: shaderBasic compiled [programID=%d]\n", shaderBasic->GetProgramID());

    Shader* shaderFramebuffersScene = new Shader("Shaders/framebuffers_scene.vs", "Shaders/framebuffers_scene.fs");
    shaders.insert(std::make_pair("framebuffers_scene", shaderFramebuffersScene));
    printf("RendererCubemaps: shaderFramebuffersScene compiled [programID=%d]\n", shaderFramebuffersScene->GetProgramID());

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

    EnableTransparency();

    SceneCubemaps* sceneCubemaps = (SceneCubemaps*)scene;

    // cube
    shaders["cubemaps"]->Bind();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(2.0f));
    shaders["cubemaps"]->setMat4("model", model);
    shaders["cubemaps"]->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
    shaders["cubemaps"]->setMat4("projection", projectionMatrix);
    shaders["cubemaps"]->setVec3("cameraPos", scene->GetCamera()->GetPosition());

    glBindVertexArray(sceneCubemaps->GetCubeVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTextureID());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Draw the Nanosuit model
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.2f));
    shaders["cubemaps"]->setMat4("model", model);
    // models["nanosuit"]->Draw(shaders["cubemaps"]);

    // Experimenting with ray casting and MousePicker
    MousePicker* mp = MousePicker::Get();
    mp->GetPointOnRay(scene->GetCamera()->GetPosition(), mp->GetCurrentRay(), mp->m_RayRange);
    Raycast* raycast = sceneCubemaps->GetRaycast();
    raycast->m_Hit = mp->m_Hit;
    raycast->Draw(mp->m_RayStartPoint + scene->GetCamera()->GetFront() * 0.1f, mp->GetCurrentRay() * mp->m_RayRange, raycast->m_Color,
        shaders["basic"], projectionMatrix, scene->GetCamera()->CalculateViewMatrix());

    /* Floor */
    shaders["framebuffers_scene"]->Bind();
    shaders["framebuffers_scene"]->setMat4("projection", projectionMatrix);
    shaders["framebuffers_scene"]->setMat4("view", scene->GetCamera()->CalculateViewMatrix());

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f));
    shaders["framebuffers_scene"]->setMat4("model", model);
    shaders["framebuffers_scene"]->setInt("texture1", 0);
    scene->GetTextures()["semi_transparent"]->Bind(0);
    scene->GetMeshes()["quad"]->Render();

    // draw skybox as last
    glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
    shaders["skybox"]->Bind();
    glm::mat4 view = glm::mat4(glm::mat3(scene->GetCamera()->CalculateViewMatrix())); // remove translation from the view matrix
    shaders["skybox"]->setMat4("view", view);
    shaders["skybox"]->setMat4("projection", projectionMatrix);

    // skybox cube
    glBindVertexArray(sceneCubemaps->GetSkyboxVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTextureID());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererCubemaps::~RendererCubemaps()
{
}
