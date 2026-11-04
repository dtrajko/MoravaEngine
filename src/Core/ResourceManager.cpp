#include "Core/ResourceManager.h"
#include "Core/EnginePaths.h"

#if defined(GRAPHICS_API_VULKAN)
#include "EnvMapVulkan/EnvMapVulkanRenderTexture.h" // compiled only with Vulkan (its folder)
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#endif
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "Texture/TextureLoader.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>


std::map<std::string, std::string> ResourceManager::s_TextureInfo;
std::map<std::string, TextureInfo> ResourceManager::s_MaterialInfo;

std::map<std::string, H2M::RefH2M<MoravaTexture>> ResourceManager::s_Textures;
std::map<std::string, H2M::RefH2M<Material>> ResourceManager::s_Materials;

float ResourceManager::s_MaterialSpecular = 1.0f;
float ResourceManager::s_MaterialShininess = 256.0f;

std::map<ResourceManager::TextureCacheKey, H2M::RefH2M<H2M::Texture2D_H2M>> ResourceManager::s_HazelTextures2D;

std::map<std::string, H2M::RefH2M<MoravaShader>> ResourceManager::s_ShaderCacheByTitle;

std::map<std::string, H2M::RefH2M<MoravaShader>> ResourceManager::s_ShadersCacheByFilepath;


void ResourceManager::Shutdown()
{
    // Released while main still runs: as static members they would be destroyed after it returns, when the graphics API
    // (Vulkan's validation layer) may already be torn down
    s_Textures.clear();
    s_Materials.clear();
    s_HazelTextures2D.clear();
#if defined(GRAPHICS_API_VULKAN)
    EnvMapVulkanRenderTexture::ClearCache();
#endif
    s_ShaderCacheByTitle.clear();
    s_ShadersCacheByFilepath.clear();
}

void ResourceManager::Init()
{
    // Setup Textures
    s_TextureInfo.insert(std::make_pair("none", TEXTURES_DIR "/plain.png"));
    s_TextureInfo.insert(std::make_pair("semi_transparent", TEXTURES_DIR "/semi_transparent.png"));
    s_TextureInfo.insert(std::make_pair("texture_checker", TEXTURES_DIR "/texture_checker.png"));
    s_TextureInfo.insert(std::make_pair("wood", TEXTURES_DIR "/wood.png"));
    s_TextureInfo.insert(std::make_pair("container2", TEXTURES_DIR "/container/container2.png"));
    s_TextureInfo.insert(std::make_pair("plank", TEXTURES_DIR "/texture_plank.png"));
    s_TextureInfo.insert(std::make_pair("rock", TEXTURES_DIR "/rock.png"));
    s_TextureInfo.insert(std::make_pair("pyramid", TEXTURES_DIR "/pyramid.png"));
    s_TextureInfo.insert(std::make_pair("lego", TEXTURES_DIR "/lego.png"));
    s_TextureInfo.insert(std::make_pair("marble", TEXTURES_DIR "/marble.jpg"));
    s_TextureInfo.insert(std::make_pair("metal", TEXTURES_DIR "/metal.png"));
    s_TextureInfo.insert(std::make_pair("brick", TEXTURES_DIR "/brick.png"));
    s_TextureInfo.insert(std::make_pair("crate", TEXTURES_DIR "/crate.png"));
    s_TextureInfo.insert(std::make_pair("grass", TEXTURES_DIR "/grass.jpg"));
    s_TextureInfo.insert(std::make_pair("water", TEXTURES_DIR "/water.png"));
    s_TextureInfo.insert(std::make_pair("rock2", TEXTURES_DIR "/rock/Rock-Texture-Surface.jpg"));
    s_TextureInfo.insert(std::make_pair("planet", TEXTURES_DIR "/planet/planet_Quom1200.png"));
    s_TextureInfo.insert(std::make_pair("gold_albedo", TEXTURES_DIR "/PBR/gold/albedo.png"));
    s_TextureInfo.insert(std::make_pair("silver_albedo", TEXTURES_DIR "/PBR/silver/albedo.png"));
    s_TextureInfo.insert(std::make_pair("rusted_iron", TEXTURES_DIR "/PBR/rusted_iron/albedo.png"));
    s_TextureInfo.insert(std::make_pair("grass_albedo", TEXTURES_DIR "/PBR/grass/albedo.png"));
    s_TextureInfo.insert(std::make_pair("wall_albedo", TEXTURES_DIR "/PBR/wall/albedo.png"));
    s_TextureInfo.insert(std::make_pair("plastic_albedo", TEXTURES_DIR "/PBR/plastic/albedo.png"));
    s_TextureInfo.insert(std::make_pair("wal67ar_small", TEXTURES_DIR "/OGLdev/buddha/wal67ar_small.jpg"));
    s_TextureInfo.insert(std::make_pair("wal69ar_small", TEXTURES_DIR "/OGLdev/buddha/wal69ar_small.jpg"));
    s_TextureInfo.insert(std::make_pair("hheli", TEXTURES_DIR "/OGLdev/hheli/hheli.bmp"));
    s_TextureInfo.insert(std::make_pair("jeep_army", TEXTURES_DIR "/OGLdev/jeep/jeep_army.jpg"));
    s_TextureInfo.insert(std::make_pair("jeep_rood", TEXTURES_DIR "/OGLdev/jeep/jeep_rood.jpg"));
    s_TextureInfo.insert(std::make_pair("pine", TEXTURES_DIR "/ThinMatrix/pine.png"));
    s_TextureInfo.insert(std::make_pair("terrain_ground", TEXTURES_DIR "/terrain_ground.jpg"));
    s_TextureInfo.insert(std::make_pair("boulder", TEXTURES_DIR "/ThinMatrix/boulder.png"));
    s_TextureInfo.insert(std::make_pair("fire", TEXTURES_DIR "/Particles/fire.png"));
    s_TextureInfo.insert(std::make_pair("fog", TEXTURES_DIR "/Particles/fog.png"));
    s_TextureInfo.insert(std::make_pair("snowflake", TEXTURES_DIR "/Particles/snowflake.png"));
    s_TextureInfo.insert(std::make_pair("particle_atlas", TEXTURES_DIR "/ThinMatrix/particles/particleAtlas.png"));
    s_TextureInfo.insert(std::make_pair("particle_atlas_cosmic", TEXTURES_DIR "/ThinMatrix/particles/cosmic.png"));
    s_TextureInfo.insert(std::make_pair("particle_atlas_fire", TEXTURES_DIR "/ThinMatrix/particles/fire.png"));
    s_TextureInfo.insert(std::make_pair("particle_atlas_smoke", TEXTURES_DIR "/ThinMatrix/particles/smoke.png"));
    s_TextureInfo.insert(std::make_pair("waterNormal", TEXTURES_DIR "/water/waterNormal.png"));
    s_TextureInfo.insert(std::make_pair("waterDuDv", TEXTURES_DIR "/water/waterDuDv.png"));
    s_TextureInfo.insert(std::make_pair("Checkerboard", TEXTURES_DIR "/Hazel/Checkerboard.tga"));

    // Setup Materials

    // none (placeholder)
    TextureInfo textureInfoNone = {};
    textureInfoNone.albedo = TEXTURES_DIR "/plain.png";
    textureInfoNone.normal = TEXTURES_DIR "/normal_map_default.png";
    textureInfoNone.metallic = TEXTURES_DIR "/plain.png";
    textureInfoNone.roughness = TEXTURES_DIR "/plain.png";
    textureInfoNone.ao = TEXTURES_DIR "/plain.png";
    s_MaterialInfo.insert(std::make_pair("none", textureInfoNone));

    // gold
    TextureInfo textureInfoGold = {};
    textureInfoGold.albedo = TEXTURES_DIR "/PBR/gold/albedo.png";
    textureInfoGold.normal = TEXTURES_DIR "/PBR/gold/normal.png";
    textureInfoGold.metallic = TEXTURES_DIR "/PBR/gold/metallic.png";
    textureInfoGold.roughness = TEXTURES_DIR "/PBR/gold/roughness.png";
    textureInfoGold.ao = TEXTURES_DIR "/PBR/gold/ao.png";
    s_MaterialInfo.insert(std::make_pair("gold", textureInfoGold));

    // silver
    TextureInfo textureInfoSilver = {};
    textureInfoSilver.albedo = TEXTURES_DIR "/PBR/silver/albedo.png";
    textureInfoSilver.normal = TEXTURES_DIR "/PBR/silver/normal.png";
    textureInfoSilver.metallic = TEXTURES_DIR "/PBR/silver/metallic.png";
    textureInfoSilver.roughness = TEXTURES_DIR "/PBR/silver/roughness.png";
    textureInfoSilver.ao = TEXTURES_DIR "/PBR/silver/ao.png";
    s_MaterialInfo.insert(std::make_pair("silver", textureInfoSilver));

    // rusted iron
    TextureInfo textureInfoRustedIron = {};
    textureInfoRustedIron.albedo = TEXTURES_DIR "/PBR/rusted_iron/albedo.png";
    textureInfoRustedIron.normal = TEXTURES_DIR "/PBR/rusted_iron/normal.png";
    textureInfoRustedIron.metallic = TEXTURES_DIR "/PBR/rusted_iron/metallic.png";
    textureInfoRustedIron.roughness = TEXTURES_DIR "/PBR/rusted_iron/roughness.png";
    textureInfoRustedIron.ao = TEXTURES_DIR "/PBR/rusted_iron/ao.png";
    s_MaterialInfo.insert(std::make_pair("rusted_iron", textureInfoRustedIron));

    // plastic
    TextureInfo textureInfoPlastic = {};
    textureInfoPlastic.albedo = TEXTURES_DIR "/PBR/plastic/albedo.png";
    textureInfoPlastic.normal = TEXTURES_DIR "/PBR/plastic/normal.png";
    textureInfoPlastic.metallic = TEXTURES_DIR "/PBR/plastic/metallic.png";
    textureInfoPlastic.roughness = TEXTURES_DIR "/PBR/plastic/roughness.png";
    textureInfoPlastic.ao = TEXTURES_DIR "/PBR/plastic/ao.png";
    s_MaterialInfo.insert(std::make_pair("plastic", textureInfoPlastic));

    // futur_panel
    TextureInfo textureInfoFuturPanel = {};
    textureInfoFuturPanel.albedo = TEXTURES_DIR "/PBR/futuristic_panel_1/futuristic-panels1-albedo.png";
    textureInfoFuturPanel.normal = TEXTURES_DIR "/PBR/futuristic_panel_1/futuristic-panels1-normal-dx.png";
    textureInfoFuturPanel.metallic = TEXTURES_DIR "/PBR/futuristic_panel_1/futuristic-panels1-metallic.png";
    textureInfoFuturPanel.roughness = TEXTURES_DIR "/PBR/futuristic_panel_1/futuristic-panels1-roughness.png";
    textureInfoFuturPanel.ao = TEXTURES_DIR "/PBR/futuristic_panel_1/futuristic-panels1-ao.png";
    s_MaterialInfo.insert(std::make_pair("futur_panel", textureInfoFuturPanel));

    // dark tiles
    TextureInfo textureInfoDarkTiles = {};
    textureInfoDarkTiles.albedo = TEXTURES_DIR "/PBR/dark_tiles_1/darktiles1_basecolor.png";
    textureInfoDarkTiles.normal = TEXTURES_DIR "/PBR/dark_tiles_1/darktiles1_normal-DX.png";
    textureInfoDarkTiles.metallic = TEXTURES_DIR "/PBR/metalness.png";
    textureInfoDarkTiles.roughness = TEXTURES_DIR "/PBR/dark_tiles_1/darktiles1_roughness.png";
    textureInfoDarkTiles.ao = TEXTURES_DIR "/PBR/dark_tiles_1/darktiles1_AO.png";
    s_MaterialInfo.insert(std::make_pair("dark_tiles", textureInfoDarkTiles));

    // mahogany floor
    TextureInfo textureInfoMahoganyFloor = {};
    textureInfoMahoganyFloor.albedo = TEXTURES_DIR "/PBR/mahogany_floor/mahogfloor_basecolor.png";
    textureInfoMahoganyFloor.normal = TEXTURES_DIR "/PBR/mahogany_floor/mahogfloor_normal.png";
    textureInfoMahoganyFloor.metallic = TEXTURES_DIR "/PBR/mahogany_floor/mahogfloor_metalness.png";
    textureInfoMahoganyFloor.roughness = TEXTURES_DIR "/PBR/mahogany_floor/mahogfloor_roughness.png";
    textureInfoMahoganyFloor.ao = TEXTURES_DIR "/PBR/mahogany_floor/mahogfloor_AO.png";
    s_MaterialInfo.insert(std::make_pair("mahogany_floor", textureInfoMahoganyFloor));

    // aged planks
    TextureInfo textureInfoAgedPlanks = {};
    textureInfoAgedPlanks.albedo = TEXTURES_DIR "/PBR/aged_planks_1/agedplanks1-albedo.png";
    textureInfoAgedPlanks.normal = TEXTURES_DIR "/PBR/aged_planks_1/agedplanks1-normal4-ue.png";
    textureInfoAgedPlanks.metallic = TEXTURES_DIR "/PBR/aged_planks_1/agedplanks1-metalness.png";
    textureInfoAgedPlanks.roughness = TEXTURES_DIR "/PBR/aged_planks_1/agedplanks1-roughness.png";
    textureInfoAgedPlanks.ao = TEXTURES_DIR "/PBR/aged_planks_1/agedplanks1-ao.png";
    s_MaterialInfo.insert(std::make_pair("aged_planks", textureInfoAgedPlanks));

    // harsh bricks
    TextureInfo textureInfoHarshBricks = {};
    textureInfoHarshBricks.albedo = TEXTURES_DIR "/PBR/harsh_bricks/harshbricks-albedo.png";
    textureInfoHarshBricks.normal = TEXTURES_DIR "/PBR/harsh_bricks/harshbricks-normal.png";
    textureInfoHarshBricks.metallic = TEXTURES_DIR "/PBR/metalness.png";
    textureInfoHarshBricks.roughness = TEXTURES_DIR "/PBR/harsh_bricks/harshbricks-roughness.png";
    textureInfoHarshBricks.ao = TEXTURES_DIR "/PBR/harsh_bricks/harshbricks-ao2.png";
    s_MaterialInfo.insert(std::make_pair("harsh_bricks", textureInfoHarshBricks));

    // Stone Carved (Quixel Megascans)
    TextureInfo textureInfoStoneCarved = {};
    textureInfoStoneCarved.albedo = TEXTURES_DIR "/PBR/Stone_Carved/Albedo.jpg";
    textureInfoStoneCarved.normal = TEXTURES_DIR "/PBR/Stone_Carved/Normal_LOD0.jpg";
    textureInfoStoneCarved.metallic = TEXTURES_DIR "/PBR/Stone_Carved/Metalness.jpg";
    textureInfoStoneCarved.roughness = TEXTURES_DIR "/PBR/Stone_Carved/Roughness.jpg";
    textureInfoStoneCarved.ao = TEXTURES_DIR "/PBR/Stone_Carved/Displacement.jpg";
    s_MaterialInfo.insert(std::make_pair("stone_carved", textureInfoStoneCarved));

    // Old Stove (Quixel Megascans)
    TextureInfo textureInfoOldStove = {};
    textureInfoOldStove.albedo = TEXTURES_DIR "/PBR/Old_Stove/Albedo.jpg";
    textureInfoOldStove.normal = TEXTURES_DIR "/PBR/Old_Stove/Normal_LOD0.jpg";
    textureInfoOldStove.metallic = TEXTURES_DIR "/PBR/Old_Stove/Metalness.jpg";
    textureInfoOldStove.roughness = TEXTURES_DIR "/PBR/Old_Stove/Roughness.jpg";
    textureInfoOldStove.ao = TEXTURES_DIR "/PBR/Old_Stove/Displacement.jpg";
    s_MaterialInfo.insert(std::make_pair("old_stove", textureInfoOldStove));

    // Animated Character (Sebastian Lague / ThinMatrix)
    TextureInfo textureInfoAnimBoy = {};
    textureInfoAnimBoy.albedo    = MODELS_DIR "/ThinMatrix/AnimatedCharacter/AnimatedCharacterDiffuse.png";
    textureInfoAnimBoy.normal    = TEXTURES_DIR "/PBR/plastic/normal.png";
    textureInfoAnimBoy.metallic  = TEXTURES_DIR "/PBR/plastic/metallic.png";
    textureInfoAnimBoy.roughness = TEXTURES_DIR "/PBR/plastic/roughness.png";
    textureInfoAnimBoy.ao        = TEXTURES_DIR "/PBR/plastic/ao.png";
    s_MaterialInfo.insert(std::make_pair("anim_boy", textureInfoAnimBoy));

    // Buddha
    TextureInfo textureInfoBuddha = {};
    textureInfoBuddha.albedo = TEXTURES_DIR "/OGLdev/buddha/wal67ar_small.jpg";
    textureInfoBuddha.normal = TEXTURES_DIR "/PBR/silver/normal.png";
    textureInfoBuddha.metallic = TEXTURES_DIR "/PBR/silver/metallic.png";
    textureInfoBuddha.roughness = TEXTURES_DIR "/PBR/silver/roughness.png";
    textureInfoBuddha.ao = TEXTURES_DIR "/PBR/silver/ao.png";
    s_MaterialInfo.insert(std::make_pair("buddha", textureInfoBuddha));

    // Damaged Helmet glTF PBR
    TextureInfo textureInfoDamagedHelmet = {};
    textureInfoDamagedHelmet.albedo = TEXTURES_DIR "/PBR/DamagedHelmet/Default_albedo.jpg";
    textureInfoDamagedHelmet.normal = TEXTURES_DIR "/PBR/DamagedHelmet/Default_normal.jpg";
    textureInfoDamagedHelmet.metallic = TEXTURES_DIR "/PBR/DamagedHelmet/Default_metalRoughness.jpg";
    textureInfoDamagedHelmet.roughness = TEXTURES_DIR "/PBR/DamagedHelmet/Default_emissive.jpg";
    textureInfoDamagedHelmet.ao = TEXTURES_DIR "/PBR/DamagedHelmet/Default_AO.jpg";
    s_MaterialInfo.insert(std::make_pair("damaged_helmet", textureInfoDamagedHelmet));

    // SF Helmet glTF PBR
    TextureInfo textureInfoSFHelmet = {};
    textureInfoSFHelmet.albedo    = TEXTURES_DIR "/PBR/SciFiHelmet/SciFiHelmet_BaseColor.png";
    textureInfoSFHelmet.normal    = TEXTURES_DIR "/PBR/SciFiHelmet/SciFiHelmet_Normal.png";
    textureInfoSFHelmet.metallic  = TEXTURES_DIR "/PBR/SciFiHelmet/SciFiHelmet_MetallicRoughness.png";
    textureInfoSFHelmet.roughness = TEXTURES_DIR "/PBR/SciFiHelmet/SciFiHelmet_Emissive.png";
    textureInfoSFHelmet.ao        = TEXTURES_DIR "/PBR/SciFiHelmet/SciFiHelmet_AmbientOcclusion.png";
    s_MaterialInfo.insert(std::make_pair("sf_helmet", textureInfoSFHelmet));

    // Cerberus model PBR textures
    TextureInfo textureInfoCerberus = {};
    textureInfoCerberus.albedo    = MODELS_DIR "/Cerberus/Textures/Cerberus_A.tga";
    textureInfoCerberus.normal    = MODELS_DIR "/Cerberus/Textures/Cerberus_N.tga";
    textureInfoCerberus.metallic  = MODELS_DIR "/Cerberus/Textures/Cerberus_M.tga";
    textureInfoCerberus.roughness = MODELS_DIR "/Cerberus/Textures/Cerberus_R.tga";
    textureInfoCerberus.ao        = MODELS_DIR "/Cerberus/Textures/Cerberus_AO.tga";
    s_MaterialInfo.insert(std::make_pair("cerberus", textureInfoCerberus));

    // Concrete 3 Free PBR Materials freepbr.com
    TextureInfo textureInfoConcrete = {};
    textureInfoConcrete.albedo    = TEXTURES_DIR "/PBR/concrete3/concrete3-albedo.png";
    textureInfoConcrete.normal    = TEXTURES_DIR "/PBR/concrete3/concrete3-Normal-dx.png";
    textureInfoConcrete.metallic  = TEXTURES_DIR "/PBR/concrete3/concrete3-Metallic.png";
    textureInfoConcrete.roughness = TEXTURES_DIR "/PBR/concrete3/concrete3-Roughness.png";
    textureInfoConcrete.ao        = TEXTURES_DIR "/PBR/concrete3/concrete3-ao.png";
    s_MaterialInfo.insert(std::make_pair("concrete", textureInfoConcrete));

    // Modern Brick Wall 1 Free PBR Materials freepbr.com
    TextureInfo textureInfoModernBrickWall = {};
    textureInfoModernBrickWall.albedo    = TEXTURES_DIR "/PBR/modern_brick_1/modern-brick1_albedo.png";
    textureInfoModernBrickWall.normal    = TEXTURES_DIR "/PBR/modern_brick_1/modern-brick1_normal-dx.png";
    textureInfoModernBrickWall.metallic  = TEXTURES_DIR "/PBR/modern_brick_1/modern-brick1_metallic.png";
    textureInfoModernBrickWall.roughness = TEXTURES_DIR "/PBR/modern_brick_1/modern-brick1_roughness.png";
    textureInfoModernBrickWall.ao        = TEXTURES_DIR "/PBR/modern_brick_1/modern-brick1_ao.png";
    s_MaterialInfo.insert(std::make_pair("modern_brick_wall", textureInfoModernBrickWall));

    // ThinMatrix Boulder
    TextureInfo textureInfoBoulder = {};
    textureInfoBoulder.albedo    = TEXTURES_DIR "/ThinMatrix/boulder.png";
    textureInfoBoulder.normal    = TEXTURES_DIR "/ThinMatrix/boulderNormal.png";
    textureInfoBoulder.metallic  = TEXTURES_DIR "/metalness.png";
    textureInfoBoulder.roughness = TEXTURES_DIR "/plain.png";
    textureInfoBoulder.ao        = TEXTURES_DIR "/plain.png";
    s_MaterialInfo.insert(std::make_pair("boulder", textureInfoBoulder));

    // M1911
    TextureInfo textureInfoM1911 = {};
    textureInfoM1911.albedo    = MODELS_DIR "/M1911/m1911_color.png";
    textureInfoM1911.normal    = MODELS_DIR "/M1911/m1911_normal.png";
    textureInfoM1911.metallic  = MODELS_DIR "/M1911/m1911_metalness.png";
    textureInfoM1911.roughness = MODELS_DIR "/M1911/m1911_roughness.png";
    textureInfoM1911.ao        = TEXTURES_DIR "/plain.png";
    s_MaterialInfo.insert(std::make_pair("M1911", textureInfoM1911));
}

void ResourceManager::LoadTexture(std::string name, std::string filePath)
{
    LoadTexture(name, filePath, GL_LINEAR, false);
}

void ResourceManager::LoadTexture(std::string name, std::string filePath, GLenum filter, bool force)
{
    if (force) {
        // remove previous entry from the map
        auto it = s_Textures.find(name);
        if (it != s_Textures.end()) {
            s_Textures.erase(it);
        }
    }

    s_Textures.insert(std::make_pair(name, TextureLoader::Get()->GetTexture(filePath.c_str(), false, filter, force)));
}

void ResourceManager::LoadMaterial(std::string name, TextureInfo textureInfo)
{
    s_Materials.insert(std::make_pair(name, H2M::RefH2M<Material>::Create(textureInfo, s_MaterialSpecular, s_MaterialShininess)));
}

H2M::RefH2M<MoravaTexture> ResourceManager::HotLoadTexture(std::string textureName)
{
    // Load texture if not available in textures map
    auto textureInfoIterator = s_TextureInfo.find(textureName);
    auto textureIterator = s_Textures.find(textureName);

    if (textureInfoIterator == s_TextureInfo.end())
        return H2M::RefH2M<MoravaTexture>();

    if (textureIterator != s_Textures.end())
        return textureIterator->second;

    LoadTexture(textureName, textureInfoIterator->second);

    textureIterator = s_Textures.find(textureName);

    if (textureIterator == s_Textures.end())
        return H2M::RefH2M<MoravaTexture>();

    return textureIterator->second;
}

H2M::RefH2M<Material> ResourceManager::HotLoadMaterial(std::string materialName)
{
    // Load Material if not available in materials map
    auto materialInfoIterator = s_MaterialInfo.find(materialName);
    auto materialIterator = s_Materials.find(materialName);

    if (materialInfoIterator == s_MaterialInfo.end()) {
        return H2M::RefH2M<Material>();
    }

    if (materialIterator != s_Materials.end()) {
        return materialIterator->second;
    }

    LoadMaterial(materialInfoIterator->first, materialInfoIterator->second);

    materialIterator = s_Materials.find(materialName);

    if (materialIterator == s_Materials.end()) {
        return H2M::RefH2M<Material>();
    }

    return materialIterator->second;
}

// Every spelling of a file gives the same key: absolute (relative paths are relative to the working directory), with "." and ".."
// resolved, forward slashes, and lowercase (Windows paths are not case-sensitive)
std::string ResourceManager::NormalizeTexturePath(const std::string& filePath)
{
    std::error_code error;
    std::filesystem::path path = std::filesystem::weakly_canonical(std::filesystem::absolute(std::filesystem::path(filePath), error), error);
    std::string normalized = error ? filePath : path.generic_string();

    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return normalized;
}

H2M::RefH2M<H2M::Texture2D_H2M> ResourceManager::LoadTexture2D_H2M(std::string filePath, bool sRGB)
{
#if defined(GRAPHICS_API_VULKAN)
    // A render texture (7.5): its own texture, the same for every color space (its format decides how it is read)
    if (EnvMapVulkanRenderTexture::IsRenderTextureFile(filePath))
    {
        const std::shared_ptr<EnvMapVulkanRenderTexture> renderTexture = EnvMapVulkanRenderTexture::Get(filePath);
        return renderTexture ? renderTexture->GetTexture() : H2M::RefH2M<H2M::Texture2D_H2M>();
    }
#endif

    TextureCacheKey key = { NormalizeTexturePath(filePath), sRGB };

    auto entry = s_HazelTextures2D.find(key);
    if (entry != s_HazelTextures2D.end()) {
        // A cache HIT
        return entry->second;
    }

    // A cache MISS
    H2M::RefH2M<H2M::Texture2D_H2M> texture;
    try
    {
        texture = H2M::Texture2D_H2M::Create(filePath, sRGB);
    }
    catch (...)
    {
        Log::GetLogger()->warn("Failed to create a texture '{0}'!", filePath);
        return H2M::RefH2M<H2M::Texture2D_H2M>();
    }

    // A file that could not be read is not cached, so a later request tries again
    if (texture && texture->Loaded()) {
        s_HazelTextures2D.insert(std::make_pair(key, texture));
        Log::GetLogger()->info("ResourceManager: A texture created and stored in cache [key: '{0}', {1}]", key.Path, sRGB ? "sRGB" : "linear");
    }
    return texture;
}

H2M::RefH2M<H2M::Texture2D_H2M> ResourceManager::LoadMaterialMap(const std::string& filePath, TextureCook::MapKind kind)
{
    const bool sRGB = kind == TextureCook::MapKind::Color;
#if defined(GRAPHICS_API_VULKAN)
    std::string extension = std::filesystem::u8path(filePath).extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    const bool isImage = extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga" || extension == ".bmp";

    // Not on a GPU without these formats (they are optional in Vulkan): such a machine goes on with the source images
    if (isImage && H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::Vulkan &&
        H2M::VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetFeatures().textureCompressionBC)
    {
        const TextureCacheKey key = { NormalizeTexturePath(filePath), sRGB, (int)kind };
        auto entry = s_HazelTextures2D.find(key);
        if (entry != s_HazelTextures2D.end())
        {
            return entry->second;
        }

        TextureCook::CookedTexture cooked;
        double milliseconds = 0.0;
        if (TextureCook::Read(filePath, kind, cooked, milliseconds))
        {
            const auto start = std::chrono::steady_clock::now();
            H2M::RefH2M<H2M::Texture2D_H2M> texture = H2M::RefH2M<H2M::VulkanTexture2D_H2M>::Create(filePath, cooked);
            const double upload = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            s_HazelTextures2D.insert(std::make_pair(key, texture));
            Log::GetLogger()->info("ResourceManager: A cooked texture read [key: '{0}', {1} x {2}, read {3:.1f} ms, upload {4:.1f} ms]", key.Path, cooked.Width, cooked.Height, milliseconds, upload);
            return texture;
        }

        const bool alreadyLoaded = s_HazelTextures2D.count({ key.Path, sRGB }) > 0;
        const auto start = std::chrono::steady_clock::now();
        H2M::RefH2M<H2M::Texture2D_H2M> texture = LoadTexture2D_H2M(filePath, sRGB);
        if (texture && texture->Loaded())
        {
            if (!alreadyLoaded)
            {
                TextureCook::AddSourceLoad(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
            }
            TextureCook::Request(filePath, kind);
        }
        return texture;
    }
#endif
    return LoadTexture2D_H2M(filePath, sRGB);
}

uint32_t ResourceManager::PurgeUnusedTextures2D()
{
    uint32_t released = 0;
    for (auto it = s_HazelTextures2D.begin(); it != s_HazelTextures2D.end();)
    {
        if (it->second->GetRefCount() == 1) // the cache's own reference
        {
            Log::GetLogger()->info("ResourceManager: Texture released from the cache [key: '{0}', {1}]", it->first.Path, it->first.SRGB ? "sRGB" : "linear");
            it = s_HazelTextures2D.erase(it);
            released++;
        }
        else
        {
            ++it;
        }
    }
    return released;
}

void ResourceManager::AddShader(std::string name, H2M::RefH2M<MoravaShader> shader)
{
    if (s_ShaderCacheByTitle.find(name) == s_ShaderCacheByTitle.end()) {
        s_ShaderCacheByTitle.insert(std::make_pair(name, shader));
    }
}

H2M::RefH2M<MoravaShader> ResourceManager::GetShader(std::string name)
{
    auto it = s_ShaderCacheByTitle.find(name);
    if (it != s_ShaderCacheByTitle.end())
        return it->second;

    return {};
}

H2M::RefH2M<MoravaShader> ResourceManager::CreateOrLoadShader(MoravaShaderSpecification spec)
{
    const std::string& key = spec.PixelShaderPath;

    auto it = s_ShadersCacheByFilepath.find(key);
    if (it != s_ShadersCacheByFilepath.end())
    {
        Log::GetLogger()->info(
            "ResourceManager: Shader loaded from cache [key: '{0}']",
            key
        );
        return it->second;
    }

    auto shader = MoravaShader::Create(spec);
    s_ShadersCacheByFilepath[key] = shader;

    Log::GetLogger()->info(
        "ResourceManager: Shader created and cached [key: '{0}']",
        key
    );

    return shader;
}
