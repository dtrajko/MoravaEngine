#include "RendererCubemaps.h"

#include "SceneCubemaps.h"
#include "MousePicker.h"
#include "GeometryFactory.h"

#include <stdexcept>


RendererCubemaps::RendererCubemaps()
{
}

void RendererCubemaps::Init(Scene* scene)
{
    SceneCubemaps* sceneCubemaps = (SceneCubemaps*)scene;
    models = ((SceneCubemaps*)scene)->GetModels();

	SetUniforms();
	SetShaders();

    if (sceneCubemaps->m_AABBEnabled)
        m_CubeAABB = new AABB(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

    m_PivotCube = new Pivot(glm::vec3(0.0f), glm::vec3(2.0f));
    m_PivotScene = new Pivot(glm::vec3(0.0f), glm::vec3(60.0f));
}

void RendererCubemaps::SetUniforms()
{
}

void RendererCubemaps::SetShaders()
{
	Shader* shaderCubemaps = new Shader("Shaders/LearnOpenGL/6.2.cubemaps.vs", "Shaders/LearnOpenGL/6.2.cubemaps.fs");
	shaders.insert(std::make_pair("cubemaps", shaderCubemaps));
	printf("RendererCubemaps: shaderCubemaps compiled [programID=%d]\n", shaderCubemaps->GetProgramID());

    Shader* shaderCubemapsNanosuit = new Shader("Shaders/LearnOpenGL/6.2.cubemaps_nanosuit.vs", "Shaders/LearnOpenGL/6.2.cubemaps.fs");
    shaders.insert(std::make_pair("cubemaps_nanosuit", shaderCubemapsNanosuit));
    printf("RendererCubemaps: shaderCubemapsNanosuit compiled [programID=%d]\n", shaderCubemapsNanosuit->GetProgramID());

	Shader* shaderSkybox = new Shader("Shaders/LearnOpenGL/6.2.skybox.vs", "Shaders/LearnOpenGL/6.2.skybox.fs");
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

    shaders["cubemaps_nanosuit"]->Bind();
    shaders["cubemaps_nanosuit"]->setInt("skybox", 0);

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
    MousePicker* mp = MousePicker::Get();

    // Experimenting with ray casting and MousePicker
    mp->GetPointOnRay(scene->GetCamera()->GetPosition(), mp->GetCurrentRay(), mp->m_RayRange);
    Raycast* raycast = sceneCubemaps->GetRaycast();
    raycast->m_Hit = mp->m_Hit;
    raycast->Draw(mp->m_RayStartPoint + scene->GetCamera()->GetFront() * 0.1f, mp->GetCurrentRay() * mp->m_RayRange, raycast->m_Color,
        shaders["basic"], projectionMatrix, scene->GetCameraController()->CalculateViewMatrix());

    shaders["cubemaps"]->Bind();
    shaders["cubemaps"]->setMat4("projection", projectionMatrix);
    shaders["cubemaps"]->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());
    shaders["cubemaps"]->setVec3("cameraPos", scene->GetCamera()->GetPosition());

    shaders["cubemaps_nanosuit"]->Bind();
    shaders["cubemaps_nanosuit"]->setMat4("projection", projectionMatrix);
    shaders["cubemaps_nanosuit"]->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());
    shaders["cubemaps_nanosuit"]->setVec3("cameraPos", scene->GetCamera()->GetPosition());

    glm::mat4 model = glm::mat4(1.0f);
 
    int terrainWidth = sceneCubemaps->GetTerrain()->GetHeightMap()->GetWidth();
    int terrainHeight = sceneCubemaps->GetTerrain()->GetHeightMap()->GetHeight();

    shaders["cubemaps"]->Bind();

    // Draw cube terrain
    if (sceneCubemaps->m_CubeTerrainEnabled)
    {
        for (int th = -(terrainHeight / 2) + 1; th < (terrainHeight / 2); th++)
        {
            for (int tw = -(terrainWidth / 2) + 1; tw < (terrainWidth / 2); tw++)
            {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(tw, (int)sceneCubemaps->GetTerrain()->GetMaxY(tw, th), th));
                model = glm::scale(model, glm::vec3(1.0f));
                shaders["cubemaps"]->setMat4("model", model);
                shaders["cubemaps"]->setVec4("tintColor", glm::vec4(1.0f, 0.6f, 0.4f, 0.9f));
                glBindVertexArray(GeometryFactory::CubeNormals::GetVAO());
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTextureID());
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glBindVertexArray(0);
            }
        }
    }

    // cube
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        if (mp->m_TestPoint.x > -(terrainWidth / 2) && mp->m_TestPoint.x <= (terrainWidth / 2) &&
            mp->m_TestPoint.z > -(terrainHeight / 2) && mp->m_TestPoint.z <= (terrainHeight / 2))
        {
            m_ModelCube = glm::mat4(1.0f);
            m_ModelCube = glm::scale(m_ModelCube, glm::vec3(1.0f));
            glm::vec3 cubePosition = glm::vec3(mp->m_TestPoint.x, (int)mp->m_TerrainHeight + 1.0f, mp->m_TestPoint.z);
            m_ModelCube = glm::translate(m_ModelCube, cubePosition);
    
            if (sceneCubemaps->m_AABBEnabled)
                m_CubeAABB->UpdatePosition(cubePosition);

            m_PivotCube->UpdatePosition(cubePosition);
        }
    }

    shaders["cubemaps"]->setMat4("model", m_ModelCube);
    shaders["cubemaps"]->setVec4("tintColor", glm::vec4(0.0f, 1.0f, 1.0f, 0.9f));
    glBindVertexArray(GeometryFactory::CubeNormals::GetVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTextureID());
    if (sceneCubemaps->m_ModelCubeEnabled)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    shaders["basic"]->Bind();
    shaders["basic"]->setMat4("model", glm::mat4(1.0f));
    shaders["basic"]->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());
    shaders["basic"]->setMat4("projection", projectionMatrix);

    if (sceneCubemaps->m_AABBEnabled)
        m_CubeAABB->Draw();

    m_PivotCube->Draw(shaders["basic"], projectionMatrix, scene->GetCameraController()->CalculateViewMatrix());
    m_PivotScene->Draw(shaders["basic"], projectionMatrix, scene->GetCameraController()->CalculateViewMatrix());

    // Draw the Nanosuit model
    shaders["cubemaps_nanosuit"]->Bind();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.2f));
    shaders["cubemaps_nanosuit"]->setMat4("model", model);
    // shaders["cubemaps"]->setVec4("tintColor", glm::vec4(0.8281f, 0.6836f, 0.2148f, 0.9f)); // Gold color
    shaders["cubemaps_nanosuit"]->setVec4("tintColor", glm::vec4(0.7529f, 0.7529f, 0.7529f, 0.9f)); // Silver color

    if (sceneCubemaps->m_NanosuitModelEnabled)
        models["nanosuit"]->Draw(shaders["cubemaps_nanosuit"]);

    shaders["framebuffers_scene"]->Bind();
    shaders["framebuffers_scene"]->setMat4("projection", projectionMatrix);
    shaders["framebuffers_scene"]->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());

    /* Floor */
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f));
    shaders["framebuffers_scene"]->setMat4("model", model);
    shaders["framebuffers_scene"]->setInt("texture1", 0);
    scene->GetTextures()["semi_transparent"]->Bind(0);
    // scene->GetMeshes()["quad"]->Render();
 
    // Terrain
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f));
    shaders["framebuffers_scene"]->setMat4("model", model);
    shaders["framebuffers_scene"]->setInt("texture1", 0);
    scene->GetTextures()["semi_transparent"]->Bind(0);
    if (sceneCubemaps->m_TerrainEnabled)
        scene->GetMeshes()["terrain"]->Render();

    // draw skybox as last
    glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
    shaders["skybox"]->Bind();
    shaders["skybox"]->setMat4("view", glm::mat4(glm::mat3(scene->GetCameraController()->CalculateViewMatrix()))); // remove translation from the view matrix
    shaders["skybox"]->setMat4("projection", projectionMatrix);

    // skybox cube
    glBindVertexArray(GeometryFactory::Skybox::GetVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTextureID());
    if (sceneCubemaps->m_SkyboxEnabled)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
       
    std::string passType = "main";
    scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

RendererCubemaps::~RendererCubemaps()
{
}
