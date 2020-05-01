#pragma once

#include "Scene.h"

#include "TextureCubeMap.h"
#include "LearnOpenGL/ModelJoey.h"


class SceneCubemaps : public Scene
{

public:
	SceneCubemaps();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;

	inline std::map<std::string, ModelJoey*> GetModels() const { return models; };
	void SetGeometry();
	void CleanupGeometry();

	inline unsigned int GetCubeVAO() const { return m_CubeVAO; };
	inline unsigned int GetSkyboxVAO() const { return m_SkyboxVAO; };
	inline unsigned int GetCubemapTextureID() const { return m_TextureCubeMapID; };

	virtual ~SceneCubemaps() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	TextureCubeMap* m_TextureCubeMap;
	unsigned int m_TextureCubeMapID;

	std::map<std::string, ModelJoey*> models;

	unsigned int m_CubeVAO;
	unsigned int m_CubeVBO;
	unsigned int m_SkyboxVAO;
	unsigned int m_SkyboxVBO;

};
