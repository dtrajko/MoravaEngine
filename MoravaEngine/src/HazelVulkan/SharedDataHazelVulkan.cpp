#include "SharedDataHazelVulkan.h"


std::map<std::string, unsigned int> SharedDataHazelVulkan::s_SamplerSlots;

Hazel::Ref<Hazel::HazelScene> SharedDataHazelVulkan::s_RuntimeScene;
Hazel::Ref<Hazel::HazelScene> SharedDataHazelVulkan::s_EditorScene;
Hazel::EditorCamera* SharedDataHazelVulkan::s_EditorCamera;
RuntimeCamera* SharedDataHazelVulkan::s_RuntimeCamera;
Hazel::HazelCamera* SharedDataHazelVulkan::s_ActiveCamera;

Hazel::Ref<CubeSkybox> SharedDataHazelVulkan::s_SkyboxCube;
Hazel::Ref<Quad> SharedDataHazelVulkan::s_Quad;

bool SharedDataHazelVulkan::s_DisplayOutline;
float SharedDataHazelVulkan::s_SkyboxExposureFactor;
bool SharedDataHazelVulkan::s_RadiancePrefilter;
float SharedDataHazelVulkan::s_EnvMapRotation;
glm::mat4 SharedDataHazelVulkan::s_DirLightTransform;
bool SharedDataHazelVulkan::s_DisplayHazelGrid;
bool SharedDataHazelVulkan::s_DisplayRay;
glm::vec3 SharedDataHazelVulkan::s_NewRay;
Hazel::Ref<ShadowMap> SharedDataHazelVulkan::s_ShadowMapDirLight;

Hazel::Ref<Shader> SharedDataHazelVulkan::s_ShaderHazelPBR;
Hazel::Ref<Shader> SharedDataHazelVulkan::s_ShaderOutline;

Hazel::Entity SharedDataHazelVulkan::s_PointLightEntity;
Hazel::Ref<OmniShadowMap> SharedDataHazelVulkan::s_OmniShadowMapPointLight;

Hazel::Entity SharedDataHazelVulkan::s_SpotLightEntity;
Hazel::Ref<OmniShadowMap> SharedDataHazelVulkan::s_OmniShadowMapSpotLight;
