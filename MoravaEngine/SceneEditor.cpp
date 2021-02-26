#define _CRT_SECURE_NO_WARNINGS

#include "SceneEditor.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Hazel/Renderer/HazelMesh.h"

#include "Application.h"
#include "Block.h"
#include "Cylinder.h"
#include "Cone.h"
#include "ImGuiWrapper.h"
#include "Log.h"
#include "Math.h"
#include "MousePicker.h"
#include "ParticleMaster.h"
#include "Pyramid.h"
#include "Profiler.h"
#include "PerlinNoise/PerlinNoise.hpp"
#include "ResourceManager.h"
#include "Ring.h"
#include "Shader.h"
#include "Sphere.h"
#include "TextureLoader.h"
#include "Tile2D.h"
#include "Timer.h"
#include "TerrainHeightMap.h"

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>


SceneEditor::SceneEditor()
{
    sceneSettings.cameraPosition   = glm::vec3(0.0f, 8.0f, 24.0f);
    sceneSettings.cameraStartYaw   = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
    sceneSettings.cameraMoveSpeed  = 1.0f;
    sceneSettings.waterHeight      = 0.0f;
    sceneSettings.waterWaveSpeed   = 0.05f;
    sceneSettings.enableSkybox       = false;
    sceneSettings.enablePointLights  = true;
    sceneSettings.enableSpotLights   = true;
    sceneSettings.enableShadows      = true;
    sceneSettings.enableOmniShadows  = true;
    sceneSettings.enableWaterEffects = true;
    sceneSettings.enableParticles    = true;

    // directional light
    sceneSettings.directionalLight.base.enabled = true;
    sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.directionalLight.direction = glm::vec3(0.6f, -0.5f, -0.6f);
    sceneSettings.directionalLight.base.ambientIntensity = 0.75f;
    sceneSettings.directionalLight.base.diffuseIntensity = 0.4f;
    sceneSettings.lightProjectionMatrix = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 0.1f, 40.0f);

    // point lights
    sceneSettings.pointLights[0].base.enabled = false;
    sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.pointLights[0].position = glm::vec3(-5.0f, 0.5f, 5.0f);
    sceneSettings.pointLights[0].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[0].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[1].base.enabled = false;
    sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.pointLights[1].position = glm::vec3(5.0f, 0.5f, 5.0f);
    sceneSettings.pointLights[1].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[1].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[2].base.enabled = false;
    sceneSettings.pointLights[2].base.color = glm::vec3(0.0f, 1.0f, 1.0f);
    sceneSettings.pointLights[2].position = glm::vec3(-5.0f, 0.5f, -5.0f);
    sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[2].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[3].base.enabled = false;
    sceneSettings.pointLights[3].base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.pointLights[3].position = glm::vec3(5.0f, 0.5f, -5.0f);
    sceneSettings.pointLights[3].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[3].base.diffuseIntensity = 1.0f;

    // spot lights
    sceneSettings.spotLights[0].base.base.enabled = false;
    sceneSettings.spotLights[0].base.base.color = glm::vec3(1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[0].base.position = glm::vec3(-5.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[0].direction = glm::vec3(1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[0].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[0].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[0].edge = 0.5f;

    sceneSettings.spotLights[1].base.base.enabled = false;
    sceneSettings.spotLights[1].base.base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[1].base.position = glm::vec3(5.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[1].direction = glm::vec3(-1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[1].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[1].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[1].edge = 0.5f;

    sceneSettings.spotLights[2].base.base.enabled = false;
    sceneSettings.spotLights[2].base.base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[2].base.position = glm::vec3(0.0f, 1.0f, -5.0f);
    sceneSettings.spotLights[2].direction = glm::vec3(0.0f, 0.0f, 1.0f);
    sceneSettings.spotLights[2].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[2].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[2].edge = 0.5f;

    sceneSettings.spotLights[3].base.base.enabled = false;
    sceneSettings.spotLights[3].base.base.color = glm::vec3(1.0f, 0.0f, 1.0f);
    sceneSettings.spotLights[3].base.position = glm::vec3(0.0f, 1.0f, 5.0f);
    sceneSettings.spotLights[3].direction = glm::vec3(0.0f, 0.0f, -1.0f);
    sceneSettings.spotLights[3].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[3].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[3].edge = 0.5f;

    m_IsViewportEnabled = true;

    ResourceManager::Init();

    SetCamera();
    SetSkybox();
    SetupTextures();
    SetupMaterials();
    SetupMeshes();
    SetupModels();
    SetupRenderFramebuffer();
    SetGeometry();
    SetLightManager();
    AddLightsToSceneObjects();

    // Initialize the PBR/IBL Material Workflow component
    m_MaterialWorkflowPBR = new MaterialWorkflowPBR();

    m_BlurEffect = new BlurEffect();

    m_CurrentTimestamp = 0.0f;
    m_StartTimestamp = (float)glfwGetTime();

    m_ObjectSelect      = { 0.0f, 0.2f };
    m_ObjectAdd         = { 0.0f, 1.0f };
    m_ObjectCopy        = { 0.0f, 1.0f };
    m_ObjectDelete      = { 0.0f, 1.0f };
    m_SceneSave         = { 0.0f, 1.0f };
    m_SceneLoad         = { 0.0f, 1.0f };
    m_SceneReset        = { 0.0f, 1.0f };
    m_ProjectionChange  = { 0.0f, 0.5f };
    m_ParticlesGenerate = { -1.0f, 0.5f };
    m_ResizeViewport    = { 0.0f, 0.5f };

    m_OrthographicViewEnabled = false;
    m_SelectedIndex = 0;
    m_CurrentObjectTypeID = MESH_TYPE_CUBE;
    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };
    m_Grid = new Grid(20);
    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f));
    m_Gizmo = new Gizmo();

    m_PositionEdit             = new glm::vec3(0.0f);
    m_RotationEdit             = new glm::vec3(0.0f);
    m_ScaleEdit                = new glm::vec3(1.0f);
    m_ColorEdit                = new glm::vec4(1.0f);
    m_TextureNameEdit          = new std::string;
    m_TilingFactorEdit         = new float(1.0f);
    m_MaterialNameEdit         = new std::string;
    m_TilingFactorMaterialEdit = new float(1.0f);
    m_DrawGizmos = true;
    m_PBR_Map_Edit = PBR_MAP_ENVIRONMENT;
    m_HDRI_Edit = HDRI_EARLY_EVE_WARM_SKY;
    m_HDRI_Edit_Prev = -1;
    m_BlurLevel = 0;
    m_BlurLevelPrev = m_BlurLevel;
    m_SkyboxLOD = 0.0f;

    // required for directional light enable/disable feature
    m_DirLightEnabledPrev = sceneSettings.directionalLight.base.enabled;
    m_DirLightColorPrev = sceneSettings.directionalLight.base.color;

    m_DisplayLightSources = true;
    m_MouseButton_1_Prev = false;
    m_UseCubeMaps = false;

    TextureLoader::Get()->Print();

    m_ParticleSettingsEdit = new ParticleSettings;
    m_ParticleSettingsEdit->textureName = "particle_atlas";
    m_ParticleSettingsPrev = m_ParticleSettingsEdit;

    m_ViewportFocused = false;
    m_ViewportHovered = false;

    //  // PBR texture inputs
    m_SamplerSlots.insert(std::make_pair("albedo",     1)); // uniform sampler2D u_AlbedoTexture
    m_SamplerSlots.insert(std::make_pair("normal",     2)); // uniform sampler2D u_NormalTexture
    m_SamplerSlots.insert(std::make_pair("metalness",  3)); // uniform sampler2D u_MetalnessTexture
    m_SamplerSlots.insert(std::make_pair("roughness",  4)); // uniform sampler2D u_RoughnessTexture
    m_SamplerSlots.insert(std::make_pair("ao",         5)); // uniform sampler2D u_AOTexture
    // Environment maps
    m_SamplerSlots.insert(std::make_pair("irradiance", 6)); // uniform samplerCube u_IrradianceMap
    m_SamplerSlots.insert(std::make_pair("prefilter",  7)); // uniform samplerCube u_PrefilterMap
    // BRDF LUT
    m_SamplerSlots.insert(std::make_pair("BRDF_LUT",   8)); // uniform sampler2D u_BRDFLUT

    m_SkyboxRotationSpeed = 0.0f;
}

void SceneEditor::SetLightManager()
{
    // Skip if Light Manager already initialized
    if (LightManager::pointLightCount > 0 || LightManager::spotLightCount > 0) return;
    LightManager::Init(sceneSettings);
}

void SceneEditor::SetupRenderFramebuffer()
{
    if (!m_IsViewportEnabled) return;

    uint32_t width = Application::Get()->GetWindow()->GetWidth();
    uint32_t height = Application::Get()->GetWindow()->GetHeight();
    m_RenderFramebuffer = new Framebuffer(width, height);

    m_RenderFramebuffer->AddAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    m_RenderFramebuffer->AddAttachmentSpecification(width, height, AttachmentType::Renderbuffer, AttachmentFormat::Depth);

    m_RenderFramebuffer->Generate(width, height);
}

void SceneEditor::SetSkybox()
{
}

void SceneEditor::SetupTextures()
{
    ResourceManager::LoadTexture(
        ResourceManager::GetTextureInfo()->find("none")->first,
        ResourceManager::GetTextureInfo()->find("none")->second);
    ResourceManager::LoadTexture(
        ResourceManager::GetTextureInfo()->find("fire")->first,
        ResourceManager::GetTextureInfo()->find("fire")->second);
    ResourceManager::LoadTexture(
        ResourceManager::GetTextureInfo()->find("waterNormal")->first,
        ResourceManager::GetTextureInfo()->find("waterNormal")->second);
    ResourceManager::LoadTexture(
        ResourceManager::GetTextureInfo()->find("waterDuDv")->first,
        ResourceManager::GetTextureInfo()->find("waterDuDv")->second);
}

void SceneEditor::SetupMaterials()
{
    ResourceManager::LoadMaterial(
        ResourceManager::GetMaterialInfo()->find("none")->first,
        ResourceManager::GetMaterialInfo()->find("none")->second);
    ResourceManager::LoadMaterial(
        ResourceManager::GetMaterialInfo()->find("M1911")->first,
        ResourceManager::GetMaterialInfo()->find("M1911")->second);
}

void SceneEditor::SetupMeshes()
{
    m_Quad = new Quad();
}

void SceneEditor::SetupModels()
{
    Sphere* sphere = new Sphere(glm::vec3(1.0f));
    meshes.insert(std::make_pair("sphere", sphere));

    Cone* cone = new Cone(glm::vec3(1.0f));
    meshes.insert(std::make_pair("cone", cone));
}

void SceneEditor::SetGeometry()
{
}

void SceneEditor::SelectNextFromMultipleObjects(std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectSelect.lastTime < m_ObjectSelect.cooldown) return;
    m_ObjectSelect.lastTime = m_CurrentTimestamp;

    std::vector<unsigned int> sceneObjectsInFocusIndices = std::vector<unsigned int>();

    for (unsigned int i = 0; i < sceneObjects->size(); i++) {
        if (sceneObjects->at(i)->isSelected) {
            selectedIndex = i;
            sceneObjectsInFocusIndices.push_back(i);
        }
    }

    // if there is 0 or 1 elements in focus - finish
    if (sceneObjectsInFocusIndices.size() <= 1) return;

    // handle multiple selections
    m_ObjectInFocusPrev++;
    if (m_ObjectInFocusPrev > sceneObjectsInFocusIndices.size() - 1)
        m_ObjectInFocusPrev = 0;
    selectedIndex = sceneObjectsInFocusIndices[m_ObjectInFocusPrev];
}

bool SceneEditor::IsObjectSelected(unsigned int objectIndex)
{
    return objectIndex = m_SelectedIndex;
}

void SceneEditor::SaveScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneSave.lastTime < m_SceneSave.cooldown) return;
    m_SceneSave.lastTime = m_CurrentTimestamp;

    printf("SaveScene: Saving %zu objects...\n", m_SceneObjects.size());

    std::vector<std::string> lines;
    for (int i = 0; i < m_SceneObjects.size(); i++)
    {
        lines.push_back("BeginObject\t" + std::to_string(i));
        lines.push_back("Position\t" +
            std::to_string(m_SceneObjects[i]->position.x) + "\t" +
            std::to_string(m_SceneObjects[i]->position.y) + "\t" +
            std::to_string(m_SceneObjects[i]->position.z));
        lines.push_back("Rotation\t" +
            std::to_string(glm::eulerAngles(m_SceneObjects[i]->rotation).x / toRadians) + "\t" +
            std::to_string(glm::eulerAngles(m_SceneObjects[i]->rotation).y / toRadians) + "\t" +
            std::to_string(glm::eulerAngles(m_SceneObjects[i]->rotation).z / toRadians));
        lines.push_back("Scale\t" +
            std::to_string(m_SceneObjects[i]->scale.x) + "\t" +
            std::to_string(m_SceneObjects[i]->scale.y) + "\t" +
            std::to_string(m_SceneObjects[i]->scale.z));
        lines.push_back("PositionAABB\t" +
            std::to_string(m_SceneObjects[i]->positionAABB.x) + "\t" +
            std::to_string(m_SceneObjects[i]->positionAABB.y) + "\t" +
            std::to_string(m_SceneObjects[i]->positionAABB.z));
        lines.push_back("ScaleAABB\t" +
            std::to_string(m_SceneObjects[i]->scaleAABB.x) + "\t" +
            std::to_string(m_SceneObjects[i]->scaleAABB.y) + "\t" +
            std::to_string(m_SceneObjects[i]->scaleAABB.z));
        lines.push_back("Color\t" +
            std::to_string(m_SceneObjects[i]->color.r) + "\t" +
            std::to_string(m_SceneObjects[i]->color.g) + "\t" +
            std::to_string(m_SceneObjects[i]->color.b) + "\t" +
            std::to_string(m_SceneObjects[i]->color.a));

        lines.push_back("Name\t" + m_SceneObjects[i]->name);
        lines.push_back("TextureName\t" + m_SceneObjects[i]->textureName);
        lines.push_back("TilingFactor\t" + std::to_string(m_SceneObjects[i]->tilingFactor));
        std::string isSelected = m_SceneObjects[i]->isSelected ? "1" : "0";
        lines.push_back("IsSelected\t" + isSelected);
        lines.push_back("Type\t" + m_SceneObjects[i]->m_Type);
        lines.push_back("TypeID\t" + std::to_string(m_SceneObjects[i]->m_TypeID));
        lines.push_back("MaterialName\t" + m_SceneObjects[i]->materialName);
        lines.push_back("TilingFactorMaterial\t" + std::to_string(m_SceneObjects[i]->tilingFMaterial));
        lines.push_back("EndObject");
    }

    std::ofstream ofs;
    ofs.open(m_SceneFilename);
    for (auto& line : lines)
    {
        ofs << line << std::endl;
        // printf("%s\n", line.c_str());
    }
    ofs.close();

    printf("SaveScene: Content saved to file '%s'\n", m_SceneFilename);
}

void SceneEditor::LoadScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneLoad.lastTime < m_SceneLoad.cooldown) return;
    m_SceneLoad.lastTime = m_CurrentTimestamp;

    printf("LoadScene: Loading objects...\n");

    std::string sceneFileContent = Shader::ReadFile(m_SceneFilename);

    std::vector<std::string> lines;
    std::istringstream iss(sceneFileContent);
    std::string tmpLine;
    while (std::getline(iss, tmpLine, '\n')) {
        lines.push_back(tmpLine);
    }

    unsigned int objectId;
    SceneObject* sceneObject = CreateNewSceneObject();
    m_SceneObjects.clear();

    for (auto& line : lines)
    {
        // printf("%s\n", line.c_str());
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string tmpToken;
        while (std::getline(iss, tmpToken, '\t')) {
            tokens.push_back(tmpToken);
        }
        
        if (tokens.size() >= 2 && tokens[0] == "BeginObject") {
            objectId = (unsigned int)std::stoi(tokens[1]);
            sceneObject = CreateNewSceneObject();
            // printf("ObjectID=%i\n", objectId);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Position") {
            sceneObject->position = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Position %.2ff %.2ff %.2ff\n", sceneObject.position.x, sceneObject.position.y, sceneObject.position.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Rotation") {
            sceneObject->rotation = glm::quat(glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3])) * toRadians);
            // printf("Rotation %.2ff %.2ff %.2ff\n", sceneObject.rotation.x, sceneObject.rotation.y, sceneObject.rotation.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Scale") {
            sceneObject->scale = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Scale %.2ff %.2ff %.2ff\n", sceneObject.scale.x, sceneObject.scale.y, sceneObject.scale.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "PositionAABB") {
            sceneObject->positionAABB = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Position %.2ff %.2ff %.2ff\n", sceneObject.position.x, sceneObject.position.y, sceneObject.position.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "ScaleAABB") {
            sceneObject->scaleAABB = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Scale %.2ff %.2ff %.2ff\n", sceneObject.scale.x, sceneObject.scale.y, sceneObject.scale.z);
        }
        else if (tokens.size() >= 5 && tokens[0] == "Color") {
            sceneObject->color = glm::vec4(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]), std::stof(tokens[4]));
            // printf("Color %.2ff %.2ff %.2ff %.2ff\n", sceneObject.color.r, sceneObject.color.g, sceneObject.color.b, sceneObject.color.a);
        }
        else if (tokens.size() >= 2 && tokens[0] == "Name") {
            sceneObject->name = tokens[1];
            // printf("sceneObject->name %d\n", sceneObject->name
        }
        else if (tokens.size() >= 2 && tokens[0] == "TextureName") {
            sceneObject->textureName = tokens[1];
            // printf("TextureName %s\n", sceneObject.textureName.c_str());
        }
        else if (tokens.size() >= 2 && tokens[0] == "TilingFactor") {
            sceneObject->tilingFactor = std::stof(tokens[1]);
            // printf("TilingFactor %.2f\n", sceneObject.tilingFactor);
        }
        else if (tokens.size() >= 2 && tokens[0] == "IsSelected") {
            sceneObject->isSelected = std::stoi(tokens[1]) == 1 ? true : false;
            if (sceneObject->isSelected) m_SelectedIndex = (unsigned int)m_SceneObjects.size();
            // printf("IsSelected %d\n", sceneObject.isSelected);
        }
        else if (tokens.size() >= 2 && tokens[0] == "Type") {
            sceneObject->m_Type = tokens[1];
            // printf("ObjectType %s\n", sceneObject.objectType.c_str());
        }
        else if (tokens.size() >= 2 && tokens[0] == "TypeID") {
            sceneObject->m_TypeID = std::stoi(tokens[1]);
            // printf("MeshType %d\n", sceneObject.meshType);
        }
        else if (tokens.size() >= 2 && tokens[0] == "MaterialName") {
            sceneObject->materialName = tokens[1];
            // printf("MaterialName %s\n", sceneObject.materialName.c_str());
        }
        else if (tokens.size() >= 2 && tokens[0] == "TilingFactorMaterial") {
            sceneObject->tilingFMaterial = std::stof(tokens[1]);
            // printf("TilingFactorMaterial %s\n", sceneObject.tilingFactorMaterial.c_str());
        }
        else if (tokens.size() >= 1 && tokens[0] == "EndObject") {
            sceneObject->id = (int)m_SceneObjects.size();
            sceneObject->transform = Math::CreateTransform(sceneObject->position, sceneObject->rotation, sceneObject->scale);
            sceneObject->SetAABB(new AABB(sceneObject->positionAABB, sceneObject->rotation, sceneObject->scaleAABB));
            sceneObject->pivot = new Pivot(sceneObject->position, sceneObject->scale);
            Mesh* mesh = nullptr;
            Model* model = nullptr;
            if (sceneObject->m_Type == "mesh") {
                std::string objectName;
                sceneObject->mesh  = CreateNewMesh(sceneObject->m_TypeID, sceneObject->scale, &objectName);
            }
            else if (sceneObject->m_Type == "model")
                sceneObject->model = AddNewModel(sceneObject->m_TypeID, sceneObject->scale);
            m_SceneObjects.push_back(sceneObject);
            // printf("EndObject: New SceneObject added to m_SceneObjects...\n");
        }
    }

    if (m_SceneObjects.size() > 0)
        m_Gizmo->SetSceneObject(m_SceneObjects[m_SceneObjects.size() - 1]);
}

void SceneEditor::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    m_ImGuiMainViewportX = (int)ImGui::GetMainViewport()->GetWorkPos().x;
    m_ImGuiMainViewportY = (int)ImGui::GetMainViewport()->GetWorkPos().y;

    MousePicker* mp = MousePicker::Get();

    ImGui::Begin("Transform");
    {
        if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
        {
            glm::vec3 quatToVec3 = glm::eulerAngles(m_SceneObjects[m_SelectedIndex]->rotation) / toRadians;
            m_PositionEdit = &m_SceneObjects[m_SelectedIndex]->position;
            m_RotationEdit = &quatToVec3;
            m_ScaleEdit = &m_SceneObjects[m_SelectedIndex]->scale;
            m_ColorEdit = &m_SceneObjects[m_SelectedIndex]->color;
            m_TextureNameEdit = &m_SceneObjects[m_SelectedIndex]->textureName;
            m_TilingFactorEdit = &m_SceneObjects[m_SelectedIndex]->tilingFactor;
            m_MaterialNameEdit = &m_SceneObjects[m_SelectedIndex]->materialName;
            m_TilingFactorMaterialEdit = &m_SceneObjects[m_SelectedIndex]->tilingFMaterial;
        }

        ImGui::SliderFloat3("Position", (float*)m_PositionEdit, -10.0f, 10.0f);
        ImGui::SliderFloat3("Rotation", (float*)m_RotationEdit, -360.0f, 360.0f);
        ImGui::SliderFloat3("Scale", (float*)m_ScaleEdit, 0.1f, 20.0f);
    }
    ImGui::End();

    ImGui::Begin("Object Properties");
    {
        if (ImGui::CollapsingHeader("Show Details"))
        {
            if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
            {
                std::string objectName = "Name: " + m_SceneObjects[m_SelectedIndex]->name;
                ImGui::Text(objectName.c_str());
                ImGui::SliderInt("Selected Object", (int*)&m_SelectedIndex, 0, (int)(m_SceneObjects.size() - 1));
                ImGui::Checkbox("Cast Shadow", &m_SceneObjects[m_SelectedIndex]->castShadow);
                ImGui::Checkbox("Receive Shadows", &m_SceneObjects[m_SelectedIndex]->receiveShadows);
            }
        }
    }
    ImGui::End();

    ImGui::Begin("Material");
    {
        ImGui::ColorEdit4("Color", (float*)m_ColorEdit);

        // Begin TextureName ImGui drop-down list
        std::vector<const char*> itemsTexture;
        std::map<std::string, std::string>::iterator itTexture;
        for (itTexture = ResourceManager::GetTextureInfo()->begin(); itTexture != ResourceManager::GetTextureInfo()->end(); itTexture++)
            itemsTexture.push_back(itTexture->first.c_str());

        static const char* currentItemTexture = m_TextureNameEdit->c_str();

        if (ImGui::BeginCombo("Texture Name", currentItemTexture))
        {
            for (int n = 0; n < itemsTexture.size(); n++)
            {
                bool isSelected = (currentItemTexture == itemsTexture[n]);
                if (ImGui::Selectable(itemsTexture[n], isSelected))
                {
                    currentItemTexture = itemsTexture[n];
                    if (m_SelectedIndex < m_SceneObjects.size())
                        m_SceneObjects[m_SelectedIndex]->textureName = itemsTexture[n];
                    else
                        m_SelectedIndex = m_SceneObjects.size() > 0 ? (unsigned int)m_SceneObjects.size() - 1 : 0;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        // End TextureName ImGui drop-down list

        ImGui::SliderFloat("Tiling Factor", m_TilingFactorEdit, 0.0f, 10.0f);

        // Begin MaterialName ImGui drop-down list
        std::vector<const char*> itemsMaterial;
        std::map<std::string, TextureInfo>::iterator itMaterial;
        for (itMaterial = ResourceManager::GetMaterialInfo()->begin(); itMaterial != ResourceManager::GetMaterialInfo()->end(); itMaterial++)
            itemsMaterial.push_back(itMaterial->first.c_str());
        static const char* currentItemMaterial = m_MaterialNameEdit->c_str();
        if (ImGui::BeginCombo("Material Name", currentItemMaterial))
        {
            for (int n = 0; n < itemsMaterial.size(); n++)
            {
                bool isSelectedMaterial = (currentItemMaterial == itemsMaterial[n]);
                if (ImGui::Selectable(itemsMaterial[n], isSelectedMaterial))
                {
                    currentItemMaterial = itemsMaterial[n];
                    if (m_SelectedIndex < m_SceneObjects.size())
                        m_SceneObjects[m_SelectedIndex]->materialName = itemsMaterial[n];
                    else
                        m_SelectedIndex = m_SceneObjects.size() > 0 ? (unsigned int)m_SceneObjects.size() - 1 : 0;
                }
                if (isSelectedMaterial)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        // End MaterialName ImGui drop-down list

        ImGui::SliderFloat("Material Tiling Factor", m_TilingFactorMaterialEdit, 0.0f, 10.0f);
    }
    ImGui::End();

    ImGui::Begin("Scene Settings");
    {
        float FOV = GetFOV();
        ImGui::SliderFloat("FOV", &FOV, 1.0f, 120.0f);
        SetFOV(FOV);

        glm::vec4 waterColor = m_WaterManager->GetWaterColor();
        ImGui::ColorEdit4("Water Color", (float*)&waterColor);
        m_WaterManager->SetWaterColor(waterColor);

        if (ImGui::CollapsingHeader("Select HDRI"))
        {
            ImGui::RadioButton("Greenwich Park", &m_HDRI_Edit, HDRI_GREENWICH_PARK);
            ImGui::RadioButton("San Giuseppe Bridge", &m_HDRI_Edit, HDRI_SAN_GIUSEPPE_BRIDGE);
            ImGui::RadioButton("Tropical Beach", &m_HDRI_Edit, HDRI_TROPICAL_BEACH);
            ImGui::RadioButton("Vignaioli Night", &m_HDRI_Edit, HDRI_VIGNAIOLI_NIGHT);
            ImGui::RadioButton("Early Eve & Warm Sky", &m_HDRI_Edit, HDRI_EARLY_EVE_WARM_SKY);
            ImGui::RadioButton("Birchwood", &m_HDRI_Edit, HDRI_BIRCHWOOD);
            ImGui::RadioButton("Pink Sunrise", &m_HDRI_Edit, HDRI_PINK_SUNRISE);
            ImGui::RadioButton("Rooitou Park", &m_HDRI_Edit, HDRI_ROOITOU_PARK);
            ImGui::RadioButton("Venice Dawn", &m_HDRI_Edit, HDRI_VENICE_DAWN);
            ImGui::RadioButton("Peppermint Powerplant", &m_HDRI_Edit, HDRI_PEPPERMINT_POWERPLANT);

            ImGui::SliderInt("Blur Level", &m_BlurLevel, 0, 10);
            ImGui::SliderFloat("Skybox LOD", &m_SkyboxLOD, 0.0f, 6.0f);
            ImGui::SliderFloat("Skybox Rotation Speed", &m_SkyboxRotationSpeed, -20.0f, 20.0f);            
        }

        if (ImGui::CollapsingHeader("Cube Maps"))
        {
            ImGui::Checkbox("Use Cube Maps", &m_UseCubeMaps);
            ImGui::RadioButton("Environment Map", &m_PBR_Map_Edit, PBR_MAP_ENVIRONMENT);
            ImGui::RadioButton("Irradiance Map", &m_PBR_Map_Edit, PBR_MAP_IRRADIANCE);
            ImGui::RadioButton("Prefilter Map", &m_PBR_Map_Edit, PBR_MAP_PREFILTER);
        }

        if (ImGui::CollapsingHeader("Gizmos"))
        {
            ImGui::Checkbox("Draw Gizmos", &m_DrawGizmos);
            ImGui::Checkbox("Orthographic View", &m_OrthographicViewEnabled);

            bool gizmoActive = m_Gizmo->GetActive();
            int sceneObjectCount = (int)m_SceneObjects.size();
            Bool3 axesEnabled = m_Gizmo->GetAxesEnabled();

            ImGui::Separator();
            ImGui::Text("Transform Gizmo");
            ImGui::SliderInt("Scene Objects Count", &sceneObjectCount, 0, 100);
            ImGui::Checkbox("Gizmo Active", &gizmoActive);
            ImGui::Separator();
            ImGui::Text("Axes Enabled");
            ImGui::Indent();
            ImGui::Checkbox("Axis X", &axesEnabled.x);
            ImGui::Checkbox("Axis Y", &axesEnabled.y);
            ImGui::Checkbox("Axis Z", &axesEnabled.z);
            ImGui::Unindent();
        }
    }
    ImGui::End();

    ImGui::Begin("Particles");
    {
        if (ImGui::CollapsingHeader("Particle System Settings"))
        {
            glm::vec3 positionSOPS = glm::vec3(0.0f);
            glm::vec3 scaleSOPS = glm::vec3(1.0f);
            if (m_CurrentSOPS != nullptr) {
                positionSOPS = m_CurrentSOPS->position;
                scaleSOPS = m_CurrentSOPS->scale;
            }
            ImGui::SliderFloat3("Origin Area Position", glm::value_ptr(positionSOPS), -20.0f, 20.0f);
            ImGui::SliderFloat3("Origin Area Scale", glm::value_ptr(scaleSOPS), 0.0f, 20.0f);
            if (m_CurrentSOPS != nullptr) {
                m_CurrentSOPS->position = positionSOPS;
                m_CurrentSOPS->scale = scaleSOPS;
            }

            // Begin ParticleTextureName ImGui drop-down list
            std::vector<const char*> itemsTexture;
            std::map<std::string, std::string>::iterator itTexture;
            for (itTexture = ResourceManager::GetTextureInfo()->begin(); itTexture != ResourceManager::GetTextureInfo()->end(); itTexture++)
                itemsTexture.push_back(itTexture->first.c_str());
            static const char* currentItemTexture = m_ParticleSettingsEdit->textureName.c_str();

            if (ImGui::BeginCombo("Texture Name", currentItemTexture))
            {
                for (int n = 0; n < itemsTexture.size(); n++)
                {
                    bool isSelected = (currentItemTexture == itemsTexture[n]);
                    if (ImGui::Selectable(itemsTexture[n], isSelected))
                    {
                        currentItemTexture = itemsTexture[n];
                        m_ParticleSettingsEdit->textureName = std::string(itemsTexture[n]);
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            // End ParticleTextureName ImGui drop-down list

            ImGui::SliderInt(  "Number of Rows",       &m_ParticleSettingsEdit->numRows,                   1, 10);
            ImGui::SliderInt("Particles Per Second",   &m_ParticleSettingsEdit->PPS,                       0, m_MaxInstances);
            ImGui::SliderFloat3("Direction",           glm::value_ptr(m_ParticleSettingsEdit->direction), -1.0f, 1.0f);
            ImGui::SliderFloat("Intensity",            &m_ParticleSettingsEdit->intensity,                 0.0f, 20.0f);
            ImGui::SliderFloat("Gravity Complient",    &m_ParticleSettingsEdit->gravityComplient,         -40.0f, 40.0f);
            ImGui::SliderFloat("Life Length",          &m_ParticleSettingsEdit->lifeLength,                0.0f, 20.0f);
            ImGui::SliderFloat("Diameter",             &m_ParticleSettingsEdit->diameter,                  0.0f, 1.0f);

            ImGui::Separator();
            ImGui::Checkbox("Instanced Rendering", &m_ParticleSettingsEdit->instanced);
            std::map<int, int> counts = std::map<int, int>();
            if (m_CurrentSOPS != nullptr) {
                counts = m_CurrentSOPS->GetMaster()->GetCounts();
            }
            for (auto it = counts.begin(); it != counts.end(); it++) {
                ImGui::Separator();
                std::string textLine = "TextureID: " + std::to_string(it->first) + " Particles: " + std::to_string(it->second);
                ImGui::Text(textLine.c_str());
            }
        }
    }
    ImGui::End();

    ImGui::Begin("Help");
    {
        if (ImGui::CollapsingHeader("Show Details"))
        {
            ImGui::Text("* Add Mesh or Model: Left CTRL + Left Mouse Button");
            ImGui::Text("* Camera Rotation: Right Mouse Button");
            ImGui::Text("* Camera Movement: W) forward, A) left, S) back, D) right, Q) down, E) up");
            ImGui::Text("* Fast Movement: Left SHIFT + W|A|S|D|Q|E");
            ImGui::Text("* Enable Gizmo: TAB + Left Mouse Button");
            ImGui::Text("* Toggle Gizmo Modes: 1) Translate, 2) Scale, 3) Rotate, 4) Disable");
            ImGui::Text("* Scene Save: Left CTRL + S");
            ImGui::Text("* Scene Reset: Left CTRL + R");
            ImGui::Text("* Scene Load: Left CTRL + L");
            ImGui::Text("* Toggle Wireframe Mode: R");
            ImGui::Text("* Copy scene object: Left CTRL + C");
        }
    }
    ImGui::End();

    ImGui::Begin("Renderer");
    {
        ImGui::Checkbox("Enable Shadows", &sceneSettings.enableShadows);
        ImGui::Checkbox("Enable Omni Shadows", &sceneSettings.enableOmniShadows);
        ImGui::Checkbox("Enable Water Effects", &sceneSettings.enableWaterEffects);
        ImGui::Checkbox("Enable Particles", &sceneSettings.enableParticles);
    }
    ImGui::End();

    ImGui::Begin("Profiler");
    {
        float realFPS = Timer::Get()->GetRealFPS();
        std::string sRealFPS = "Real FPS: " + std::to_string(realFPS);

        float deltaTimeMS = Timer::Get()->GetDeltaTime() * 1000.0f;
        std::string sDeltaTimeMS = "Delta Time: " + std::to_string(deltaTimeMS) + " ms";

        if (ImGui::CollapsingHeader("Timer"))
        {
            ImGui::Text(sRealFPS.c_str());
            ImGui::Text(sDeltaTimeMS.c_str());
        }

        if (ImGui::CollapsingHeader("Active Render Passes"))
        {
            ImGui::Indent();
            for (auto& renderPassName : m_ActiveRenderPasses)
                ImGui::Text(renderPassName.c_str());
            ImGui::Unindent();
        }

        m_ActiveRenderPasses.clear();

        // print profiler results
        if (ImGui::CollapsingHeader("Profiler results:"))
        {
            // profiler results
            for (auto& profilerResult : m_ProfilerResults)
            {
                char label[100];
                strcpy(label, "%.2fms ");
                strcat(label, profilerResult.first.c_str());
                ImGui::Text(label, profilerResult.second);
            }
            m_ProfilerResults.clear();
        }
    }
    ImGui::End();

    ImGui::Begin("Mouse Picker");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            char buffer[100];

            sprintf(buffer, "Main Window [ X %i Y %i ]", m_ImGuiMainViewportX, m_ImGuiMainViewportY);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Viewport [ X %i Y %i W %i H %i ]", mp->m_Viewport.X, mp->m_Viewport.Y, mp->m_Viewport.Width, mp->m_Viewport.Height);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Screen Mouse [ %i %i ]", mp->m_ScreenMouseX, mp->m_ScreenMouseY);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Viewport Mouse [ %i %i ]", mp->m_Viewport.MouseX, mp->m_Viewport.MouseY);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Normalized Coords [ %.2ff %.2ff ]", mp->m_NormalizedCoords.x, mp->m_NormalizedCoords.y);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Clip Coords [ %.2ff %.2ff ]", mp->m_ClipCoords.x, mp->m_ClipCoords.y);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Eye Coords [ %.2ff %.2ff %.2ff %.2ff ]", mp->m_EyeCoords.x, mp->m_EyeCoords.y, mp->m_EyeCoords.z, mp->m_EyeCoords.w);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "World Ray [ %.2ff %.2ff %.2ff ]", mp->m_WorldRay.x, mp->m_WorldRay.y, mp->m_WorldRay.z);
            ImGui::Text(buffer);
        }
    }
    ImGui::End();

    ImGui::Begin("Scene Editor");
    {
        if (ImGui::CollapsingHeader("Add Mesh"))
        {
            ImGui::RadioButton("Cube",         &m_CurrentObjectTypeID, MESH_TYPE_CUBE);
            ImGui::RadioButton("Pyramid",      &m_CurrentObjectTypeID, MESH_TYPE_PYRAMID);
            ImGui::RadioButton("Sphere",       &m_CurrentObjectTypeID, MESH_TYPE_SPHERE);
            ImGui::RadioButton("Cylinder",     &m_CurrentObjectTypeID, MESH_TYPE_CYLINDER);
            ImGui::RadioButton("Cone",         &m_CurrentObjectTypeID, MESH_TYPE_CONE);
            ImGui::RadioButton("Ring",         &m_CurrentObjectTypeID, MESH_TYPE_RING);
            ImGui::RadioButton("Bob Lamp",     &m_CurrentObjectTypeID, MESH_TYPE_BOB_LAMP);
            ImGui::RadioButton("Anim Boy",     &m_CurrentObjectTypeID, MESH_TYPE_ANIM_BOY);
            ImGui::RadioButton("Terrain",      &m_CurrentObjectTypeID, MESH_TYPE_TERRAIN);
            ImGui::RadioButton("Water",        &m_CurrentObjectTypeID, MESH_TYPE_WATER);
            ImGui::RadioButton("Buster Drone", &m_CurrentObjectTypeID, MESH_TYPE_DRONE);
            ImGui::RadioButton("M1911 Pistol", &m_CurrentObjectTypeID, MESH_TYPE_M1911);
        }

        if (ImGui::CollapsingHeader("Add Model"))
        {
            ImGui::RadioButton("Stone Carved",   &m_CurrentObjectTypeID, MODEL_STONE_CARVED);
            ImGui::RadioButton("Old Stove",      &m_CurrentObjectTypeID, MODEL_OLD_STOVE);
            ImGui::RadioButton("Buddha",         &m_CurrentObjectTypeID, MODEL_BUDDHA);
            ImGui::RadioButton("HHeli",          &m_CurrentObjectTypeID, MODEL_HHELI);
            ImGui::RadioButton("Jeep",           &m_CurrentObjectTypeID, MODEL_JEEP);
            ImGui::RadioButton("Damaged Helmet", &m_CurrentObjectTypeID, MODEL_DAMAGED_HELMET);
            ImGui::RadioButton("SF Helmet",      &m_CurrentObjectTypeID, MODEL_SF_HELMET);
            ImGui::RadioButton("Cerberus",       &m_CurrentObjectTypeID, MODEL_CERBERUS);
            ImGui::RadioButton("Pine",           &m_CurrentObjectTypeID, MODEL_PINE);
            ImGui::RadioButton("Boulder",        &m_CurrentObjectTypeID, MODEL_BOULDER);
        }
        if (ImGui::CollapsingHeader("Add Particle System"))
        {
            ImGui::RadioButton("Particle System", &m_CurrentObjectTypeID, PARTICLE_SYSTEM);
        }
    }
    ImGui::End();

    ImGui::Begin("Textures");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            ImVec2 imageSize(64.0f, 64.0f);

            for (std::map<std::string, Texture*>::iterator it = textures.begin(); it != textures.end(); ++it)
            {
                ImGui::Text(it->first.c_str());
                ImGui::Image((void*)(intptr_t)it->second->GetID(), imageSize);
            }
        }
    }
    ImGui::End();

    ImGui::Begin("Framebuffers");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            ImVec2 imageSize(128.0f, 128.0f);

            ImGui::Text("Shadow Map");
            ImGui::Image((void*)(intptr_t)LightManager::directionalLight.GetShadowMap()->GetTextureID(), imageSize);

            if (m_IsViewportEnabled)
            {
                ImGui::Text("Viewport");
                ImGui::Image((void*)(intptr_t)m_RenderFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);
            }

            if (ImGui::CollapsingHeader("Omni Shadow Maps"))
            {
                ImGui::Text("Omni Shadow Map 0\n(Point Light 0)");
                ImGui::Image((void*)(intptr_t)LightManager::pointLights[0].GetShadowMap()->GetTextureID(), imageSize);
                ImGui::Text("Omni Shadow Map 1\n(Point Light 1)");
                ImGui::Image((void*)(intptr_t)LightManager::pointLights[1].GetShadowMap()->GetTextureID(), imageSize);
                ImGui::Text("Omni Shadow Map 2\n(Point Light 2)");
                ImGui::Image((void*)(intptr_t)LightManager::pointLights[2].GetShadowMap()->GetTextureID(), imageSize);
                ImGui::Text("Omni Shadow Map 3\n(Point Light 3)");
                ImGui::Image((void*)(intptr_t)LightManager::pointLights[3].GetShadowMap()->GetTextureID(), imageSize);

                ImGui::Text("Omni Shadow Map 4\n(Spot Light 0)");
                ImGui::Image((void*)(intptr_t)LightManager::spotLights[0].GetShadowMap()->GetTextureID(), imageSize);
                ImGui::Text("Omni Shadow Map 5\n(Spot Light 1)");
                ImGui::Image((void*)(intptr_t)LightManager::spotLights[1].GetShadowMap()->GetTextureID(), imageSize);
                ImGui::Text("Omni Shadow Map 6\n(Spot Light 2)");
                ImGui::Image((void*)(intptr_t)LightManager::spotLights[2].GetShadowMap()->GetTextureID(), imageSize);
                ImGui::Text("Omni Shadow Map 7\n(Spot Light 3)");
                ImGui::Image((void*)(intptr_t)LightManager::spotLights[3].GetShadowMap()->GetTextureID(), imageSize);
            }

            ImGui::Text("Water Reflection\nColor Attachment");
            ImGui::Image((void*)(intptr_t)m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->GetID(), imageSize);
            ImGui::Text("Water Refraction\nColor Attachment");
            ImGui::Image((void*)(intptr_t)m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->GetID(), imageSize);
            ImGui::Text("Water Refraction\nDepth Attachment");
            ImGui::Image((void*)(intptr_t)m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->GetID(), imageSize);

            ImGui::Text("Blur Effect Horizontal");
            ImGui::Image((void*)(intptr_t)m_BlurEffect->GetHorizontalOutputTexture()->GetID(), imageSize);
            ImGui::Text("Blur Effect Vertical");
            ImGui::Image((void*)(intptr_t)m_BlurEffect->GetVerticalOutputTexture()->GetID(), imageSize);
        }
    }
    ImGui::End();

    if (m_IsViewportEnabled)
    {
        ImGui::Begin("Viewport Info");
        {
            glm::ivec2 colorAttachmentSize = glm::ivec2(
                m_RenderFramebuffer->GetTextureAttachmentColor()->GetWidth(),
                m_RenderFramebuffer->GetTextureAttachmentColor()->GetHeight());
            glm::ivec2 depthAttachmentSize = glm::ivec2(
                m_RenderFramebuffer->GetAttachmentDepth()->GetWidth(),
                m_RenderFramebuffer->GetAttachmentDepth()->GetHeight());

            ImGui::SliderInt2("Color Attachment Size", glm::value_ptr(colorAttachmentSize), 0, 2048);
            ImGui::SliderInt2("Depth Attachment Size", glm::value_ptr(depthAttachmentSize), 0, 2048);
        }
        ImGui::End();
    }

    ImGui::Begin("Lights");
    {
        ImGui::Checkbox("Display Light Sources", &m_DisplayLightSources);

        if (ImGui::CollapsingHeader("Directional Light"))
        {
            // Directional Light
            SDirectionalLight directionalLight;
            directionalLight.base.enabled = LightManager::directionalLight.GetEnabled();
            directionalLight.base.color = LightManager::directionalLight.GetColor();
            directionalLight.base.ambientIntensity = LightManager::directionalLight.GetAmbientIntensity();
            directionalLight.base.diffuseIntensity = LightManager::directionalLight.GetDiffuseIntensity();
            directionalLight.direction = LightManager::directionalLight.GetDirection();

            ImGui::Checkbox("DL Enabled", &directionalLight.base.enabled);
            ImGui::ColorEdit3("DL Color", glm::value_ptr(directionalLight.base.color));
            ImGui::SliderFloat3("DL Direction", glm::value_ptr(directionalLight.direction), -100.0f, 100.0f);
            ImGui::SliderFloat("DL Ambient Intensity", &directionalLight.base.ambientIntensity, 0.0f, 4.0f);
            ImGui::SliderFloat("DL Diffuse Intensity", &directionalLight.base.diffuseIntensity, 0.0f, 4.0f);

            // Shutdown directional light (it appears it's better to do it here than in shader
            if (directionalLight.base.enabled != m_DirLightEnabledPrev)
            {
                if (directionalLight.base.enabled)
                    directionalLight.base.color = m_DirLightColorPrev;
                else
                    directionalLight.base.color = glm::vec3(0.0f, 0.0f, 0.0f);

                m_DirLightEnabledPrev = directionalLight.base.enabled;
            }

            LightManager::directionalLight.SetEnabled(directionalLight.base.enabled);
            LightManager::directionalLight.SetColor(directionalLight.base.color);
            LightManager::directionalLight.SetAmbientIntensity(directionalLight.base.ambientIntensity);
            LightManager::directionalLight.SetDiffuseIntensity(directionalLight.base.diffuseIntensity);
            LightManager::directionalLight.SetDirection(directionalLight.direction);
        }

        if (ImGui::CollapsingHeader("Point Lights"))
        {
            ImGui::Indent();

            // Point Lights
            SPointLight pointLights[4];
            char locBuff[100] = { '\0' };
            for (unsigned int pl = 0; pl < LightManager::pointLightCount; pl++)
            {
                pointLights[pl].base.enabled = LightManager::pointLights[pl].GetEnabled();
                pointLights[pl].base.color = LightManager::pointLights[pl].GetColor();
                pointLights[pl].base.ambientIntensity = LightManager::pointLights[pl].GetAmbientIntensity();
                pointLights[pl].base.diffuseIntensity = LightManager::pointLights[pl].GetDiffuseIntensity();
                pointLights[pl].position = LightManager::pointLights[pl].GetPosition();
                pointLights[pl].constant = LightManager::pointLights[pl].GetConstant();
                pointLights[pl].linear = LightManager::pointLights[pl].GetLinear();
                pointLights[pl].exponent = LightManager::pointLights[pl].GetExponent();

                snprintf(locBuff, sizeof(locBuff), "Point Light %i", pl);
                if (ImGui::CollapsingHeader(locBuff))
                {
                    snprintf(locBuff, sizeof(locBuff), "PL %i Enabled", pl);
                    ImGui::Checkbox(locBuff, &pointLights[pl].base.enabled);
                    snprintf(locBuff, sizeof(locBuff), "PL %i Color", pl);
                    ImGui::ColorEdit3(locBuff, glm::value_ptr(pointLights[pl].base.color));
                    snprintf(locBuff, sizeof(locBuff), "PL %i Position", pl);
                    ImGui::SliderFloat3(locBuff, glm::value_ptr(pointLights[pl].position), -20.0f, 20.0f);
                    snprintf(locBuff, sizeof(locBuff), "PL %i Ambient Intensity", pl);
                    ImGui::SliderFloat(locBuff, &pointLights[pl].base.ambientIntensity, -2.0f, 2.0f);
                    snprintf(locBuff, sizeof(locBuff), "PL %i Diffuse Intensity", pl);
                    ImGui::SliderFloat(locBuff, &pointLights[pl].base.diffuseIntensity, -2.0f, 2.0f);
                    snprintf(locBuff, sizeof(locBuff), "PL %i Constant", pl);
                    ImGui::SliderFloat(locBuff, &pointLights[pl].constant, -2.0f, 2.0f);
                    snprintf(locBuff, sizeof(locBuff), "PL %i Linear", pl);
                    ImGui::SliderFloat(locBuff, &pointLights[pl].linear, -2.0f, 2.0f);
                    snprintf(locBuff, sizeof(locBuff), "PL %i Exponent", pl);
                    ImGui::SliderFloat(locBuff, &pointLights[pl].exponent, -2.0f, 2.0f);
                }

                LightManager::pointLights[pl].SetEnabled(pointLights[pl].base.enabled);
                LightManager::pointLights[pl].SetColor(pointLights[pl].base.color);
                LightManager::pointLights[pl].SetAmbientIntensity(pointLights[pl].base.ambientIntensity);
                LightManager::pointLights[pl].SetDiffuseIntensity(pointLights[pl].base.diffuseIntensity);
                LightManager::pointLights[pl].SetPosition(pointLights[pl].position);
                LightManager::pointLights[pl].SetConstant(pointLights[pl].constant);
                LightManager::pointLights[pl].SetLinear(pointLights[pl].linear);
                LightManager::pointLights[pl].SetExponent(pointLights[pl].exponent);
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Spot Lights"))
        {
            ImGui::Indent();

            // Spot Lights
            SSpotLight spotLights[4];
            char locBuff[100] = { '\0' };
            for (unsigned int sl = 0; sl < LightManager::spotLightCount; sl++)
            {
                spotLights[sl].base.base.enabled = LightManager::spotLights[sl].GetBasePL()->GetEnabled();
                spotLights[sl].base.base.color = LightManager::spotLights[sl].GetBasePL()->GetColor();
                spotLights[sl].base.base.ambientIntensity = LightManager::spotLights[sl].GetBasePL()->GetAmbientIntensity();
                spotLights[sl].base.base.diffuseIntensity = LightManager::spotLights[sl].GetBasePL()->GetDiffuseIntensity();
                spotLights[sl].base.position = LightManager::spotLights[sl].GetBasePL()->GetPosition();
                spotLights[sl].base.constant = LightManager::spotLights[sl].GetBasePL()->GetConstant();
                spotLights[sl].base.linear = LightManager::spotLights[sl].GetBasePL()->GetLinear();
                spotLights[sl].base.exponent = LightManager::spotLights[sl].GetBasePL()->GetExponent();
                spotLights[sl].direction = LightManager::spotLights[sl].GetDirection();
                spotLights[sl].edge = LightManager::spotLights[sl].GetEdge();

                snprintf(locBuff, sizeof(locBuff), "Spot Light %i", sl);
                if (ImGui::CollapsingHeader(locBuff))
                {
                    snprintf(locBuff, sizeof(locBuff), "SL %i Enabled", sl);
                    ImGui::Checkbox(locBuff, &spotLights[sl].base.base.enabled);
                    snprintf(locBuff, sizeof(locBuff), "SL %i Color", sl);
                    ImGui::ColorEdit3(locBuff, glm::value_ptr(spotLights[sl].base.base.color));
                    snprintf(locBuff, sizeof(locBuff), "SL %i Position", sl);
                    ImGui::SliderFloat3(locBuff, glm::value_ptr(spotLights[sl].base.position), -20.0f, 20.0f);
                    snprintf(locBuff, sizeof(locBuff), "SL %i Direction", sl);
                    ImGui::SliderFloat3(locBuff, glm::value_ptr(spotLights[sl].direction), -1.0f, 1.0f);
                    snprintf(locBuff, sizeof(locBuff), "SL %i Edge", sl);
                    ImGui::SliderFloat(locBuff, &spotLights[sl].edge, -100.0f, 100.0f);
                    snprintf(locBuff, sizeof(locBuff), "SL %i Ambient Intensity", sl);
                    ImGui::SliderFloat(locBuff, &spotLights[sl].base.base.ambientIntensity, -2.0f, 2.0f);
                    snprintf(locBuff, sizeof(locBuff), "SL %i Diffuse Intensity", sl);
                    ImGui::SliderFloat(locBuff, &spotLights[sl].base.base.diffuseIntensity, -2.0f, 2.0f);
                    snprintf(locBuff, sizeof(locBuff), "SL %i Constant", sl);
                    ImGui::SliderFloat(locBuff, &spotLights[sl].base.constant, -2.0f, 2.0f);
                    snprintf(locBuff, sizeof(locBuff), "SL %i Linear", sl);
                    ImGui::SliderFloat(locBuff, &spotLights[sl].base.linear, -2.0f, 2.0f);
                    snprintf(locBuff, sizeof(locBuff), "SL %i Exponent", sl);
                    ImGui::SliderFloat(locBuff, &spotLights[sl].base.exponent, -2.0f, 2.0f);
                }

                LightManager::spotLights[sl].GetBasePL()->SetEnabled(spotLights[sl].base.base.enabled);
                LightManager::spotLights[sl].GetBasePL()->SetColor(spotLights[sl].base.base.color);
                LightManager::spotLights[sl].GetBasePL()->SetAmbientIntensity(spotLights[sl].base.base.ambientIntensity);
                LightManager::spotLights[sl].GetBasePL()->SetDiffuseIntensity(spotLights[sl].base.base.diffuseIntensity);
                LightManager::spotLights[sl].GetBasePL()->SetPosition(spotLights[sl].base.position);
                LightManager::spotLights[sl].GetBasePL()->SetConstant(spotLights[sl].base.constant);
                LightManager::spotLights[sl].GetBasePL()->SetLinear(spotLights[sl].base.linear);
                LightManager::spotLights[sl].GetBasePL()->SetExponent(spotLights[sl].base.exponent);
                LightManager::spotLights[sl].SetDirection(spotLights[sl].direction);
                LightManager::spotLights[sl].SetEdge(spotLights[sl].edge);
            }
            ImGui::Unindent();
        }
    }
    ImGui::End();

    ImGui::Begin("Experimental Section");
    {
        if (ImGui::CollapsingHeader("Details"))
        {
            ImGui::Text("File");
            std::string fullpath = (m_LoadedFile != "") ? m_LoadedFile : "None";
            size_t found = fullpath.find_last_of("/\\");
            std::string path = found != std::string::npos ? fullpath.substr(found + 1) : fullpath;
            ImGui::Text(path.c_str()); ImGui::SameLine();

            if (ImGui::Button("...##Mesh"))
            {
                m_LoadedFile = Application::Get()->OpenFile("");
                if (m_LoadedFile != "")
                {
                    Log::GetLogger()->info("ImGui OpenFile - filename: '{0}'", m_LoadedFile);
                }
            }

            if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                ImGui::Image(m_LoadedTexture ? (void*)(intptr_t)m_LoadedTexture->GetID() : (void*)(intptr_t)ResourceManager::HotLoadTexture("Checkerboard")->GetID(), ImVec2(64, 64));
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered())
                {
                    if (m_LoadedTexture)
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(m_LoadedTextureFilepath.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::Image((void*)(intptr_t)m_LoadedTexture->GetID(), ImVec2(384, 384));
                        ImGui::EndTooltip();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        std::string filename = Application::Get()->OpenFile("");
                        if (filename != "")
                            m_LoadedTexture = new Texture(filename.c_str(), false);
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Use##NormalMap", &m_UseLoadedTexture);
            }
        }
    }
    ImGui::End();

    ImGui::ShowMetricsWindow();

    if (m_IsViewportEnabled)
    {
        // TheCherno ImGui Viewport displaying the framebuffer content
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Viewport");
        {
            m_ImGuiViewport.X = (int)(ImGui::GetWindowPos().x - m_ImGuiMainViewportX);
            m_ImGuiViewport.Y = (int)(ImGui::GetWindowPos().y - m_ImGuiMainViewportY);
            m_ImGuiViewport.Width = (int)ImGui::GetWindowWidth();
            m_ImGuiViewport.Height = (int)ImGui::GetWindowHeight();
            m_ImGuiViewport.MouseX = (int)ImGui::GetMousePos().x;
            m_ImGuiViewport.MouseY = (int)ImGui::GetMousePos().y;

            m_ViewportFocused = ImGui::IsWindowFocused();
            m_ViewportHovered = ImGui::IsWindowHovered();

            ImVec2 viewportPanelSizeImGui = ImGui::GetContentRegionAvail();
            glm::vec2 viewportPanelSize = glm::vec2(viewportPanelSizeImGui.x, viewportPanelSizeImGui.y);

            ResizeViewport(viewportPanelSize);

            uint64_t textureID = m_RenderFramebuffer->GetTextureAttachmentColor()->GetID();
            // uint64_t textureID = m_BlurEffect->GetVerticalOutputTexture()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

void SceneEditor::ResizeViewport(glm::vec2 viewportPanelSize)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ResizeViewport.lastTime < m_ResizeViewport.cooldown) return;
    m_ResizeViewport.lastTime = m_CurrentTimestamp;

    if (viewportPanelSize != m_ViewportSize && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        m_RenderFramebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
        m_ViewportSize = glm::vec2(viewportPanelSize.x, viewportPanelSize.y);

        m_CameraController->OnResize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
    }
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void SceneEditor::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
    if (!m_IsViewportEnabled) {
        Scene::ShowExampleAppDockSpace(p_open, mainWindow);
        return;
    }

    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // ImGuiDockNodeFlags_None |
    // ImGuiDockNodeFlags_PassthruCentralNode |
    // ImGuiDockNodeFlags_NoDockingInCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", p_open, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    else
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
        ImGui::Text("Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::SmallButton("click here"))
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit")) mainWindow->SetShouldClose(true);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::MenuItem("Cut");
            ImGui::MenuItem("Copy");
            ImGui::MenuItem("Paste");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Docking"))
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows, 
            // which we can't undo at the moment without finer window depth/z control.
            //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

            if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
            if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
            if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
            ImGui::EndMenu();
        }

        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n\n"
                " > if io.ConfigDockingWithShift==false (default):" "\n"
                "   drag windows from title bar to dock" "\n"
                " > if io.ConfigDockingWithShift==true:" "\n"
                "   drag windows from anywhere and hold Shift to dock" "\n\n"
                "This demo app has nothing to do with it!" "\n\n"
                "This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)." "\n\n"
                "ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
                "(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void SceneEditor::Update(float timestep, Window* mainWindow)
{
    m_Camera->OnUpdate(timestep);

    m_CurrentTimestamp = timestep;

    if (m_SelectedIndex >= m_SceneObjects.size())
        m_SelectedIndex = (unsigned int)m_SceneObjects.size() - 1;

    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = (int)mainWindow->GetWidth();
    int viewportHeight = (int)mainWindow->GetHeight();
    if (m_IsViewportEnabled) {
        viewportX = (int)m_ImGuiViewport.X;
        viewportY = (int)m_ImGuiViewport.Y;
        viewportWidth = m_ImGuiViewport.Width;
        viewportHeight = m_ImGuiViewport.Height;
    }

    MousePicker::Get()->Update(
        (int)mainWindow->GetMouseX(), (int)mainWindow->GetMouseY(),
        viewportX, viewportY, viewportWidth, viewportHeight,
        RendererBasic::GetProjectionMatrix(), m_Camera->GetViewMatrix());

    MousePicker::Get()->GetPointOnRay(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

    for (auto& object : m_SceneObjects) {
        object->isSelected = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(),
            object->GetAABB()->GetMin(), object->GetAABB()->GetMax(), glm::vec2(0.0f));

        if (object->m_Type == "mesh" && object->mesh != nullptr)
            object->mesh->Update(object->scale);
        else if (object->m_Type == "model" && object->model != nullptr)
            object->model->Update(object->scale);

        if (object->name == "water")
            m_WaterManager->SetWaterHeight(object->position.y);

        if (object->name == "particle_system") {
            ((SceneObjectParticleSystem*)object)->Update(sceneSettings.enableParticles, GetProfilerResults());
        }
    }

    if (m_HDRI_Edit != m_HDRI_Edit_Prev || m_BlurLevel != m_BlurLevelPrev)
    {
        if (m_HDRI_Edit == HDRI_GREENWICH_PARK) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/greenwich_park_02_1k.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_SAN_GIUSEPPE_BRIDGE) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/san_giuseppe_bridge_1k.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_TROPICAL_BEACH) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/Tropical_Beach_3k.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_VIGNAIOLI_NIGHT) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/vignaioli_night_1k.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_EARLY_EVE_WARM_SKY) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/006_hdrmaps_com_free.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_BIRCHWOOD) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/birchwood_4k.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_PINK_SUNRISE) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/pink_sunrise_4k.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_ROOITOU_PARK) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/rooitou_park_4k.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_VENICE_DAWN) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/venice_dawn_1_4k.hdr", m_BlurLevel);
        }
        else if (m_HDRI_Edit == HDRI_PEPPERMINT_POWERPLANT) {
            m_MaterialWorkflowPBR->Init("Textures/HDR/peppermint_powerplant_1k.hdr", m_BlurLevel);
        }

        m_BlurEffect->Init(m_MaterialWorkflowPBR->m_CaptureSize, m_MaterialWorkflowPBR->m_CaptureSize, ResourceManager::HotLoadTexture("crate")->GetID());

        m_HDRI_Edit_Prev = m_HDRI_Edit;
        m_BlurLevelPrev = m_BlurLevel;
    }

    m_Gizmo->Update(m_Camera->GetPosition(), mainWindow);
    m_Gizmo->SetDrawAABBs(m_DrawGizmos);

    // Switching between scene objects that are currently in focus (mouse over)
    if (mainWindow->getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        m_Gizmo->OnMousePress(mainWindow, &m_SceneObjects, m_SelectedIndex);
        // UpdateLightDirection(m_Gizmo->GetRotation());
        m_MouseButton_1_Prev = true;
    }

    if (!mainWindow->getMouseButtons()[GLFW_MOUSE_BUTTON_1] && m_MouseButton_1_Prev)
    {
        SelectNextFromMultipleObjects(&m_SceneObjects, m_SelectedIndex);
        m_Gizmo->OnMouseRelease(mainWindow, &m_SceneObjects, m_SelectedIndex);
        m_MouseButton_1_Prev = false;

        // Connect "Particle System" ImGui to currently selected particle system
        if (m_SceneObjects.at(m_SelectedIndex)->name == "particle_system") {
            // printf("Change Particle System ImGui\n");
            SceneObjectParticleSystem* sops = (SceneObjectParticleSystem*)m_SceneObjects.at(m_SelectedIndex);
            m_CurrentSOPS = sops;
            m_ParticleSettingsEdit = sops->GetSettings();
            m_ParticleSettingsPrev = sops->GetSettings();
        }
    }

    // Add new scene object with default settings
    if (mainWindow->getMouseButtons()[GLFW_MOUSE_BUTTON_1] && mainWindow->getKeys()[GLFW_KEY_LEFT_CONTROL])
    {
        AddSceneObject();
    }

    // Copy selected scene object
    if (mainWindow->getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow->getKeys()[GLFW_KEY_C])
    {
        CopySceneObject(mainWindow, &m_SceneObjects, m_SelectedIndex);
        m_SceneObjects[m_SelectedIndex]->isSelected = false;
    }

    // Delete selected object
    if (mainWindow->getKeys()[GLFW_KEY_DELETE])
    {
        DeleteSceneObject(mainWindow, &m_SceneObjects, m_SelectedIndex);
    }

    if (mainWindow->getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow->getKeys()[GLFW_KEY_R])
        ResetScene();

    if (mainWindow->getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow->getKeys()[GLFW_KEY_S])
        SaveScene();

    if (mainWindow->getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow->getKeys()[GLFW_KEY_L])
        LoadScene();

    // Gizmo switching modes
    if (mainWindow->getKeys()[GLFW_KEY_1])
        m_Gizmo->ChangeMode(GIZMO_MODE_TRANSLATE);

    if (mainWindow->getKeys()[GLFW_KEY_2])
        m_Gizmo->ChangeMode(GIZMO_MODE_SCALE);

    if (mainWindow->getKeys()[GLFW_KEY_3])
        m_Gizmo->ChangeMode(GIZMO_MODE_ROTATE);

    if (mainWindow->getKeys()[GLFW_KEY_4])
        m_Gizmo->ChangeMode(GIZMO_MODE_NONE);

    for (auto& object : m_SceneObjects)
    {
        object->GetAABB()->Update(object->position, object->rotation, object->scale);
        object->pivot->Update(object->position, object->scale + 1.0f);
    }

    UpdateLightDirection(m_Gizmo->GetRotation());
}

void SceneEditor::UpdateLightDirection(glm::quat rotation)
{
    if (rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.0f) return;

    glm::vec3 direction = glm::normalize(glm::eulerAngles(rotation) / toRadians);
    // printf("UpdateLightDirection direction: [ %.2ff %.2ff %.2ff ]\n", direction.x, direction.y, direction.z);

    if (m_SceneObjects[m_SelectedIndex]->name == "Light.directional") {
        LightManager::directionalLight.SetDirection(direction);
    }
    else if (m_SceneObjects[m_SelectedIndex]->name.substr(0, 10) == "Light.spot") {
        unsigned int spotLightIndex = m_SelectedIndex - 4 - 1; // minus 4 point lights, minus 1 directional light
        assert(spotLightIndex >= 0 && spotLightIndex <= 3);
        LightManager::spotLights[spotLightIndex].SetDirection(direction);
    }
}

Mesh* SceneEditor::CreateNewMesh(int meshTypeID, glm::vec3 scale, std::string* name)
{
    Mesh* mesh;
    switch (meshTypeID)
    {
    case MESH_TYPE_CUBE:
        mesh = new Block(scale);
        *name = "cube";
        break;
    case MESH_TYPE_PYRAMID:
        mesh = new Pyramid(scale);
        *name = "pyramid";
        break;
    case MESH_TYPE_SPHERE:
        mesh = new Sphere(scale);
        *name = "sphere";
        break;
    case MESH_TYPE_CYLINDER:
        mesh = new Cylinder(scale);
        *name = "cylinder";
        break;
    case MESH_TYPE_CONE:
        mesh = new Cone(scale);
        *name = "cone";
        break;
    case MESH_TYPE_RING:
        mesh = new Ring(scale);
        *name = "ring";
        break;
    case MESH_TYPE_BOB_LAMP:
        mesh = new SkinnedMesh("Models/OGLdev/BobLamp/boblampclean.md5mesh", "Models/OGLdev/BobLamp");
        ((SkinnedMesh*)mesh)->SetTimeMultiplier(1.0f);
        *name = "bob_lamp";
        break;
    case MESH_TYPE_ANIM_BOY:
        mesh = new SkinnedMesh("Models/ThinMatrix/AnimatedCharacter/AnimatedCharacter.dae", "Textures");
        ((SkinnedMesh*)mesh)->SetTimeMultiplier(800.0f);
        *name = "anim_boy";
        break;
    case MESH_TYPE_TERRAIN:
        mesh = new TerrainHeightMap("Textures/horizon_mountains.png", 4.0f, nullptr);
        *name = "terrain";
        break;
    case MESH_TYPE_WATER:
        mesh = new Tile2D();
        *name = "water";
        break;
    case MESH_TYPE_DRONE:
        mesh = new SkinnedMesh("Models/BusterDrone/busterDrone.gltf", "Textures/BusterDrone");
        *name = "drone";
        break;
    case MESH_TYPE_M1911:
        mesh = new Hazel::HazelMesh("Models/M1911/m1911.fbx", Hazel::Ref<Shader>(RendererBasic::s_Shaders["hybrid_anim_pbr"]), (*ResourceManager::GetMaterials())["M1911"], true);
        *name = "M1911";
        break;
    default:
        mesh = new Block(scale);
        *name = "cube";
        break;
    }
    return mesh;
}

SceneObject* SceneEditor::CreateNewSceneObject()
{
    // Add Scene Object here
    SceneObject* sceneObject = new SceneObject();

    sceneObject->id              = (int)m_SceneObjects.size();
    sceneObject->name            = "";
    sceneObject->transform       = glm::mat4(1.0f);
    sceneObject->position        = defaultSpawnPosition;
    sceneObject->rotation        = glm::quat(glm::vec3(0.0f));
    sceneObject->scale           = glm::vec3(1.0f);
    sceneObject->positionAABB    = defaultSpawnPosition;
    sceneObject->scaleAABB       = glm::vec3(1.0f);
    sceneObject->color           = glm::vec4(1.0f);
    sceneObject->textureName     = "none";
    sceneObject->tilingFactor    = 1.0f;
    sceneObject->isSelected      = true;
    sceneObject->SetAABB(new AABB(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
    sceneObject->pivot           = new Pivot(glm::vec3(0.0f), glm::vec3(1.0f));
    sceneObject->m_Type          = "";
    sceneObject->mesh            = nullptr;
    sceneObject->m_TypeID        = -1;
    sceneObject->model           = nullptr;
    sceneObject->materialName    = "";
    sceneObject->tilingFMaterial = 1.0f;

    return sceneObject;
}

void SceneEditor::AddSceneObject()
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectAdd.lastTime < m_ObjectAdd.cooldown) return;
    m_ObjectAdd.lastTime = m_CurrentTimestamp;

    m_Gizmo->SetActive(false);

    Mesh* mesh = nullptr;
    Model* model = nullptr;
    SceneObjectParticleSystem* particle_system = nullptr;

    std::string objectName = "";
    std::string objectType = "";
    std::string materialName = "";
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 scaleAABB = glm::vec3(1.0f);
    glm::vec3 positionAABB = glm::vec3(0.0f);

    // Mesh - ID range 0-999
    if (m_CurrentObjectTypeID < 1000) {
        mesh = CreateNewMesh(m_CurrentObjectTypeID, glm::vec3(1.0f), &objectName);
        objectName += "_" + std::to_string(m_SceneObjects.size());
        objectType = "mesh";

        if (m_CurrentObjectTypeID == MESH_TYPE_BOB_LAMP) {
            objectName = "bob_lamp";
            materialName = "none";
            rotation = glm::vec3(-90.0f, 0.0f, 0.0f);
            scale = glm::vec3(0.1f);
            positionAABB = glm::vec3(0.0f, 0.0f, 30.0f);
            scaleAABB = glm::vec3(20.0f, 20.0f, 60.0f);
        }
        else if (m_CurrentObjectTypeID == MESH_TYPE_ANIM_BOY) {
            objectName = "anim_boy";
            materialName = "anim_boy";
            rotation = glm::vec3(-90.0f, 0.0f, 0.0f);
            scale = glm::vec3(0.5f);
            positionAABB = glm::vec3(0.0f, 0.0f, 4.4f);
            scaleAABB = glm::vec3(2.4f, 2.0f, 8.8f);
        }
        else if (m_CurrentObjectTypeID == MESH_TYPE_TERRAIN) {
            objectName = "terrain";
            materialName = "none";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 0.0f, 0.0f);
            scaleAABB = glm::vec3(128.0f, 0.5f, 128.0f);
        }
        else if (m_CurrentObjectTypeID == MESH_TYPE_WATER) {
            objectName = "water";
            materialName = "none";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 0.0f, 0.0f);
            scaleAABB = glm::vec3(2.0f, 0.5f, 2.0f);
        }
        else if (m_CurrentObjectTypeID == MESH_TYPE_DRONE) {
            objectName = "buster_drone";
            materialName = "buster_drone";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 0.0f, 0.0f);
            scaleAABB = glm::vec3(1.0f, 1.0f, 1.0f);
        }
        else if (m_CurrentObjectTypeID == MESH_TYPE_M1911) {
            objectName = "M1911";
            materialName = "M1911";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(20.0f);
            positionAABB = glm::vec3(0.0f, 0.0f, 0.0f);
            scaleAABB = glm::vec3(0.24f, 0.14f, 0.03f);
        }
    }
    else if (m_CurrentObjectTypeID >= 1000 && m_CurrentObjectTypeID < 2000) { // Model - ID range 1000 - 2000
        model = AddNewModel(m_CurrentObjectTypeID, glm::vec3(1.0f));
        objectType = "model";
        if (m_CurrentObjectTypeID == MODEL_STONE_CARVED) {
            objectName = "stone_carved";
            materialName = "stone_carved";
            scale = glm::vec3(0.05f);
            positionAABB = glm::vec3(0.0f, 58.0f, 0.0f);
            scaleAABB = glm::vec3(74.0f, 116.0f, 40.0f);    
        }
        else if (m_CurrentObjectTypeID == MODEL_OLD_STOVE) {
            objectName = "old_stove";
            materialName = "old_stove";
            scale = glm::vec3(0.08f);
            positionAABB = glm::vec3(0.0f, 42.0f, 0.0f);
            scaleAABB = glm::vec3(30.0f, 84.0f, 30.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_BUDDHA) {
            objectName = "buddha";
            materialName = "none";
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 5.0f, 0.0f);
            scaleAABB = glm::vec3(4.0f, 10.0f, 4.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_HHELI) {
            objectName = "hheli";
            materialName = "none";
            rotation = glm::vec3(0.0f, 90.0f, 0.0f);
            scale = glm::vec3(0.05f);
            positionAABB = glm::vec3(20.0f, 40.0f, 0.0f);
            scaleAABB = glm::vec3(260.0f, 80.0f, 100.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_JEEP) {
            objectName = "jeep";
            materialName = "none";
            rotation = glm::vec3(0.0f, -90.0f, 0.0f);
            scale = glm::vec3(0.01f);
            positionAABB = glm::vec3(0.0f, 130.0f, -10.0f);
            scaleAABB = glm::vec3(780.0f, 260.0f, 400.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_DAMAGED_HELMET) {
            objectName = "damaged_helmet";
            materialName = "damaged_helmet";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, -0.2f, 0.0f);
            scaleAABB = glm::vec3(1.6f, 2.0f, 1.6f);
        }
        else if (m_CurrentObjectTypeID == MODEL_SF_HELMET) {
            objectName = "sf_helmet";
            materialName = "sf_helmet";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 0.0f, 0.0f);
            scaleAABB = glm::vec3(2.0f, 3.0f, 2.2f);
        }
        else if (m_CurrentObjectTypeID == MODEL_CERBERUS) {
            objectName = "cerberus";
            materialName = "cerberus";
            position = glm::vec3(0.0f, 5.0f, 0.0f);
            rotation = glm::vec3(-90.0f, 90.0f, 0.0f);
            scale = glm::vec3(0.05f);
            positionAABB = glm::vec3(0.0f, -50.0f, -8.0f);
            scaleAABB = glm::vec3(20.0f, 150.0f, 45.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_PINE) {
            objectName = "pine";
            materialName = "none";
            position = glm::vec3(0.0f, 0.0f, 0.0f);
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 10.0f, 0.0f);
            scaleAABB = glm::vec3(2.0f, 20.0f, 2.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_BOULDER) {
            objectName = "boulder";
            materialName = "boulder";
            position = glm::vec3(0.0f, 1.0f, 0.0f);
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(0.2f);
            positionAABB = glm::vec3(0.0f, 4.0f, 0.0f);
            scaleAABB = glm::vec3(10.0f, 20.0f, 10.0f);
        }
    }
    else if (m_CurrentObjectTypeID >= 2000) { // Model - ID range 2000+
        objectType = "particle_system";
        if (m_CurrentObjectTypeID == PARTICLE_SYSTEM) {
            // TODO - needed in case of multiple types of particle systems
        }
    }

    SceneObject* sceneObject;
    if (objectType == "mesh" || objectType == "model")
    {
        // Add Scene Object here
        sceneObject = CreateNewSceneObject();
        sceneObject->name = objectName;
        sceneObject->m_Type = objectType;
        sceneObject->position = position;
        sceneObject->rotation = glm::quat(rotation * toRadians);
        sceneObject->scale = scale;
        sceneObject->mesh = mesh;
        sceneObject->m_TypeID = m_CurrentObjectTypeID;
        sceneObject->model = model;
        sceneObject->materialName = materialName;
        sceneObject->positionAABB = positionAABB;
        sceneObject->scaleAABB = scaleAABB;
        sceneObject->SetAABB(new AABB(positionAABB, glm::vec3(0.0f), scaleAABB));
    }
    else if (objectType == "particle_system")
    {
        sceneObject = AddNewSceneObjectParticleSystem(m_CurrentObjectTypeID, glm::vec3(1.0f));
        sceneObject->name = "particle_system";
        sceneObject->m_Type = "particle_system";
        sceneObject->position = position;
        sceneObject->rotation = glm::quat(rotation * toRadians);
        sceneObject->scale = scale;
        sceneObject->mesh = nullptr;
        sceneObject->m_TypeID = m_CurrentObjectTypeID;
        sceneObject->model = nullptr;
        sceneObject->materialName = materialName;
        sceneObject->positionAABB = positionAABB;
        sceneObject->scaleAABB = scaleAABB;
        sceneObject->SetAABB(new AABB(positionAABB, glm::vec3(0.0f), scaleAABB));
    }

    m_SceneObjects.push_back(sceneObject);
    m_SelectedIndex = (unsigned int)m_SceneObjects.size() - 1;
}

void SceneEditor::CopySceneObject(Window* mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectCopy.lastTime < m_ObjectCopy.cooldown) return;
    m_ObjectCopy.lastTime = m_CurrentTimestamp;

    SceneObject* oldSceneObject = nullptr;

    if (selectedIndex < (unsigned int)sceneObjects->size())
        oldSceneObject = sceneObjects->at(selectedIndex);

    if (oldSceneObject == nullptr) return;

    Mesh* mesh = nullptr;
    Model* model = nullptr;
    std::string objectName = oldSceneObject->name;

    if (oldSceneObject->m_Type == "mesh" && oldSceneObject->mesh != nullptr) {
        mesh = CreateNewMesh(oldSceneObject->m_TypeID, oldSceneObject->mesh->GetScale(), &objectName);
    }
    else if (oldSceneObject->m_Type == "model" && oldSceneObject->model != nullptr) {
        model = AddNewModel(m_CurrentObjectTypeID, oldSceneObject->scale); // TODO: m_CurrentModelID hard-coded, must be in SceneObject
    }

    SceneObject* newSceneObject = new SceneObject();

    newSceneObject->id              = (int)sceneObjects->size();
    newSceneObject->name            = objectName;
    newSceneObject->transform       = oldSceneObject->transform;
    newSceneObject->position        = oldSceneObject->position;
    newSceneObject->rotation        = oldSceneObject->rotation;
    newSceneObject->scale           = oldSceneObject->scale;
    newSceneObject->positionAABB    = oldSceneObject->positionAABB;
    newSceneObject->scaleAABB       = oldSceneObject->scaleAABB;
    newSceneObject->color           = oldSceneObject->color;
    newSceneObject->textureName     = oldSceneObject->textureName;
    newSceneObject->tilingFactor    = oldSceneObject->tilingFactor;
    newSceneObject->isSelected      = true;
    newSceneObject->SetAABB(new AABB(newSceneObject->positionAABB, newSceneObject->rotation, newSceneObject->scaleAABB));
    newSceneObject->pivot           = new Pivot(newSceneObject->position, newSceneObject->scale);
    newSceneObject->m_Type          = oldSceneObject->m_Type;
    newSceneObject->mesh            = mesh;
    newSceneObject->m_TypeID        = m_CurrentObjectTypeID;
    newSceneObject->model           = model;
    newSceneObject->materialName    = oldSceneObject->materialName;
    newSceneObject->tilingFMaterial = oldSceneObject->tilingFMaterial;

    sceneObjects->push_back(newSceneObject);

    m_Gizmo->OnMouseRelease(mainWindow, sceneObjects, selectedIndex);
}

void SceneEditor::DeleteSceneObject(Window* mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectDelete.lastTime < m_ObjectDelete.cooldown) return;
    m_ObjectDelete.lastTime = m_CurrentTimestamp;

    delete m_SceneObjects[m_SelectedIndex];

    if (m_SelectedIndex < m_SceneObjects.size())
        m_SceneObjects.erase(m_SceneObjects.begin() + m_SelectedIndex);

    if (m_SceneObjects.size() > 0) {
        m_SelectedIndex = (unsigned int)m_SceneObjects.size() - 1;
        m_Gizmo->OnMousePress(mainWindow, sceneObjects, selectedIndex);
    }

    // refresh scene object IDs
    for (int i = 0; i < m_SceneObjects.size(); i++)
        m_SceneObjects[i]->id = i;

    // delete Gizmo if there's no objects
    if (m_SceneObjects.size() == 0)
        m_Gizmo->SetActive(false);
}

Model* SceneEditor::AddNewModel(int modelID, glm::vec3 scale)
{
    Model* model;
    switch (modelID)
    {
    case MODEL_STONE_CARVED:
        model = new Model("Models/Stone_Carved/tf3pfhzda_LOD0.fbx");
        break;
    case MODEL_OLD_STOVE:
        model = new Model("Models/Old_Stove/udmheheqx_LOD0.fbx");
        break;
    case MODEL_BUDDHA:
        model = new Model("Models/OGLdev/buddha/buddha.obj", "Textures/OGLdev/buddha");
        break;
    case MODEL_HHELI:
        model = new Model("Models/OGLdev/hheli/hheli.obj", "Textures/OGLdev/hheli");
        break;
    case MODEL_JEEP:
        model = new Model("Models/OGLdev/jeep/jeep.obj", "Textures/OGLdev/jeep");
        break;
    case MODEL_DAMAGED_HELMET:
        model = new Model("Models/DamagedHelmet.gltf", "Textures/PBR/DamagedHelmet");
        break;
    case MODEL_SF_HELMET:
        model = new Model("Models/SciFiHelmet.gltf", "Textures/PBR/SciFiHelmet");
        break;
    case MODEL_CERBERUS:
        model = new Model("Models/Cerberus/Cerberus_LP.FBX", "Models/Cerberus/Textures");
        break;
    case MODEL_PINE:
        model = new Model("Models/ThinMatrix/pine.obj", "Textures/ThinMatrix");
        break;
    case MODEL_BOULDER:
        model = new Model("Models/ThinMatrix/boulder.obj", "Textures/ThinMatrix");
        break;
    default:
        model = new Model("Models/Stone_Carved/tf3pfhzda_LOD0.fbx");
        break;
    }
    return model;
}

SceneObjectParticleSystem* SceneEditor::AddNewSceneObjectParticleSystem(int objectTypeID, glm::vec3 scale)
{
    SceneObjectParticleSystem* particle_system = new SceneObjectParticleSystem(true, m_MaxInstances, m_Camera, m_CameraController);
    m_ParticleSettingsEdit = particle_system->GetSettings();
    m_ParticleSettingsPrev = m_ParticleSettingsEdit;

    m_CurrentSOPS = particle_system;

    return particle_system;
}

void SceneEditor::SetUniformsShaderEditor(Shader* shaderEditor, Texture* texture, SceneObject* sceneObject)
{
    shaderEditor->Bind();

    shaderEditor->setMat4("model", sceneObject->transform);
    shaderEditor->setVec4("tintColor", sceneObject->color);
    shaderEditor->setBool("isSelected", sceneObject->isSelected);

    shaderEditor->setFloat("material.specularIntensity", ResourceManager::s_MaterialSpecular);  // TODO - use material attribute
    shaderEditor->setFloat("material.shininess", ResourceManager::s_MaterialShininess); // TODO - use material attribute

    if (texture != nullptr)
        texture->Bind(0);

    shaderEditor->setInt("albedoMap", 0);
    shaderEditor->setFloat("tilingFactor", sceneObject->tilingFactor);

    if (m_PBR_Map_Edit == PBR_MAP_ENVIRONMENT)
        m_MaterialWorkflowPBR->BindEnvironmentCubemap(1);
    else if (m_PBR_Map_Edit == PBR_MAP_IRRADIANCE)
        m_MaterialWorkflowPBR->BindIrradianceMap(1);
    else if (m_PBR_Map_Edit == PBR_MAP_PREFILTER)
        m_MaterialWorkflowPBR->BindPrefilterMap(1);

    shaderEditor->setInt("cubeMap", 1);
    shaderEditor->setBool("useCubeMaps", m_UseCubeMaps);

    // Shadows in shaderEditor
    if (LightManager::directionalLight.GetShadowMap() != nullptr)
        LightManager::directionalLight.GetShadowMap()->Read(2);
    shaderEditor->setInt("shadowMap", 2);
}

void SceneEditor::SetUniformsShaderEditorPBR(Shader* shaderEditorPBR, Texture* texture, Hazel::Ref<Material> material, SceneObject* sceneObject)
{
    shaderEditorPBR->Bind();

    shaderEditorPBR->setMat4("model",         sceneObject->transform);
    shaderEditorPBR->setVec4("tintColor",     sceneObject->color);
    shaderEditorPBR->setBool("isSelected",    sceneObject->isSelected);

    shaderEditorPBR->setFloat("material.specularIntensity", ResourceManager::s_MaterialSpecular);  // TODO - use material attribute
    shaderEditorPBR->setFloat("material.shininess", ResourceManager::s_MaterialShininess); // TODO - use material attribute

    m_MaterialWorkflowPBR->BindTextures(0); // texture slots 0, 1, 2
    material->BindTextures(3);              // texture slots 3, 4, 5, 6, 7

    // Override albedo map from material with texture, if texture is available
    if (sceneObject->textureName != "" && sceneObject->textureName != "none") {
        texture->Bind(3); // Albedo is at slot 3
        shaderEditorPBR->setFloat("tilingFactor", sceneObject->tilingFactor);
    }
    else {
        shaderEditorPBR->setFloat("tilingFactor", sceneObject->tilingFMaterial);
    }

    // Shadows in shaderEditorPBR
    LightManager::directionalLight.GetShadowMap()->Read(8); // texture slots 8
    shaderEditorPBR->setInt("shadowMap", 8);
}

void SceneEditor::SetUniformsShaderSkinning(Shader* shaderSkinning, SceneObject* sceneObject, float runningTime)
{
    RendererBasic::DisableCulling();

    shaderSkinning->Bind();

    SkinnedMesh* skinnedMesh = (SkinnedMesh*)sceneObject->mesh;
    skinnedMesh->BoneTransform(runningTime, m_SkinningTransforms[sceneObject->name]);
    shaderSkinning->setMat4("model", sceneObject->transform);
    shaderSkinning->setMat4("view", m_Camera->GetViewMatrix());
    shaderSkinning->setInt("gColorMap", 0);
    shaderSkinning->setVec3("gEyeWorldPos", m_Camera->GetPosition());
    shaderSkinning->setFloat("gMatSpecularIntensity", ResourceManager::s_MaterialSpecular);
    shaderSkinning->setFloat("gSpecularPower", ResourceManager::s_MaterialShininess);
    char locBuff[100] = { '\0' };
    for (unsigned int i = 0; i < m_SkinningTransforms[sceneObject->name].size(); i++)
    {
        snprintf(locBuff, sizeof(locBuff), "gBones[%d]", i);
        shaderSkinning->setMat4(locBuff, m_SkinningTransforms[sceneObject->name][i]);
    }
}

void SceneEditor::SetUniformsShaderHybridAnimPBR(Shader* shaderHybridAnimPBR, Texture* texture, SceneObject* sceneObject, float runningTime)
{
    RendererBasic::DisableCulling();

    shaderHybridAnimPBR->Bind();

    shaderHybridAnimPBR->setInt("u_AlbedoTexture",    m_SamplerSlots["albedo"]);
    shaderHybridAnimPBR->setInt("u_NormalTexture",    m_SamplerSlots["normal"]);
    shaderHybridAnimPBR->setInt("u_MetalnessTexture", m_SamplerSlots["metalness"]);
    shaderHybridAnimPBR->setInt("u_RoughnessTexture", m_SamplerSlots["roughness"]);
    shaderHybridAnimPBR->setInt("u_AOTexture",        m_SamplerSlots["ao"]);
    shaderHybridAnimPBR->setInt("u_EnvRadianceTex",   m_SamplerSlots["irradiance"]);
    shaderHybridAnimPBR->setInt("u_PrefilterMap",     m_SamplerSlots["prefilter"]);
    shaderHybridAnimPBR->setInt("u_BRDFLUT",          m_SamplerSlots["BRDF_LUT"]);

    shaderHybridAnimPBR->setMat4("u_ViewProjectionMatrix", RendererBasic::GetProjectionMatrix() * m_Camera->GetViewMatrix());
    shaderHybridAnimPBR->setVec3("u_CameraPosition", m_Camera->GetPosition());

    Hazel::Ref<Material> baseMaterial = ResourceManager::HotLoadMaterial(sceneObject->materialName);

    baseMaterial->GetTextureAlbedo()->Bind(m_SamplerSlots["albedo"]);
    baseMaterial->GetTextureNormal()->Bind(m_SamplerSlots["normal"]);
    baseMaterial->GetTextureMetallic()->Bind(m_SamplerSlots["metalness"]);
    baseMaterial->GetTextureRoughness()->Bind(m_SamplerSlots["roughness"]);
    baseMaterial->GetTextureAO()->Bind(m_SamplerSlots["ao"]);

    // Override albedo map from material with texture, if texture is available
    if (sceneObject->textureName != "" && sceneObject->textureName != "none") {
        texture->Bind(m_SamplerSlots["albedo"]); // Albedo is at slot 3
        shaderHybridAnimPBR->setFloat("u_TilingFactor", sceneObject->tilingFactor);
    }
    else {
        shaderHybridAnimPBR->setFloat("u_TilingFactor", sceneObject->tilingFMaterial);
    }

    m_MaterialWorkflowPBR->BindTextures(m_SamplerSlots["irradiance"]);

    Hazel::HazelMesh* meshAnimPBR = (Hazel::HazelMesh*)sceneObject->mesh;

    float deltaTime = Timer::Get()->GetDeltaTime();
    meshAnimPBR->OnUpdate(deltaTime, false);

    meshAnimPBR->m_VertexBuffer->Bind();
    meshAnimPBR->m_Pipeline->Bind();
    meshAnimPBR->m_IndexBuffer->Bind();

    auto& materials = meshAnimPBR->GetMaterials();

    int submeshIndex = 0;
    for (Hazel::Submesh& submesh : meshAnimPBR->GetSubmeshes())
    {
        // Material
        auto material = materials[submesh.MaterialIndex];

        for (size_t i = 0; i < meshAnimPBR->m_BoneTransforms.size(); i++)
        {
            std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
            shaderHybridAnimPBR->setMat4(uniformName, meshAnimPBR->m_BoneTransforms[i]);
        }

        glm::mat4 transform = sceneObject->transform * submesh.Transform;
        transform = glm::scale(transform, sceneObject->scale);
        shaderHybridAnimPBR->setMat4("u_Transform", transform);
        shaderHybridAnimPBR->Validate();

        // TODO move to virtual HazelMesh::Render() method
        glEnable(GL_DEPTH_TEST);
        glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);

        submeshIndex++;
    }
}

void SceneEditor::SetUniformsShaderWater(Shader* shaderWater, SceneObject* sceneObject, glm::mat4& projectionMatrix)
{
    RendererBasic::EnableTransparency();

    shaderWater->Bind();
    shaderWater->setInt("reflectionTexture", 0);
    shaderWater->setInt("refractionTexture", 1);
    shaderWater->setInt("normalMap",         2);
    shaderWater->setInt("dudvMap",           3);
    shaderWater->setInt("depthMap",          4);

    shaderWater->setMat4("model",          sceneObject->transform);
    shaderWater->setMat4("view",           m_Camera->GetViewMatrix());
    shaderWater->setMat4("projection",     projectionMatrix);
    shaderWater->setVec3("lightPosition",  -(LightManager::directionalLight.GetDirection()));
    shaderWater->setVec3("cameraPosition", m_Camera->GetPosition());
    shaderWater->setVec3("lightColor",     LightManager::directionalLight.GetColor());
    shaderWater->setFloat("moveFactor",    m_WaterManager->GetWaterMoveFactor());
    shaderWater->setFloat("nearPlane",     sceneSettings.nearPlane);
    shaderWater->setFloat("farPlane",      sceneSettings.farPlane);

    ResourceManager::GetTexture("none")->Bind(0);
    ResourceManager::GetTexture("none")->Bind(1);
    ResourceManager::GetTexture("waterNormal")->Bind(2);
    ResourceManager::GetTexture("waterDuDv")->Bind(3);
    ResourceManager::GetTexture("none")->Bind(4);

    if (sceneSettings.enableWaterEffects) {
        m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(0);
        m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(1);
        m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(4);
    }
}

void SceneEditor::SwitchOrthographicView(Window* mainWindow, glm::mat4& projectionMatrix)
{
    if (mainWindow->getKeys()[GLFW_KEY_O])
    {
        if (Timer::Get()->GetCurrentTimestamp() - m_ProjectionChange.lastTime > m_ProjectionChange.cooldown)
        {
            m_OrthographicViewEnabled = !m_OrthographicViewEnabled;
            m_ProjectionChange.lastTime = Timer::Get()->GetCurrentTimestamp();
        }
    }

    if (m_OrthographicViewEnabled)
    {
        float left   = -(float)mainWindow->GetWidth() / 2.0f / m_FOV;
        float right  = (float)mainWindow->GetWidth() / 2.0f / m_FOV;
        float bottom = -(float)mainWindow->GetHeight() / 2.0f / m_FOV;
        float top    = (float)mainWindow->GetHeight() / 2.0f / m_FOV;

        projectionMatrix = glm::ortho(left, right, bottom, top, sceneSettings.nearPlane, sceneSettings.farPlane);
    }
}

glm::mat4 SceneEditor::CalculateRenderTransform(SceneObject* sceneObject)
{
    glm::vec3 renderScale = glm::vec3(1.0f);

    // For meshes that can't be scaled on vertex level
    if (m_FixedVertexMeshes.find(sceneObject->m_TypeID) != m_FixedVertexMeshes.end() ||
        m_SkinnedMeshes.find(sceneObject->m_TypeID) != m_SkinnedMeshes.end())
        renderScale = sceneObject->scale;

    // Quixel Megascans models should be downscaled to 2% of their original size
    if (sceneObject->m_Type == "model") {
        renderScale = sceneObject->scale;
    }

    return Math::CreateTransform(sceneObject->position, sceneObject->rotation, renderScale);
}

bool SceneEditor::IsWaterOnScene()
{
    for (auto& object : m_SceneObjects) {
        if (object->name == "water") return true;
    }
    return false;
}

void SceneEditor::AddLightsToSceneObjects()
{
    // Directional Light - Cone Mesh
    SceneObject* sceneObject = CreateNewSceneObject();

    glm::vec3 position = glm::vec3(-10.0f, 10.0f, 10.0f);
    glm::vec3 rotation = LightManager::directionalLight.GetDirection();
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, glm::radians(LightManager::directionalLight.GetDirection().x * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::rotate(transform, glm::radians(LightManager::directionalLight.GetDirection().y * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(LightManager::directionalLight.GetDirection().z * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::scale(transform, scale);

    sceneObject->name = "Light.directional";
    sceneObject->m_Type = "mesh";
    sceneObject->position = position;
    sceneObject->rotation = glm::quat(rotation * toRadians);
    sceneObject->scale = scale;
    sceneObject->transform = transform;
    sceneObject->m_TypeID = MESH_TYPE_CONE;
    if (sceneObject->m_Type == "mesh") {
        std::string objectNameVoid = "";
        sceneObject->mesh = CreateNewMesh(sceneObject->m_TypeID, sceneObject->scale, &objectNameVoid);
    }
    sceneObject->model = nullptr;
    sceneObject->materialName = "none";
    sceneObject->positionAABB = glm::vec3(0.0f);
    sceneObject->scaleAABB = sceneObject->scale;
    sceneObject->SetAABB(new AABB(sceneObject->positionAABB, glm::vec3(0.0f), sceneObject->scaleAABB));
    sceneObject->pivot = new Pivot(sceneObject->position, sceneObject->scale);

    m_SceneObjects.push_back(sceneObject);

    printf("AddLightsToSceneObjects Add Directional Light m_SceneObjects[%i]\n", (int)m_SceneObjects.size() - 1);

    for (unsigned int i = 0; i < LightManager::pointLightCount; i++)
    {
        // Point lights - Sphere mesh
        SceneObject* sceneObject = CreateNewSceneObject();

        glm::vec3 position = LightManager::pointLights[i].GetPosition();
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = glm::scale(transform, scale);

        sceneObject->name = "Light.point." + std::to_string(i);
        sceneObject->m_Type = "mesh";
        sceneObject->position = position;
        sceneObject->rotation = glm::quat(rotation * toRadians);
        sceneObject->scale = scale;
        sceneObject->transform = transform;
        sceneObject->m_TypeID = MESH_TYPE_SPHERE;
        if (sceneObject->m_Type == "mesh") {
            std::string objectNameVoid = "";
            sceneObject->mesh = CreateNewMesh(sceneObject->m_TypeID, sceneObject->scale, &objectNameVoid);
        }
        sceneObject->model = nullptr;
        sceneObject->materialName = "none";
        sceneObject->positionAABB = glm::vec3(0.0f);
        sceneObject->scaleAABB = sceneObject->scale;
        sceneObject->SetAABB(new AABB(sceneObject->positionAABB, glm::vec3(0.0f), sceneObject->scaleAABB));
        sceneObject->pivot = new Pivot(sceneObject->position, sceneObject->scale);

        m_SceneObjects.push_back(sceneObject);

        printf("AddLightsToSceneObjects Add Point Light m_SceneObjects[%i]\n", (int)m_SceneObjects.size() - 1);
    }

    for (unsigned int i = 0; i < LightManager::spotLightCount; i++)
    {
        // Spot lights - Sphere mesh
        SceneObject* sceneObject = CreateNewSceneObject();

        glm::vec3 position = LightManager::pointLights[i].GetPosition();
        glm::vec3 rotation = LightManager::spotLights[i].GetDirection();
        glm::vec3 scale = glm::vec3(1.0f);

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = glm::rotate(transform, glm::radians(LightManager::spotLights[i].GetDirection().x * 90.0f),  glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::rotate(transform, glm::radians(LightManager::spotLights[i].GetDirection().y * 90.0f),  glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(LightManager::spotLights[i].GetDirection().z * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::scale(transform, scale);

        sceneObject->name = "Light.spot." + std::to_string(i);
        sceneObject->m_Type = "mesh";
        sceneObject->position = position;
        sceneObject->rotation = glm::quat(rotation * toRadians);
        sceneObject->scale = scale;
        sceneObject->transform = transform;
        sceneObject->m_TypeID = MESH_TYPE_CONE;
        if (sceneObject->m_Type == "mesh") {
            std::string objectNameVoid = "";
            sceneObject->mesh = CreateNewMesh(sceneObject->m_TypeID, sceneObject->scale, &objectNameVoid);
        }
        sceneObject->model = nullptr;
        sceneObject->materialName = "none";
        sceneObject->positionAABB = glm::vec3(0.0f);
        sceneObject->scaleAABB = sceneObject->scale;
        sceneObject->SetAABB(new AABB(sceneObject->positionAABB, glm::vec3(0.0f), sceneObject->scaleAABB));
        sceneObject->pivot = new Pivot(sceneObject->position, sceneObject->scale);

        m_SceneObjects.push_back(sceneObject);

        printf("AddLightsToSceneObjects Add Spot Light m_SceneObjects[%i]\n", (int)m_SceneObjects.size() - 1);
    }
}

void SceneEditor::RenderLightSources(Shader* shaderGizmo)
{
    shaderGizmo->Bind();

    ResourceManager::GetTexture("none")->Bind(0);
    ResourceManager::GetTexture("none")->Bind(1);
    ResourceManager::GetTexture("none")->Bind(2);

    glm::mat4 model;

    // Directional light (somewhere on pozitive Y axis, at X=0, Z=0)
    model = glm::mat4(1.0f);
    model = glm::translate(model, m_SceneObjects[0]->position);
    model = glm::rotate(model, glm::radians(LightManager::directionalLight.GetDirection().x *  90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(LightManager::directionalLight.GetDirection().y *  90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(LightManager::directionalLight.GetDirection().z * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f));

    glm::vec3 dir = LightManager::directionalLight.GetDirection();
    // printf("Render Dir Light Direction [ %.2ff %.2ff %.2ff ]\n", dir.x, dir.y, dir.z);

    m_SceneObjects[0]->transform = model;
    shaderGizmo->setMat4("model", model);
    shaderGizmo->setVec4("tintColor", glm::vec4(LightManager::directionalLight.GetColor(), 1.0f));
    if (m_DisplayLightSources && LightManager::directionalLight.GetEnabled())
        m_SceneObjects[0]->Render();

    // Point lights - render Sphere (Light source)
    unsigned int offsetPoint = 1;
    for (unsigned int i = 0; i < LightManager::pointLightCount; i++)
    {
        LightManager::pointLights[i].SetPosition(m_SceneObjects[offsetPoint + i]->position);

        model = glm::mat4(1.0f);
        model = glm::translate(model, m_SceneObjects[offsetPoint + i]->position);
        model = glm::scale(model, glm::vec3(0.5f));
        m_SceneObjects[offsetPoint + i]->transform = model;
        shaderGizmo->setMat4("model", model);
        shaderGizmo->setVec4("tintColor", glm::vec4(LightManager::pointLights[i].GetColor(), 1.0f));
        if (m_DisplayLightSources && LightManager::pointLights[i].GetEnabled())
            m_SceneObjects[offsetPoint + i]->Render();
    }

    // Spot lights - render cone
    unsigned int offsetSpot = offsetPoint + LightManager::pointLightCount;
    for (unsigned int i = 0; i < LightManager::spotLightCount; i++)
    {
        LightManager::spotLights[i].GetBasePL()->SetPosition(m_SceneObjects[offsetSpot + i]->position);
        // LightManager::spotLights[i].SetDirection(glm::vec3(
        //     glm::eulerAngles(m_SceneObjects[offsetSpot + i]->rotation / toRadians).x ,
        //     glm::eulerAngles(m_SceneObjects[offsetSpot + i]->rotation / toRadians).y,
        //     glm::eulerAngles(m_SceneObjects[offsetSpot + i]->rotation / toRadians).z));

        model = glm::mat4(1.0f);
        model = glm::translate(model, m_SceneObjects[offsetSpot + i]->position);
        model = glm::rotate(model, glm::radians(LightManager::spotLights[i].GetDirection().x *  90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(LightManager::spotLights[i].GetDirection().y *  90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(LightManager::spotLights[i].GetDirection().z * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        m_SceneObjects[offsetPoint + i]->transform = model;
        shaderGizmo->setMat4("model", model);
        shaderGizmo->setVec4("tintColor", glm::vec4(LightManager::spotLights[i].GetBasePL()->GetColor(), 1.0f));
        if (m_DisplayLightSources && LightManager::spotLights[i].GetBasePL()->GetEnabled())
            m_SceneObjects[offsetSpot + i]->Render();
    }
}

void SceneEditor::RenderSkybox(Shader* shaderBackground)
{
    m_BlurEffect->Render();

    if (m_IsViewportEnabled) {
        m_RenderFramebuffer->Bind();
    }
    else {
        RendererBasic::SetDefaultFramebuffer(Application::Get()->GetWindow()->GetWidth(), Application::Get()->GetWindow()->GetHeight());
    }

    int environmentMapSlot = 0;

    // Skybox shaderBackground
    RendererBasic::DisableCulling();
    shaderBackground->Bind();
    // render skybox (render as last to prevent overdraw)

    glm::mat4 transform = glm::mat4(1.0f);
    float angleRadians = glm::radians((GLfloat)glfwGetTime());
    transform = glm::rotate(transform, angleRadians * m_SkyboxRotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
    shaderBackground->setMat4("model", transform);

    shaderBackground->setInt("environmentMap", environmentMapSlot);
    shaderBackground->setFloat("u_TextureLOD", m_SkyboxLOD);

    if (m_PBR_Map_Edit == PBR_MAP_ENVIRONMENT)
        m_MaterialWorkflowPBR->BindEnvironmentCubemap(environmentMapSlot);
    else if (m_PBR_Map_Edit == PBR_MAP_IRRADIANCE)
        m_MaterialWorkflowPBR->BindIrradianceMap(environmentMapSlot);
    else if (m_PBR_Map_Edit == PBR_MAP_PREFILTER)
        m_MaterialWorkflowPBR->BindPrefilterMap(environmentMapSlot);

    // m_BlurEffect->GetVerticalFBO()->GetTextureAttachmentColor()->Bind(environmentMapSlot);

    m_MaterialWorkflowPBR->GetSkyboxCube()->Render();
}

void SceneEditor::RenderLineElements(Shader* shaderBasic, glm::mat4 projectionMatrix)
{
    if (!m_DrawGizmos) return;

    shaderBasic->Bind();
    if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
    {
        shaderBasic->setMat4("model", glm::mat4(1.0f));

        // TODO: don't rely on hard coded array indices for lights (0 to 8) in m_SceneObjects
        bool drawAABB = true;
        if (m_SceneObjects[m_SelectedIndex]->name.substr(0, 6) == "Light.") {
            // Directional light
            if (m_SelectedIndex == 0)
                drawAABB = LightManager::directionalLight.GetEnabled();
            // Point Lights
            else if (m_SelectedIndex >= 1 && m_SelectedIndex <= 4)
                drawAABB = LightManager::pointLights[m_SelectedIndex].GetEnabled();
            // Spot Lights
            else if (m_SelectedIndex >= 5 && m_SelectedIndex <= 8)
                drawAABB = LightManager::spotLights[m_SelectedIndex].GetBasePL()->GetEnabled();
        }

        if (drawAABB) {
            m_SceneObjects[m_SelectedIndex]->GetAABB()->Draw();
            m_SceneObjects[m_SelectedIndex]->pivot->Draw(shaderBasic, projectionMatrix, m_Camera->GetViewMatrix());
        }
    }

    m_Grid->Draw(shaderBasic, projectionMatrix, m_Camera->GetViewMatrix());
    m_PivotScene->Draw(shaderBasic, projectionMatrix, m_Camera->GetViewMatrix());
}

void SceneEditor::RenderFramebufferTextures(Shader* shaderEditor)
{
    // A quad for displaying a shadow map on it
    shaderEditor->Bind();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 10.0f, -20.0f));
    model = glm::scale(model, glm::vec3(16.0f, 9.0f, 1.0f));
    shaderEditor->setMat4("model", model);
    LightManager::directionalLight.GetShadowMap()->Read(0);
    shaderEditor->setInt("shadowMap", 0);
    m_Quad->Render();
}

void SceneEditor::RenderGlassObjects(Shader* shaderGlass)
{
    // Glass objects (Reflection/Refraction/Fresnel)
    shaderGlass->Bind();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -10.0f));
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(0.05f));
    shaderGlass->setMat4("model", model);

    if (m_PBR_Map_Edit == PBR_MAP_ENVIRONMENT)
        m_MaterialWorkflowPBR->BindEnvironmentCubemap(1);
    else if (m_PBR_Map_Edit == PBR_MAP_IRRADIANCE)
        m_MaterialWorkflowPBR->BindIrradianceMap(1);
    else if (m_PBR_Map_Edit == PBR_MAP_PREFILTER)
        m_MaterialWorkflowPBR->BindPrefilterMap(1);

    shaderGlass->setInt("uCubemap", 1);

    m_GlassShaderModel->RenderPBR();
}

void SceneEditor::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    m_ActiveRenderPasses.push_back(passType); // for displaying all render passes in ImGui

    SwitchOrthographicView(mainWindow, projectionMatrix);

    if (passType == "main") {
        // RenderSkybox(shaders["background"]);
    }

    bool shouldRenderObject;

    for (auto& object : m_SceneObjects)
    {
        shouldRenderObject = true;

        object->transform = CalculateRenderTransform(object);

        if (passType == "shadow_dir") {
            shaders["shadow_map"]->Bind();
            shaders["shadow_map"]->setMat4("model", object->transform);

            if (!object->castShadow)
                shouldRenderObject = false;
        }
        if (passType == "shadow_omni") {
            shaders["omni_shadow_map"]->Bind();
            shaders["omni_shadow_map"]->setMat4("model", object->transform);

            if (!object->castShadow)
                shouldRenderObject = false;
        }
        if (passType == "main" || passType == "water_reflect" || passType == "water_refract") {
            shaders["editor_object"]->Bind();
            shaders["editor_object"]->setMat4("model", object->transform);
            shaders["editor_object_pbr"]->Bind();
            shaders["editor_object_pbr"]->setMat4("model", object->transform);
            shaders["skinning"]->Bind();
            shaders["skinning"]->setMat4("model", object->transform);
        }
        if (passType == "main") {
            ResourceManager::GetTexture("none")->Bind(0); // Default fallback for Albedo texture
            shaders["water"]->Bind();
            shaders["water"]->setMat4("model", object->transform);
        }

        float runningTime = ((float)glfwGetTime() * 1000.0f - m_StartTimestamp) / 1000.0f;

        Texture* texture = ResourceManager::HotLoadTexture(object->textureName);
        Hazel::Ref<Material> material = ResourceManager::HotLoadMaterial(object->materialName);

        // Don't render Lights (id = 0 to 8), it's done in RenderLightSources()
        if (object->name.substr(0, 6) == "Light.")
            shouldRenderObject = false;

        // Don't render Water tiles in Water Render Passes
        if (object->name == "water" && passType != "main")
            shouldRenderObject = false;

        // Setup Shader Uniforms
        if (object->name == "water") { // is it a water tile
            // Render with 'water' shader
            if (passType == "main")
                SetUniformsShaderWater(shaders["water"], object, projectionMatrix);
        }
        else if (m_SkinnedMeshes.find(object->m_TypeID) != m_SkinnedMeshes.end()) // is it a skinned mesh?
        {
            // Render with 'skinning' shader
            if (passType == "main" || passType == "water_reflect" || passType == "water_refract")
                SetUniformsShaderSkinning(shaders["skinning"], object, runningTime);
        }
        else if (m_AnimPBRMeshes.find(object->m_TypeID) != m_AnimPBRMeshes.end()) // is it a animated PBR mesh?
        {
            // Render with 'skinning' shader
            if (passType == "main" || passType == "water_reflect" || passType == "water_refract")
                SetUniformsShaderHybridAnimPBR(shaders["hybrid_anim_pbr"], texture, object, runningTime);
        }
        else if (material && object->materialName != "none") { // is it using a material?
            // Render with 'editor_object_pbr' shader
            if (passType == "main" || passType == "water_reflect" || passType == "water_refract")
                SetUniformsShaderEditorPBR(shaders["editor_object_pbr"], texture, material, object);
        }
        else if (object->name == "particle_system") {
            shouldRenderObject = false;
        }
        else { // defaults to a texture only
            // Render with 'editor_object' shader
            if (passType == "main" || passType == "water_reflect" || passType == "water_refract")
                SetUniformsShaderEditor(shaders["editor_object"], texture, object);
        }

        if (shouldRenderObject) {
            // m_BlurEffect->GetVerticalFBO()->GetTextureAttachmentColor()->Bind(0);
            object->Render();
        }
    }

    if (passType == "main")
    {
        RenderLightSources(shaders["gizmo"]);
        RenderLineElements(shaders["basic"], projectionMatrix);
        RenderSkybox(shaders["background"]);
        // RenderFramebufferTextures(shaders["editor_object"]);
        
        // Render gizmo on front of everything (depth mask enabled)
        if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
            m_Gizmo->Render(shaders["gizmo"]);
    }

    if (passType == "water_reflect")
    {
        RenderSkybox(shaders["background"]);
    }

    if (sceneSettings.enableParticles && (passType == "main" || passType == "water_reflect"))
    {
        for (auto& object : m_SceneObjects)
        {
            if (object->name == "particle_system")
            {
                Profiler profiler("SE::SOPS::Render [SO_ID=" + std::to_string(object->id) + "]");
                object->Render();
                GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
            }
        }
    }
}

void SceneEditor::ResetScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneReset.lastTime < m_SceneReset.cooldown) return;
    m_SceneReset.lastTime = m_CurrentTimestamp;

    printf("ResetScene: Deleting %zu objects...\n", m_SceneObjects.size());

    for (auto& object : m_SceneObjects)
    {
        // delete object->AABB;
        // delete object->pivot;
        // delete object->mesh;
    }
 
    m_SceneObjects.clear();
    AddLightsToSceneObjects();
}

void SceneEditor::CleanupGeometry()
{
}

SceneEditor::~SceneEditor()
{
    SaveScene();
    CleanupGeometry();
    delete m_ParticleSettingsEdit;
    delete m_ParticleSettingsPrev;
    delete m_CurrentSOPS;
    delete m_PivotScene;
    delete m_Grid;
    delete m_Raycast;
    delete m_Gizmo;
    ResetScene();
}
