#pragma once

#include "Scene.h"

#include "LearnOpenGL/ModelSSAO.h"
#include "LearnOpenGL/ModelJoey.h"
#include "Framebuffer.h"


class SceneDeferred : public Scene
{

public:
	SceneDeferred();
	virtual ~SceneDeferred() override;

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;

private:
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

};
