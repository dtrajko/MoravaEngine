#pragma once

#include "RendererBasic.h"

#include "RadianceHDR.h"
#include "Cubemap.h"
#include "Cube.h"
#include "Quad.h"
#include "learnopengl/shaderJoey.h"
#include "SphereJoey.h"


class RendererJoey : public RendererBasic
{

public:
	RendererJoey();
	virtual void Init(Scene* scene) override;
	void SetGeometry();
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;

	void SetFramebuffers();
	void LoadHDREnvironmentMap();
	void SetupCubemap();
	void SetupMatrices();
	void ConvertHDREquirectangularToCubemap();
	void CreateIrradianceCubemap();
	void SolveDiffuseIntegralByConvolution();
	void CreatePreFilterCubemap();
	void RunQuasiMonteCarloSimulation();
	void Generate2DLUTFromBRDF();

	void RenderPass();
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	virtual ~RendererJoey() override;

private:
	unsigned int m_HDRTexture;
	unsigned int m_CaptureFBO;
	unsigned int m_CaptureRBO;
	unsigned int m_EnvCubemap;
	glm::mat4 m_CaptureProjection;
	glm::mat4 m_CaptureViews[6];
	unsigned int m_CubeVAO = 0;
	unsigned int m_CubeVBO = 0;
	unsigned int m_IrradianceMap;
	unsigned int m_PrefilterMap;
	unsigned int m_BRDF_LUT_Texture;
	unsigned int m_QuadVAO = 0;
	unsigned int m_QuadVBO;

	SphereJoey* m_SphereJoey;
	Cube* m_Cube;
	Quad* m_Quad;

	float m_Timestep = 0.0f;

};
