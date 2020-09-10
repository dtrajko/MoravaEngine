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
    s_Shaders.insert(std::make_pair("cubemaps", shaderCubemaps));
	printf("RendererCubemaps: shaderCubemaps compiled [programID=%d]\n", shaderCubemaps->GetProgramID());

    Shader* shaderCubemapsNanosuit = new Shader("Shaders/LearnOpenGL/6.2.cubemaps_nanosuit.vs", "Shaders/LearnOpenGL/6.2.cubemaps.fs");
    s_Shaders.insert(std::make_pair("cubemaps_nanosuit", shaderCubemapsNanosuit));
    printf("RendererCubemaps: shaderCubemapsNanosuit compiled [programID=%d]\n", shaderCubemapsNanosuit->GetProgramID());

	Shader* shaderSkybox = new Shader("Shaders/LearnOpenGL/6.2.skybox.vs", "Shaders/LearnOpenGL/6.2.skybox.fs");
    s_Shaders.insert(std::make_pair("skybox", shaderSkybox));
	printf("RendererCubemaps: shaderSkybox compiled [programID=%d]\n", shaderSkybox->GetProgramID());

    Shader* shaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    s_Shaders.insert(std::make_pair("basic", shaderBasic));
    printf("RendererCubemaps: shaderBasic compiled [programID=%d]\n", shaderBasic->GetProgramID());

    Shader* shaderFramebuffersScene = new Shader("Shaders/framebuffers_scene.vs", "Shaders/framebuffers_scene.fs");
    s_Shaders.insert(std::make_pair("framebuffers_scene", shaderFramebuffersScene));
    printf("RendererCubemaps: shaderFramebuffersScene compiled [programID=%d]\n", shaderFramebuffersScene->GetProgramID());

	// shader configuration
    s_Shaders["cubemaps"]->Bind();
    s_Shaders["cubemaps"]->setInt("skybox", 0);

    s_Shaders["cubemaps_nanosuit"]->Bind();
    s_Shaders["cubemaps_nanosuit"]->setInt("skybox", 0);

    s_Shaders["skybox"]->Bind();
    s_Shaders["skybox"]->setInt("skybox", 0);
}

void RendererCubemaps::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererCubemaps::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    EnableTransparency();

    SceneCubemaps* sceneCubemaps = (SceneCubemaps*)scene;
    MousePicker* mp = MousePicker::Get();

    // Experimenting with ray casting and MousePicker
    mp->GetPointOnRay(scene->GetCamera()->GetPosition(), mp->GetCurrentRay(), mp->m_RayRange);
    Raycast* raycast = sceneCubemaps->GetRaycast();
    raycast->m_Hit = mp->m_Hit;
    raycast->Draw(mp->m_RayStartPoint + scene->GetCamera()->GetFront() * 0.1f, mp->GetCurrentRay() * mp->m_RayRange, raycast->m_Color,
        s_Shaders["basic"], projectionMatrix, scene->GetCameraController()->CalculateViewMatrix());

    s_Shaders["cubemaps"]->Bind();
    s_Shaders["cubemaps"]->setMat4("projection", projectionMatrix);
    s_Shaders["cubemaps"]->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());
    s_Shaders["cubemaps"]->setVec3("cameraPos", scene->GetCamera()->GetPosition());

    s_Shaders["cubemaps_nanosuit"]->Bind();
    s_Shaders["cubemaps_nanosuit"]->setMat4("projection", projectionMatrix);
    s_Shaders["cubemaps_nanosuit"]->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());
    s_Shaders["cubemaps_nanosuit"]->setVec3("cameraPos", scene->GetCamera()->GetPosition());

    glm::mat4 model = glm::mat4(1.0f);
 
    int terrainWidth = sceneCubemaps->GetTerrain()->GetHeightMap()->GetWidth();
    int terrainHeight = sceneCubemaps->GetTerrain()->GetHeightMap()->GetHeight();

    s_Shaders["cubemaps"]->Bind();

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
                s_Shaders["cubemaps"]->setMat4("model", model);
                s_Shaders["cubemaps"]->setVec4("tintColor", glm::vec4(1.0f, 0.6f, 0.4f, 0.9f));
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

    s_Shaders["cubemaps"]->setMat4("model", m_ModelCube);
    s_Shaders["cubemaps"]->setVec4("tintColor", glm::vec4(0.0f, 1.0f, 1.0f, 0.9f));
    glBindVertexArray(GeometryFactory::CubeNormals::GetVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTextureID());
    if (sceneCubemaps->m_ModelCubeEnabled)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    s_Shaders["basic"]->Bind();
    s_Shaders["basic"]->setMat4("model", glm::mat4(1.0f));
    s_Shaders["basic"]->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());
    s_Shaders["basic"]->setMat4("projection", projectionMatrix);

    if (sceneCubemaps->m_AABBEnabled)
        m_CubeAABB->Draw();

    m_PivotCube->Draw(s_Shaders["basic"], projectionMatrix, scene->GetCameraController()->CalculateViewMatrix());
    m_PivotScene->Draw(s_Shaders["basic"], projectionMatrix, scene->GetCameraController()->CalculateViewMatrix());

    // Draw the Nanosuit model
    s_Shaders["cubemaps_nanosuit"]->Bind();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.2f));
    s_Shaders["cubemaps_nanosuit"]->setMat4("model", model);
    // s_Shaders["cubemaps"]->setVec4("tintColor", glm::vec4(0.8281f, 0.6836f, 0.2148f, 0.9f)); // Gold color
    s_Shaders["cubemaps_nanosuit"]->setVec4("tintColor", glm::vec4(0.7529f, 0.7529f, 0.7529f, 0.9f)); // Silver color

    if (sceneCubemaps->m_NanosuitModelEnabled)
        models["nanosuit"]->Draw(s_Shaders["cubemaps_nanosuit"]);

    s_Shaders["framebuffers_scene"]->Bind();
    s_Shaders["framebuffers_scene"]->setMat4("projection", projectionMatrix);
    s_Shaders["framebuffers_scene"]->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());

    /* Floor */
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f));
    s_Shaders["framebuffers_scene"]->setMat4("model", model);
    s_Shaders["framebuffers_scene"]->setInt("texture1", 0);
    scene->GetTextures()["semi_transparent"]->Bind(0);
    // scene->GetMeshes()["quad"]->Render();
 
    // Terrain
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f));
    s_Shaders["framebuffers_scene"]->setMat4("model", model);
    s_Shaders["framebuffers_scene"]->setInt("texture1", 0);
    scene->GetTextures()["semi_transparent"]->Bind(0);
    if (sceneCubemaps->m_TerrainEnabled)
        scene->GetMeshes()["terrain"]->Render();

    // draw skybox as last
    glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
    s_Shaders["skybox"]->Bind();
    s_Shaders["skybox"]->setMat4("view", glm::mat4(glm::mat3(scene->GetCameraController()->CalculateViewMatrix()))); // remove translation from the view matrix
    s_Shaders["skybox"]->setMat4("projection", projectionMatrix);

    // skybox cube
    glBindVertexArray(GeometryFactory::Skybox::GetVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sceneCubemaps->GetCubemapTextureID());
    if (sceneCubemaps->m_SkyboxEnabled)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
       
    std::string passType = "main";
    scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

RendererCubemaps::~RendererCubemaps()
{
}
