#include "Core/ResourceManager.h"

#include "Texture/TextureLoader.h"


std::map<std::string, std::string> ResourceManager::s_TextureInfo;
std::map<std::string, TextureInfo> ResourceManager::s_MaterialInfo;

std::map<std::string, Hazel::Ref<MoravaTexture>> ResourceManager::s_Textures;
std::map<std::string, Hazel::Ref<Material>> ResourceManager::s_Materials;

float ResourceManager::s_MaterialSpecular = 1.0f;
float ResourceManager::s_MaterialShininess = 256.0f;

std::map<std::string, Hazel::Ref<Hazel::HazelTexture2D>> ResourceManager::s_HazelTextures2D;

std::map<std::string, Hazel::Ref<MoravaShader>> ResourceManager::s_ShaderCacheByTitle;

std::map<std::string, Hazel::Ref<MoravaShader>> ResourceManager::s_ShadersCacheByFilepath;


void ResourceManager::Init()
{
    // Setup Textures
    s_TextureInfo.insert(std::make_pair("none", "Textures/plain.png"));
    s_TextureInfo.insert(std::make_pair("semi_transparent", "Textures/semi_transparent.png"));
    s_TextureInfo.insert(std::make_pair("texture_checker", "Textures/texture_checker.png"));
    s_TextureInfo.insert(std::make_pair("wood", "Textures/wood.png"));
    s_TextureInfo.insert(std::make_pair("container2", "Textures/container/container2.png"));
    s_TextureInfo.insert(std::make_pair("plank", "Textures/texture_plank.png"));
    s_TextureInfo.insert(std::make_pair("rock", "Textures/rock.png"));
    s_TextureInfo.insert(std::make_pair("pyramid", "Textures/pyramid.png"));
    s_TextureInfo.insert(std::make_pair("lego", "Textures/lego.png"));
    s_TextureInfo.insert(std::make_pair("marble", "Textures/marble.jpg"));
    s_TextureInfo.insert(std::make_pair("metal", "Textures/metal.png"));
    s_TextureInfo.insert(std::make_pair("brick", "Textures/brick.png"));
    s_TextureInfo.insert(std::make_pair("crate", "Textures/crate.png"));
    s_TextureInfo.insert(std::make_pair("grass", "Textures/grass.jpg"));
    s_TextureInfo.insert(std::make_pair("water", "Textures/water.png"));
    s_TextureInfo.insert(std::make_pair("rock2", "Textures/rock/Rock-Texture-Surface.jpg"));
    s_TextureInfo.insert(std::make_pair("planet", "Textures/planet/planet_Quom1200.png"));
    s_TextureInfo.insert(std::make_pair("gold_albedo", "Textures/PBR/gold/albedo.png"));
    s_TextureInfo.insert(std::make_pair("silver_albedo", "Textures/PBR/silver/albedo.png"));
    s_TextureInfo.insert(std::make_pair("rusted_iron", "Textures/PBR/rusted_iron/albedo.png"));
    s_TextureInfo.insert(std::make_pair("grass_albedo", "Textures/PBR/grass/albedo.png"));
    s_TextureInfo.insert(std::make_pair("wall_albedo", "Textures/PBR/wall/albedo.png"));
    s_TextureInfo.insert(std::make_pair("plastic_albedo", "Textures/PBR/plastic/albedo.png"));
    s_TextureInfo.insert(std::make_pair("wal67ar_small", "Textures/OGLdev/buddha/wal67ar_small.jpg"));
    s_TextureInfo.insert(std::make_pair("wal69ar_small", "Textures/OGLdev/buddha/wal69ar_small.jpg"));
    s_TextureInfo.insert(std::make_pair("hheli", "Textures/OGLdev/hheli/hheli.bmp"));
    s_TextureInfo.insert(std::make_pair("jeep_army", "Textures/OGLdev/jeep/jeep_army.jpg"));
    s_TextureInfo.insert(std::make_pair("jeep_rood", "Textures/OGLdev/jeep/jeep_rood.jpg"));
    s_TextureInfo.insert(std::make_pair("pine", "Textures/ThinMatrix/pine.png"));
    s_TextureInfo.insert(std::make_pair("terrain_ground", "Textures/terrain_ground.jpg"));
    s_TextureInfo.insert(std::make_pair("boulder", "Textures/ThinMatrix/boulder.png"));
    s_TextureInfo.insert(std::make_pair("fire", "Textures/Particles/fire.png"));
    s_TextureInfo.insert(std::make_pair("fog", "Textures/Particles/fog.png"));
    s_TextureInfo.insert(std::make_pair("snowflake", "Textures/Particles/snowflake.png"));
    s_TextureInfo.insert(std::make_pair("particle_atlas", "Textures/ThinMatrix/particles/particleAtlas.png"));
    s_TextureInfo.insert(std::make_pair("particle_atlas_cosmic", "Textures/ThinMatrix/particles/cosmic.png"));
    s_TextureInfo.insert(std::make_pair("particle_atlas_fire", "Textures/ThinMatrix/particles/fire.png"));
    s_TextureInfo.insert(std::make_pair("particle_atlas_smoke", "Textures/ThinMatrix/particles/smoke.png"));
    s_TextureInfo.insert(std::make_pair("waterNormal", "Textures/water/waterNormal.png"));
    s_TextureInfo.insert(std::make_pair("waterDuDv", "Textures/water/waterDuDv.png"));
    s_TextureInfo.insert(std::make_pair("Checkerboard", "Textures/Hazel/Checkerboard.tga"));

    // Setup Materials

    // none (placeholder)
    TextureInfo textureInfoNone = {};
    textureInfoNone.albedo = "Textures/plain.png";
    textureInfoNone.normal = "Textures/normal_map_default.png";
    textureInfoNone.metallic = "Textures/plain.png";
    textureInfoNone.roughness = "Textures/plain.png";
    textureInfoNone.ao = "Textures/plain.png";
    s_MaterialInfo.insert(std::make_pair("none", textureInfoNone));

    // gold
    TextureInfo textureInfoGold = {};
    textureInfoGold.albedo = "Textures/PBR/gold/albedo.png";
    textureInfoGold.normal = "Textures/PBR/gold/normal.png";
    textureInfoGold.metallic = "Textures/PBR/gold/metallic.png";
    textureInfoGold.roughness = "Textures/PBR/gold/roughness.png";
    textureInfoGold.ao = "Textures/PBR/gold/ao.png";
    s_MaterialInfo.insert(std::make_pair("gold", textureInfoGold));

    // silver
    TextureInfo textureInfoSilver = {};
    textureInfoSilver.albedo = "Textures/PBR/silver/albedo.png";
    textureInfoSilver.normal = "Textures/PBR/silver/normal.png";
    textureInfoSilver.metallic = "Textures/PBR/silver/metallic.png";
    textureInfoSilver.roughness = "Textures/PBR/silver/roughness.png";
    textureInfoSilver.ao = "Textures/PBR/silver/ao.png";
    s_MaterialInfo.insert(std::make_pair("silver", textureInfoSilver));

    // rusted iron
    TextureInfo textureInfoRustedIron = {};
    textureInfoRustedIron.albedo = "Textures/PBR/rusted_iron/albedo.png";
    textureInfoRustedIron.normal = "Textures/PBR/rusted_iron/normal.png";
    textureInfoRustedIron.metallic = "Textures/PBR/rusted_iron/metallic.png";
    textureInfoRustedIron.roughness = "Textures/PBR/rusted_iron/roughness.png";
    textureInfoRustedIron.ao = "Textures/PBR/rusted_iron/ao.png";
    s_MaterialInfo.insert(std::make_pair("rusted_iron", textureInfoRustedIron));

    // plastic
    TextureInfo textureInfoPlastic = {};
    textureInfoPlastic.albedo = "Textures/PBR/plastic/albedo.png";
    textureInfoPlastic.normal = "Textures/PBR/plastic/normal.png";
    textureInfoPlastic.metallic = "Textures/PBR/plastic/metallic.png";
    textureInfoPlastic.roughness = "Textures/PBR/plastic/roughness.png";
    textureInfoPlastic.ao = "Textures/PBR/plastic/ao.png";
    s_MaterialInfo.insert(std::make_pair("plastic", textureInfoPlastic));

    // futur_panel
    TextureInfo textureInfoFuturPanel = {};
    textureInfoFuturPanel.albedo = "Textures/PBR/futuristic_panel_1/futuristic-panels1-albedo.png";
    textureInfoFuturPanel.normal = "Textures/PBR/futuristic_panel_1/futuristic-panels1-normal-dx.png";
    textureInfoFuturPanel.metallic = "Textures/PBR/futuristic_panel_1/futuristic-panels1-metallic.png";
    textureInfoFuturPanel.roughness = "Textures/PBR/futuristic_panel_1/futuristic-panels1-roughness.png";
    textureInfoFuturPanel.ao = "Textures/PBR/futuristic_panel_1/futuristic-panels1-ao.png";
    s_MaterialInfo.insert(std::make_pair("futur_panel", textureInfoFuturPanel));

    // dark tiles
    TextureInfo textureInfoDarkTiles = {};
    textureInfoDarkTiles.albedo = "Textures/PBR/dark_tiles_1/darktiles1_basecolor.png";
    textureInfoDarkTiles.normal = "Textures/PBR/dark_tiles_1/darktiles1_normal-DX.png";
    textureInfoDarkTiles.metallic = "Textures/PBR/metalness.png";
    textureInfoDarkTiles.roughness = "Textures/PBR/dark_tiles_1/darktiles1_roughness.png";
    textureInfoDarkTiles.ao = "Textures/PBR/dark_tiles_1/darktiles1_AO.png";
    s_MaterialInfo.insert(std::make_pair("dark_tiles", textureInfoDarkTiles));

    // mahogany floor
    TextureInfo textureInfoMahoganyFloor = {};
    textureInfoMahoganyFloor.albedo = "Textures/PBR/mahogany_floor/mahogfloor_basecolor.png";
    textureInfoMahoganyFloor.normal = "Textures/PBR/mahogany_floor/mahogfloor_normal.png";
    textureInfoMahoganyFloor.metallic = "Textures/PBR/mahogany_floor/mahogfloor_metalness.png";
    textureInfoMahoganyFloor.roughness = "Textures/PBR/mahogany_floor/mahogfloor_roughness.png";
    textureInfoMahoganyFloor.ao = "Textures/PBR/mahogany_floor/mahogfloor_AO.png";
    s_MaterialInfo.insert(std::make_pair("mahogany_floor", textureInfoMahoganyFloor));

    // aged planks
    TextureInfo textureInfoAgedPlanks = {};
    textureInfoAgedPlanks.albedo = "Textures/PBR/aged_planks_1/agedplanks1-albedo.png";
    textureInfoAgedPlanks.normal = "Textures/PBR/aged_planks_1/agedplanks1-normal4-ue.png";
    textureInfoAgedPlanks.metallic = "Textures/PBR/aged_planks_1/agedplanks1-metalness.png";
    textureInfoAgedPlanks.roughness = "Textures/PBR/aged_planks_1/agedplanks1-roughness.png";
    textureInfoAgedPlanks.ao = "Textures/PBR/aged_planks_1/agedplanks1-ao.png";
    s_MaterialInfo.insert(std::make_pair("aged_planks", textureInfoAgedPlanks));

    // harsh bricks
    TextureInfo textureInfoHarshBricks = {};
    textureInfoHarshBricks.albedo = "Textures/PBR/harsh_bricks/harshbricks-albedo.png";
    textureInfoHarshBricks.normal = "Textures/PBR/harsh_bricks/harshbricks-normal.png";
    textureInfoHarshBricks.metallic = "Textures/PBR/metalness.png";
    textureInfoHarshBricks.roughness = "Textures/PBR/harsh_bricks/harshbricks-roughness.png";
    textureInfoHarshBricks.ao = "Textures/PBR/harsh_bricks/harshbricks-ao2.png";
    s_MaterialInfo.insert(std::make_pair("harsh_bricks", textureInfoHarshBricks));

    // Stone Carved (Quixel Megascans)
    TextureInfo textureInfoStoneCarved = {};
    textureInfoStoneCarved.albedo = "Textures/PBR/Stone_Carved/Albedo.jpg";
    textureInfoStoneCarved.normal = "Textures/PBR/Stone_Carved/Normal_LOD0.jpg";
    textureInfoStoneCarved.metallic = "Textures/PBR/Stone_Carved/Metalness.jpg";
    textureInfoStoneCarved.roughness = "Textures/PBR/Stone_Carved/Roughness.jpg";
    textureInfoStoneCarved.ao = "Textures/PBR/Stone_Carved/Displacement.jpg";
    s_MaterialInfo.insert(std::make_pair("stone_carved", textureInfoStoneCarved));

    // Old Stove (Quixel Megascans)
    TextureInfo textureInfoOldStove = {};
    textureInfoOldStove.albedo = "Textures/PBR/Old_Stove/Albedo.jpg";
    textureInfoOldStove.normal = "Textures/PBR/Old_Stove/Normal_LOD0.jpg";
    textureInfoOldStove.metallic = "Textures/PBR/Old_Stove/Metalness.jpg";
    textureInfoOldStove.roughness = "Textures/PBR/Old_Stove/Roughness.jpg";
    textureInfoOldStove.ao = "Textures/PBR/Old_Stove/Displacement.jpg";
    s_MaterialInfo.insert(std::make_pair("old_stove", textureInfoOldStove));

    // Animated Character (Sebastian Lague / ThinMatrix)
    TextureInfo textureInfoAnimBoy = {};
    textureInfoAnimBoy.albedo    = "Models/ThinMatrix/AnimatedCharacter/AnimatedCharacterDiffuse.png";
    textureInfoAnimBoy.normal    = "Textures/PBR/plastic/normal.png";
    textureInfoAnimBoy.metallic  = "Textures/PBR/plastic/metallic.png";
    textureInfoAnimBoy.roughness = "Textures/PBR/plastic/roughness.png";
    textureInfoAnimBoy.ao        = "Textures/PBR/plastic/ao.png";
    s_MaterialInfo.insert(std::make_pair("anim_boy", textureInfoAnimBoy));

    // Buddha
    TextureInfo textureInfoBuddha = {};
    textureInfoBuddha.albedo = "Textures/OGLdev/buddha/wal67ar_small.jpg";
    textureInfoBuddha.normal = "Textures/PBR/silver/normal.png";
    textureInfoBuddha.metallic = "Textures/PBR/silver/metallic.png";
    textureInfoBuddha.roughness = "Textures/PBR/silver/roughness.png";
    textureInfoBuddha.ao = "Textures/PBR/silver/ao.png";
    s_MaterialInfo.insert(std::make_pair("buddha", textureInfoBuddha));

    // Damaged Helmet glTF PBR
    TextureInfo textureInfoDamagedHelmet = {};
    textureInfoDamagedHelmet.albedo = "Textures/PBR/DamagedHelmet/Default_albedo.jpg";
    textureInfoDamagedHelmet.normal = "Textures/PBR/DamagedHelmet/Default_normal.jpg";
    textureInfoDamagedHelmet.metallic = "Textures/PBR/DamagedHelmet/Default_metalRoughness.jpg";
    textureInfoDamagedHelmet.roughness = "Textures/PBR/DamagedHelmet/Default_emissive.jpg";
    textureInfoDamagedHelmet.ao = "Textures/PBR/DamagedHelmet/Default_AO.jpg";
    s_MaterialInfo.insert(std::make_pair("damaged_helmet", textureInfoDamagedHelmet));

    // SF Helmet glTF PBR
    TextureInfo textureInfoSFHelmet = {};
    textureInfoSFHelmet.albedo    = "Textures/PBR/SciFiHelmet/SciFiHelmet_BaseColor.png";
    textureInfoSFHelmet.normal    = "Textures/PBR/SciFiHelmet/SciFiHelmet_Normal.png";
    textureInfoSFHelmet.metallic  = "Textures/PBR/SciFiHelmet/SciFiHelmet_MetallicRoughness.png";
    textureInfoSFHelmet.roughness = "Textures/PBR/SciFiHelmet/SciFiHelmet_Emissive.png";
    textureInfoSFHelmet.ao        = "Textures/PBR/SciFiHelmet/SciFiHelmet_AmbientOcclusion.png";
    s_MaterialInfo.insert(std::make_pair("sf_helmet", textureInfoSFHelmet));

    // Cerberus model PBR textures
    TextureInfo textureInfoCerberus = {};
    textureInfoCerberus.albedo    = "Models/Cerberus/Textures/Cerberus_A.tga";
    textureInfoCerberus.normal    = "Models/Cerberus/Textures/Cerberus_N.tga";
    textureInfoCerberus.metallic  = "Models/Cerberus/Textures/Cerberus_M.tga";
    textureInfoCerberus.roughness = "Models/Cerberus/Textures/Cerberus_R.tga";
    textureInfoCerberus.ao        = "Models/Cerberus/Textures/Cerberus_AO.tga";
    s_MaterialInfo.insert(std::make_pair("cerberus", textureInfoCerberus));

    // Concrete 3 Free PBR Materials freepbr.com
    TextureInfo textureInfoConcrete = {};
    textureInfoConcrete.albedo    = "Textures/PBR/concrete3/concrete3-albedo.png";
    textureInfoConcrete.normal    = "Textures/PBR/concrete3/concrete3-Normal-dx.png";
    textureInfoConcrete.metallic  = "Textures/PBR/concrete3/concrete3-Metallic.png";
    textureInfoConcrete.roughness = "Textures/PBR/concrete3/concrete3-Roughness.png";
    textureInfoConcrete.ao        = "Textures/PBR/concrete3/concrete3-ao.png";
    s_MaterialInfo.insert(std::make_pair("concrete", textureInfoConcrete));

    // Modern Brick Wall 1 Free PBR Materials freepbr.com
    TextureInfo textureInfoModernBrickWall = {};
    textureInfoModernBrickWall.albedo    = "Textures/PBR/modern_brick_1/modern-brick1_albedo.png";
    textureInfoModernBrickWall.normal    = "Textures/PBR/modern_brick_1/modern-brick1_normal-dx.png";
    textureInfoModernBrickWall.metallic  = "Textures/PBR/modern_brick_1/modern-brick1_metallic.png";
    textureInfoModernBrickWall.roughness = "Textures/PBR/modern_brick_1/modern-brick1_roughness.png";
    textureInfoModernBrickWall.ao        = "Textures/PBR/modern_brick_1/modern-brick1_ao.png";
    s_MaterialInfo.insert(std::make_pair("modern_brick_wall", textureInfoModernBrickWall));

    // ThinMatrix Boulder
    TextureInfo textureInfoBoulder = {};
    textureInfoBoulder.albedo    = "Textures/ThinMatrix/boulder.png";
    textureInfoBoulder.normal    = "Textures/ThinMatrix/boulderNormal.png";
    textureInfoBoulder.metallic  = "Textures/metalness.png";
    textureInfoBoulder.roughness = "Textures/plain.png";
    textureInfoBoulder.ao        = "Textures/plain.png";
    s_MaterialInfo.insert(std::make_pair("boulder", textureInfoBoulder));

    // M1911
    TextureInfo textureInfoM1911 = {};
    textureInfoM1911.albedo    = "Models/M1911/m1911_color.png";
    textureInfoM1911.normal    = "Models/M1911/m1911_normal.png";
    textureInfoM1911.metallic  = "Models/M1911/m1911_metalness.png";
    textureInfoM1911.roughness = "Models/M1911/m1911_roughness.png";
    textureInfoM1911.ao        = "Textures/plain.png";
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
    s_Materials.insert(std::make_pair(name, Hazel::Ref<Material>::Create(textureInfo, s_MaterialSpecular, s_MaterialShininess)));
}

Hazel::Ref<MoravaTexture> ResourceManager::HotLoadTexture(std::string textureName)
{
    // Load texture if not available in textures map
    auto textureInfoIterator = s_TextureInfo.find(textureName);
    auto textureIterator = s_Textures.find(textureName);

    if (textureInfoIterator == s_TextureInfo.end())
        return Hazel::Ref<MoravaTexture>();

    if (textureIterator != s_Textures.end())
        return textureIterator->second;

    LoadTexture(textureName, textureInfoIterator->second);

    textureIterator = s_Textures.find(textureName);

    if (textureIterator == s_Textures.end())
        return Hazel::Ref<MoravaTexture>();

    return textureIterator->second;
}

Hazel::Ref<Material> ResourceManager::HotLoadMaterial(std::string materialName)
{
    // Load Material if not available in materials map
    auto materialInfoIterator = s_MaterialInfo.find(materialName);
    auto materialIterator = s_Materials.find(materialName);

    if (materialInfoIterator == s_MaterialInfo.end()) {
        return Hazel::Ref<Material>();
    }

    if (materialIterator != s_Materials.end()) {
        return materialIterator->second;
    }

    LoadMaterial(materialInfoIterator->first, materialInfoIterator->second);

    materialIterator = s_Materials.find(materialName);

    if (materialIterator == s_Materials.end()) {
        return Hazel::Ref<Material>();
    }

    return materialIterator->second;
}

Hazel::Ref<Hazel::HazelTexture2D> ResourceManager::LoadHazelTexture2D(std::string filePath)
{
    Hazel::Ref<Hazel::HazelTexture2D> texture;

    std::map<std::string, Hazel::Ref<Hazel::HazelTexture2D>>::iterator entry = s_HazelTextures2D.find(filePath);
    if (entry != s_HazelTextures2D.end()) {
        // A cache HIT
        texture = entry->second;
        // Log::GetLogger()->info("ResourceManager: A texture loaded from the cache [key: '{0}']", filePath);
    }
    else {
        // A cache MISS
        try
        {
            texture = Hazel::HazelTexture2D::Create(filePath);
            s_HazelTextures2D.insert(std::make_pair(filePath, texture));
            Log::GetLogger()->info("ResourceManager: A texture created and stored in cache [key: '{0}']", filePath);
        }
        catch (...)
        {
            Log::GetLogger()->warn("Failed to create a texture '{0}'!", filePath);
        }
    }

    return texture;
}

void ResourceManager::AddShader(std::string name, Hazel::Ref<MoravaShader> shader)
{
    if (s_ShaderCacheByTitle.find(name) == s_ShaderCacheByTitle.end()) {
        s_ShaderCacheByTitle.insert(std::make_pair(name, shader));
    }
}

const Hazel::Ref<MoravaShader>& ResourceManager::GetShader(std::string name)
{
    if (s_ShaderCacheByTitle.find(name) != s_ShaderCacheByTitle.end()) {
        return s_ShaderCacheByTitle.find(name)->second;
    }
    return Hazel::Ref<MoravaShader>();
}

const Hazel::Ref<MoravaShader>& ResourceManager::CreateOrLoadShader(MoravaShaderSpecification moravaShaderSpecification)
{
    Hazel::Ref<MoravaShader> moravaShader;

    std::string pixelShaderPath = moravaShaderSpecification.PixelShaderPath;
    std::map<std::string, Hazel::Ref<MoravaShader>>::iterator entry = s_ShadersCacheByFilepath.find(pixelShaderPath);
    if (entry != s_ShadersCacheByFilepath.end()) {
        // A cache HIT
        moravaShader = entry->second;
        Log::GetLogger()->info("ResourceManager: A shader loaded from the cache [key: '{0}']", pixelShaderPath);
    }
    else {
        // A cache MISS
        moravaShader = MoravaShader::Create(moravaShaderSpecification);
        s_ShadersCacheByFilepath.insert(std::make_pair(pixelShaderPath, moravaShader));
        Log::GetLogger()->info("ResourceManager: A shader created and stored in cache [key: '{0}']", pixelShaderPath);
    }

    return moravaShader;
}
