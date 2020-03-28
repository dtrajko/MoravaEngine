#pragma once

#include "Scene.h"

#include "btBulletDynamicsCommon.h"


class SceneBullet : public Scene
{

public:
	SceneBullet();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual ~SceneBullet() override;

private:
	void BulletSetup();
	void BulletSimulation(float timestep);
	void BulletCleanup();
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;
	void Fire();

private:
	btDiscreteDynamicsWorld* dynamicsWorld;
	btAlignedObjectArray<btCollisionShape*> collisionShapes;
	btSequentialImpulseConstraintSolver* solver;
	btBroadphaseInterface* overlappingPairCache;
	btCollisionDispatcher* dispatcher;
	btDefaultCollisionConfiguration* collisionConfiguration;

	int m_SphereCount = 0;
	int gravityIntensity = -1;

	float m_LastTimestep = 0.0f;
	float m_FireCooldown = 0.2f;
	int m_SpheresOffset = 0;

	float m_Bounciness = 0.6f;

};
