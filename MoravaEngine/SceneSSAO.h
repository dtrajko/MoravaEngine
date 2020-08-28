#pragma once

#include "Scene.h"

#include "LearnOpenGL/ModelSSAO.h"
#include "LearnOpenGL/ModelJoey.h"
#include "Framebuffer.h"


class SceneSSAO : public Scene
{

public:
	SceneSSAO();
	virtual ~SceneSSAO() override;

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;

private:
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

public:
	// Framebuffer* m_GBuffer;
	// Framebuffer* m_SSAO_FBO;
	// Framebuffer* m_SSAO_BlurFBO;

	// Texture* m_GPositionTexture;
	// Texture* m_GNormalTexture;
	// Texture* m_NoiseTexture;
	// Texture* m_SSAOColorBufferTexture;
	// Texture* m_AlbedoTexture;
	// Texture* m_SSAOColorBufferBlurTexture;

	std::map<std::string, ModelSSAO*> modelsSSAO;
	std::map<std::string, ModelJoey*> modelsJoey;

	uint32_t* gPosition;
	uint32_t* gNormal;
	uint32_t* gAlbedo;
	uint32_t* rboDepth;

	// SSAO processing stage
	uint32_t* ssaoFBO;
	uint32_t* ssaoBlurFBO;
	uint32_t* ssaoColorBuffer;
	uint32_t* ssaoColorBufferBlur;

	uint32_t* gBuffer;
	uint32_t* noiseTexture;

};
