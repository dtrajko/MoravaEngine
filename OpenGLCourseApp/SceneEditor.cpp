#include "SceneEditor.h"

#include "glm/gtc/matrix_transform.hpp"

#include "ImGuiWrapper.h"
#include "MousePicker.h"
#include "Block.h"
#include "Sphere.h"
#include "Pyramid.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Ring.h"
#include "Shader.h"
#include "Math.h"
#include "Timer.h"
#include "RendererBasic.h"
#include "TextureLoader.h"

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>


SceneEditor::SceneEditor()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 2.0f, 12.0f);
	sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.enableSkybox = false;
    sceneSettings.enablePointLights = true;
    sceneSettings.enableSpotLights = true;
    sceneSettings.enableShadows = true;
    sceneSettings.enableOmniShadows = false;

    // directional light
    sceneSettings.directionalLight.base.enabled = true;
    sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.directionalLight.direction = glm::vec3(0.6f, -0.6f, -0.6f);
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
    sceneSettings.spotLights[0].direction = glm::vec3(1.0f, -1.0f, 0.0f);
    sceneSettings.spotLights[0].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[0].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[0].edge = 0.5f;

    sceneSettings.spotLights[1].base.base.enabled = false;
    sceneSettings.spotLights[1].base.base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[1].base.position = glm::vec3(5.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[1].direction = glm::vec3(-1.0f, -1.0f, 0.0f);
    sceneSettings.spotLights[1].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[1].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[1].edge = 0.5f;

    sceneSettings.spotLights[2].base.base.enabled = false;
    sceneSettings.spotLights[2].base.base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[2].base.position = glm::vec3(0.0f, 1.0f, -5.0f);
    sceneSettings.spotLights[2].direction = glm::vec3(0.0f, -1.0f, 1.0f);
    sceneSettings.spotLights[2].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[2].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[2].edge = 0.5f;

    sceneSettings.spotLights[3].base.base.enabled = false;
    sceneSettings.spotLights[3].base.base.color = glm::vec3(1.0f, 0.0f, 1.0f);
    sceneSettings.spotLights[3].base.position = glm::vec3(0.0f, 1.0f, 5.0f);
    sceneSettings.spotLights[3].direction = glm::vec3(0.0f, -1.0f, -1.0f);
    sceneSettings.spotLights[3].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[3].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[3].edge = 0.5f;

	SetCamera();
	SetSkybox();
	SetTextures();
    SetupMaterials();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    // Initialize the PBR/IBL Material Workflow component
    m_MaterialWorkflowPBR = new MaterialWorkflowPBR();
    m_MaterialWorkflowPBR->Init("Textures/HDR/greenwich_park_02_1k.hdr");

    m_CurrentTimestamp = 0.0f;

    m_ObjectSelect     = { 0.0f, 0.2f };
    m_ObjectAdd        = { 0.0f, 1.0f };
    m_ObjectCopy       = { 0.0f, 1.0f };
    m_ObjectDelete     = { 0.0f, 1.0f };
    m_SceneSave        = { 0.0f, 1.0f };
    m_SceneLoad        = { 0.0f, 1.0f };
    m_SceneReset       = { 0.0f, 1.0f };
    m_ProjectionChange = { 0.0f, 0.5f };

    m_OrthographicViewEnabled = false;

    m_SelectedIndex = 0;

    m_ActionAddType = 0;
    m_CurrentMeshTypeID = MESH_TYPE_CUBE;
    m_CurrentModelID = 0;

    m_Raycast = new Raycast();
    m_Raycast->m_Color = { 1.0f, 0.0f, 1.0f, 1.0f };

    m_Grid = new Grid(20);

    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f));

    m_Gizmo = new Gizmo();

    m_PositionEdit             = new glm::vec3(0.0f);
    m_RotationEdit             = new glm::vec3(0.0f);
    m_ScaleEdit                = new glm::vec3(1.0f);
    m_ColorEdit                = new glm::vec4(1.0f);
    m_UseTextureEdit           = new bool(false);
    m_TextureNameEdit          = new std::string;
    m_TilingFactorEdit         = new float(1.0f);
    m_MaterialNameEdit         = new std::string;
    m_TilingFactorMaterialEdit = new float(1.0f);
    m_DrawScenePivot = true;
    m_PBR_Map_Edit = 0;

    // required for directional light enable/disable feature
    m_DirLightEnabledPrev = sceneSettings.directionalLight.base.enabled;
    m_DirLightColorPrev = sceneSettings.directionalLight.base.color;

    m_DisplayLightSources = true;

    m_CurrentSkyboxInt = SKYBOX_DAY;

    m_MouseButton_1_Prev = false;

    m_UseCubeMaps = false;

    TextureLoader::Get()->Print();
}

void SceneEditor::SetSkybox()
{
    m_SkyboxFacesDay.push_back("Textures/skybox_4/right.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/left.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/top.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/bottom.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/back.png");
    m_SkyboxFacesDay.push_back("Textures/skybox_4/front.png");

    m_SkyboxFacesNight.push_back("Textures/skybox_2/right.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/left.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/top.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/bottom.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/back.png");
    m_SkyboxFacesNight.push_back("Textures/skybox_2/front.png");

    m_SkyboxDay   = new Skybox(m_SkyboxFacesDay);
    m_SkyboxNight = new Skybox(m_SkyboxFacesNight);
    m_Skybox = m_SkyboxDay;

    m_TextureCubeMapDay   = new TextureCubeMap(m_SkyboxFacesDay);
    m_TextureCubeMapNight = new TextureCubeMap(m_SkyboxFacesNight);
    m_TextureCubeMap = m_TextureCubeMapDay;
}

void SceneEditor::SetTextures()
{
    textures.insert(std::make_pair("plain",            TextureLoader::Get()->GetTexture("Textures/plain.png")));
    textures.insert(std::make_pair("semi_transparent", TextureLoader::Get()->GetTexture("Textures/semi_transparent.png")));
    textures.insert(std::make_pair("texture_checker",  TextureLoader::Get()->GetTexture("Textures/texture_checker.png")));
    textures.insert(std::make_pair("wood",             TextureLoader::Get()->GetTexture("Textures/wood.png")));
    textures.insert(std::make_pair("plank",            TextureLoader::Get()->GetTexture("Textures/texture_plank.png")));
    textures.insert(std::make_pair("rock",             TextureLoader::Get()->GetTexture("Textures/rock.png")));
    textures.insert(std::make_pair("pyramid",          TextureLoader::Get()->GetTexture("Textures/pyramid.png")));
    textures.insert(std::make_pair("lego",             TextureLoader::Get()->GetTexture("Textures/lego.png")));
    textures.insert(std::make_pair("marble",           TextureLoader::Get()->GetTexture("Textures/marble.jpg")));
    textures.insert(std::make_pair("metal",            TextureLoader::Get()->GetTexture("Textures/metal.png")));
    textures.insert(std::make_pair("brick",            TextureLoader::Get()->GetTexture("Textures/brick.png")));
    textures.insert(std::make_pair("crate",            TextureLoader::Get()->GetTexture("Textures/crate.png")));
    textures.insert(std::make_pair("grass",            TextureLoader::Get()->GetTexture("Textures/grass.jpg")));
    textures.insert(std::make_pair("water",            TextureLoader::Get()->GetTexture("Textures/water.png")));
    textures.insert(std::make_pair("rock2",            TextureLoader::Get()->GetTexture("Textures/rock/Rock-Texture-Surface.jpg")));
    textures.insert(std::make_pair("planet",           TextureLoader::Get()->GetTexture("Textures/planet/planet_Quom1200.png")));
    textures.insert(std::make_pair("gold_albedo",      TextureLoader::Get()->GetTexture("Textures/PBR/gold/albedo.png")));
    textures.insert(std::make_pair("silver_albedo",    TextureLoader::Get()->GetTexture("Textures/PBR/silver/albedo.png")));
    textures.insert(std::make_pair("rusted_iron",      TextureLoader::Get()->GetTexture("Textures/PBR/rusted_iron/albedo.png")));
    textures.insert(std::make_pair("grass_albedo",     TextureLoader::Get()->GetTexture("Textures/PBR/grass/albedo.png")));
    textures.insert(std::make_pair("wall_albedo",      TextureLoader::Get()->GetTexture("Textures/PBR/wall/albedo.png")));
    textures.insert(std::make_pair("plastic_albedo",   TextureLoader::Get()->GetTexture("Textures/PBR/plastic/albedo.png")));

    // PBR/IBL - gold material
    textures.insert(std::make_pair("goldAlbedoMap",    TextureLoader::Get()->GetTexture("Textures/PBR/gold/albedo.png")));
    textures.insert(std::make_pair("goldNormalMap",    TextureLoader::Get()->GetTexture("Textures/PBR/gold/normal.png")));
    textures.insert(std::make_pair("goldMetallicMap",  TextureLoader::Get()->GetTexture("Textures/PBR/gold/metallic.png")));
    textures.insert(std::make_pair("goldRoughnessMap", TextureLoader::Get()->GetTexture("Textures/PBR/gold/roughness.png")));
    textures.insert(std::make_pair("goldAOMap",        TextureLoader::Get()->GetTexture("Textures/PBR/gold/ao.png")));
}

void SceneEditor::SetupMaterials()
{
    // none (placeholder)
    TextureInfo textureInfoNone = {};
    textureInfoNone.albedo    = "Textures/plain.png";
    textureInfoNone.normal    = "Textures/normal_map_default.png";
    textureInfoNone.metallic  = "Textures/plain.png";
    textureInfoNone.roughness = "Textures/plain.png";
    textureInfoNone.ao        = "Textures/plain.png";
    materials.insert(std::make_pair("none", new Material(textureInfoNone, m_MaterialSpecular, m_MaterialShininess)));

    // gold
    TextureInfo textureInfoGold     = {};
    textureInfoGold.albedo          = "Textures/PBR/gold/albedo.png";
    textureInfoGold.normal          = "Textures/PBR/gold/normal.png";
    textureInfoGold.metallic        = "Textures/PBR/gold/metallic.png";
    textureInfoGold.roughness       = "Textures/PBR/gold/roughness.png";
    textureInfoGold.ao              = "Textures/PBR/gold/ao.png";
    materials.insert(std::make_pair("gold", new Material(textureInfoGold, m_MaterialSpecular, m_MaterialShininess)));

    // silver
    TextureInfo textureInfoSilver   = {};
    textureInfoSilver.albedo        = "Textures/PBR/silver/albedo.png";
    textureInfoSilver.normal        = "Textures/PBR/silver/normal.png";
    textureInfoSilver.metallic      = "Textures/PBR/silver/metallic.png";
    textureInfoSilver.roughness     = "Textures/PBR/silver/roughness.png";
    textureInfoSilver.ao            = "Textures/PBR/silver/ao.png";
    materials.insert(std::make_pair("silver", new Material(textureInfoSilver, m_MaterialSpecular, m_MaterialShininess)));

    // rusted iron
    TextureInfo textureInfoRustedIron = {};
    textureInfoRustedIron.albedo    = "Textures/PBR/rusted_iron/albedo.png";
    textureInfoRustedIron.normal    = "Textures/PBR/rusted_iron/normal.png";
    textureInfoRustedIron.metallic  = "Textures/PBR/rusted_iron/metallic.png";
    textureInfoRustedIron.roughness = "Textures/PBR/rusted_iron/roughness.png";
    textureInfoRustedIron.ao        = "Textures/PBR/rusted_iron/ao.png";
    materials.insert(std::make_pair("rusted_iron", new Material(textureInfoRustedIron, m_MaterialSpecular, m_MaterialShininess)));

    // plastic
    TextureInfo textureInfoPlastic  = {};
    textureInfoPlastic.albedo       = "Textures/PBR/plastic/albedo.png";
    textureInfoPlastic.normal       = "Textures/PBR/plastic/normal.png";
    textureInfoPlastic.metallic     = "Textures/PBR/plastic/metallic.png";
    textureInfoPlastic.roughness    = "Textures/PBR/plastic/roughness.png";
    textureInfoPlastic.ao           = "Textures/PBR/plastic/ao.png";
    materials.insert(std::make_pair("plastic", new Material(textureInfoPlastic, m_MaterialSpecular, m_MaterialShininess)));

    // futuristic_panel_1
    TextureInfo textureInfoFuturPanel = {};
    textureInfoFuturPanel.albedo    = "Textures/PBR/futuristic_panel_1/futuristic-panels1-albedo.png";
    textureInfoFuturPanel.normal    = "Textures/PBR/futuristic_panel_1/futuristic-panels1-normal-dx.png";
    textureInfoFuturPanel.metallic  = "Textures/PBR/futuristic_panel_1/futuristic-panels1-metallic.png";
    textureInfoFuturPanel.roughness = "Textures/PBR/futuristic_panel_1/futuristic-panels1-roughness.png";
    textureInfoFuturPanel.ao        = "Textures/PBR/futuristic_panel_1/futuristic-panels1-ao.png";
    materials.insert(std::make_pair("futur_panel_1", new Material(textureInfoFuturPanel, m_MaterialSpecular, m_MaterialShininess)));

    // dark tiles
    TextureInfo textureInfoDarkTiles = {};
    textureInfoDarkTiles.albedo    = "Textures/PBR/dark_tiles_1/darktiles1_basecolor.png";
    textureInfoDarkTiles.normal    = "Textures/PBR/dark_tiles_1/darktiles1_normal-DX.png";
    textureInfoDarkTiles.metallic  = "Textures/PBR/metalness.png";
    textureInfoDarkTiles.roughness = "Textures/PBR/dark_tiles_1/darktiles1_roughness.png";
    textureInfoDarkTiles.ao        = "Textures/PBR/dark_tiles_1/darktiles1_AO.png";
    materials.insert(std::make_pair("dark_tiles", new Material(textureInfoDarkTiles, m_MaterialSpecular, m_MaterialShininess)));

    // mahogany floor
    TextureInfo textureInfoMahoganyFloor = {};
    textureInfoMahoganyFloor.albedo    = "Textures/PBR/mahogany_floor/mahogfloor_basecolor.png";
    textureInfoMahoganyFloor.normal    = "Textures/PBR/mahogany_floor/mahogfloor_normal.png";
    textureInfoMahoganyFloor.metallic  = "Textures/PBR/mahogany_floor/mahogfloor_metalness.png";
    textureInfoMahoganyFloor.roughness = "Textures/PBR/mahogany_floor/mahogfloor_roughness.png";
    textureInfoMahoganyFloor.ao        = "Textures/PBR/mahogany_floor/mahogfloor_AO.png";
    materials.insert(std::make_pair("mahogany_floor", new Material(textureInfoMahoganyFloor, m_MaterialSpecular, m_MaterialShininess)));

    // aged planks
    TextureInfo textureInfoAgedPlanks = {};
    textureInfoAgedPlanks.albedo    = "Textures/PBR/aged_planks_1/agedplanks1-albedo.png";
    textureInfoAgedPlanks.normal    = "Textures/PBR/aged_planks_1/agedplanks1-normal4-ue.png";
    textureInfoAgedPlanks.metallic  = "Textures/PBR/aged_planks_1/agedplanks1-metalness.png";
    textureInfoAgedPlanks.roughness = "Textures/PBR/aged_planks_1/agedplanks1-roughness.png";
    textureInfoAgedPlanks.ao        = "Textures/PBR/aged_planks_1/agedplanks1-ao.png";
    materials.insert(std::make_pair("aged_planks", new Material(textureInfoAgedPlanks, m_MaterialSpecular, m_MaterialShininess)));

    // harsh bricks
    TextureInfo textureInfoHarshBricks = {};
    textureInfoHarshBricks.albedo    = "Textures/PBR/harsh_bricks/harshbricks-albedo.png";
    textureInfoHarshBricks.normal    = "Textures/PBR/harsh_bricks/harshbricks-normal.png";
    textureInfoHarshBricks.metallic  = "Textures/PBR/metalness.png";
    textureInfoHarshBricks.roughness = "Textures/PBR/harsh_bricks/harshbricks-roughness.png";
    textureInfoHarshBricks.ao        = "Textures/PBR/harsh_bricks/harshbricks-ao2.png";
    materials.insert(std::make_pair("harsh_bricks", new Material(textureInfoHarshBricks, m_MaterialSpecular, m_MaterialShininess)));

    // Stone Carved (Quixel Megascans)
    TextureInfo textureInfoStoneCarved = {};
    textureInfoStoneCarved.albedo    = "Textures/PBR/Stone_Carved/Albedo.jpg";
    textureInfoStoneCarved.normal    = "Textures/PBR/Stone_Carved/Normal_LOD0.jpg";
    textureInfoStoneCarved.metallic  = "Textures/PBR/Stone_Carved/Metalness.jpg";
    textureInfoStoneCarved.roughness = "Textures/PBR/Stone_Carved/Roughness.jpg";
    textureInfoStoneCarved.ao        = "Textures/PBR/Stone_Carved/Displacement.jpg";
    materials.insert(std::make_pair("stone_carved", new Material(textureInfoStoneCarved, m_MaterialSpecular, m_MaterialShininess)));

    // Old Stove (Quixel Megascans)
    TextureInfo textureInfoOldStove = {};
    textureInfoOldStove.albedo    = "Textures/PBR/Old_Stove/Albedo.jpg";
    textureInfoOldStove.normal    = "Textures/PBR/Old_Stove/Normal_LOD0.jpg";
    textureInfoOldStove.metallic  = "Textures/PBR/Old_Stove/Metalness.jpg";
    textureInfoOldStove.roughness = "Textures/PBR/Old_Stove/Roughness.jpg";
    textureInfoOldStove.ao        = "Textures/PBR/Old_Stove/Displacement.jpg";
    materials.insert(std::make_pair("old_stove", new Material(textureInfoOldStove, m_MaterialSpecular, m_MaterialShininess)));
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

void SceneEditor::Update(float timestep, Window& mainWindow)
{
    m_CurrentTimestamp = timestep;

    MousePicker::Get()->GetPointOnRay(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

    // printf("SceneEditor::Update m_SceneObjects.size = %zu\n", m_SceneObjects.size());

    for (auto& object : m_SceneObjects) {
        object->isSelected = AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(),
            object->AABB->GetMin(), object->AABB->GetMax(), glm::vec2(0.0f));

        if (object->objectType == "mesh" && object->mesh != nullptr)
            object->mesh->Update(object->scale);
        else if (object->objectType == "model" && object->model != nullptr)
            object->model->Update(object->scale);
    }

    if (m_CurrentSkyboxInt == SKYBOX_DAY) {
        m_Skybox = m_SkyboxDay;
        m_TextureCubeMap = m_TextureCubeMapDay;
    }
    else if (m_CurrentSkyboxInt == SKYBOX_NIGHT) {
        m_Skybox = m_SkyboxNight;
        m_TextureCubeMap = m_TextureCubeMapNight;
    }

    m_Gizmo->Update(m_Camera->GetPosition(), mainWindow);

    // Switching between scene objects that are currently in focus (mouse over)
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        m_Gizmo->OnMousePress(mainWindow, &m_SceneObjects, m_SelectedIndex);
        m_MouseButton_1_Prev = true;
    }

    if (!mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1] && m_MouseButton_1_Prev)
    {
        SelectNextFromMultipleObjects(&m_SceneObjects, m_SelectedIndex);
        m_Gizmo->OnMouseRelease(mainWindow, &m_SceneObjects, m_SelectedIndex);
        m_MouseButton_1_Prev = false;
    }

    // Add new scene object with default settings
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1] && mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL])
    {
        AddSceneObject();
    }

    // Copy selected scene object
    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_C])
    {
        CopySceneObject(mainWindow, &m_SceneObjects, m_SelectedIndex);
        m_SceneObjects[m_SelectedIndex]->isSelected = false;
    }

    // Delete selected object
    if (mainWindow.getKeys()[GLFW_KEY_DELETE])
    {
        DeleteSceneObject(mainWindow, &m_SceneObjects, m_SelectedIndex);
    }

    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_R])
        ResetScene();

    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_S])
        SaveScene();

    if (mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL] && mainWindow.getKeys()[GLFW_KEY_L])
        LoadScene();

    // Gizmo switching modes
    if (mainWindow.getKeys()[GLFW_KEY_1])
        m_Gizmo->ChangeMode(GIZMO_MODE_TRANSLATE);

    if (mainWindow.getKeys()[GLFW_KEY_2])
        m_Gizmo->ChangeMode(GIZMO_MODE_SCALE);

    if (mainWindow.getKeys()[GLFW_KEY_3])
        m_Gizmo->ChangeMode(GIZMO_MODE_ROTATE);

    if (mainWindow.getKeys()[GLFW_KEY_4])
        m_Gizmo->ChangeMode(GIZMO_MODE_NONE);
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

    printf("SceneEditor::SaveScene: Saving %zu objects!\n", m_SceneObjects.size());

    std::vector<std::string> lines;
    for (int i = 0; i < m_SceneObjects.size(); i++)
    {
        lines.push_back("BeginObject\t" + std::to_string(i));
        lines.push_back("Position\t" +
            std::to_string(m_SceneObjects[i]->position.x) + "\t" +
            std::to_string(m_SceneObjects[i]->position.y) + "\t" +
            std::to_string(m_SceneObjects[i]->position.z));
        lines.push_back("Rotation\t" +
            std::to_string(m_SceneObjects[i]->rotation.x) + "\t" +
            std::to_string(m_SceneObjects[i]->rotation.y) + "\t" +
            std::to_string(m_SceneObjects[i]->rotation.z));
        lines.push_back("Scale\t" +
            std::to_string(m_SceneObjects[i]->scale.x) + "\t" +
            std::to_string(m_SceneObjects[i]->scale.y) + "\t" +
            std::to_string(m_SceneObjects[i]->scale.z));
        lines.push_back("Color\t" +
            std::to_string(m_SceneObjects[i]->color.r) + "\t" +
            std::to_string(m_SceneObjects[i]->color.g) + "\t" +
            std::to_string(m_SceneObjects[i]->color.b) + "\t" +
            std::to_string(m_SceneObjects[i]->color.a));
        std::string useTexture = m_SceneObjects[i]->useTexture ? "1" : "0";
        lines.push_back("UseTexture\t" + useTexture);
        lines.push_back("TextureName\t" + m_SceneObjects[i]->textureName);
        lines.push_back("TilingFactor\t" + std::to_string(m_SceneObjects[i]->tilingFactor));
        std::string isSelected = m_SceneObjects[i]->isSelected ? "1" : "0";
        lines.push_back("IsSelected\t" + isSelected);
        lines.push_back("ObjectType\t" + m_SceneObjects[i]->objectType);
        lines.push_back("MeshType\t" + std::to_string(m_SceneObjects[i]->meshType));
        lines.push_back("MaterialName\t" + m_SceneObjects[i]->materialName);
        lines.push_back("TilingFactorMaterial\t" + std::to_string(m_SceneObjects[i]->tilingFactorMaterial));
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

    printf("SceneEditor::SaveScene: Content saved to file '%s'\n", m_SceneFilename);
}

void SceneEditor::LoadScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneLoad.lastTime < m_SceneLoad.cooldown) return;
    m_SceneLoad.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::LoadScene: Loading objects...\n");

    std::string sceneFileContent = Shader::ReadFile(m_SceneFilename);

    // printf(sceneFileContent.c_str());

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

        // for (auto& token : tokens)
        //     printf("%s\n", token.c_str());
        
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
            sceneObject->rotation = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Rotation %.2ff %.2ff %.2ff\n", sceneObject.rotation.x, sceneObject.rotation.y, sceneObject.rotation.z);
        }
        else if (tokens.size() >= 4 && tokens[0] == "Scale") {
            sceneObject->scale = glm::vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            // printf("Scale %.2ff %.2ff %.2ff\n", sceneObject.scale.x, sceneObject.scale.y, sceneObject.scale.z);
        }
        else if (tokens.size() >= 5 && tokens[0] == "Color") {
            sceneObject->color = glm::vec4(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]), std::stof(tokens[4]));
            // printf("Color %.2ff %.2ff %.2ff %.2ff\n", sceneObject.color.r, sceneObject.color.g, sceneObject.color.b, sceneObject.color.a);
        }
        else if (tokens.size() >= 2 && tokens[0] == "UseTexture") {
            sceneObject->useTexture = std::stoi(tokens[1]) == 1 ? true : false;
            // printf("UseTexture %d\n", sceneObject.useTexture);
        }
        else if (tokens.size() >= 2 && tokens[0] == "TextureName") {
            sceneObject->textureName = tokens[1];
            // printf("UseTexture %s\n", sceneObject.textureName.c_str());
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
        else if (tokens.size() >= 2 && tokens[0] == "ObjectType") {
            sceneObject->objectType = tokens[1];
            // printf("UseTexture %s\n", sceneObject.textureName.c_str());
        }
        else if (tokens.size() >= 2 && tokens[0] == "MeshType") {
            sceneObject->meshType = std::stoi(tokens[1]);
            // printf("MeshType %d\n", sceneObject.meshType);
        }
        else if (tokens.size() >= 2 && tokens[0] == "MaterialName") {
            sceneObject->materialName = tokens[1];
            // printf("UseTexture %s\n", sceneObject.textureName.c_str());
        }
        else if (tokens.size() >= 2 && tokens[0] == "TilingFactorMaterial") {
            sceneObject->tilingFactorMaterial = std::stof(tokens[1]);
            // printf("UseTexture %s\n", sceneObject.textureName.c_str());
        }
        else if (tokens.size() >= 1 && tokens[0] == "EndObject") {
            sceneObject->id = (int)m_SceneObjects.size();
            sceneObject->transform = Math::CreateTransform(sceneObject->position, sceneObject->rotation, sceneObject->scale);
            sceneObject->AABB  = new AABB(sceneObject->position, sceneObject->rotation, sceneObject->scale);
            sceneObject->pivot = new Pivot(sceneObject->position, sceneObject->scale);
            Mesh* mesh = nullptr;
            Model* model = nullptr;
            if (sceneObject->objectType == "mesh") {
                sceneObject->mesh  = CreateNewPrimitive(sceneObject->meshType, sceneObject->scale);
            }
            else if (sceneObject->objectType == "model")
                sceneObject->model = AddNewModel(m_CurrentModelID, glm::vec3(1.0f));
            m_SceneObjects.push_back(sceneObject);
            // printf("EndObject: New SceneObject added to m_SceneObjects...\n");
        }
    }

    if (m_SceneObjects.size() > 0)
        m_Gizmo->SetSceneObject(m_SceneObjects[m_SceneObjects.size() - 1]);
}

void SceneEditor::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{

#if 0

    // DockSpace
    static ImGuiDockNodeFlags dockspace_flags =
        ImGuiDockNodeFlags_PassthruCentralNode |
        ImGuiDockNodeFlags_NoDockingInCentralNode;

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        // ImGui::DockSpace(dockspace_id, ImVec2((float)mainWindow.GetBufferWidth(), (float)mainWindow.GetBufferWidth()), dockspace_flags);

        ImGui::DockSpaceOverViewport();

        ImGui::Begin("Mouse Picker Info");

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
        ImGui::End();

        ImGui::Begin("Left");
        ImGui::Text("Left");
        ImGui::End();
        ImGui::Begin("Right");
        ImGui::Text("Right");
        ImGui::End();
        ImGui::Begin("Top");
        ImGui::Text("Top");
        ImGui::End();
        ImGui::Begin("Bottom");
        ImGui::Text("Bottom");
        ImGui::End();

        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("Center");
        ImGui::Text("Center");
        ImGui::End();
    }
    else
    {
        printf("ERROR: ImGui Docking feature is disabled!\n");
    }

#endif

    MousePicker* mp = MousePicker::Get();

    if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
    {
        m_PositionEdit = &m_SceneObjects[m_SelectedIndex]->position;
        m_RotationEdit = &m_SceneObjects[m_SelectedIndex]->rotation;
        m_ScaleEdit = &m_SceneObjects[m_SelectedIndex]->scale;
        m_ColorEdit = &m_SceneObjects[m_SelectedIndex]->color;
        m_UseTextureEdit = &m_SceneObjects[m_SelectedIndex]->useTexture;
        m_TextureNameEdit = &m_SceneObjects[m_SelectedIndex]->textureName;
        m_TilingFactorEdit = &m_SceneObjects[m_SelectedIndex]->tilingFactor;
        m_MaterialNameEdit = &m_SceneObjects[m_SelectedIndex]->materialName;
        m_TilingFactorMaterialEdit = &m_SceneObjects[m_SelectedIndex]->tilingFactorMaterial;
    }

    ImGui::Begin("Transform");
    ImGui::SliderFloat3("Position", (float*)m_PositionEdit, -10.0f, 10.0f);
    ImGui::SliderFloat3("Rotation", (float*)m_RotationEdit, -179.0f, 180.0f);
    ImGui::SliderFloat3("Scale", (float*)m_ScaleEdit, 0.1f, 20.0f);
    ImGui::ColorEdit4("Color", (float*)m_ColorEdit);
    ImGui::Checkbox("Use Texture", m_UseTextureEdit);

    // Begin TextureName ImGui drop-down list
    std::vector<const char*> itemsTexture;
    std::map<std::string, Texture*>::iterator itTexture;
    for (itTexture = textures.begin(); itTexture != textures.end(); itTexture++)
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
    std::map<std::string, Material*>::iterator itMaterial;
    for (itMaterial = materials.begin(); itMaterial != materials.end(); itMaterial++)
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

    ImGui::SliderInt("Selected Object", (int*)&m_SelectedIndex, 0, (int)(m_SceneObjects.size() - 1));

    bool gizmoActive = m_Gizmo->GetActive();
    int sceneObjectCount = (int)m_SceneObjects.size();
    Bool3 axesEnabled = m_Gizmo->GetAxesEnabled();

    ImGui::Separator();
    ImGui::Text("Cube Maps");
    ImGui::Checkbox("Use Cube Maps", &m_UseCubeMaps);
    ImGui::RadioButton("Environment Map", &m_PBR_Map_Edit, PBR_MAP_ENVIRONMENT);
    ImGui::RadioButton("Irradiance Map", &m_PBR_Map_Edit, PBR_MAP_IRRADIANCE);
    ImGui::RadioButton("Prefilter Map", &m_PBR_Map_Edit, PBR_MAP_PREFILTER);

    ImGui::Checkbox("Draw Scene Pivot", &m_DrawScenePivot);
    ImGui::Checkbox("Orthographic View", &m_OrthographicViewEnabled);

    ImGui::Separator();
    ImGui::Text("Transform Gizmo");
    ImGui::SliderInt("Scene Objects Count", &sceneObjectCount, 0, 100);
    ImGui::Checkbox("Gizmo Active", &gizmoActive);
    ImGui::Text("Axes Enabled");
    ImGui::Checkbox("Axis X", &axesEnabled.x);
    ImGui::Checkbox("Axis Y", &axesEnabled.y);
    ImGui::Checkbox("Axis Z", &axesEnabled.z);

    ImGui::Separator();
    ImGui::Text("Select Add Action Mode");
    ImGui::RadioButton("Add Mesh Primitive", &m_ActionAddType, ACTION_ADD_MESH);
    ImGui::RadioButton("Add Model", &m_ActionAddType, ACTION_ADD_MODEL);

    ImGui::Separator();
    ImGui::Text("Select Object Type");
    ImGui::RadioButton("Cube", &m_CurrentMeshTypeID, MESH_TYPE_CUBE);
    ImGui::RadioButton("Pyramid", &m_CurrentMeshTypeID, MESH_TYPE_PYRAMID);
    ImGui::RadioButton("Sphere", &m_CurrentMeshTypeID, MESH_TYPE_SPHERE);
    ImGui::RadioButton("Cylinder", &m_CurrentMeshTypeID, MESH_TYPE_CYLINDER);
    ImGui::RadioButton("Cone", &m_CurrentMeshTypeID, MESH_TYPE_CONE);
    ImGui::RadioButton("Ring", &m_CurrentMeshTypeID, MESH_TYPE_RING);

    ImGui::Separator();
    ImGui::Text("Select Model");
    ImGui::RadioButton("Stone Carved", &m_CurrentModelID, MODEL_STONE_CARVED);
    ImGui::RadioButton("Old Stove", &m_CurrentModelID, MODEL_OLD_STOVE);

    ImGui::Separator();
    ImGui::Text("Select Skybox");
    ImGui::RadioButton("Day", &m_CurrentSkyboxInt, SKYBOX_DAY);
    ImGui::RadioButton("Night", &m_CurrentSkyboxInt, SKYBOX_NIGHT);

    ImGui::Separator();
    float FOV = GetFOV();
    ImGui::SliderFloat("FOV", &FOV, 1.0f, 120.0f);
    SetFOV(FOV);

    ImGui::Separator();
    ImGui::Text("Timer");

    float realFPS = Timer::Get()->GetRealFPS();
    std::string sRealFPS = "Real FPS: " + std::to_string(realFPS);

    float deltaTimeMS = Timer::Get()->GetDeltaTime() * 1000.0f;
    std::string sDeltaTimeMS = "Delta Time: " + std::to_string(deltaTimeMS) + " ms";

    ImGui::Text(sRealFPS.c_str());
    ImGui::Text(sDeltaTimeMS.c_str());

    ImGui::Separator();
    ImGui::Text("Lights");
    ImGui::Checkbox("Display Light Sources", &m_DisplayLightSources);

    SDirectionalLight directionalLight;

    directionalLight.base.enabled = m_LightManager->directionalLight.GetEnabled();
    directionalLight.base.color = m_LightManager->directionalLight.GetColor();
    directionalLight.base.ambientIntensity = m_LightManager->directionalLight.GetAmbientIntensity();
    directionalLight.base.diffuseIntensity = m_LightManager->directionalLight.GetDiffuseIntensity();
    directionalLight.direction = m_LightManager->directionalLight.GetDirection();

    ImGui::Separator();
    ImGui::Text("Directional Light");
    ImGui::Checkbox("DL Enabled", &directionalLight.base.enabled);
    ImGui::ColorEdit3("DL Color", glm::value_ptr(directionalLight.base.color));
    ImGui::SliderFloat3("DL Direction", glm::value_ptr(directionalLight.direction), -1.0f, 1.0f);
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

    m_LightManager->directionalLight.SetEnabled(directionalLight.base.enabled);
    m_LightManager->directionalLight.SetColor(directionalLight.base.color);
    m_LightManager->directionalLight.SetAmbientIntensity(directionalLight.base.ambientIntensity);
    m_LightManager->directionalLight.SetDiffuseIntensity(directionalLight.base.diffuseIntensity);
    m_LightManager->directionalLight.SetDirection(directionalLight.direction);

    // Point Lights
    ImGui::Separator();
    ImGui::Text("Point Lights");
    ImGui::Separator();

    SPointLight pointLights[4];
    char locBuff[100] = { '\0' };
    for (unsigned int pl = 0; pl < m_LightManager->pointLightCount; pl++)
    {
        pointLights[pl].base.enabled = m_LightManager->pointLights[pl].GetEnabled();
        pointLights[pl].base.color = m_LightManager->pointLights[pl].GetColor();
        pointLights[pl].base.ambientIntensity = m_LightManager->pointLights[pl].GetAmbientIntensity();
        pointLights[pl].base.diffuseIntensity = m_LightManager->pointLights[pl].GetDiffuseIntensity();
        pointLights[pl].position = m_LightManager->pointLights[pl].GetPosition();
        pointLights[pl].constant = m_LightManager->pointLights[pl].GetConstant();
        pointLights[pl].linear = m_LightManager->pointLights[pl].GetLinear();
        pointLights[pl].exponent = m_LightManager->pointLights[pl].GetExponent();

        snprintf(locBuff, sizeof(locBuff), "Point Light %i", pl);
        ImGui::Text(locBuff);

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

        m_LightManager->pointLights[pl].SetEnabled(pointLights[pl].base.enabled);
        m_LightManager->pointLights[pl].SetColor(pointLights[pl].base.color);
        m_LightManager->pointLights[pl].SetAmbientIntensity(pointLights[pl].base.ambientIntensity);
        m_LightManager->pointLights[pl].SetDiffuseIntensity(pointLights[pl].base.diffuseIntensity);
        m_LightManager->pointLights[pl].SetPosition(pointLights[pl].position);
        m_LightManager->pointLights[pl].SetConstant(pointLights[pl].constant);
        m_LightManager->pointLights[pl].SetLinear(pointLights[pl].linear);
        m_LightManager->pointLights[pl].SetExponent(pointLights[pl].exponent);

        ImGui::Separator();
    }

    ImGui::Separator();
    ImGui::Text("Spot Lights");
    ImGui::Separator();

    SSpotLight spotLights[4];

    for (unsigned int sl = 0; sl < m_LightManager->spotLightCount; sl++)
    {
        spotLights[sl].base.base.enabled = m_LightManager->spotLights[sl].GetBasePL()->GetEnabled();
        spotLights[sl].base.base.color = m_LightManager->spotLights[sl].GetBasePL()->GetColor();
        spotLights[sl].base.base.ambientIntensity = m_LightManager->spotLights[sl].GetBasePL()->GetAmbientIntensity();
        spotLights[sl].base.base.diffuseIntensity = m_LightManager->spotLights[sl].GetBasePL()->GetDiffuseIntensity();
        spotLights[sl].base.position = m_LightManager->spotLights[sl].GetBasePL()->GetPosition();
        spotLights[sl].base.constant = m_LightManager->spotLights[sl].GetBasePL()->GetConstant();
        spotLights[sl].base.linear = m_LightManager->spotLights[sl].GetBasePL()->GetLinear();
        spotLights[sl].base.exponent = m_LightManager->spotLights[sl].GetBasePL()->GetExponent();
        spotLights[sl].direction = m_LightManager->spotLights[sl].GetDirection();
        spotLights[sl].edge = m_LightManager->spotLights[sl].GetEdge();

        snprintf(locBuff, sizeof(locBuff), "Spot Light %i", sl);
        ImGui::Text(locBuff);

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

        m_LightManager->spotLights[sl].GetBasePL()->SetEnabled(spotLights[sl].base.base.enabled);
        m_LightManager->spotLights[sl].GetBasePL()->SetColor(spotLights[sl].base.base.color);
        m_LightManager->spotLights[sl].GetBasePL()->SetAmbientIntensity(spotLights[sl].base.base.ambientIntensity);
        m_LightManager->spotLights[sl].GetBasePL()->SetDiffuseIntensity(spotLights[sl].base.base.diffuseIntensity);
        m_LightManager->spotLights[sl].GetBasePL()->SetPosition(spotLights[sl].base.position);
        m_LightManager->spotLights[sl].GetBasePL()->SetConstant(spotLights[sl].base.constant);
        m_LightManager->spotLights[sl].GetBasePL()->SetLinear(spotLights[sl].base.linear);
        m_LightManager->spotLights[sl].GetBasePL()->SetExponent(spotLights[sl].base.exponent);
        m_LightManager->spotLights[sl].SetDirection(spotLights[sl].direction);
        m_LightManager->spotLights[sl].SetEdge(spotLights[sl].edge);

        ImGui::Separator();
    }
    ImGui::End();
}

SceneObject* SceneEditor::CreateNewSceneObject()
{
    // Add Scene Object here
    SceneObject* sceneObject = new SceneObject{
        (int)m_SceneObjects.size(),
        glm::mat4(1.0f),
        defaultSpawnPosition,
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec4(1.0f),
        false,
        "plain",
        1.0f,
        true,
        new AABB(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)),
        new Pivot(glm::vec3(0.0f), glm::vec3(1.0f)),
        "",      // Object Type
        nullptr, // Mesh
        0,
        nullptr, // Model
        "",
        1.0f,
    };

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
    std::string objectType = "";
    std::string materialName = "";

    if (m_ActionAddType == ACTION_ADD_MESH) {
        mesh = CreateNewPrimitive(m_CurrentMeshTypeID, glm::vec3(1.0f));
        objectType = "mesh";
    }
    else if (m_ActionAddType == ACTION_ADD_MODEL) {
        model = AddNewModel(m_CurrentModelID, glm::vec3(1.0f));
        objectType = "model";
        if (m_CurrentModelID == MODEL_STONE_CARVED)
            materialName = "stone_carved";
        else if (m_CurrentModelID == MODEL_OLD_STOVE)
            materialName = "old_stove";
    }

    // Add Scene Object here
    SceneObject* sceneObject = CreateNewSceneObject();
    sceneObject->objectType = objectType;
    sceneObject->mesh = mesh;
    sceneObject->meshType = m_CurrentMeshTypeID;
    sceneObject->model = model;
    sceneObject->materialName = materialName;

    m_SceneObjects.push_back(sceneObject);
    m_SelectedIndex = (unsigned int)m_SceneObjects.size() - 1;
}

void SceneEditor::CopySceneObject(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectCopy.lastTime < m_ObjectCopy.cooldown) return;
    m_ObjectCopy.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::CopySceneObject sceneObjects = %zu selectedIndex = %i\n", sceneObjects->size(), selectedIndex);

    SceneObject* oldSceneObject = nullptr;

    if (selectedIndex < (unsigned int)sceneObjects->size())
        oldSceneObject = sceneObjects->at(selectedIndex);

    if (oldSceneObject == nullptr) return;

    Mesh* mesh = nullptr;
    Model* model = nullptr;

    if (oldSceneObject->objectType == "mesh" && oldSceneObject->mesh != nullptr) {
        mesh = CreateNewPrimitive(oldSceneObject->meshType, oldSceneObject->mesh->GetScale());
    }
    else if (oldSceneObject->objectType == "model" && oldSceneObject->model != nullptr) {
        model = AddNewModel(m_CurrentModelID, oldSceneObject->mesh->GetScale()); // TODO: m_CurrentModelID hard-coded, most be in SceneObject
    }

    SceneObject* newSceneObject = new SceneObject{
        (int)sceneObjects->size(),
        oldSceneObject->transform,
        oldSceneObject->position,
        oldSceneObject->rotation,
        oldSceneObject->scale,
        oldSceneObject->color,
        oldSceneObject->useTexture,
        oldSceneObject->textureName,
        oldSceneObject->tilingFactor,
        true,
        new AABB(oldSceneObject->position, oldSceneObject->rotation, oldSceneObject->scale),
        new Pivot(oldSceneObject->position, oldSceneObject->scale),
        oldSceneObject->objectType,
        mesh,
        m_CurrentMeshTypeID,
        model,
        oldSceneObject->materialName,
        oldSceneObject->tilingFactorMaterial
    };

    sceneObjects->push_back(newSceneObject);

    m_Gizmo->OnMouseRelease(mainWindow, sceneObjects, selectedIndex);
}

void SceneEditor::DeleteSceneObject(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectDelete.lastTime < m_ObjectDelete.cooldown) return;
    m_ObjectDelete.lastTime = m_CurrentTimestamp;

    delete m_SceneObjects[m_SelectedIndex]->AABB;
    delete m_SceneObjects[m_SelectedIndex]->pivot;
    if (m_SceneObjects[m_SelectedIndex]->mesh != nullptr)
        delete m_SceneObjects[m_SelectedIndex]->mesh;
    if (m_SceneObjects[m_SelectedIndex]->model != nullptr)
        delete m_SceneObjects[m_SelectedIndex]->model;

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

Mesh* SceneEditor::CreateNewPrimitive(int meshTypeID, glm::vec3 scale)
{
    Mesh* mesh;
    switch (meshTypeID)
    {
    case MESH_TYPE_CUBE:
        mesh = new Block(scale);
        break;
    case MESH_TYPE_PYRAMID:
        mesh = new Pyramid(scale);
        break;
    case MESH_TYPE_SPHERE:
        mesh = new Sphere(scale);
        break;
    case MESH_TYPE_CYLINDER:
        mesh = new Cylinder(scale);
        break;
    case MESH_TYPE_CONE:
        mesh = new Cone(scale);
        break;
    case MESH_TYPE_RING:
        mesh = new Ring(scale);
        break;
    default:
        mesh = new Block(scale);
        break;
    }
    return mesh;
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
    default:
        model = new Model("Models/Stone_Carved/tf3pfhzda_LOD0.fbx");
        break;
    }
    return model;
}

void SceneEditor::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    /**** Begin switch projection/orthographic view ****/
    if (mainWindow.getKeys()[GLFW_KEY_O])
    {
        if (Timer::Get()->GetCurrentTimestamp() - m_ProjectionChange.lastTime > m_ProjectionChange.cooldown)
        {
            m_OrthographicViewEnabled = !m_OrthographicViewEnabled;
            m_ProjectionChange.lastTime = Timer::Get()->GetCurrentTimestamp();
        }
    }

    if (m_OrthographicViewEnabled)
    {
        float left   = -(float)mainWindow.GetBufferWidth()  / 2.0f / m_FOV;
        float right  =  (float)mainWindow.GetBufferWidth()  / 2.0f / m_FOV;
        float bottom = -(float)mainWindow.GetBufferHeight() / 2.0f / m_FOV;
        float top    =  (float)mainWindow.GetBufferHeight() / 2.0f / m_FOV;

        projectionMatrix = glm::ortho(left, right, bottom, top, sceneSettings.nearPlane, sceneSettings.farPlane);
    }
    /**** End switch projection/orthographic view ****/

    Shader* shaderEditor     = shaders["editor_object"];
    Shader* shaderEditorPBR  = shaders["editor_object_pbr"];
    Shader* shaderBasic      = shaders["basic"];
    Shader* shaderBackground = shaders["background"];
    Shader* shaderShadowMap  = shaders["shadow_map"];
    Shader* shaderGizmo      = shaders["gizmo"];

    for (auto& object : m_SceneObjects)
    {
        object->transform = Math::CreateTransform(object->position, object->rotation, glm::vec3(1.0f));

        // For meshes that can't be scaled on vertex level
        if (object->meshType == MESH_TYPE_RING)
            object->transform = glm::scale(object->transform, object->scale);

        // Quixel Megascans models should be downscaled to 2% of their original size
        if (object->objectType == "model") {
            object->transform = glm::scale(object->transform, object->scale);
            object->transform = glm::scale(object->transform, glm::vec3(0.02f));
        }

        shaderEditor->Bind();
        shaderEditor->setMat4("model", object->transform);
        shaderEditor->setMat4("dirLightTransform", m_LightManager->directionalLight.CalculateLightTransform());

        shaderEditorPBR->Bind();
        shaderEditorPBR->setMat4("model", object->transform);
        shaderEditorPBR->setMat4("dirLightTransform", m_LightManager->directionalLight.CalculateLightTransform());

        shaderShadowMap->Bind();
        shaderShadowMap->setMat4("model", object->transform);
        shaderShadowMap->setMat4("dirLightTransform", m_LightManager->directionalLight.CalculateLightTransform());

        if (object->objectType == "mesh" && object->mesh != nullptr)
        {
            if (object->materialName == "" || object->materialName == "none")
            {
                if (passType == "main")
                {
                    // Render with shaderEditor
                    shaderEditor->Bind();
                    shaderEditor->setMat4("model", object->transform);
                    shaderEditor->setVec4("tintColor", object->color);
                    shaderEditor->setBool("isSelected", object->isSelected);
                    
                    if (object->useTexture && object->textureName != "")
                        textures[object->textureName]->Bind(0);
                    else
                        textures["plain"]->Bind(0);
                    
                    shaderEditor->setInt("albedoMap", 0);
                    shaderEditor->setFloat("tilingFactor", object->tilingFactor);
                    
                    // m_TextureCubeMap->Bind(1);
                    glActiveTexture(GL_TEXTURE1);
                    if (m_PBR_Map_Edit == PBR_MAP_ENVIRONMENT)
                        glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetEnvironmentCubemap());
                    else if (m_PBR_Map_Edit == PBR_MAP_IRRADIANCE)
                        glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetIrradianceMap());
                    else if (m_PBR_Map_Edit == PBR_MAP_PREFILTER)
                        glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetPrefilterMap());
                    shaderEditor->setInt("cubeMap", 1);
                    shaderEditor->setBool("useCubeMaps", m_UseCubeMaps);

                    // Shadows in shaderEditor
                    LightManager::directionalLight.GetShadowMap()->Read(2);
                    shaderEditor->setInt("shadowMap", 2);
                }
            }
            else {
                if (passType == "main")
                {
                    // Render with shaderEditorPBR
                    shaderEditorPBR->Bind();
                    shaderEditorPBR->setMat4("model", object->transform);
                    shaderEditorPBR->setVec4("tintColor", object->color);
                    shaderEditorPBR->setBool("isSelected", object->isSelected);
                    shaderEditorPBR->setFloat("tilingFactor", object->tilingFactorMaterial);

                    shaderEditorPBR->setFloat("material.specularIntensity", m_MaterialSpecular); // TODO - use material attribute
                    shaderEditorPBR->setFloat("material.shininess", m_MaterialShininess);        // TODO - use material attribute

                    m_MaterialWorkflowPBR->BindTextures(0);                 // texture slots 0, 1, 2
                    materials[object->materialName]->BindTextures(3);       // texture slots 3, 4, 5, 6, 7

                    // Shadows in shaderEditorPBR
                    LightManager::directionalLight.GetShadowMap()->Read(8); // texture slots 8
                    shaderEditorPBR->setInt("shadowMap", 8);
                }
            }

            // Render by shaderEditor OR shaderEditorPBR
            object->mesh->Render();
        }

        if (object->objectType == "model" && object->model != nullptr)
        {
            // Quixel Megascans model
            if (passType == "main")
            {
                shaderEditorPBR->Bind();

                shaderEditorPBR->setMat4("model", object->transform);
                shaderEditorPBR->setVec4("tintColor", object->color);
                shaderEditorPBR->setBool("isSelected", object->isSelected);
                shaderEditorPBR->setFloat("tilingFactor", object->tilingFactorMaterial);

                shaderEditorPBR->setFloat("material.specularIntensity", m_MaterialSpecular); // TODO - use material attribute
                shaderEditorPBR->setFloat("material.shininess", m_MaterialShininess);        // TODO - use material attribute

                m_MaterialWorkflowPBR->BindTextures(0);                 // texture slots 0, 1, 2
                materials[object->materialName]->BindTextures(3);       // texture slots 3, 4, 5, 6, 7
                
                // Shadows in shaderEditorPBR
                LightManager::directionalLight.GetShadowMap()->Read(8); // texture slots 8
                shaderEditorPBR->setInt("shadowMap", 8);
            }
        
            object->model->RenderModelPBR();
        }

        object->AABB->Update(object->position, object->rotation, object->scale);
        object->pivot->Update(object->position, object->scale + 1.0f);
    }

    if (passType == "main")
    {
        // Render spheres on light positions
        // Directional light (somewhere on pozitive Y axis, at X=0, Z=0)
        // Render Sphere (Light source)
        glm::mat4 model;

        textures["plain"]->Bind(0);
        textures["plain"]->Bind(1);
        textures["plain"]->Bind(2);

        shaderEditor->Bind();
        shaderEditor->setVec4("tintColor", glm::vec4(1.0f));

        // Directional Light
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20.0f, 20.0f, 20.0f));
        model = glm::rotate(model, glm::radians(m_LightManager->directionalLight.GetDirection().x * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(m_LightManager->directionalLight.GetDirection().y * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_LightManager->directionalLight.GetDirection().z * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        model = glm::scale(model, glm::vec3(0.5f));
        shaderEditor->setMat4("model", model);
        if (m_DisplayLightSources)
            meshes["cone"]->Render();

        // Point lights
        for (unsigned int i = 0; i < m_LightManager->pointLightCount; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, m_LightManager->pointLights[i].GetPosition());
            model = glm::scale(model, glm::vec3(0.25f));
            shaderEditor->setMat4("model", model);
            if (m_DisplayLightSources && m_LightManager->pointLights[i].GetEnabled())
                meshes["sphere"]->Render();
        }

        // Spot lights
        for (unsigned int i = 0; i < m_LightManager->spotLightCount; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, m_LightManager->spotLights[i].GetBasePL()->GetPosition());
            model = glm::rotate(model, glm::radians(m_LightManager->spotLights[i].GetDirection().x * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::rotate(model, glm::radians(m_LightManager->spotLights[i].GetDirection().y * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(m_LightManager->spotLights[i].GetDirection().z * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.25f));
            shaderEditor->setMat4("model", model);
            if (m_DisplayLightSources && m_LightManager->spotLights[i].GetBasePL()->GetEnabled())
                meshes["cone"]->Render();
        }
        /* End of shaderEditor */

        // Skybox shaderBackground
        /* Begin backgroundShader */
        RendererBasic::DisableCulling();
        Shader* shaderBackground = shaders["background"];
        // render skybox (render as last to prevent overdraw)
        shaderBackground->Bind();
        shaderBackground->setMat4("projection", projectionMatrix);
        shaderBackground->setMat4("view", m_Camera->CalculateViewMatrix());
        shaderBackground->setInt("environmentMap", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetEnvironmentCubemap());
        // glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetIrradianceMap()); // display irradiance map
        // glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetPrefilterMap()); // display prefilter map
        m_MaterialWorkflowPBR->GetSkyboxCube()->Render();
        /* End backgroundShader */

        /* Begin of shaderBasic */
        shaderBasic->Bind();
        shaderBasic->setMat4("projection", projectionMatrix);
        shaderBasic->setMat4("view", m_Camera->CalculateViewMatrix());

        if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
        {
            shaderBasic->setMat4("model", glm::mat4(1.0f));
            m_SceneObjects[m_SelectedIndex]->AABB->Draw();
            m_SceneObjects[m_SelectedIndex]->pivot->Draw(shaderBasic, projectionMatrix, m_Camera->CalculateViewMatrix());
        }

        m_Grid->Draw(shaderBasic, projectionMatrix, m_Camera->CalculateViewMatrix());
        if (m_DrawScenePivot)
            m_PivotScene->Draw(shaderBasic, projectionMatrix, m_Camera->CalculateViewMatrix());

        // A quad for displaying a shadow map on it
        shaderEditor->Bind();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 10.0f, -20.0f));
        model = glm::scale(model, glm::vec3(16.0f, 9.0f, 1.0f));
        shaderEditor->setMat4("model", model);
        LightManager::directionalLight.GetShadowMap()->Read(0);
        shaderEditor->setInt("shadowMap", 0);
        // m_Quad->Render();

        // Render gizmo on front of everything (depth mask enabled)
        if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
        {
            m_Gizmo->Render(shaderGizmo);
        }
    }
}

void SceneEditor::ResetScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneReset.lastTime < m_SceneReset.cooldown) return;
    m_SceneReset.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::ResetScene: Deleting %zu objects!\n", m_SceneObjects.size());

    for (auto& object : m_SceneObjects)
    {
        // delete object->AABB;
        // delete object->pivot;
        // delete object->mesh;
    }
    m_SceneObjects.clear();
}

void SceneEditor::CleanupGeometry()
{
}

SceneEditor::~SceneEditor()
{
    SaveScene();

	CleanupGeometry();

    delete m_PivotScene;
    delete m_Grid;
    delete m_Raycast;
    delete m_Gizmo;
    delete m_TextureCubeMap;

    ResetScene();
}
