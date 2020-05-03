#include "SceneCubemaps.h"

#include "ImGuiWrapper.h"
#include "MousePicker.h"

#include <vector>
#include <string>


SceneCubemaps::SceneCubemaps()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 2.0f, 5.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.ambientIntensity = 0.4f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };
}

void SceneCubemaps::SetSkybox()
{
}

void SceneCubemaps::SetTextures()
{
    textures.insert(std::make_pair("semi_transparent", new Texture("Textures/semi_transparent.png")));
}

void SceneCubemaps::SetupMeshes()
{
    m_Quad = new Quad();
    meshes.insert(std::make_pair("quad", m_Quad));
}

void SceneCubemaps::SetupModels()
{
    // ModelJoey* nanosuit = new ModelJoey("Models/nanosuit.obj", "Textures/nanosuit");
    // models.insert(std::make_pair("nanosuit", nanosuit));
}

void SceneCubemaps::SetGeometry()
{
    float cubeVertices[] =
    {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    float skyboxVertices[] =
    {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // cube VAO
    glGenVertexArrays(1, &m_CubeVAO);
    glGenBuffers(1, &m_CubeVBO);
    glBindVertexArray(m_CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // skybox VAO
    glGenVertexArrays(1, &m_SkyboxVAO);
    glGenBuffers(1, &m_SkyboxVBO);
    glBindVertexArray(m_SkyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_SkyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

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

    m_TextureCubeMap = new TextureCubeMap(faces);
    m_TextureCubeMapID = m_TextureCubeMap->GetID();
}

void SceneCubemaps::Update(float timestep, Window& mainWindow)
{
}

void SceneCubemaps::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
    MousePicker* mp = MousePicker::Get();

    ImGui::Begin("Ray Casting");

    ImGui::Separator();
    std::string cameraPosition = "Camera Position: X = " + std::to_string(mp->m_CameraPosition.x) +
                                                 " Y = " + std::to_string(mp->m_CameraPosition.y) +
                                                 " Z = " + std::to_string(mp->m_CameraPosition.z);
    ImGui::Text(cameraPosition.c_str());
    ImGui::Separator();

    std::string mouseCoords = "Mouse Coordinates: MouseX = " + std::to_string(mp->m_MouseX) +
        " MouseY = " + std::to_string(mp->m_MouseY);
    ImGui::Text(mouseCoords.c_str());
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

    ImGui::SliderFloat3("Ray Start", glm::value_ptr(m_Raycast->m_LineStart), -10.0f, 10.0f);
    ImGui::SliderFloat3("Ray End",   glm::value_ptr(m_Raycast->m_LineEnd),   -10.0f, 10.0f);
    // ImGui::ColorEdit4("Ray Color",   glm::value_ptr(m_Raycast->m_Color));

    ImGui::Separator();
    ImGui::SliderFloat3("Intersection point", glm::value_ptr(mp->m_IntersectionPoint), -10.0f, 10.0f);

    ImGui::End();
}

void SceneCubemaps::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

void SceneCubemaps::CleanupGeometry()
{
    glDeleteVertexArrays(1, &m_CubeVAO);
    glDeleteVertexArrays(1, &m_SkyboxVAO);
    glDeleteBuffers(1, &m_CubeVBO);
    glDeleteBuffers(1, &m_SkyboxVAO);
}

SceneCubemaps::~SceneCubemaps()
{
	CleanupGeometry();
    delete m_TextureCubeMap;
    delete m_Raycast;
}
