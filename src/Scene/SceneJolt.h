#pragma once

#include "Scene/Scene.h"

#include "Game/GameLoop.h"
#include "Physics/PhysicsWorld.h"

#include <memory>
#include <vector>


// Jolt Physics in an OpenGL scene: a walled floor, two crates and a tower of planks; the left mouse button fires spheres
// (docs/engine/Game_Engine_Runtime_Guide.html, 3.2: formerly SceneBullet, with Bullet)
class SceneJolt : public Scene, private GameLoopSystems
{

public:
	SceneJolt();
	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
	virtual void SetLightManager() override;
	virtual ~SceneJolt() override;

private:
	void SetupPhysics();
	virtual void SetSkybox() override;
	virtual void SetupTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	void Fire();
	JPH::BodyID AddBox(glm::vec3 position, glm::vec3 halfExtents, float mass, float bounciness);
	JPH::BodyID AddSphere(glm::vec3 position, float radius, float mass, float bounciness);
	JPH::BodyID AddBody(const JPH::Shape* shape, glm::vec3 position, float mass, float bounciness);

	virtual void FixedUpdate(float fixedStep) override; // GameLoopSystems

private:
	std::unique_ptr<PhysicsWorld> m_World;
	GameLoop m_GameLoop;

	JPH::BodyID m_Crates[2];
	std::vector<JPH::BodyID> m_Planks;  // 4 per floor of the tower: two along Z, then two along X
	std::vector<JPH::BodyID> m_Spheres; // fired

	int m_GravityIntensity = -5;
	int m_SphereCountMax = 100;
	float m_Bounciness = 0.6f;
	float m_SphereMass = 8.0f;
	float m_SphereRadius = 1.5f;
	int m_PlankFloors = 10;
	float m_PlankMass = 4.0f;
	float m_PlankBounciness = 0.0f;
	float m_FireIntensity = 400.0f;    // impulse applied to a fired sphere
	float m_FireIntensityMax = 800.0f; // slider range in the Settings panel
	bool m_FireEnabled = true;
	float m_LastTimestep = 0.0f;          // time of the last Fire (cooldown)
	float m_LastPhysicsTimestamp = -1.0f; // time of the previous frame (-1: first frame)
	float m_FireCooldown = 0.2f;
	float m_TilingFactor = 0.25f;
	bool m_SettingsDocked = false; // the first frame checked whether the Settings panel has a place in the dockspace

};
