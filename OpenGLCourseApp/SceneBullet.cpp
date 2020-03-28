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
	textures.insert(std::make_pair("texture_gray", new Texture("Textures/texture_gray.png")));
	textures.insert(std::make_pair("texture_orange", new Texture("Textures/texture_orange.png")));
	textures.insert(std::make_pair("texture_blue", new Texture("Textures/texture_blue.png")));
	textures.insert(std::make_pair("crate_diffuse", new Texture("Textures/crate.png")));
	textures.insert(std::make_pair("crate_normal", new Texture("Textures/crateNormal.png")));
	textures.insert(std::make_pair("rusted_iron_diffuse", new Texture("Textures/PBR/rusted_iron/albedo.png")));
	textures.insert(std::make_pair("rusted_iron_normal", new Texture("Textures/PBR/rusted_iron/normal.png")));
}

void SceneBullet::SetupModels()
{
	Sphere* sphere = new Sphere();
	sphere->Create();
	meshes.insert(std::make_pair("sphere", sphere));
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
		btCollisionShape* groundShape;
		btTransform groundTransform;
		btScalar mass;
		bool isDynamic;
		btVector3 localInertia;
		btDefaultMotionState* myMotionState;
		btRigidBody* body;

		// Floor
		groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(1.), btScalar(50.)));
		collisionShapes.push_back(groundShape);
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, 0, 0));
		mass = btScalar(0.);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		isDynamic = (mass != 0.f);
		localInertia = btVector3(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfoF(mass, myMotionState, groundShape, localInertia);
		body = new btRigidBody(rbInfoF);
		body->setRestitution(m_Bounciness);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
		m_SpheresOffset++;

		// Wall 1
		groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(10.), btScalar(1.)));
		collisionShapes.push_back(groundShape);
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, 10, -50));
		mass = btScalar(0.);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		isDynamic = (mass != 0.f);
		localInertia = btVector3(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfoW1(mass, myMotionState, groundShape, localInertia);
		body = new btRigidBody(rbInfoW1);
		body->setRestitution(m_Bounciness);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
		m_SpheresOffset++;

		// Wall 2
		groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(10.), btScalar(1.)));
		collisionShapes.push_back(groundShape);
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, 10, 50));
		mass = btScalar(0.);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		isDynamic = (mass != 0.f);
		localInertia = btVector3(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfoW2(mass, myMotionState, groundShape, localInertia);
		body = new btRigidBody(rbInfoW2);
		body->setRestitution(m_Bounciness);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
		m_SpheresOffset++;

		// Wall 3
		groundShape = new btBoxShape(btVector3(btScalar(1.), btScalar(10.), btScalar(50.)));
		collisionShapes.push_back(groundShape);
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(-50, 10, 0));
		mass = btScalar(0.);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		isDynamic = (mass != 0.f);
		localInertia = btVector3(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfoW3(mass, myMotionState, groundShape, localInertia);
		body = new btRigidBody(rbInfoW3);
		body->setRestitution(m_Bounciness);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
		m_SpheresOffset++;

		// Wall 4
		groundShape = new btBoxShape(btVector3(btScalar(1.), btScalar(10.), btScalar(50.)));
		collisionShapes.push_back(groundShape);
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(50, 10, 0));
		mass = btScalar(0.);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		isDynamic = (mass != 0.f);
		localInertia = btVector3(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfoW4(mass, myMotionState, groundShape, localInertia);
		body = new btRigidBody(rbInfoW4);
		body->setRestitution(m_Bounciness);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
		m_SpheresOffset++;

		// Cube 1
		groundShape = new btBoxShape(btVector3(btScalar(3.), btScalar(3.), btScalar(3.)));
		collisionShapes.push_back(groundShape);
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(10, 3, 10));
		mass = btScalar(20.);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		isDynamic = (mass != 0.f);
		localInertia = btVector3(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfoC1(mass, myMotionState, groundShape, localInertia);
		body = new btRigidBody(rbInfoC1);
		body->setRestitution(0.2f);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
		m_SpheresOffset++;

		// Cube 2
		groundShape = new btBoxShape(btVector3(btScalar(4.), btScalar(4.), btScalar(4.)));
		collisionShapes.push_back(groundShape);
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(-10, 8, -10));
		mass = btScalar(40.);
		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		isDynamic = (mass != 0.f);
		localInertia = btVector3(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);
		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfoC2(mass, myMotionState, groundShape, localInertia);
		body = new btRigidBody(rbInfoC2);
		body->setRestitution(0.2f);
		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
		m_SpheresOffset++;
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
	}
}

void SceneBullet::Fire()
{
	// Sphere 1 create a dynamic rigidbody
	btCollisionShape* colShape = new btSphereShape(btScalar(1.5));
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
	glm::vec3 origin = m_Camera->GetPosition() + glm::vec3(0.0f, -1.0f, 0.0f) + m_Camera->GetFront() * 2.0f;
	startTransform.setOrigin(btVector3(origin.x, origin.y, origin.z));
	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	body->setRestitution(m_Bounciness);
	dynamicsWorld->addRigidBody(body);
	m_SphereCount++;

	// apply the force
	float force = 50.0f;
	glm::vec3 impulse = m_Camera->GetDirection() * force;
	body->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));

	printf("SceneBullet::Fire: BOOOM! m_SphereCount: %i\n", m_SphereCount);
}

void SceneBullet::Update(float timestep, Window& mainWindow)
{
	if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_LEFT])
	{
		if (timestep - m_LastTimestep > m_FireCooldown)
		{
			Fire();
			m_LastTimestep = timestep;
		}
	}

	glm::vec3 lightDirection = m_LightManager->directionalLight.GetDirection();

	ImGui::SliderFloat3("Directional Light Direction", glm::value_ptr(lightDirection), -40.0f, 40.0f);
	ImGui::SliderInt("Gravity Intensity", &gravityIntensity, -10, 10);
	ImGui::SliderFloat("Bouncincess", &m_Bounciness, 0.0f, 2.0f);

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

	for (int i = 0; i < m_SphereCount; i++)
	{
		sphereObj = dynamicsWorld->getCollisionObjectArray()[i + m_SpheresOffset];
		sphereRB = btRigidBody::upcast(sphereObj);
		
		if (sphereRB && sphereRB->getMotionState())
		{
			sphereRB->getMotionState()->getWorldTransform(sphereTrans);
		}
		else
		{
			sphereTrans = sphereObj->getWorldTransform();
		}

		/* Sphere bullet */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(
			float(sphereTrans.getOrigin().getX()),
			float(sphereTrans.getOrigin().getY()),
			float(sphereTrans.getOrigin().getZ())
		));
		model = glm::rotate(model, sphereTrans.getRotation().getX(), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, sphereTrans.getRotation().getY(), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, sphereTrans.getRotation().getZ(), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.5f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["rusted_iron_diffuse"]->Bind(textureSlots["diffuse"]);
		textures["rusted_iron_normal"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["sphere"]->Render();
	}

	btCollisionObject* cubeObj;
	btRigidBody* cubeRB;
	btTransform cubeTrans;

	/* Cube 1 */
	cubeObj = dynamicsWorld->getCollisionObjectArray()[5];
	cubeRB = btRigidBody::upcast(cubeObj);

	if (cubeRB && cubeRB->getMotionState())
	{
		cubeRB->getMotionState()->getWorldTransform(cubeTrans);
	}
	else
	{
		cubeTrans = cubeObj->getWorldTransform();
	}

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(
		float(cubeTrans.getOrigin().getX()),
		float(cubeTrans.getOrigin().getY()),
		float(cubeTrans.getOrigin().getZ())
		));
	model = glm::rotate(model, cubeTrans.getRotation().getX(), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, cubeTrans.getRotation().getY(), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, cubeTrans.getRotation().getZ(), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(6.0f, 6.0f, 6.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	textures["crate_diffuse"]->Bind(textureSlots["diffuse"]);
	textures["crate_normal"]->Bind(textureSlots["normal"]);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["cube"]->Render();

	/* Cube 2 */
	cubeObj = dynamicsWorld->getCollisionObjectArray()[6];
	cubeRB = btRigidBody::upcast(cubeObj);

	if (cubeRB && cubeRB->getMotionState())
	{
		cubeRB->getMotionState()->getWorldTransform(cubeTrans);
	}
	else
	{
		cubeTrans = cubeObj->getWorldTransform();
	}

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(
		float(cubeTrans.getOrigin().getX()),
		float(cubeTrans.getOrigin().getY()),
		float(cubeTrans.getOrigin().getZ())
		));
	model = glm::rotate(model, cubeTrans.getRotation().getX(), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, cubeTrans.getRotation().getY(), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, cubeTrans.getRotation().getZ(), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(8.0f, 8.0f, 8.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	textures["crate_diffuse"]->Bind(textureSlots["diffuse"]);
	textures["crate_normal"]->Bind(textureSlots["normal"]);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["cube"]->Render();

	if (passType == "main")
	{
		/* Floor */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(100.0f, 2.0f, 100.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_gray"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["cube"]->Render();

		/* Wall 1 */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, -50.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(100.0f, 20.0f, 2.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_blue"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["cube"]->Render();

		/* Wall 2 */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, 50.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(100.0f, 20.0f, 2.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_blue"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["cube"]->Render();

		/* Wall 3 */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-50.0f, 10.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(2.0f, 20.0f, 100.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_blue"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["cube"]->Render();

		/* Wall */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(50.0f, 10.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(2.0f, 20.0f, 100.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_blue"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["cube"]->Render();
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

SceneBullet::~SceneBullet()
{
	BulletCleanup();
}
