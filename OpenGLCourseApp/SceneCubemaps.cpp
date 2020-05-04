#include "SceneCubemaps.h"

#include "ImGuiWrapper.h"
#include "MousePicker.h"
#include "GeometryFactory.h"

#include <vector>
#include <string>


SceneCubemaps::SceneCubemaps()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 6.0f, 15.0f);
	sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = -20.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;

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
    if (m_NanosuitModelEnabled)
    {
        ModelJoey* nanosuit = new ModelJoey("Models/nanosuit.obj", "Textures/nanosuit");
        models.insert(std::make_pair("nanosuit", nanosuit));
    }

    m_Terrain = new Terrain("Textures/heightmap_16x16.png", 4.0f, nullptr);
    Mesh* mesh = new Mesh();
    mesh->Create(m_Terrain->GetVertices(), m_Terrain->GetIndices(), m_Terrain->GetVertexCount(), m_Terrain->GetIndexCount());
    meshes.insert(std::make_pair("terrain", mesh));

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

    m_TextureCubeMap = new TextureCubeMap(faces);
    m_TextureCubeMapID = m_TextureCubeMap->GetID();
}

void SceneCubemaps::Update(float timestep, Window& mainWindow)
{
}

void SceneCubemaps::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
    MousePicker* mp = MousePicker::Get();

    // No support in shader for directional light
    // glm::vec3 dirLightDirection = m_LightManager->directionalLight.GetDirection();
    // glm::vec3 dirLightColor = m_LightManager->directionalLight.GetColor();

    ImGui::Begin("Ray Casting");
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
    ImGui::Checkbox("Terrain Enabled", &m_TerrainEnabled);
    ImGui::Checkbox("Cube Terrain Enabled", &m_CubeTerrainEnabled);
    ImGui::Separator();
    ImGui::SliderFloat3("Test Point", glm::value_ptr(mp->m_TestPoint), -20.0f, 20.0f);
    ImGui::SliderInt("Terrain Height", &mp->m_TerrainHeight, -20, 20);
    ImGui::SliderFloat3("Ray Start", glm::value_ptr(m_Raycast->m_LineStart), -10.0f, 10.0f);
    ImGui::SliderFloat3("Ray End",   glm::value_ptr(m_Raycast->m_LineEnd),   -10.0f, 10.0f);
    // ImGui::ColorEdit4("Ray Color",   glm::value_ptr(m_Raycast->m_Color));
    ImGui::Separator();
    ImGui::SliderFloat3("Intersection point", glm::value_ptr(mp->m_IntersectionPoint), -10.0f, 10.0f);
    // No support in shader for directional light
    // ImGui::Separator();
    // ImGui::SliderFloat3("DirLight Direction", glm::value_ptr(dirLightDirection), -1.0f, 1.0f);
    // ImGui::ColorEdit3("DirLight Color", glm::value_ptr(dirLightColor));
    ImGui::End();

    // No support in shader for directional light
    // m_LightManager->directionalLight.SetDirection(dirLightDirection);
    // m_LightManager->directionalLight.SetColor(dirLightColor);
}

void SceneCubemaps::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
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
