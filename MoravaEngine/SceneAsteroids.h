#pragma once

#include "Scene.h"
#include "LearnOpenGL/ModelJoey.h"


class SceneAsteroids : public Scene
{

public:
	SceneAsteroids();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	inline std::map<std::string, ModelJoey*> GetModels() const { return models; };
	inline float GetFOV() { return m_FOV; };
	virtual ~SceneAsteroids() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	std::map<std::string, ModelJoey*> models;

	float m_FOV = 60.0f;

};
