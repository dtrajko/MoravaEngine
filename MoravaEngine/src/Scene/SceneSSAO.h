#pragma once

#include "Scene/Scene.h"

#include "Framebuffer/MoravaFramebuffer.h"
#include "LearnOpenGL/ModelJoey.h"
#include "LearnOpenGL/ModelSSAO.h"
#include "SSAO/SSAO.h"


class SceneSSAO : public Scene
{

public:
	SceneSSAO();
	virtual ~SceneSSAO() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, MoravaShader*> shaders, std::map<std::string, int> uniforms) override;

private:
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;
	void SetupSSAO();

private:
	SSAO m_SSAO;
	std::map<std::string, ModelSSAO*> modelsSSAO;

	enum class RenderTarget
	{
		GBuffer_Position     = 0,
		GBuffer_Normal       = 1,
		GBuffer_Albedo       = 2,
		GBuffer_TexCoord     = 3,
		SSAO_Composite       = 4,
		SSAO_ColorBuffer     = 5,
		SSAO_ColorBufferBlur = 6,
	};

private:
	int m_RenderTarget;

};
