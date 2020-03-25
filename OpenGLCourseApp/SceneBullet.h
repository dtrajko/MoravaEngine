#pragma once

#include "Scene.h"

#include "btBulletDynamicsCommon.h"


class SceneBullet : public Scene
{

public:
	SceneBullet();
	virtual void Update(float timestep) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual ~SceneBullet() override;

private:
	void BulletSetup();
	void BulletSimulation();
	void BulletCleanup();
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;

private:
	btDiscreteDynamicsWorld* dynamicsWorld;
	btAlignedObjectArray<btCollisionShape*> collisionShapes;
	btSequentialImpulseConstraintSolver* solver;
	btBroadphaseInterface* overlappingPairCache;
	btCollisionDispatcher* dispatcher;
	btDefaultCollisionConfiguration* collisionConfiguration;

};
