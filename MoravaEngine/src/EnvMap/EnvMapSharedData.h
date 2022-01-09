#pragma once

#include "pch.h"

#include "Framebuffer/OmniShadowMap.h"
#include "Mesh/CubeSkybox.h"
#include "Mesh/Quad.h"

#include "H2M/Scene/SceneH2M.h"


class EnvMapSharedData
{
public:
	static std::map<std::string, unsigned int> s_SamplerSlots;

	static Scene* s_Scene; // reference to SceneHazelEnvMap (the original engine scene object)

	static H2M::RefH2M<CubeSkybox>s_SkyboxCube;
	static H2M::RefH2M<Quad> s_Quad;
	static bool s_DisplayOutline;
	static bool s_DisplayWireframe;
	static float s_SkyboxExposureFactor;
	static bool s_RadiancePrefilter;
	static float s_EnvMapRotation;
	static glm::mat4 s_DirLightTransform; // sent to shaders as an uniform dirLightTransform / u_DirLightTransform
	static bool s_DisplayHazelGrid;
	static bool s_DisplayRay;
	static glm::vec3 s_NewRay;

	static H2M::RefH2M<MoravaShader> s_ShaderHazelPBR; // currently used PBR shader, m_ShaderHazelPBR_Anim or m_ShaderHazelPBR_Static
	static H2M::RefH2M<MoravaShader> s_ShaderOutline;

	// static H2M::EntityH2M s_PointLightEntity; // temporary, for experimental use
	// static H2M::EntityH2M s_SpotLightEntity;  // temporary, for experimental use

	static H2M::RefH2M<OmniShadowMap> s_OmniShadowMapPointLight;
	static H2M::RefH2M<OmniShadowMap> s_OmniShadowMapSpotLight;

	static H2M::RefH2M<ShadowMap> s_ShadowMapDirLight;

};
