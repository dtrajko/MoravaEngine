#include "EnvironmentMap.h"
#include "Log.h"


EnvironmentMap::EnvironmentMap(const std::string& filepath)
{
    SetupShaders();
    Load(filepath);

    m_CheckerboardTex = new Texture("Textures/Hazel/Checkerboard.tga", false, false, GL_LINEAR);

    // TextureCubemap* environmentRadianceMap = new TextureCubemap("Textures/Hazel/Environments/Arches_E_PineTree_Radiance.tga");
    // TextureCubemap* environmentIrradianceMap = new TextureCubemap("Textures/Hazel/Environments/Arches_E_PineTree_Irradiance.tga");

    // Set lights
    m_Lights.Direction = { -0.5f, -0.5f, 1.0f };
    m_Lights.Radiance = { 1.0f, 1.0f, 1.0f };
}

Environment EnvironmentMap::Load(const std::string& filepath)
{
    auto [radiance, irradiance] = CreateEnvironmentMap(filepath);
    return { radiance, irradiance };
}

void EnvironmentMap::SetEnvironment(Environment* environment)
{
    m_Environment = environment;
    SetSkybox(environment->RadianceMap);
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

    m_ShaderHazelAnimPBR->setVec3("u_AlbedoColor", m_AlbedoInput.Color);
    m_ShaderHazelAnimPBR->setFloat("u_Metalness", m_MetalnessInput.Value);
    m_ShaderHazelAnimPBR->setFloat("u_Roughness", m_RoughnessInput.Value);
    m_ShaderHazelAnimPBR->setVec3("lights.Direction", m_Lights.Direction);
    m_ShaderHazelAnimPBR->setVec3("lights.Radiance", m_Lights.Radiance);
    m_ShaderHazelAnimPBR->setFloat("u_RadiancePrefilter", m_RadiancePrefilter ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_AlbedoTexToggle", m_AlbedoInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_NormalTexToggle", m_NormalInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_MetalnessTexToggle", m_MetalnessInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_RoughnessTexToggle", m_RoughnessInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_EnvMapRotation", m_EnvMapRotation);

}

std::pair<TextureCubemap*, TextureCubemap*> EnvironmentMap::CreateEnvironmentMap(const std::string& filepath)
{
	return std::pair<TextureCubemap*, TextureCubemap*>();
}
