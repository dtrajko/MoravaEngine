#include "EnvironmentMap.h"
#include "Log.h"


EnvironmentMap::EnvironmentMap(const std::string& filepath)
{
    m_SamplerSlots = new std::map<std::string, unsigned int>();

    //  // PBR texture inputs
    m_SamplerSlots->insert(std::make_pair("albedo",     1)); // uniform sampler2D u_AlbedoTexture
    m_SamplerSlots->insert(std::make_pair("normal",     2)); // uniform sampler2D u_NormalTexture
    m_SamplerSlots->insert(std::make_pair("metalness",  3)); // uniform sampler2D u_MetalnessTexture
    m_SamplerSlots->insert(std::make_pair("roughness",  4)); // uniform sampler2D u_RoughnessTexture
    // m_SamplerSlots.insert(std::make_pair("ao", 5)); // uniform sampler2D u_AOTexture
    // Environment maps
    m_SamplerSlots->insert(std::make_pair("radiance",   5)); // uniform samplerCube u_EnvRadianceTex
    m_SamplerSlots->insert(std::make_pair("irradiance", 6)); // uniform samplerCube u_EnvIrradianceTex
    // BRDF LUT
    m_SamplerSlots->insert(std::make_pair("BRDF_LUT",   7)); // uniform sampler2D u_BRDFLUTTexture

    SetupShaders();

    m_Environment = Load(filepath);

    m_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");

    // Set lights
    m_Light.Direction = { -0.5f, -0.5f, 1.0f };
    m_Light.Radiance = { 1.0f, 1.0f, 1.0f };
}

Environment EnvironmentMap::Load(const std::string& filepath)
{
    auto [radiance, irradiance] = CreateEnvironmentMap(filepath);
    return { radiance, irradiance };
}

void EnvironmentMap::SetEnvironment(Environment environment)
{
    m_Environment = environment;
    SetSkybox(environment.RadianceMap);
}

std::pair<TextureCubemap*, TextureCubemap*> EnvironmentMap::CreateEnvironmentMap(const std::string& filepath)
{
    return std::pair<TextureCubemap*, TextureCubemap*>();
}

void EnvironmentMap::SetSkybox(TextureCubemap* skybox)
{
    m_SkyboxTexture = skybox;
    m_ShaderSkybox->setInt("u_Texture", skybox->GetID());
}

EnvironmentMap::~EnvironmentMap()
{
}

void EnvironmentMap::SetupShaders()
{
    m_ShaderEquirectangularConversion = new Shader("Shaders/Hazel/EquirectangularToCubeMap.cs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderEquirectangularConversion compiled [programID={0}]", m_ShaderEquirectangularConversion->GetProgramID());

    m_ShaderEnvFiltering = new Shader("Shaders/Hazel/EnvironmentMipFilter.cs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderEnvFiltering compiled [programID={0}]", m_ShaderEnvFiltering->GetProgramID());

    m_ShaderEnvIrradiance = new Shader("Shaders/Hazel/EnvironmentIrradiance.cs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderEnvIrradiance compiled [programID={0}]", m_ShaderEnvIrradiance->GetProgramID());

    m_ShaderSkybox = new Shader("Shaders/Hazel/Skybox.vs", "Shaders/Hazel/Skybox.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderSkybox compiled [programID={0}]", m_ShaderSkybox->GetProgramID());

    m_ShaderHazelAnimPBR = new Shader("Shaders/Hazel/HazelPBR_Anim.vs", "Shaders/Hazel/HazelPBR_Anim.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderHazelAnimPBR compiled [programID={0}]", m_ShaderHazelAnimPBR->GetProgramID());
}

void EnvironmentMap::UpdateUniforms()
{
    /**** BEGIN HazelPBR_Anim ***/
    m_ShaderHazelAnimPBR->Bind();

    m_ShaderHazelAnimPBR->setVec3("lights.Direction", m_Light.Direction);
    m_ShaderHazelAnimPBR->setVec3("lights.Radiance", m_Light.Radiance);

    m_ShaderHazelAnimPBR->setInt("u_AlbedoTexture", m_SamplerSlots->at("albedo"));
    m_ShaderHazelAnimPBR->setInt("u_NormalTexture", m_SamplerSlots->at("normal"));
    m_ShaderHazelAnimPBR->setInt("u_MetalnessTexture", m_SamplerSlots->at("metalness"));
    m_ShaderHazelAnimPBR->setInt("u_RoughnessTexture", m_SamplerSlots->at("roughness"));
    // m_ShaderHazelAnimPBR->setInt("u_AOTexture", m_SamplerSlots->at("ao"));

    m_ShaderHazelAnimPBR->setInt("u_EnvRadianceTex", m_SamplerSlots->at("radiance"));
    m_ShaderHazelAnimPBR->setInt("u_EnvIrradianceTex", m_SamplerSlots->at("irradiance"));

    m_ShaderHazelAnimPBR->setInt("u_BRDFLUTTexture", m_SamplerSlots->at("BRDF_LUT"));

    m_ShaderHazelAnimPBR->setVec3("u_AlbedoColor", m_AlbedoInput.Color);
    m_ShaderHazelAnimPBR->setFloat("u_Metalness", m_MetalnessInput.Value);
    m_ShaderHazelAnimPBR->setFloat("u_Roughness", m_RoughnessInput.Value);

    m_ShaderHazelAnimPBR->setFloat("u_EnvMapRotation", m_EnvMapRotation);

    m_ShaderHazelAnimPBR->setFloat("u_RadiancePrefilter", m_RadiancePrefilter ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_AlbedoTexToggle", m_AlbedoInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_NormalTexToggle", m_NormalInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_MetalnessTexToggle", m_MetalnessInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_RoughnessTexToggle", m_RoughnessInput.UseTexture ? 1.0f : 0.0f);
    /**** END HazelPBR_Anim ***/
}

void EnvironmentMap::Update()
{
    UpdateUniforms();
}
