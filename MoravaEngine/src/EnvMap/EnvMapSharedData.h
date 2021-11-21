#pragma once

#include "../../pch.h"

#include "Framebuffer/OmniShadowMap.h"
#include "Mesh/CubeSkybox.h"
#include "Mesh/Quad.h"

#include "HazelLegacy/Scene/SceneHazelLegacy.h"


class EnvMapSharedData
{
public:
	static std::map<std::string, unsigned int> s_SamplerSlots;

	static Hazel::Ref<Hazel::SceneHazelLegacy> s_RuntimeScene;
	static Hazel::Ref<Hazel::SceneHazelLegacy> s_EditorScene;

	static Scene* s_Scene; // reference to SceneHazelEnvMap (the original engine scene object)

	static Hazel::EditorCamera* s_EditorCamera;
	static RuntimeCamera* s_RuntimeCamera;
	static Hazel::HazelCamera* s_ActiveCamera;

	static Hazel::Ref<CubeSkybox>s_SkyboxCube;
	static Hazel::Ref<Quad> s_Quad;
	static bool s_DisplayOutline;
	static bool s_DisplayWireframe;
	static float s_SkyboxExposureFactor;
	static bool s_RadiancePrefilter;
	static float s_EnvMapRotation;
	static glm::mat4 s_DirLightTransform; // sent to shaders as an uniform dirLightTransform / u_DirLightTransform
	static bool s_DisplayHazelGrid;
	static bool s_DisplayRay;
	static glm::vec3 s_NewRay;

	static Hazel::Ref<MoravaShader> s_ShaderHazelPBR; // currently used PBR shader, m_ShaderHazelPBR_Anim or m_ShaderHazelPBR_Static
	static Hazel::Ref<MoravaShader> s_ShaderOutline;

	static Hazel::EntityHazelLegacy s_PointLightEntity; // temporary, for experimental use
	static Hazel::Ref<OmniShadowMap> s_OmniShadowMapPointLight;

	static Hazel::EntityHazelLegacy s_SpotLightEntity;  // temporary, for experimental use
	static Hazel::Ref<OmniShadowMap> s_OmniShadowMapSpotLight;

	static Hazel::Ref<ShadowMap> s_ShadowMapDirLight;

private:

};
