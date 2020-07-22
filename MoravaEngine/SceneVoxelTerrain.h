#pragma once

#include "Scene.h"

#include "TerrainVoxel.h"
#include "RenderInstanced.h"
#include "Player.h"
#include "PlayerController.h"
#include "Raycast.h"


class SceneVoxelTerrain : public Scene
{

public:
	SceneVoxelTerrain();
	virtual ~SceneVoxelTerrain() override;

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	void UpdateCooldown(float timestep, Window& mainWindow);
	void Release();
	void Dig(bool* keys, float timestep);
	inline Raycast* GetRaycast() const { return m_Raycast; };

private:
	virtual void SetCamera() override;
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMeshes() override;
	bool IsTerrainConfigChanged();

	TerrainVoxel* m_TerrainVoxel;
	glm::mat4 m_Transform;
	RenderInstanced* m_RenderInstanced;
	glm::vec3 m_TerrainScale;
	glm::vec3 m_TerrainScalePrev;
	float m_TerrainNoiseFactor;
	float m_TerrainNoiseFactorPrev;
	EventCooldown m_UpdateCooldown;
	EventCooldown m_DigCooldown;
	Player* m_Player;
	PlayerController* m_PlayerController;
	bool m_TerrainSettingsChanged;
	bool m_DrawGizmos;
	Pivot* m_PivotScene;
	float m_DigDistance;
	Raycast* m_Raycast;

};
