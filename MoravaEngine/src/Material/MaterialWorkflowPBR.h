#pragma once

#include "Mesh/CubeSkybox.h"
#include "Mesh/Quad.h"
#include "Shader/MoravaShader.h"

#include <string>


class MaterialWorkflowPBR
{
public:
	MaterialWorkflowPBR();
	~MaterialWorkflowPBR();

	void Init(std::string envMapHDR, uint32_t blurLevel = 0);
	void BindTextures(unsigned int slot);
	inline const unsigned int GetHDRTexture() const { return m_HDRTexture; };
	inline const unsigned int GetIrradianceMap() const { return m_IrradianceMap; };
	inline const unsigned int GetPrefilterMap() const { return m_PrefilterMap; };
	inline const unsigned int GetBRDF_LUT_Texture() const { return m_BRDF_LUT_Texture; };
	inline const unsigned int GetEnvironmentCubemap() const { return m_EnvironmentCubemap; };
	inline CubeSkybox* GetSkyboxCube() const { return m_SkyboxCube; };
	void BindEnvironmentCubemap(unsigned int slot);
	void BindIrradianceMap(unsigned int slot);
	void BindPrefilterMap(unsigned int slot);
	void SetGlobalRenderState();
	void Cleanup();

private:
	void SetupShaders();
	void SetupGeometry();
	void SetupFramebuffers();
	void LoadHDREnvironmentMap(std::string envMapHDR);
	void SetupCubemap();
	void SetupMatrices();
	void ConvertHDREquirectangularToCubemap();
	void ApplyBlurToCubemap();
	void CreateIrradianceCubemap();
	void SolveDiffuseIntegralByConvolution();
	void CreatePreFilterCubemap();
	void RunQuasiMonteCarloSimulation();
	void Generate2DLUTFromBRDF();
	void ResetResourceIDs();

public:
	int m_CaptureSize;
	int m_CaptureSizeBlur;
	int m_PrefilterMapSize;
	int m_IrradianceMapSize;

private:
	unsigned int m_HDRTexture;
	glm::mat4 m_CaptureProjection;
	unsigned int m_CaptureFBO;
	unsigned int m_CaptureRBO;
	glm::mat4 m_CaptureViews[6];
	unsigned int m_EnvironmentCubemap;
	CubeSkybox* m_SkyboxCube;
	Quad* m_Quad;
	unsigned int m_IrradianceMap;
	unsigned int m_PrefilterMap;
	unsigned int m_BRDF_LUT_Texture;

	// Shaders
	MoravaShader* m_ShaderEquirectangularToCubemap;
	MoravaShader* m_ShaderIrradiance;
	MoravaShader* m_ShaderPrefilter;
	MoravaShader* m_ShaderBRDF;
	MoravaShader* m_ShaderHorizontalBlur;
	MoravaShader* m_ShaderVerticalBlur;

	uint32_t m_BlurLevel;
	int m_HDR_Map_Width;
	int m_HDR_Map_Height;

	int m_MinFramebufferSize;

};
