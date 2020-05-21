#pragma once

#include "Shader.h"
#include "Cube.h"
#include "Quad.h"

#include <string>


class MaterialWorkflowPBR
{
public:
	MaterialWorkflowPBR();
	~MaterialWorkflowPBR();

	void Init(std::string envMapHDR);
	void BindTextures(unsigned int slot);
	inline const unsigned int GetHDRTexture() const { return m_HDRTexture; };
	inline const unsigned int GetIrradianceMap() const { return m_IrradianceMap; };
	inline const unsigned int GetPrefilterMap() const { return m_PrefilterMap; };
	inline const unsigned int GetBRDF_LUT_Texture() const { return m_BRDF_LUT_Texture; };
	inline const unsigned int GetEnvironmentCubemap() const { return m_EnvironmentCubemap; };
	inline Cube* GetSkyboxCube() const { return m_SkyboxCube; };

private:
	void SetupShaders();
	void SetupGeometry();
	void SetupFramebuffers();
	void LoadHDREnvironmentMap(std::string envMapHDR);
	void SetupCubemap();
	void SetupMatrices();
	void ConvertHDREquirectangularToCubemap();
	void CreateIrradianceCubemap();
	void SolveDiffuseIntegralByConvolution();
	void CreatePreFilterCubemap();
	void RunQuasiMonteCarloSimulation();
	void Generate2DLUTFromBRDF();

private:
	unsigned int m_HDRTexture;
	glm::mat4 m_CaptureProjection;
	unsigned int m_CaptureFBO;
	unsigned int m_CaptureRBO;
	glm::mat4 m_CaptureViews[6];
	unsigned int m_EnvironmentCubemap;
	Cube* m_SkyboxCube;
	Quad* m_Quad;
	unsigned int m_IrradianceMap;
	unsigned int m_PrefilterMap;
	unsigned int m_BRDF_LUT_Texture;

	// Shaders
	Shader* m_ShaderEquirectangularToCubemap;
	Shader* m_ShaderIrradiance;
	Shader* m_ShaderPrefilter;
	Shader* m_ShaderBRDF;

};
