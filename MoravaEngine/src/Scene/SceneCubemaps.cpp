#include "Scene/SceneCubemaps.h"

#include "../../ImGuiWrapper.h"
#include "../../MousePicker.h"

#include "Mesh/GeometryFactory.h"

#include <vector>
#include <string>


SceneCubemaps::SceneCubemaps()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 6.0f, 15.0f);
	sceneSettings.cameraStartYaw   = -90.0f;
    sceneSettings.cameraStartPitch = -20.0f;
	sceneSettings.cameraMoveSpeed  = 1.0f;

	SetCamera();
	SetSkybox();
	SetupTextures();
    SetupShaders();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };

    if (m_AABBEnabled)
        m_CubeAABB = new AABB(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

    m_PivotCube = new Pivot(glm::vec3(0.0f), glm::vec3(2.0f));
    m_PivotScene = new Pivot(glm::vec3(0.0f), glm::vec3(60.0f));
}

void SceneCubemaps::SetSkybox()
{
}

void SceneCubemaps::SetupTextures()
{
    textures.insert(std::make_pair("semi_transparent", TextureLoader::Get()->GetTexture("Textures/semi_transparent.png", false, false)));
}

void SceneCubemaps::SetupShaders()
{
    m_ShaderCubemaps = new Shader("Shaders/LearnOpenGL/6.2.cubemaps.vs", "Shaders/LearnOpenGL/6.2.cubemaps.fs");
    printf("SceneCubemaps: m_ShaderCubemaps compiled [programID=%d]\n", m_ShaderCubemaps->GetProgramID());

    m_ShaderCubemapsNanosuit = new Shader("Shaders/LearnOpenGL/6.2.cubemaps_nanosuit.vs", "Shaders/LearnOpenGL/6.2.cubemaps.fs");
    printf("SceneCubemaps: m_ShaderCubemapsNanosuit compiled [programID=%d]\n", m_ShaderCubemapsNanosuit->GetProgramID());

    m_ShaderSkybox = new Shader("Shaders/LearnOpenGL/6.2.skybox.vs", "Shaders/LearnOpenGL/6.2.skybox.fs");
    printf("SceneCubemaps: m_ShaderSkybox compiled [programID=%d]\n", m_ShaderSkybox->GetProgramID());

    m_ShaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    printf("SceneCubemaps: m_ShaderBasic compiled [programID=%d]\n", m_ShaderBasic->GetProgramID());

    m_ShaderFramebuffersScene = new Shader("Shaders/framebuffers_scene.vs", "Shaders/framebuffers_scene.fs");
    printf("SceneCubemaps: m_ShaderFramebuffersScene compiled [programID=%d]\n", m_ShaderFramebuffersScene->GetProgramID());

    // shader configuration
    m_ShaderCubemaps->Bind();
    m_ShaderCubemaps->setInt("skybox", 0);

    m_ShaderCubemapsNanosuit->Bind();
    m_ShaderCubemapsNanosuit->setInt("skybox", 0);

    m_ShaderSkybox->Bind();
    m_ShaderSkybox->setInt("skybox", 0);
}

void SceneCubemaps::SetupMeshes()
{
    m_Quad = new Quad();
    meshes.insert(std::make_pair("quad", m_Quad));
}

void SceneCubemaps::SetupModels()
{
    if (m_NanosuitModelEnabled)
    {
        ModelJoey* nanosuit = new ModelJoey("Models/nanosuit.obj", "Textures/nanosuit");
        models.insert(std::make_pair("nanosuit", nanosuit));
    }

    m_Terrain = new TerrainHeightMap("Textures/heightmap_16x16.png", 4.0f, nullptr);
    meshes.insert(std::make_pair("terrain", m_Terrain));

    MousePicker::Get()->SetTerrain(m_Terrain);
}

void SceneCubemaps::SetGeometry()
{
    GeometryFactory::CubeNormals::Create();
    GeometryFactory::Skybox::Create();

    // load textures
    // -------------
    std::vector<std::string> faces
    {
        "Textures/skybox_cubemaps/right.jpg",
        "Textures/skybox_cubemaps/left.jpg",
        "Textures/skybox_cubemaps/top.jpg",
        "Textures/skybox_cubemaps/bottom.jpg",
        "Textures/skybox_cubemaps/front.jpg",
        "Textures/skybox_cubemaps/back.jpg",
    };

    m_TextureCubeMap = new TextureCubemapFaces(faces);
    m_TextureCubeMapID = m_TextureCubeMap->GetID();
}

void SceneCubemaps::Update(float timestep, Window* mainWindow)
{
    m_Camera->OnUpdate(timestep);
}

void SceneCubemaps::UpdateImGui(float timestep, Window* mainWindow)
{
    MousePicker* mp = MousePicker::Get();

    ImGui::Begin("Ray Casting");
    {
        ImGui::Separator();
        std::string screenMouseCoords = "Screen Mouse Coord: MouseX = " + std::to_string(mp->m_ScreenMouseX) +
            " MouseY = " + std::to_string(mp->m_ScreenMouseY);
        ImGui::Text(screenMouseCoords.c_str());

        ImGui::Separator();
        std::string viewportMouseCoords = "Viewport Mouse Coord: MouseX = " + std::to_string(mp->m_Viewport.MouseX) +
            " MouseY = " + std::to_string(mp->m_Viewport.MouseY);
        ImGui::Text(viewportMouseCoords.c_str());

        ImGui::Separator();
        std::string normalizedCoords = "Normalized Coords: X = " + std::to_string(mp->m_NormalizedCoords.x) +
                                                         " Y = " + std::to_string(mp->m_NormalizedCoords.y);
        ImGui::Text(normalizedCoords.c_str());
        ImGui::Separator();
        std::string clipCoords = "Clip Coords: X = " + std::to_string(mp->m_ClipCoords.x) +
                                             " Y = " + std::to_string(mp->m_ClipCoords.y);
        ImGui::Text(clipCoords.c_str());
        ImGui::Separator();
        std::string eyeCoords = "Eye Coords: X = " + std::to_string(mp->m_EyeCoords.x) + " Y = " + std::to_string(mp->m_EyeCoords.y) +
            " Z = " + std::to_string(mp->m_EyeCoords.z) + " W = " + std::to_string(mp->m_EyeCoords.w);
        ImGui::Text(eyeCoords.c_str());
        ImGui::Separator();
        std::string worldRay = "World Ray: X = " + std::to_string(mp->m_WorldRay.x) +
            " Y = " + std::to_string(mp->m_WorldRay.y) +
            " Z = " + std::to_string(mp->m_WorldRay.z);
        ImGui::Text(worldRay.c_str());
        ImGui::Separator();
        ImGui::Checkbox("Terrain Enabled", &m_TerrainEnabled);
        ImGui::Checkbox("Cube Terrain Enabled", &m_CubeTerrainEnabled);
        ImGui::Separator();
        ImGui::SliderFloat3("Test Point", glm::value_ptr(mp->m_TestPoint), -20.0f, 20.0f);
        ImGui::SliderInt("Terrain Height", &mp->m_TerrainHeight, -20, 20);
        ImGui::SliderFloat3("Ray Start", glm::value_ptr(m_Raycast->m_LineStart), -10.0f, 10.0f);
        ImGui::SliderFloat3("Ray End", glm::value_ptr(m_Raycast->m_LineEnd), -10.0f, 10.0f);
        // ImGui::ColorEdit4("Ray Color",   glm::value_ptr(m_Raycast->m_Color));
        ImGui::Separator();
        ImGui::SliderFloat3("Intersection point", glm::value_ptr(mp->m_IntersectionPoint), -10.0f, 10.0f);
    }
    ImGui::End();
}

void SceneCubemaps::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    RendererBasic::EnableTransparency();

    MousePicker* mp = MousePicker::Get();

    // Experimenting with ray casting and MousePicker
    mp->GetPointOnRay(m_Camera->GetPosition(), mp->GetCurrentRay(), mp->m_RayRange);
    m_Raycast->m_Hit = mp->m_Hit;
    m_Raycast->Draw(mp->m_RayStartPoint + m_Camera->GetFront() * 0.1f, mp->GetCurrentRay() * mp->m_RayRange, m_Raycast->m_Color,
        m_ShaderBasic, projectionMatrix, m_Camera->GetViewMatrix());

    m_ShaderCubemaps->Bind();
    m_ShaderCubemaps->setMat4("projection", projectionMatrix);
    m_ShaderCubemaps->setMat4("view", m_Camera->GetViewMatrix());
    m_ShaderCubemaps->setVec3("cameraPos", m_Camera->GetPosition());

    m_ShaderCubemapsNanosuit->Bind();
    m_ShaderCubemapsNanosuit->setMat4("projection", projectionMatrix);
    m_ShaderCubemapsNanosuit->setMat4("view", m_Camera->GetViewMatrix());
    m_ShaderCubemapsNanosuit->setVec3("cameraPos", m_Camera->GetPosition());

    glm::mat4 model = glm::mat4(1.0f);

    int terrainWidth = m_Terrain->GetHeightMap()->GetWidth();
    int terrainHeight = m_Terrain->GetHeightMap()->GetHeight();

    m_ShaderCubemaps->Bind();

    // Draw cube terrain
    if (m_CubeTerrainEnabled)
    {
        for (int th = -(terrainHeight / 2) + 1; th < (terrainHeight / 2); th++)
        {
            for (int tw = -(terrainWidth / 2) + 1; tw < (terrainWidth / 2); tw++)
            {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(tw, (int)m_Terrain->GetMaxY(tw, th), th));
                model = glm::scale(model, glm::vec3(1.0f));
                m_ShaderCubemaps->setMat4("model", model);
                m_ShaderCubemaps->setVec4("tintColor", glm::vec4(1.0f, 0.6f, 0.4f, 0.9f));
                glBindVertexArray(GeometryFactory::CubeNormals::GetVAO());
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureCubeMapID);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glBindVertexArray(0);
            }
        }
    }

    // cube
    if (mainWindow->getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        if (mp->m_TestPoint.x > -(terrainWidth / 2) && mp->m_TestPoint.x <= (terrainWidth / 2) &&
            mp->m_TestPoint.z > -(terrainHeight / 2) && mp->m_TestPoint.z <= (terrainHeight / 2))
        {
            m_ModelCube = glm::mat4(1.0f);
            m_ModelCube = glm::scale(m_ModelCube, glm::vec3(1.0f));
            glm::vec3 cubePosition = glm::vec3(mp->m_TestPoint.x, (int)mp->m_TerrainHeight + 1.0f, mp->m_TestPoint.z);
            m_ModelCube = glm::translate(m_ModelCube, cubePosition);

            if (m_AABBEnabled)
                m_CubeAABB->UpdatePosition(cubePosition);

            m_PivotCube->UpdatePosition(cubePosition);
        }
    }

    m_ShaderCubemaps->setMat4("model", m_ModelCube);
    m_ShaderCubemaps->setVec4("tintColor", glm::vec4(0.0f, 1.0f, 1.0f, 0.9f));
    glBindVertexArray(GeometryFactory::CubeNormals::GetVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureCubeMapID);
    if (m_ModelCubeEnabled)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    m_ShaderBasic->Bind();
    m_ShaderBasic->setMat4("model", glm::mat4(1.0f));
    m_ShaderBasic->setMat4("view", m_Camera->GetViewMatrix());
    m_ShaderBasic->setMat4("projection", projectionMatrix);

    if (m_AABBEnabled)
        m_CubeAABB->Draw();

    m_PivotCube->Draw(m_ShaderBasic, projectionMatrix, m_Camera->GetViewMatrix());
    m_PivotScene->Draw(m_ShaderBasic, projectionMatrix, m_Camera->GetViewMatrix());

    // Draw the Nanosuit model
    m_ShaderCubemapsNanosuit->Bind();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.2f));
    m_ShaderCubemapsNanosuit->setMat4("model", model);
    // m_ShaderCubemaps->setVec4("tintColor", glm::vec4(0.8281f, 0.6836f, 0.2148f, 0.9f)); // Gold color
    m_ShaderCubemapsNanosuit->setVec4("tintColor", glm::vec4(0.7529f, 0.7529f, 0.7529f, 0.9f)); // Silver color

    if (m_NanosuitModelEnabled)
        models["nanosuit"]->Draw(m_ShaderCubemapsNanosuit);

    m_ShaderFramebuffersScene->Bind();
    m_ShaderFramebuffersScene->setMat4("projection", projectionMatrix);
    m_ShaderFramebuffersScene->setMat4("view", m_Camera->GetViewMatrix());

    /* Floor */
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f));
    m_ShaderFramebuffersScene->setMat4("model", model);
    m_ShaderFramebuffersScene->setInt("texture1", 0);
    textures["semi_transparent"]->Bind(0);
    // scene->GetMeshes()["quad"]->Render();

    // Terrain
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f));
    m_ShaderFramebuffersScene->setMat4("model", model);
    m_ShaderFramebuffersScene->setInt("texture1", 0);
    textures["semi_transparent"]->Bind(0);
    if (m_TerrainEnabled)
        meshes["terrain"]->Render();

    // draw skybox as last
    glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
    m_ShaderSkybox->Bind();
    m_ShaderSkybox->setMat4("view", glm::mat4(glm::mat3(m_Camera->GetViewMatrix()))); // remove translation from the view matrix
    m_ShaderSkybox->setMat4("projection", projectionMatrix);

    // skybox cube
    glBindVertexArray(GeometryFactory::Skybox::GetVAO());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureCubeMapID);
    if (m_SkyboxEnabled)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}

void SceneCubemaps::CleanupGeometry()
{
    GeometryFactory::CubeNormals::Destroy();
    GeometryFactory::Skybox::Destroy();
}

SceneCubemaps::~SceneCubemaps()
{
	CleanupGeometry();
    delete m_TextureCubeMap;
    delete m_Raycast;
}
