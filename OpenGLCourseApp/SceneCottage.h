#pragma once

#include "Scene.h"


class SceneCottage : public Scene
{

public:
	SceneCottage();
	virtual void Update(float timestep) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual ~SceneCottage() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;
};
