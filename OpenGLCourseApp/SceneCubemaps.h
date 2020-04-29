#pragma once

#include "Scene.h"


class SceneCubemaps : public Scene
{

public:
	SceneCubemaps();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;

	void SetGeometry();
	void CleanupGeometry();

	inline unsigned int GetCubeVAO() const { return cubeVAO; };
	inline unsigned int GetSkyboxVAO() const { return skyboxVAO; };
	inline unsigned int GetCubemapTexture() const { return cubemapTexture; };

	virtual ~SceneCubemaps() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	unsigned int cubemapTexture;

	unsigned int cubeVAO;
	unsigned int cubeVBO;
	unsigned int skyboxVAO;
	unsigned int skyboxVBO;

};
