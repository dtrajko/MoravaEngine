#pragma once

#include "Scene/Scene.h"

#include "Framebuffer/Framebuffer.h"
#include "LearnOpenGL/ModelJoey.h"
#include "LearnOpenGL/ModelSSAO.h"
#include "SSAO/SSAO.h"


class SceneBloom : public Scene
{

public:
	SceneBloom();
	virtual ~SceneBloom() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;

private:
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;
	virtual void SetupShaders() override;
	void SetupLights();

	void renderCube();
	void renderQuad();
	unsigned int loadTexture(char const* path, bool gammaCorrection);

	void GenerateConditional();
	void Generate();
	void Release();
	void ResetHandlers();

private:
	Hazel::Ref<Shader> m_ShaderBloom;
	Hazel::Ref<Shader> m_ShaderLightBox;
	Hazel::Ref<Shader> m_ShaderBlur;
	Hazel::Ref<Shader> m_ShaderBloomFinal;

	// Hazel::Ref<Hazel::HazelTexture2D> m_TextureWood;
	// Hazel::Ref<Hazel::HazelTexture2D> m_TextureContainer;

	unsigned int m_TextureWood;
	unsigned int m_TextureContainer;

	std::vector<glm::vec3> m_LightPositions;
	std::vector<glm::vec3> m_LightColors;

	unsigned int m_HDR_FBO;
	unsigned int m_ColorBuffers[2];

	unsigned int m_RBO_Depth;

	unsigned int m_PingPongFBO[2];
	unsigned int m_PingPongColorbuffers[2];

	unsigned int m_CubeVAO = 0;
	unsigned int m_CubeVBO = 0;

	unsigned int m_QuadVAO = 0;
	unsigned int m_QuadVBO = 0;

	bool m_BloomEnabled = true;
	float m_Exposure = 1.0f;

	uint32_t m_Width = 0;
	uint32_t m_Height = 0;

	uint32_t m_WidthPrev = 0;
	uint32_t m_HeightPrev = 0;

};
