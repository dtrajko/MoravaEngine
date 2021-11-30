#include "EnvMapSharedData.h"


std::map<std::string, unsigned int> EnvMapSharedData::s_SamplerSlots;

H2M::Ref<H2M::SceneH2M> EnvMapSharedData::s_RuntimeScene;
H2M::Ref<H2M::SceneH2M> EnvMapSharedData::s_EditorScene;

Scene* EnvMapSharedData::s_Scene; // reference to SceneHazelEnvMap (the original engine scene object)

H2M::EditorCamera* EnvMapSharedData::s_EditorCamera;
RuntimeCamera* EnvMapSharedData::s_RuntimeCamera;
H2M::HazelCamera* EnvMapSharedData::s_ActiveCamera;

H2M::Ref<CubeSkybox> EnvMapSharedData::s_SkyboxCube;
H2M::Ref<Quad> EnvMapSharedData::s_Quad;

bool EnvMapSharedData::s_DisplayOutline;
bool EnvMapSharedData::s_DisplayWireframe;
float EnvMapSharedData::s_SkyboxExposureFactor;
bool EnvMapSharedData::s_RadiancePrefilter;
float EnvMapSharedData::s_EnvMapRotation;
glm::mat4 EnvMapSharedData::s_DirLightTransform;
bool EnvMapSharedData::s_DisplayHazelGrid;
bool EnvMapSharedData::s_DisplayRay;
glm::vec3 EnvMapSharedData::s_NewRay;
H2M::Ref<ShadowMap> EnvMapSharedData::s_ShadowMapDirLight;

H2M::Ref<MoravaShader> EnvMapSharedData::s_ShaderHazelPBR;
H2M::Ref<MoravaShader> EnvMapSharedData::s_ShaderOutline;

H2M::EntityH2M EnvMapSharedData::s_PointLightEntity;
H2M::Ref<OmniShadowMap> EnvMapSharedData::s_OmniShadowMapPointLight;

H2M::EntityH2M EnvMapSharedData::s_SpotLightEntity;
H2M::Ref<OmniShadowMap> EnvMapSharedData::s_OmniShadowMapSpotLight;
