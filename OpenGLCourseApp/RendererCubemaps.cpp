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

void RendererCubemaps::DrawLine(glm::vec3 start, glm::vec3 end, Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
    float lineVertices[] =
    {
        // position                    // color
        //    X        Y         Z        R     G     B     A
        start.x, start.y,  start.z,    1.0f, 0.0f, 0.0f, 1.0f,
        end.x,   end.y,    end.z,      0.0f, 1.0f, 0.0f, 1.0f,

        // 0.0f, 0.0f,  1.0f,    1.0f, 0.0f, 0.0f, 1.0f,
        // 0.0f, 0.0f, -1.0f,    0.0f, 1.0f, 0.0f, 1.0f,
    };

    // printf("Draw Line! Start %.2ff %.2ff %.2ff End %.2ff %.2ff %.2ff\n", start.x, start.y, start.z, end.x, end.y, end.z);

    // line VAO
    unsigned int m_LineVAO;
    unsigned int m_LineVBO;
    glGenVertexArrays(1, &m_LineVAO);
    glGenBuffers(1, &m_LineVBO);
    glBindVertexArray(m_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), &lineVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

    // line
    shader->Bind();
    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);

    glBindVertexArray(m_LineVAO);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void RendererCubemaps::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SceneCubemaps* sceneCubemaps = (SceneCubemaps*)scene;

    // Experimenting with ray casting and MousePicker
    MousePicker* mp = MousePicker::Get();
    glm::vec3 camPos = scene->GetCamera()->GetPosition();
    sceneCubemaps->m_LineStart = mp->GetPointOnRay(scene->GetCamera()->GetPosition() * glm::vec3(1.0f, 1.001f, 1.0f), 0.1f);
    sceneCubemaps->m_LineEnd = mp->GetPointOnRay(scene->GetCamera()->GetPosition(), 100.0f);
    DrawLine(sceneCubemaps->m_LineStart, sceneCubemaps->m_LineEnd, shaders["basic"], projectionMatrix, scene->GetCamera()->CalculateViewMatrix());

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
