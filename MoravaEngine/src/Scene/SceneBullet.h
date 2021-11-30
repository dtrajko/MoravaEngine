#pragma once

#include "Scene/Scene.h"

#include "Misc/BulletDebugDrawer.h"

#include "btBulletDynamicsCommon.h"


class SceneBullet : public Scene
{

public:
	SceneBullet();
	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
	virtual void SetLightManager() override;
	virtual ~SceneBullet() override;

private:
	void BulletSetup();
	void BulletSimulation(float timestep);
	void BulletCleanup();
	virtual void SetSkybox() override;
	virtual void SetupTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	void Fire();
	btRigidBody* AddRigidBodyBox(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, float mass, float bounciness);
	btRigidBody* AddRigidBodySphere(glm::vec3 position, glm::vec3 rotation, float scale, float mass, float bounciness);
	btRigidBody* AddRigidBody(btCollisionShape* collisionShape, glm::vec3 position, glm::vec3 rotation, float mass, float bounciness);
	btTransform GetCollisionObjectTransform(int id);

private:
	btDiscreteDynamicsWorld* dynamicsWorld;
	btAlignedObjectArray<btCollisionShape*> m_CollisionShapes;
	btSequentialImpulseConstraintSolver* solver;
	btBroadphaseInterface* overlappingPairCache;
	btCollisionDispatcher* dispatcher;
	btDefaultCollisionConfiguration* collisionConfiguration;

	int m_GravityIntensity = -5;
	int m_SphereCount = 0;
	int m_SphereCountMax = 100;
	int m_SpheresOffset = 0;
	float m_Bounciness = 0.6f;
	float m_SphereMass = 4.0f;
	int m_PlankOffset = 0;
	int m_PlankFloors = 10;
	float m_PlankMass = 4.0f;
	float m_PlankBounciness = 0.0f;
	float m_FireIntensity = 100.0f;
	float m_FireIntensityMax = 200.0f;
	bool m_FireEnabled = true;
	float m_LastTimestep = 0.0f;
	float m_FireCooldown = 0.2f;
	btRigidBody* m_LatestBulletBody;
	BulletDebugDrawer* m_BulletDebugDrawer;
	float m_TilingFactor = 0.25f;

};
