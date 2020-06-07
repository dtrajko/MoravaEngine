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
#include "Tile2D.h"

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>


std::mutex SceneEditor::s_MutexTextures;
std::mutex SceneEditor::s_MutexMaterials;
float SceneEditor::m_MaterialSpecular = 1.0f;
float SceneEditor::m_MaterialShininess = 256.0f;

SceneEditor::SceneEditor()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 2.0f, 12.0f);
	sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.enableSkybox       = false;
    sceneSettings.enablePointLights  = true;
    sceneSettings.enableSpotLights   = true;
    sceneSettings.enableShadows      = true;
    sceneSettings.enableOmniShadows  = true;
    sceneSettings.enableWaterEffects = true;

    // directional light
    sceneSettings.directionalLight.base.enabled = true;
    sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.directionalLight.direction = glm::vec3(1.0f, 0.5f, -0.6f);
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

	SetCamera();
	SetSkybox();
	SetTextures();
    SetupMaterials();
	SetupMeshes();
	SetupModels();
	SetGeometry();

    // Initialize the PBR/IBL Material Workflow component
    m_MaterialWorkflowPBR = new MaterialWorkflowPBR();

    m_CurrentTimestamp = 0.0f;
    m_StartTimestamp = (float)glfwGetTime();

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

    // required for directional light enable/disable feature
    m_DirLightEnabledPrev = sceneSettings.directionalLight.base.enabled;
    m_DirLightColorPrev = sceneSettings.directionalLight.base.color;

    m_DisplayLightSources = true;
    m_MouseButton_1_Prev = false;
    m_UseCubeMaps = false;

    TextureLoader::Get()->Print();
}

void SceneEditor::SetSkybox()
{
}

void SceneEditor::LoadTexture(std::map<std::string, Texture*>& textures, std::string name, std::string filePath)
{
    textures.insert(std::make_pair(name, TextureLoader::Get()->GetTexture(filePath.c_str())));
}

void SceneEditor::LoadTextureAsync(std::map<std::string, Texture*>& textures, std::string name, std::string filePath)
{
    std::lock_guard<std::mutex> lock(s_MutexTextures);
    auto texture = TextureLoader::Get()->GetTexture(filePath.c_str());
    textures.insert(std::make_pair(name, texture));
}

void SceneEditor::LoadMaterial(std::map<std::string, Material*>& materials, std::string name, TextureInfo textureInfo)
{
    materials.insert(std::make_pair(name, new Material(textureInfo, m_MaterialSpecular, m_MaterialShininess)));
}

void SceneEditor::LoadMaterialAsync(std::map<std::string, Material*>& materials, std::string name, TextureInfo textureInfo)
{
    std::lock_guard<std::mutex> lock(s_MutexMaterials);
    auto material = new Material(textureInfo, m_MaterialSpecular, m_MaterialShininess);
    materials.insert(std::make_pair(name, material));
}

Texture* SceneEditor::HotLoadTexture(std::string textureName)
{
    // Load texture if not available in textures map
    auto textureInfoIterator = m_TextureInfo.find(textureName);
    auto textureIterator = textures.find(textureName);

    if (textureInfoIterator == m_TextureInfo.end())
        return nullptr;

    if (textureIterator != textures.end())
        return textureIterator->second;

    LoadTexture(std::ref(textures), textureName, textureInfoIterator->second);

    textureIterator = textures.find(textureName);

    if (textureIterator == textures.end())
        return nullptr;

    return textureIterator->second;
}

Material* SceneEditor::HotLoadMaterial(std::string materialName)
{
    // Load Material if not available in materials map
    auto materialInfoIterator = m_MaterialInfo.find(materialName);
    auto materialIterator = materials.find(materialName);

    if (materialInfoIterator == m_MaterialInfo.end())
        return nullptr;

    if (materialIterator != materials.end())
        return materialIterator->second;

    LoadMaterial(std::ref(materials), materialInfoIterator->first, materialInfoIterator->second);

    materialIterator = materials.find(materialName);

    if (materialIterator == materials.end())
        return nullptr;

    return materialIterator->second;
}

void SceneEditor::SetTextures()
{
    m_TextureInfo.insert(std::make_pair("none",              "Textures/plain.png"));
    m_TextureInfo.insert(std::make_pair("semi_transparent",   "Textures/semi_transparent.png"));
    m_TextureInfo.insert(std::make_pair("texture_checker",    "Textures/texture_checker.png"));
    m_TextureInfo.insert(std::make_pair("wood",               "Textures/wood.png"));
    m_TextureInfo.insert(std::make_pair("plank",              "Textures/texture_plank.png"));
    m_TextureInfo.insert(std::make_pair("rock",               "Textures/rock.png"));
    m_TextureInfo.insert(std::make_pair("pyramid",            "Textures/pyramid.png"));
    m_TextureInfo.insert(std::make_pair("lego",               "Textures/lego.png"));
    m_TextureInfo.insert(std::make_pair("marble",             "Textures/marble.jpg"));
    m_TextureInfo.insert(std::make_pair("metal",              "Textures/metal.png"));
    m_TextureInfo.insert(std::make_pair("brick",              "Textures/brick.png"));
    m_TextureInfo.insert(std::make_pair("crate",              "Textures/crate.png"));
    m_TextureInfo.insert(std::make_pair("grass",              "Textures/grass.jpg"));
    m_TextureInfo.insert(std::make_pair("water",              "Textures/water.png"));
    m_TextureInfo.insert(std::make_pair("rock2",              "Textures/rock/Rock-Texture-Surface.jpg"));
    m_TextureInfo.insert(std::make_pair("planet",             "Textures/planet/planet_Quom1200.png"));
    m_TextureInfo.insert(std::make_pair("gold_albedo",        "Textures/PBR/gold/albedo.png"));
    m_TextureInfo.insert(std::make_pair("silver_albedo",      "Textures/PBR/silver/albedo.png"));
    m_TextureInfo.insert(std::make_pair("rusted_iron",        "Textures/PBR/rusted_iron/albedo.png"));
    m_TextureInfo.insert(std::make_pair("grass_albedo",       "Textures/PBR/grass/albedo.png"));
    m_TextureInfo.insert(std::make_pair("wall_albedo",        "Textures/PBR/wall/albedo.png"));
    m_TextureInfo.insert(std::make_pair("plastic_albedo",     "Textures/PBR/plastic/albedo.png"));
    m_TextureInfo.insert(std::make_pair("wal67ar_small",      "Textures/OGLdev/buddha/wal67ar_small.jpg"));
    m_TextureInfo.insert(std::make_pair("wal69ar_small",      "Textures/OGLdev/buddha/wal69ar_small.jpg"));
    m_TextureInfo.insert(std::make_pair("hheli",              "Textures/OGLdev/hheli/hheli.bmp"));
    m_TextureInfo.insert(std::make_pair("jeep_army",          "Textures/OGLdev/jeep/jeep_army.jpg"));
    m_TextureInfo.insert(std::make_pair("jeep_rood",          "Textures/OGLdev/jeep/jeep_rood.jpg"));

#define ASYNC_LOAD_TEXTURES 0
#if ASYNC_LOAD_TEXTURES
    for (auto textureInfo : m_TextureInfo)
        m_FuturesTextures.push_back(std::async(std::launch::async, LoadTextureAsync, std::ref(textures), textureInfo.first, textureInfo.second));
#else
    //  for (auto textureInfo : m_TextureInfo)
    //      LoadTexture(std::ref(textures), textureInfo.first, textureInfo.second);
#endif

    LoadTexture(std::ref(textures), m_TextureInfo.find("none")->first, m_TextureInfo.find("none")->second);
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
    m_MaterialInfo.insert(std::make_pair("none", textureInfoNone));

    // gold
    TextureInfo textureInfoGold     = {};
    textureInfoGold.albedo    = "Textures/PBR/gold/albedo.png";
    textureInfoGold.normal    = "Textures/PBR/gold/normal.png";
    textureInfoGold.metallic  = "Textures/PBR/gold/metallic.png";
    textureInfoGold.roughness = "Textures/PBR/gold/roughness.png";
    textureInfoGold.ao        = "Textures/PBR/gold/ao.png";
    m_MaterialInfo.insert(std::make_pair("gold", textureInfoGold));

    // silver
    TextureInfo textureInfoSilver   = {};
    textureInfoSilver.albedo    = "Textures/PBR/silver/albedo.png";
    textureInfoSilver.normal    = "Textures/PBR/silver/normal.png";
    textureInfoSilver.metallic  = "Textures/PBR/silver/metallic.png";
    textureInfoSilver.roughness = "Textures/PBR/silver/roughness.png";
    textureInfoSilver.ao        = "Textures/PBR/silver/ao.png";
    m_MaterialInfo.insert(std::make_pair("silver", textureInfoSilver));

    // rusted iron
    TextureInfo textureInfoRustedIron = {};
    textureInfoRustedIron.albedo    = "Textures/PBR/rusted_iron/albedo.png";
    textureInfoRustedIron.normal    = "Textures/PBR/rusted_iron/normal.png";
    textureInfoRustedIron.metallic  = "Textures/PBR/rusted_iron/metallic.png";
    textureInfoRustedIron.roughness = "Textures/PBR/rusted_iron/roughness.png";
    textureInfoRustedIron.ao        = "Textures/PBR/rusted_iron/ao.png";
    m_MaterialInfo.insert(std::make_pair("rusted_iron", textureInfoRustedIron));

    // plastic
    TextureInfo textureInfoPlastic  = {};
    textureInfoPlastic.albedo    = "Textures/PBR/plastic/albedo.png";
    textureInfoPlastic.normal    = "Textures/PBR/plastic/normal.png";
    textureInfoPlastic.metallic  = "Textures/PBR/plastic/metallic.png";
    textureInfoPlastic.roughness = "Textures/PBR/plastic/roughness.png";
    textureInfoPlastic.ao        = "Textures/PBR/plastic/ao.png";
    m_MaterialInfo.insert(std::make_pair("plastic", textureInfoPlastic));

    // futur_panel
    TextureInfo textureInfoFuturPanel = {};
    textureInfoFuturPanel.albedo    = "Textures/PBR/futuristic_panel_1/futuristic-panels1-albedo.png";
    textureInfoFuturPanel.normal    = "Textures/PBR/futuristic_panel_1/futuristic-panels1-normal-dx.png";
    textureInfoFuturPanel.metallic  = "Textures/PBR/futuristic_panel_1/futuristic-panels1-metallic.png";
    textureInfoFuturPanel.roughness = "Textures/PBR/futuristic_panel_1/futuristic-panels1-roughness.png";
    textureInfoFuturPanel.ao        = "Textures/PBR/futuristic_panel_1/futuristic-panels1-ao.png";
    m_MaterialInfo.insert(std::make_pair("futur_panel", textureInfoFuturPanel));

    // dark tiles
    TextureInfo textureInfoDarkTiles = {};
    textureInfoDarkTiles.albedo    = "Textures/PBR/dark_tiles_1/darktiles1_basecolor.png";
    textureInfoDarkTiles.normal    = "Textures/PBR/dark_tiles_1/darktiles1_normal-DX.png";
    textureInfoDarkTiles.metallic  = "Textures/PBR/metalness.png";
    textureInfoDarkTiles.roughness = "Textures/PBR/dark_tiles_1/darktiles1_roughness.png";
    textureInfoDarkTiles.ao        = "Textures/PBR/dark_tiles_1/darktiles1_AO.png";
    m_MaterialInfo.insert(std::make_pair("dark_tiles", textureInfoDarkTiles));

    // mahogany floor
    TextureInfo textureInfoMahoganyFloor = {};
    textureInfoMahoganyFloor.albedo    = "Textures/PBR/mahogany_floor/mahogfloor_basecolor.png";
    textureInfoMahoganyFloor.normal    = "Textures/PBR/mahogany_floor/mahogfloor_normal.png";
    textureInfoMahoganyFloor.metallic  = "Textures/PBR/mahogany_floor/mahogfloor_metalness.png";
    textureInfoMahoganyFloor.roughness = "Textures/PBR/mahogany_floor/mahogfloor_roughness.png";
    textureInfoMahoganyFloor.ao        = "Textures/PBR/mahogany_floor/mahogfloor_AO.png";
    m_MaterialInfo.insert(std::make_pair("mahogany_floor", textureInfoMahoganyFloor));

    // aged planks
    TextureInfo textureInfoAgedPlanks = {};
    textureInfoAgedPlanks.albedo    = "Textures/PBR/aged_planks_1/agedplanks1-albedo.png";
    textureInfoAgedPlanks.normal    = "Textures/PBR/aged_planks_1/agedplanks1-normal4-ue.png";
    textureInfoAgedPlanks.metallic  = "Textures/PBR/aged_planks_1/agedplanks1-metalness.png";
    textureInfoAgedPlanks.roughness = "Textures/PBR/aged_planks_1/agedplanks1-roughness.png";
    textureInfoAgedPlanks.ao        = "Textures/PBR/aged_planks_1/agedplanks1-ao.png";
    m_MaterialInfo.insert(std::make_pair("aged_planks", textureInfoAgedPlanks));

    // harsh bricks
    TextureInfo textureInfoHarshBricks = {};
    textureInfoHarshBricks.albedo    = "Textures/PBR/harsh_bricks/harshbricks-albedo.png";
    textureInfoHarshBricks.normal    = "Textures/PBR/harsh_bricks/harshbricks-normal.png";
    textureInfoHarshBricks.metallic  = "Textures/PBR/metalness.png";
    textureInfoHarshBricks.roughness = "Textures/PBR/harsh_bricks/harshbricks-roughness.png";
    textureInfoHarshBricks.ao        = "Textures/PBR/harsh_bricks/harshbricks-ao2.png";
    m_MaterialInfo.insert(std::make_pair("harsh_bricks", textureInfoHarshBricks));

    // Stone Carved (Quixel Megascans)
    TextureInfo textureInfoStoneCarved = {};
    textureInfoStoneCarved.albedo    = "Textures/PBR/Stone_Carved/Albedo.jpg";
    textureInfoStoneCarved.normal    = "Textures/PBR/Stone_Carved/Normal_LOD0.jpg";
    textureInfoStoneCarved.metallic  = "Textures/PBR/Stone_Carved/Metalness.jpg";
    textureInfoStoneCarved.roughness = "Textures/PBR/Stone_Carved/Roughness.jpg";
    textureInfoStoneCarved.ao        = "Textures/PBR/Stone_Carved/Displacement.jpg";
    m_MaterialInfo.insert(std::make_pair("stone_carved", textureInfoStoneCarved));

    // Old Stove (Quixel Megascans)
    TextureInfo textureInfoOldStove = {};
    textureInfoOldStove.albedo    = "Textures/PBR/Old_Stove/Albedo.jpg";
    textureInfoOldStove.normal    = "Textures/PBR/Old_Stove/Normal_LOD0.jpg";
    textureInfoOldStove.metallic  = "Textures/PBR/Old_Stove/Metalness.jpg";
    textureInfoOldStove.roughness = "Textures/PBR/Old_Stove/Roughness.jpg";
    textureInfoOldStove.ao        = "Textures/PBR/Old_Stove/Displacement.jpg";
    m_MaterialInfo.insert(std::make_pair("old_stove", textureInfoOldStove));

    // Animated Character (Sebastian Lague / ThinMatrix)
    TextureInfo textureInfoAnimBoy = {};
    textureInfoAnimBoy.albedo    = "Textures/AnimatedCharacterDiffuse.png";
    textureInfoAnimBoy.normal    = "Textures/PBR/plastic/normal.png";
    textureInfoAnimBoy.metallic  = "Textures/PBR/plastic/metallic.png";
    textureInfoAnimBoy.roughness = "Textures/PBR/plastic/roughness.png";
    textureInfoAnimBoy.ao        = "Textures/PBR/plastic/ao.png";
    m_MaterialInfo.insert(std::make_pair("anim_boy", textureInfoAnimBoy));

    // Buddha
    TextureInfo textureInfoBuddha = {};
    textureInfoBuddha.albedo    = "Textures/OGLdev/buddha/wal67ar_small.jpg";
    textureInfoBuddha.normal    = "Textures/PBR/silver/normal.png";
    textureInfoBuddha.metallic  = "Textures/PBR/silver/metallic.png";
    textureInfoBuddha.roughness = "Textures/PBR/silver/roughness.png";
    textureInfoBuddha.ao        = "Textures/PBR/silver/ao.png";
    m_MaterialInfo.insert(std::make_pair("buddha", textureInfoBuddha));

    // Damaged Helmet glTF PBR
    TextureInfo textureInfoDamagedHelmet = {};
    textureInfoDamagedHelmet.albedo    = "Textures/PBR/DamagedHelmet/Default_albedo.jpg";
    textureInfoDamagedHelmet.normal    = "Textures/PBR/DamagedHelmet/Default_normal.jpg";
    textureInfoDamagedHelmet.metallic  = "Textures/PBR/DamagedHelmet/Default_metalRoughness.jpg";
    textureInfoDamagedHelmet.roughness = "Textures/PBR/DamagedHelmet/Default_emissive.jpg";
    textureInfoDamagedHelmet.ao        = "Textures/PBR/DamagedHelmet/Default_AO.jpg";
    m_MaterialInfo.insert(std::make_pair("damaged_helmet", textureInfoDamagedHelmet));

    // SF Helmet glTF PBR
    TextureInfo textureInfoSFHelmet = {};
    textureInfoSFHelmet.albedo    = "Textures/PBR/SciFiHelmet/SciFiHelmet_BaseColor.png";
    textureInfoSFHelmet.normal    = "Textures/PBR/SciFiHelmet/SciFiHelmet_Normal.png";
    textureInfoSFHelmet.metallic  = "Textures/PBR/SciFiHelmet/SciFiHelmet_MetallicRoughness.png";
    textureInfoSFHelmet.roughness = "Textures/PBR/SciFiHelmet/SciFiHelmet_Emissive.png";
    textureInfoSFHelmet.ao        = "Textures/PBR/SciFiHelmet/SciFiHelmet_AmbientOcclusion.png";
    m_MaterialInfo.insert(std::make_pair("sf_helmet", textureInfoSFHelmet));

    // Cerberus model PBR textures
    TextureInfo textureInfoCerberus = {};
    textureInfoCerberus.albedo    = "Textures/PBR/Cerberus/Cerberus_A.tga";
    textureInfoCerberus.normal    = "Textures/PBR/Cerberus/Cerberus_N.tga";
    textureInfoCerberus.metallic  = "Textures/PBR/Cerberus/Cerberus_M.tga";
    textureInfoCerberus.roughness = "Textures/PBR/Cerberus/Cerberus_R.tga";
    textureInfoCerberus.ao        = "Textures/PBR/Cerberus/Cerberus_AO.tga";
    m_MaterialInfo.insert(std::make_pair("cerberus", textureInfoCerberus));

    // Concrete 3 Free PBR Materials freepbr.com
    TextureInfo textureInfoConcrete = {};
    textureInfoConcrete.albedo    = "Textures/PBR/concrete3/concrete3-albedo.png";
    textureInfoConcrete.normal    = "Textures/PBR/concrete3/concrete3-Normal-dx.png";
    textureInfoConcrete.metallic  = "Textures/PBR/concrete3/concrete3-Metallic.png";
    textureInfoConcrete.roughness = "Textures/PBR/concrete3/concrete3-Roughness.png";
    textureInfoConcrete.ao        = "Textures/PBR/concrete3/concrete3-ao.png";
    m_MaterialInfo.insert(std::make_pair("concrete", textureInfoConcrete));

    // Modern Brick Wall 1 Free PBR Materials freepbr.com
    TextureInfo textureInfoModernBrickWall = {};
    textureInfoModernBrickWall.albedo    = "Textures/PBR/modern_brick_1/modern-brick1_albedo.png";
    textureInfoModernBrickWall.normal    = "Textures/PBR/modern_brick_1/modern-brick1_normal-dx.png";
    textureInfoModernBrickWall.metallic  = "Textures/PBR/modern_brick_1/modern-brick1_metallic.png";
    textureInfoModernBrickWall.roughness = "Textures/PBR/modern_brick_1/modern-brick1_roughness.png";
    textureInfoModernBrickWall.ao        = "Textures/PBR/modern_brick_1/modern-brick1_ao.png";
    m_MaterialInfo.insert(std::make_pair("modern_brick_wall", textureInfoModernBrickWall));

#define ASYNC_LOAD_MATERIALS 0
#if ASYNC_LOAD_MATERIALS
    for (auto materialInfo : m_MaterialInfo)
        m_FuturesMaterials.push_back(std::async(std::launch::async, LoadMaterialAsync, std::ref(materials), materialInfo.first, materialInfo.second));
#else
    //  for (auto materialInfo : m_MaterialInfo)
    //      LoadMaterial(std::ref(materials), materialInfo.first, materialInfo.second);
#endif

    LoadMaterial(std::ref(materials), m_MaterialInfo.find("none")->first, m_MaterialInfo.find("none")->second);
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

    m_GlassShaderModel = new Model("Models/Old_Stove/udmheheqx_LOD0.fbx");
    models.insert(std::make_pair("glass", m_GlassShaderModel));
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

        if (object->m_Type == "mesh" && object->mesh != nullptr)
            object->mesh->Update(object->scale);
        else if (object->m_Type == "model" && object->model != nullptr)
            object->model->Update(object->scale);
    }

    if (m_HDRI_Edit != m_HDRI_Edit_Prev)
    {
        if (m_HDRI_Edit == HDRI_GREENWICH_PARK)
            m_MaterialWorkflowPBR->Init("Textures/HDR/greenwich_park_02_1k.hdr");
        else if (m_HDRI_Edit == HDRI_SAN_GIUSEPPE_BRIDGE)
            m_MaterialWorkflowPBR->Init("Textures/HDR/san_giuseppe_bridge_1k.hdr");
        else if (m_HDRI_Edit == HDRI_TROPICAL_BEACH)
            m_MaterialWorkflowPBR->Init("Textures/HDR/Tropical_Beach_3k.hdr");
        else if (m_HDRI_Edit == HDRI_VIGNAIOLI_NIGHT)
            m_MaterialWorkflowPBR->Init("Textures/HDR/vignaioli_night_1k.hdr");
        else if (m_HDRI_Edit == HDRI_EARLY_EVE_WARM_SKY)
            m_MaterialWorkflowPBR->Init("Textures/HDR/006_hdrmaps_com_free.hdr");

        m_HDRI_Edit_Prev = m_HDRI_Edit;
    }

    m_Gizmo->Update(m_Camera->GetPosition(), mainWindow);
    m_Gizmo->SetDrawAABBs(m_DrawGizmos);

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

    for (auto& object : m_SceneObjects)
    {
        glm::vec3 scaleAABB = object->scale * object->AABB->m_Scale;
        object->AABB->Update(object->position, object->rotation, object->scale);
        object->pivot->Update(object->position, object->scale + 1.0f);
    }
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

    printf("SceneEditor::SaveScene: Saving %zu objects...\n", m_SceneObjects.size());

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
            sceneObject->AABB  = new AABB(sceneObject->positionAABB, sceneObject->rotation, sceneObject->scaleAABB);
            sceneObject->pivot = new Pivot(sceneObject->position, sceneObject->scale);
            Mesh* mesh = nullptr;
            Model* model = nullptr;
            if (sceneObject->m_Type == "mesh") {
                sceneObject->mesh  = CreateNewMesh(sceneObject->m_TypeID, sceneObject->scale);
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

void SceneEditor::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

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
        ImGui::SliderFloat3("Rotation", (float*)m_RotationEdit, -179.0f, 180.0f);
        ImGui::SliderFloat3("Scale", (float*)m_ScaleEdit, 0.1f, 20.0f);
    }
    ImGui::End();

    ImGui::Begin("Material");
    {
        ImGui::ColorEdit4("Color", (float*)m_ColorEdit);

        // Begin TextureName ImGui drop-down list
        std::vector<const char*> itemsTexture;
        std::map<std::string, std::string>::iterator itTexture;
        for (itTexture = m_TextureInfo.begin(); itTexture != m_TextureInfo.end(); itTexture++)
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
        for (itMaterial = m_MaterialInfo.begin(); itMaterial != m_MaterialInfo.end(); itMaterial++)
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
        ImGui::SliderInt("Selected Object", (int*)&m_SelectedIndex, 0, (int)(m_SceneObjects.size() - 1));

        ImGui::Separator();
        float FOV = GetFOV();
        ImGui::SliderFloat("FOV", &FOV, 1.0f, 120.0f);
        SetFOV(FOV);

        if (ImGui::CollapsingHeader("Select HDRI"))
        {
            ImGui::RadioButton("Greenwich Park", &m_HDRI_Edit, HDRI_GREENWICH_PARK);
            ImGui::RadioButton("San Giuseppe Bridge", &m_HDRI_Edit, HDRI_SAN_GIUSEPPE_BRIDGE);
            ImGui::RadioButton("Tropical Beach", &m_HDRI_Edit, HDRI_TROPICAL_BEACH);
            ImGui::RadioButton("Vignaioli Night", &m_HDRI_Edit, HDRI_VIGNAIOLI_NIGHT);
            ImGui::RadioButton("Early Eve & Warm Sky", &m_HDRI_Edit, HDRI_EARLY_EVE_WARM_SKY);
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

    ImGui::Begin("Profiler");
    {
        ImGui::Text("Timer");

        float realFPS = Timer::Get()->GetRealFPS();
        std::string sRealFPS = "Real FPS: " + std::to_string(realFPS);

        float deltaTimeMS = Timer::Get()->GetDeltaTime() * 1000.0f;
        std::string sDeltaTimeMS = "Delta Time: " + std::to_string(deltaTimeMS) + " ms";

        ImGui::Text(sRealFPS.c_str());
        ImGui::Text(sDeltaTimeMS.c_str());

        ImGui::Separator();

        ImGui::Text("Active Render Passes:");
        ImGui::Indent();
        for (auto& renderPassName : m_ActiveRenderPasses)
            ImGui::Text(renderPassName.c_str());
        ImGui::Unindent();
        m_ActiveRenderPasses.clear();
    }
    ImGui::End();

    ImGui::Begin("Mouse Picker Info");
    {
        char buffer[100];

        sprintf_s(buffer, "Mouse Coords [ X: %.2ff Y: %.2ff ]", mp->m_MouseX, mp->m_MouseY);
        ImGui::Text(buffer);
        ImGui::Separator();
        sprintf_s(buffer, "Normalized Coords [ X: %.2ff Y: %.2ff ]", mp->m_NormalizedCoords.x, mp->m_NormalizedCoords.y);
        ImGui::Text(buffer);
        ImGui::Separator();
        sprintf_s(buffer, "Clip Coords [ X: %.2ff Y: %.2ff ]", mp->m_ClipCoords.x, mp->m_ClipCoords.y);
        ImGui::Text(buffer);
        ImGui::Separator();
        sprintf_s(buffer, "Eye Coords [ X: %.2ff Y: %.2ff Z: %.2ff W: %.2ff ]", mp->m_EyeCoords.x, mp->m_EyeCoords.y, mp->m_EyeCoords.z, mp->m_EyeCoords.w);
        ImGui::Text(buffer);
        ImGui::Separator();
        sprintf_s(buffer, "World Ray [ X: %.2ff Y: %.2ff Z: %.2ff ]", mp->m_WorldRay.x, mp->m_WorldRay.y, mp->m_WorldRay.z);
        ImGui::Text(buffer);
    }
    ImGui::End();

    ImGui::Begin("Scene Editor");
    {
        if (ImGui::CollapsingHeader("Add Mesh"))
        {
            ImGui::RadioButton("Cube",     &m_CurrentObjectTypeID, MESH_TYPE_CUBE);
            ImGui::RadioButton("Pyramid",  &m_CurrentObjectTypeID, MESH_TYPE_PYRAMID);
            ImGui::RadioButton("Sphere",   &m_CurrentObjectTypeID, MESH_TYPE_SPHERE);
            ImGui::RadioButton("Cylinder", &m_CurrentObjectTypeID, MESH_TYPE_CYLINDER);
            ImGui::RadioButton("Cone",     &m_CurrentObjectTypeID, MESH_TYPE_CONE);
            ImGui::RadioButton("Ring",     &m_CurrentObjectTypeID, MESH_TYPE_RING);
            ImGui::RadioButton("Bob Lamp", &m_CurrentObjectTypeID, MESH_TYPE_BOB_LAMP);
            ImGui::RadioButton("Anim Boy", &m_CurrentObjectTypeID, MESH_TYPE_ANIM_BOY);
            ImGui::RadioButton("Terrain",  &m_CurrentObjectTypeID, MESH_TYPE_TERRAIN);
            ImGui::RadioButton("Water",    &m_CurrentObjectTypeID, MESH_TYPE_WATER);
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
        }
    }
    ImGui::End();

    ImGui::Begin("Lights");
    {
        ImGui::Checkbox("Display Light Sources", &m_DisplayLightSources);

        if (ImGui::CollapsingHeader("Directional Light"))
        {
            // Directional Light
            SDirectionalLight directionalLight;
            directionalLight.base.enabled = m_LightManager->directionalLight.GetEnabled();
            directionalLight.base.color = m_LightManager->directionalLight.GetColor();
            directionalLight.base.ambientIntensity = m_LightManager->directionalLight.GetAmbientIntensity();
            directionalLight.base.diffuseIntensity = m_LightManager->directionalLight.GetDiffuseIntensity();
            directionalLight.direction = m_LightManager->directionalLight.GetDirection();

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
        }

        if (ImGui::CollapsingHeader("Point Lights"))
        {
            ImGui::Indent();

            // Point Lights
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

                m_LightManager->pointLights[pl].SetEnabled(pointLights[pl].base.enabled);
                m_LightManager->pointLights[pl].SetColor(pointLights[pl].base.color);
                m_LightManager->pointLights[pl].SetAmbientIntensity(pointLights[pl].base.ambientIntensity);
                m_LightManager->pointLights[pl].SetDiffuseIntensity(pointLights[pl].base.diffuseIntensity);
                m_LightManager->pointLights[pl].SetPosition(pointLights[pl].position);
                m_LightManager->pointLights[pl].SetConstant(pointLights[pl].constant);
                m_LightManager->pointLights[pl].SetLinear(pointLights[pl].linear);
                m_LightManager->pointLights[pl].SetExponent(pointLights[pl].exponent);
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Spot Lights"))
        {
            ImGui::Indent();

            // Spot Lights
            SSpotLight spotLights[4];
            char locBuff[100] = { '\0' };
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
            }
            ImGui::Unindent();
        }
        ImGui::End();
    }
    ImGui::End();
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void SceneEditor::ShowExampleAppDockSpace(bool* p_open, Window& mainWindow)
{
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags =
        ImGuiDockNodeFlags_None |
        ImGuiDockNodeFlags_PassthruCentralNode |
        ImGuiDockNodeFlags_NoDockingInCentralNode;

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
            if (ImGui::MenuItem("Exit")) mainWindow.SetShouldClose(true);
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
    sceneObject->AABB            = new AABB(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
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
    std::string modelName = "";
    std::string objectType = "";
    std::string materialName = "";
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 scaleAABB = glm::vec3(1.0f);
    glm::vec3 positionAABB = glm::vec3(0.0f);

    // Mesh - ID range 0-999
    if (m_CurrentObjectTypeID < 1000) {
        mesh = CreateNewMesh(m_CurrentObjectTypeID, glm::vec3(1.0f));
        objectType = "mesh";

        if (m_CurrentObjectTypeID == MESH_TYPE_BOB_LAMP) {
            modelName = "bob_lamp";
            materialName = "none";
            rotation = glm::vec3(-90.0f, 0.0f, 0.0f);
            scale = glm::vec3(0.1f);
            positionAABB = glm::vec3(0.0f, 0.0f, 30.0f);
            scaleAABB = glm::vec3(20.0f, 20.0f, 60.0f);
        }
        else if (m_CurrentObjectTypeID == MESH_TYPE_ANIM_BOY) {
            modelName = "anim_boy";
            materialName = "anim_boy";
            rotation = glm::vec3(-90.0f, 0.0f, 0.0f);
            scale = glm::vec3(0.5f);
            positionAABB = glm::vec3(0.0f, 0.0f, 4.4f);
            scaleAABB = glm::vec3(2.4f, 2.0f, 8.8f);
        }
        else if (m_CurrentObjectTypeID == MESH_TYPE_TERRAIN) {
            modelName = "terrain";
            materialName = "none";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 0.0f, 0.0f);
            scaleAABB = glm::vec3(128.0f, 0.5f, 128.0f);
        }
        else if (m_CurrentObjectTypeID == MESH_TYPE_WATER) {
            modelName = "water";
            materialName = "none";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 0.0f, 0.0f);
            scaleAABB = glm::vec3(2.0f, 0.5f, 2.0f);
        }
    }
    else if (m_CurrentObjectTypeID >= 1000) { // Model - ID range 1000+
        model = AddNewModel(m_CurrentObjectTypeID, glm::vec3(1.0f));
        objectType = "model";
        if (m_CurrentObjectTypeID == MODEL_STONE_CARVED) {
            modelName = "stone_carved";
            materialName = "stone_carved";
            scale = glm::vec3(0.05f);
            positionAABB = glm::vec3(0.0f, 58.0f, 0.0f);
            scaleAABB = glm::vec3(74.0f, 116.0f, 40.0f);    
        }
        else if (m_CurrentObjectTypeID == MODEL_OLD_STOVE) {
            modelName = "old_stove";
            materialName = "old_stove";
            scale = glm::vec3(0.08f);
            positionAABB = glm::vec3(0.0f, 42.0f, 0.0f);
            scaleAABB = glm::vec3(30.0f, 84.0f, 30.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_BUDDHA) {
            modelName = "buddha";
            materialName = "none";
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 5.0f, 0.0f);
            scaleAABB = glm::vec3(4.0f, 10.0f, 4.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_HHELI) {
            modelName = "hheli";
            materialName = "none";
            rotation = glm::vec3(0.0f, 90.0f, 0.0f);
            scale = glm::vec3(0.05f);
            positionAABB = glm::vec3(20.0f, 40.0f, 0.0f);
            scaleAABB = glm::vec3(260.0f, 80.0f, 100.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_JEEP) {
            modelName = "jeep";
            materialName = "none";
            rotation = glm::vec3(0.0f, -90.0f, 0.0f);
            scale = glm::vec3(0.01f);
            positionAABB = glm::vec3(0.0f, 130.0f, -10.0f);
            scaleAABB = glm::vec3(780.0f, 260.0f, 400.0f);
        }
        else if (m_CurrentObjectTypeID == MODEL_DAMAGED_HELMET) {
            modelName = "damaged_helmet";
            materialName = "damaged_helmet";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, -0.2f, 0.0f);
            scaleAABB = glm::vec3(1.6f, 2.0f, 1.6f);
        }
        else if (m_CurrentObjectTypeID == MODEL_SF_HELMET) {
            modelName = "sf_helmet";
            materialName = "sf_helmet";
            rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            positionAABB = glm::vec3(0.0f, 0.0f, 0.0f);
            scaleAABB = glm::vec3(2.0f, 3.0f, 2.2f);
        }
        else if (m_CurrentObjectTypeID == MODEL_CERBERUS) {
            modelName = "cerberus";
            materialName = "cerberus";
            rotation = glm::vec3(-90.0f, -180.0f, 0.0f);
            scale = glm::vec3(0.01f);
            positionAABB = glm::vec3(0.0f, -50.0f, -8.0f);
            scaleAABB = glm::vec3(20.0f, 150.0f, 45.0f);
        }
    }

    // Add Scene Object here
    SceneObject* sceneObject = CreateNewSceneObject();
    sceneObject->name = modelName;
    sceneObject->m_Type = objectType;
    sceneObject->rotation = glm::quat(rotation * toRadians);
    sceneObject->scale = scale;
    sceneObject->mesh = mesh;
    sceneObject->m_TypeID = m_CurrentObjectTypeID;
    sceneObject->model = model;
    sceneObject->materialName = materialName;
    sceneObject->positionAABB = positionAABB;
    sceneObject->scaleAABB = scaleAABB;
    sceneObject->AABB = new AABB(positionAABB, glm::vec3(0.0f), scaleAABB);

    // printf("SceneEditor::AddSceneObject sceneObject->AABB->m_Scale [ %.2ff %.2ff %.2ff ]\n",
    //     sceneObject->AABB->m_Scale.x, sceneObject->AABB->m_Scale.y, sceneObject->AABB->m_Scale.z);

    m_SceneObjects.push_back(sceneObject);
    m_SelectedIndex = (unsigned int)m_SceneObjects.size() - 1;
}

void SceneEditor::CopySceneObject(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ObjectCopy.lastTime < m_ObjectCopy.cooldown) return;
    m_ObjectCopy.lastTime = m_CurrentTimestamp;

    // printf("SceneEditor::CopySceneObject sceneObjects = %zu selectedIndex = %i\n", sceneObjects->size(), selectedIndex);

    SceneObject* oldSceneObject = nullptr;

    if (selectedIndex < (unsigned int)sceneObjects->size())
        oldSceneObject = sceneObjects->at(selectedIndex);

    if (oldSceneObject == nullptr) return;

    Mesh* mesh = nullptr;
    Model* model = nullptr;

    if (oldSceneObject->m_Type == "mesh" && oldSceneObject->mesh != nullptr) {
        mesh = CreateNewMesh(oldSceneObject->m_TypeID, oldSceneObject->mesh->GetScale());
    }
    else if (oldSceneObject->m_Type == "model" && oldSceneObject->model != nullptr) {
        model = AddNewModel(m_CurrentObjectTypeID, oldSceneObject->mesh->GetScale()); // TODO: m_CurrentModelID hard-coded, must be in SceneObject
    }

    SceneObject* newSceneObject = new SceneObject();

    newSceneObject->id              = (int)sceneObjects->size();
    newSceneObject->name            = oldSceneObject->name;
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
    newSceneObject->AABB            = new AABB(newSceneObject->positionAABB, newSceneObject->rotation, newSceneObject->scaleAABB);
    newSceneObject->pivot           = new Pivot(newSceneObject->position, newSceneObject->scale);
    newSceneObject->m_Type      = oldSceneObject->m_Type;
    newSceneObject->mesh            = mesh;
    newSceneObject->m_TypeID        = m_CurrentObjectTypeID;
    newSceneObject->model           = model;
    newSceneObject->materialName    = oldSceneObject->materialName;
    newSceneObject->tilingFMaterial = oldSceneObject->tilingFMaterial;

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

Mesh* SceneEditor::CreateNewMesh(int meshTypeID, glm::vec3 scale)
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
    case MESH_TYPE_BOB_LAMP:
        mesh = new SkinnedMesh("Models/OGLdev/BobLamp/boblampclean.md5mesh", "Textures/OGLdev/BobLamp");
        break;
    case MESH_TYPE_ANIM_BOY:
        mesh = new SkinnedMesh("Models/AnimatedCharacter.dae", "Textures");
        break;
    case MESH_TYPE_TERRAIN:
        mesh = new Terrain("Textures/horizon_mountains.png", 4.0f, nullptr);
        break;
    case MESH_TYPE_WATER:
        mesh = new Tile2D();
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
        model = new Model("Models/Cerberus_LP.FBX", "Textures/PBR/Cerberus");
        break;
    default:
        model = new Model("Models/Stone_Carved/tf3pfhzda_LOD0.fbx");
        break;
    }
    return model;
}

void SceneEditor::SetUniformsShaderEditorPBR(Shader* shaderEditorPBR, Texture* texture, Material* material, SceneObject* sceneObject)
{
    shaderEditorPBR->Bind();

    shaderEditorPBR->setMat4("model",         sceneObject->transform);
    shaderEditorPBR->setVec4("tintColor",     sceneObject->color);
    shaderEditorPBR->setBool("isSelected",    sceneObject->isSelected);

    shaderEditorPBR->setFloat("material.specularIntensity", m_MaterialSpecular);  // TODO - use material attribute
    shaderEditorPBR->setFloat("material.shininess",         m_MaterialShininess); // TODO - use material attribute

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

void SceneEditor::SetUniformsShaderEditor(Shader* shaderEditor, Texture* texture, SceneObject* sceneObject)
{
    shaderEditor->Bind();

    shaderEditor->setMat4("model",      sceneObject->transform);
    shaderEditor->setVec4("tintColor",  sceneObject->color);
    shaderEditor->setBool("isSelected", sceneObject->isSelected);

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
    LightManager::directionalLight.GetShadowMap()->Read(2);
    shaderEditor->setInt("shadowMap", 2);
}

void SceneEditor::SetUniformsShaderSkinning(Shader* shaderSkinning, SceneObject* sceneObject, float runningTime)
{
    RendererBasic::DisableCulling();
    shaderSkinning->Bind();
    SkinnedMesh* skinnedMesh = (SkinnedMesh*)sceneObject->mesh;
    skinnedMesh->BoneTransform(runningTime, m_SkinningTransforms[sceneObject->name]);
    shaderSkinning->setMat4("model", sceneObject->transform);
    shaderSkinning->setInt("gColorMap", 0);
    shaderSkinning->setFloat("gMatSpecularIntensity", m_MaterialSpecular);
    shaderSkinning->setFloat("gSpecularPower", m_MaterialShininess);
    char locBuff[100] = { '\0' };
    for (unsigned int i = 0; i < m_SkinningTransforms[sceneObject->name].size(); i++)
    {
        snprintf(locBuff, sizeof(locBuff), "gBones[%d]", i);
        shaderSkinning->setMat4(locBuff, m_SkinningTransforms[sceneObject->name][i]);
    }
}

void SceneEditor::SetUniformsShaderWater(Shader* shaderWater, SceneObject* sceneObject)
{
    m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(0);
    m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(1);
    textures["waterNormal"]->Bind(2);
    textures["waterDuDv"]->Bind(3);
    m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(4);
    shaderWater->setVec3("lightColor", LightManager::directionalLight.GetColor());
    shaderWater->setVec3("lightPosition", -(LightManager::directionalLight.GetDirection()));
}

void SceneEditor::SwitchOrthographicView(Window& mainWindow, glm::mat4& projectionMatrix)
{
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
        float left   = -(float)mainWindow.GetBufferWidth() / 2.0f / m_FOV;
        float right  = (float)mainWindow.GetBufferWidth() / 2.0f / m_FOV;
        float bottom = -(float)mainWindow.GetBufferHeight() / 2.0f / m_FOV;
        float top    = (float)mainWindow.GetBufferHeight() / 2.0f / m_FOV;

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

void SceneEditor::RenderLightSources(Shader* shaderGizmo)
{
    shaderGizmo->Bind();

    textures["none"]->Bind(0);
    textures["none"]->Bind(1);
    textures["none"]->Bind(2);

    glm::mat4 model;

    // Directional light (somewhere on pozitive Y axis, at X=0, Z=0)
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-10.0f, 10.0f, 10.0f));
    model = glm::rotate(model, glm::radians(m_LightManager->directionalLight.GetDirection().x *  90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(m_LightManager->directionalLight.GetDirection().y *  90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(m_LightManager->directionalLight.GetDirection().z * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    model = glm::scale(model, glm::vec3(1.0f));
    shaderGizmo->setMat4("model", model);
    shaderGizmo->setVec4("tintColor", glm::vec4(m_LightManager->directionalLight.GetColor(), 1.0f));
    if (m_DisplayLightSources && m_LightManager->directionalLight.GetEnabled())
        meshes["cone"]->Render();

    // Point lights - render Sphere (Light source)
    for (unsigned int i = 0; i < m_LightManager->pointLightCount; i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, m_LightManager->pointLights[i].GetPosition());
        model = glm::scale(model, glm::vec3(0.25f));
        shaderGizmo->setMat4("model", model);
        shaderGizmo->setVec4("tintColor", glm::vec4(m_LightManager->pointLights[i].GetColor(), 1.0f));
        if (m_DisplayLightSources && m_LightManager->pointLights[i].GetEnabled())
            meshes["sphere"]->Render();
    }

    // Spot lights - render cone
    for (unsigned int i = 0; i < m_LightManager->spotLightCount; i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, m_LightManager->spotLights[i].GetBasePL()->GetPosition());
        model = glm::rotate(model, glm::radians(m_LightManager->spotLights[i].GetDirection().x *  90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(m_LightManager->spotLights[i].GetDirection().y *  90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_LightManager->spotLights[i].GetDirection().z * -90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.25f));
        shaderGizmo->setMat4("model", model);
        shaderGizmo->setVec4("tintColor", glm::vec4(m_LightManager->spotLights[i].GetBasePL()->GetColor(), 1.0f));
        if (m_DisplayLightSources && m_LightManager->spotLights[i].GetBasePL()->GetEnabled())
            meshes["cone"]->Render();
    }
}

void SceneEditor::RenderSkybox(Shader* shaderBackground)
{
    // Skybox shaderBackground
    RendererBasic::DisableCulling();
    shaderBackground->Bind();
    // render skybox (render as last to prevent overdraw)

    m_MaterialWorkflowPBR->BindEnvironmentCubemap(0);
    shaderBackground->setInt("environmentMap", 0);

    m_MaterialWorkflowPBR->GetSkyboxCube()->Render();
}

void SceneEditor::RenderLineElements(Shader* shaderBasic, glm::mat4 projectionMatrix)
{
    if (!m_DrawGizmos) return;

    shaderBasic->Bind();
    if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
    {
        shaderBasic->setMat4("model", glm::mat4(1.0f));
        m_SceneObjects[m_SelectedIndex]->AABB->Draw();
        m_SceneObjects[m_SelectedIndex]->pivot->Draw(shaderBasic, projectionMatrix, m_Camera->CalculateViewMatrix());
    }

    m_Grid->Draw(shaderBasic, projectionMatrix, m_Camera->CalculateViewMatrix());
    m_PivotScene->Draw(shaderBasic, projectionMatrix, m_Camera->CalculateViewMatrix());
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

void SceneEditor::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    // printf("SceneEditor::Render passType = %s\n", passType.c_str());
    m_ActiveRenderPasses.push_back(passType); // for displaying all render passes in ImGui

    SwitchOrthographicView(mainWindow, projectionMatrix);

    for (auto& object : m_SceneObjects)
    {
        object->transform = CalculateRenderTransform(object);

        textures["none"]->Bind(0); // Default fallback for Albedo texture

        Texture* texture   = HotLoadTexture(object->textureName);
        Material* material = HotLoadMaterial(object->materialName);

        shaders["editor_object"]->Bind();
        shaders["editor_object"]->setMat4("model", object->transform);
        shaders["editor_object_pbr"]->Bind();
        shaders["editor_object_pbr"]->setMat4("model", object->transform);
        shaders["skinning"]->Bind();
        shaders["skinning"]->setMat4("model", object->transform);
        shaders["shadow_map"]->Bind();
        shaders["shadow_map"]->setMat4("model", object->transform);
        shaders["water"]->Bind();
        shaders["water"]->setMat4("model", object->transform);
        shaders["omni_shadow_map"]->Bind();
        shaders["omni_shadow_map"]->setMat4("model", object->transform);

        float runningTime = ((float)glfwGetTime() * 1000.0f - m_StartTimestamp) / 1000.0f;

        if (object->name == "water") { // is it a water tile
            // Render with 'water' shader
            if (passType == "main")
                SetUniformsShaderWater(shaders["water"], object);
        }
        else if (m_SkinnedMeshes.find(object->m_TypeID) != m_SkinnedMeshes.end()) // is it a skinned mesh?
        {
            // Render with 'skinning' shader
            if (passType == "main")
                SetUniformsShaderSkinning(shaders["skinning"], object, runningTime);
        }
        else if (material && object->materialName != "none") { // is it using a material?
            // Render with 'editor_object_pbr' shader
            if (passType == "main")
                SetUniformsShaderEditorPBR(shaders["editor_object_pbr"], texture, material, object);
        }
        else { // defaults to a texture only
            // Render with 'editor_object' shader
            if (passType == "main")
                SetUniformsShaderEditor(shaders["editor_object"], texture, object);
        }

        object->Render();
    }

    if (passType == "main")
    {
        RenderLightSources(shaders["gizmo"]);
        RenderSkybox(shaders["background"]);
        RenderLineElements(shaders["basic"], projectionMatrix);
        // RenderFramebufferTextures(shaders["editor_object"]);

        // Render gizmo on front of everything (depth mask enabled)
        if (m_SceneObjects.size() > 0 && m_SelectedIndex < m_SceneObjects.size())
            m_Gizmo->Render(shaders["gizmo"]);
    }
}

void SceneEditor::ResetScene()
{
    // Cooldown
    if (m_CurrentTimestamp - m_SceneReset.lastTime < m_SceneReset.cooldown) return;
    m_SceneReset.lastTime = m_CurrentTimestamp;

    printf("SceneEditor::ResetScene: Deleting %zu objects...\n", m_SceneObjects.size());

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

    ResetScene();
}
