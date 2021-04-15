#include "EnvMapSharedData.h"


std::map<std::string, unsigned int> EnvMapSharedData::s_SamplerSlots;

Hazel::Ref<Hazel::HazelScene> EnvMapSharedData::s_RuntimeScene;
Hazel::Ref<Hazel::HazelScene> EnvMapSharedData::s_EditorScene;
Hazel::EditorCamera* EnvMapSharedData::s_EditorCamera;
RuntimeCamera* EnvMapSharedData::s_RuntimeCamera;
Hazel::HazelCamera* EnvMapSharedData::s_ActiveCamera;

Hazel::Ref<CubeSkybox> EnvMapSharedData::s_SkyboxCube;
Hazel::Ref<Quad> EnvMapSharedData::s_Quad;

bool EnvMapSharedData::s_DisplayOutline;
float EnvMapSharedData::s_SkyboxExposureFactor;
bool EnvMapSharedData::s_RadiancePrefilter;
float EnvMapSharedData::s_EnvMapRotation;
glm::mat4 EnvMapSharedData::s_DirLightTransform;
bool EnvMapSharedData::s_DisplayHazelGrid;
bool EnvMapSharedData::s_DisplayRay;
glm::vec3 EnvMapSharedData::s_NewRay;
Hazel::Ref<ShadowMap> EnvMapSharedData::s_ShadowMapDirLight;

Hazel::Ref<Shader> EnvMapSharedData::s_ShaderHazelPBR;
Hazel::Ref<Shader> EnvMapSharedData::s_ShaderOutline;

Hazel::Entity EnvMapSharedData::s_PointLightEntity;
Hazel::Ref<OmniShadowMap> EnvMapSharedData::s_OmniShadowMapPointLight;

Hazel::Entity EnvMapSharedData::s_SpotLightEntity;
Hazel::Ref<OmniShadowMap> EnvMapSharedData::s_OmniShadowMapSpotLight;
