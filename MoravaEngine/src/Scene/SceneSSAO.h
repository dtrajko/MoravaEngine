#pragma once

#include "Scene/Scene.h"

#include "../../Framebuffer.h"
#include "../../LearnOpenGL/ModelSSAO.h"
#include "../../LearnOpenGL/ModelJoey.h"
#include "../../SSAO.h"


class SceneSSAO : public Scene
{

public:
	SceneSSAO();
	virtual ~SceneSSAO() override;

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
	void SetupSSAO();

private:
	SSAO* m_SSAO;
	std::map<std::string, ModelSSAO*> modelsSSAO;

};
