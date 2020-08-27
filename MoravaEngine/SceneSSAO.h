#pragma once

#include "Scene.h"

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

private:
	Framebuffer* m_GBuffer;
	std::map<std::string, ModelJoey*> models;

};
