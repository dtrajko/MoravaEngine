#include "EnvMapSharedData.h"

#include "HazelLegacy/Scene/EntityHazelLegacy.h"


std::map<std::string, unsigned int> EnvMapSharedData::s_SamplerSlots;

Hazel::Ref<Hazel::SceneHazelLegacy> EnvMapSharedData::s_RuntimeScene;
Hazel::Ref<Hazel::SceneHazelLegacy> EnvMapSharedData::s_EditorScene;

Scene* EnvMapSharedData::s_Scene; // reference to SceneHazelEnvMap (the original engine scene object)

Hazel::EditorCamera* EnvMapSharedData::s_EditorCamera;
RuntimeCamera* EnvMapSharedData::s_RuntimeCamera;
Hazel::HazelCamera* EnvMapSharedData::s_ActiveCamera;

Hazel::Ref<CubeSkybox> EnvMapSharedData::s_SkyboxCube;
Hazel::Ref<Quad> EnvMapSharedData::s_Quad;

bool EnvMapSharedData::s_DisplayOutline;
bool EnvMapSharedData::s_DisplayWireframe;
float EnvMapSharedData::s_SkyboxExposureFactor;
bool EnvMapSharedData::s_RadiancePrefilter;
float EnvMapSharedData::s_EnvMapRotation;
glm::mat4 EnvMapSharedData::s_DirLightTransform;
bool EnvMapSharedData::s_DisplayHazelGrid;
bool EnvMapSharedData::s_DisplayRay;
glm::vec3 EnvMapSharedData::s_NewRay;
Hazel::Ref<ShadowMap> EnvMapSharedData::s_ShadowMapDirLight;

Hazel::Ref<MoravaShader> EnvMapSharedData::s_ShaderHazelPBR;
Hazel::Ref<MoravaShader> EnvMapSharedData::s_ShaderOutline;

Hazel::EntityHazelLegacy EnvMapSharedData::s_PointLightEntity;
Hazel::Ref<OmniShadowMap> EnvMapSharedData::s_OmniShadowMapPointLight;

Hazel::EntityHazelLegacy EnvMapSharedData::s_SpotLightEntity;
Hazel::Ref<OmniShadowMap> EnvMapSharedData::s_OmniShadowMapSpotLight;
