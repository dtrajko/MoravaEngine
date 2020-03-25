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
	~SceneBullet();

private:
	void SetupBullet();
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;

private:
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;

};
