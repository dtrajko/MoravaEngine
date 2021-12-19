#include "EnvMapSharedData.h"


std::map<std::string, unsigned int> EnvMapSharedData::s_SamplerSlots;

Scene* EnvMapSharedData::s_Scene; // reference to SceneHazelEnvMap (the original engine scene object)

H2M::RefH2M<CubeSkybox> EnvMapSharedData::s_SkyboxCube;
H2M::RefH2M<Quad> EnvMapSharedData::s_Quad;

bool EnvMapSharedData::s_DisplayOutline;
bool EnvMapSharedData::s_DisplayWireframe;
float EnvMapSharedData::s_SkyboxExposureFactor;
bool EnvMapSharedData::s_RadiancePrefilter;
float EnvMapSharedData::s_EnvMapRotation;
glm::mat4 EnvMapSharedData::s_DirLightTransform;
bool EnvMapSharedData::s_DisplayHazelGrid;
bool EnvMapSharedData::s_DisplayRay;
glm::vec3 EnvMapSharedData::s_NewRay;
H2M::RefH2M<ShadowMap> EnvMapSharedData::s_ShadowMapDirLight;

H2M::RefH2M<MoravaShader> EnvMapSharedData::s_ShaderHazelPBR;
H2M::RefH2M<MoravaShader> EnvMapSharedData::s_ShaderOutline;

// H2M::EntityH2M EnvMapSharedData::s_PointLightEntity;
H2M::RefH2M<OmniShadowMap> EnvMapSharedData::s_OmniShadowMapPointLight;

// H2M::EntityH2M EnvMapSharedData::s_SpotLightEntity;
H2M::RefH2M<OmniShadowMap> EnvMapSharedData::s_OmniShadowMapSpotLight;
