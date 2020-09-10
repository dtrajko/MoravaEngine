#pragma once

#include "Scene.h"

#include "TextureCubeMap.h"
#include "LearnOpenGL/ModelJoey.h"
#include "Quad.h"
#include "Raycast.h"
#include "TerrainHeightMap.h"


class SceneCubemaps : public Scene
{

public:
	SceneCubemaps();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;

	inline std::map<std::string, ModelJoey*> GetModels() const { return models; };
	void SetGeometry();
	void CleanupGeometry();

	inline unsigned int GetCubemapTextureID() const { return m_TextureCubeMapID; };
	inline Raycast* GetRaycast() const { return m_Raycast; };
	inline TerrainHeightMap* GetTerrain() const { return m_Terrain; };

	virtual ~SceneCubemaps() override;

public:
	bool m_SkyboxEnabled        = true;
	bool m_ModelCubeEnabled     = true;
	bool m_TerrainEnabled       = true;
	bool m_CubeTerrainEnabled   = true;
	bool m_NanosuitModelEnabled = true;
	bool m_AABBEnabled          = true;

private:
	virtual void SetSkybox() override;
	virtual void SetupTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	TextureCubeMap* m_TextureCubeMap;
	unsigned int m_TextureCubeMapID;

	std::map<std::string, ModelJoey*> models;

	Quad* m_Quad;
	Raycast* m_Raycast;
	TerrainHeightMap* m_Terrain;

};
