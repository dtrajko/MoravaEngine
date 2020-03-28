#include "SceneBullet.h"

#include "Sphere.h"
#include "SphereJoey.h"

#include "ImGuiWrapper.h"


SceneBullet::SceneBullet()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = false;
	sceneSettings.enableSpotLights   = false;
	sceneSettings.enableWaterEffects = false;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 20.0f, 40.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 5.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 400.0f;
	sceneSettings.ambientIntensity = 0.2f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(0.05f, -30.0f, 0.05f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-36.0f, 36.0f, -36.0f, 36.0f, 0.1f, 36.0f);
	sceneSettings.pLight_0_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pLight_0_position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pLight_0_diffuseIntensity = 2.0f;
	sceneSettings.pLight_1_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pLight_1_position = glm::vec3(8.92f, 2.75f, -0.85f);
	sceneSettings.pLight_1_diffuseIntensity = 2.0f;
	sceneSettings.pLight_2_color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings.pLight_2_position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings.pLight_2_diffuseIntensity = 2.0f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.1f;
	sceneSettings.waterHeight = 6.0f; // 1.0f 5.0f
	sceneSettings.waterWaveSpeed = 0.1f;

	BulletSetup();
	SetLightManager();
	SetSkybox();
	SetTextures();
	SetupModels();
}

void SceneBullet::BulletSetup()
{
	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	collisionConfiguration = new btDefaultCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	dispatcher = new btCollisionDispatcher(collisionConfiguration);

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	overlappingPairCache = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	solver = new btSequentialImpulseConstraintSolver;

	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

	dynamicsWorld->setGravity(btVector3(0, btScalar(gravityIntensity), 0));

	{
		btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(10.), btScalar(10.), btScalar(10.)));
		collisionShapes.push_back(groundShape);
		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, 0, 0));
		btScalar mass(0.);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
	}

	for (int i = 0; i < sphereCount; i++)
	{
		// Sphere 1 create a dynamic rigidbody
		btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		collisionShapes.push_back(colShape);
		/// Create Dynamic Objects
		btTransform startTransform;
		startTransform.setIdentity();
		btScalar mass(2.0f);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);
		startTransform.setOrigin(btVector3(0, 20 + btScalar(i) * 2, 0));
		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		dynamicsWorld->addRigidBody(body);
	}

	printf("Bullet Setup complete.\n");
}

void SceneBullet::BulletSimulation(float timestep)
{
	dynamicsWorld->stepSimulation(timestep * 0.005f, 10);

	//print positions of all objects
	for (int j = dynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--)
	{
		btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[j];
		btRigidBody* body = btRigidBody::upcast(obj);
		btTransform trans;
		if (body && body->getMotionState())
		{
			body->getMotionState()->getWorldTransform(trans);
		}
		else
		{
			trans = obj->getWorldTransform();
		}
		printf("Bullet Simulation running: ");
		printf("world pos object %d = %f,%f,%f\r", j, float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
	}
}

void SceneBullet::BulletCleanup()
{
	for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
	{
		btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}
		dynamicsWorld->removeCollisionObject(obj);
		delete obj;
	}

	//delete collision shapes
	for (int j = 0; j < collisionShapes.size(); j++)
	{
		btCollisionShape* shape = collisionShapes[j];
		collisionShapes[j] = 0;
		delete shape;
	}

	//delete dynamics world
	delete dynamicsWorld;

	//delete solver
	delete solver;

	//delete broadphase
	delete overlappingPairCache;

	//delete dispatcher
	delete dispatcher;

	delete collisionConfiguration;

	//next line is optional: it will be cleared by the destructor when the array goes out of scope
	collisionShapes.clear();

	printf("Bullet cleanup complete.\n");
}

void SceneBullet::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_4/right.png");
	skyboxFaces.push_back("Textures/skybox_4/left.png");
	skyboxFaces.push_back("Textures/skybox_4/top.png");
	skyboxFaces.push_back("Textures/skybox_4/bottom.png");
	skyboxFaces.push_back("Textures/skybox_4/back.png");
	skyboxFaces.push_back("Textures/skybox_4/front.png");
	m_Skybox = new Skybox(skyboxFaces);
}

void SceneBullet::SetTextures()
{
	textures.insert(std::make_pair("pyramid", new Texture("Textures/pyramid.png")));
}

void SceneBullet::SetupModels()
{
	Sphere* sphere = new Sphere();
	sphere->Create();
	meshes.insert(std::make_pair("sphere", sphere));
}

void SceneBullet::Update(float timestep, Window& mainWindow)
{
	glm::vec3 lightDirection = m_LightManager->directionalLight.GetDirection();

	ImGui::SliderFloat3("Directional Light Direction", glm::value_ptr(lightDirection), -40.0f, 40.0f);
	ImGui::SliderInt("Gravity Intensity", &gravityIntensity, -10, 10);

	m_LightManager->directionalLight.SetDirection(lightDirection);

	dynamicsWorld->setGravity(btVector3(0, btScalar(gravityIntensity), 0));

	BulletSimulation(timestep);
}

void SceneBullet::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
	glm::mat4 model;

	btCollisionObject* sphereObj;
	btRigidBody* sphereRB;
	btTransform sphereTrans;

	for (int i = 0; i < sphereCount; i++)
	{
		sphereObj = dynamicsWorld->getCollisionObjectArray()[1 + i];
		sphereRB = btRigidBody::upcast(sphereObj);
		
		if (sphereRB && sphereRB->getMotionState())
		{
			sphereRB->getMotionState()->getWorldTransform(sphereTrans);
		}
		else
		{
			sphereTrans = sphereObj->getWorldTransform();
		}

		/* Sphere 1 */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(
			float(sphereTrans.getOrigin().getX()),
			float(sphereTrans.getOrigin().getY()),
			float(sphereTrans.getOrigin().getZ())
		));
		model = glm::rotate(model, sphereTrans.getRotation().getX(), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, sphereTrans.getRotation().getY(), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, sphereTrans.getRotation().getZ(), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["pyramid"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["sphere"]->Render();
	}

	if (passType == "main")
	{
		/* Cube */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["pyramid"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["cube"]->Render();

		/* Plane */
		// model = glm::mat4(1.0f);
		// model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		// model = glm::scale(model, glm::vec3(1.0f));
		// glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		// textures["pyramid"]->Bind(textureSlots["diffuse"]);
		// textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		// meshes["quadLarge"]->Render();
	}
}

SceneBullet::~SceneBullet()
{
	BulletCleanup();
}
