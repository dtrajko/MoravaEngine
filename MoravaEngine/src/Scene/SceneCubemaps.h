#pragma once

#include "Scene/Scene.h"

#include "../../LearnOpenGL/ModelJoey.h"
#include "../../Quad.h"
#include "../../Raycast.h"

#include "Terrain/TerrainHeightMap.h"
#include "Texture/TextureCubemapFaces.h"


class SceneCubemaps : public Scene
{

public:
	SceneCubemaps();
	virtual ~SceneCubemaps() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;

	void SetGeometry();
	void CleanupGeometry();

private:
	virtual void SetSkybox() override;
	virtual void SetupTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	void SetupShaders();

private:
	std::map<std::string, ModelJoey*> models;

	bool m_SkyboxEnabled        = true;
	bool m_ModelCubeEnabled     = true;
	bool m_TerrainEnabled       = true;
	bool m_CubeTerrainEnabled   = true;
	bool m_NanosuitModelEnabled = true;
	bool m_AABBEnabled          = true;

	TextureCubemapFaces* m_TextureCubeMap;
	unsigned int m_TextureCubeMapID;

	Quad* m_Quad;
	Raycast* m_Raycast;
	TerrainHeightMap* m_Terrain;

	Shader* m_ShaderCubemaps;
	Shader* m_ShaderCubemapsNanosuit;
	Shader* m_ShaderSkybox;
	Shader* m_ShaderBasic;
	Shader* m_ShaderFramebuffersScene;

	float m_CubeRenderLastTime = 0.0f;
	float m_CubeRenderCooldown = 1.0f;
	glm::mat4 m_ModelCube;
	AABB* m_CubeAABB;
	Pivot* m_PivotCube;
	Pivot* m_PivotScene;

};
