#include "Scene/SceneJolt.h"

#include "Core/Log.h"
#include "Core/Profiler.h"
#include "Core/Timer.h"
#include "ImGui/ImGuiWrapper.h"
#include "LearnOpenGL/SphereJoey.h"
#include "Mesh/Block.h"
#include "Mesh/MeshData.h"
#include "Mesh/Sphere.h"
#include "Physics/PhysicsLayers.h"
#include "Physics/PhysicsMath.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <string>


SceneJolt::SceneJolt()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = false;
	sceneSettings.enableWaterEffects = false;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 10.0f, 40.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 5.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 400.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.2f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.4f;
	sceneSettings.directionalLight.direction = glm::vec3(0.05f, -0.9f, 0.05f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, -60.0f, 60.0f);
	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 0.4f, 0.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pointLights[0].base.diffuseIntensity = 2.0f;
	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pointLights[1].position = glm::vec3(8.92f, 2.75f, -0.85f);
	sceneSettings.pointLights[1].base.diffuseIntensity = 2.0f;
	sceneSettings.pointLights[2].base.color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[2].position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings.pointLights[2].base.diffuseIntensity = 2.0f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.1f;
	sceneSettings.waterHeight = 6.0f; // 1.0f 5.0f
	sceneSettings.waterWaveSpeed = 0.1f;

	SetupPhysics();
	SetLightManager();
	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
}

void SceneJolt::SetLightManager()
{
	Scene::SetLightManager();

	LightManager::pointLights[0].SetAmbientIntensity(2.0f);
	LightManager::pointLights[0].SetDiffuseIntensity(1.0f);
}

void SceneJolt::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_4/right.png");
	skyboxFaces.push_back("Textures/skybox_4/left.png");
	skyboxFaces.push_back("Textures/skybox_4/top.png");
	skyboxFaces.push_back("Textures/skybox_4/bottom.png");
	skyboxFaces.push_back("Textures/skybox_4/back.png");
	skyboxFaces.push_back("Textures/skybox_4/front.png");
	m_Skybox = std::make_shared<Skybox>(skyboxFaces);
}

void SceneJolt::SetupTextures()
{
	textures.insert(std::make_pair("pyramid",             TextureLoader::Get()->GetTexture("Textures/pyramid.png", false, false)));
	textures.insert(std::make_pair("texture_gray",        TextureLoader::Get()->GetTexture("Textures/texture_gray.png", false, false)));
	textures.insert(std::make_pair("texture_orange",      TextureLoader::Get()->GetTexture("Textures/texture_orange.png", false, false)));
	textures.insert(std::make_pair("texture_blue",        TextureLoader::Get()->GetTexture("Textures/texture_blue.png", false, false)));
	textures.insert(std::make_pair("crate_diffuse",       TextureLoader::Get()->GetTexture("Textures/crate.png", false, false)));
	textures.insert(std::make_pair("crate_normal",        TextureLoader::Get()->GetTexture("Textures/crateNormal.png", false, false)));
	textures.insert(std::make_pair("silver_diffuse",      TextureLoader::Get()->GetTexture("Textures/PBR/silver/albedo.png", false, false)));
	textures.insert(std::make_pair("silver_normal",       TextureLoader::Get()->GetTexture("Textures/PBR/silver/normal.png", false, false)));
	textures.insert(std::make_pair("texture_chess",       TextureLoader::Get()->GetTexture("Textures/texture_chess.png", false, GL_NEAREST, false)));
	textures.insert(std::make_pair("texture_checker",     TextureLoader::Get()->GetTexture("Textures/texture_checker.png", false, GL_NEAREST, false)));
	textures.insert(std::make_pair("texture_wall_albedo", TextureLoader::Get()->GetTexture("Textures/PBR/wall/albedo.png", false, GL_LINEAR, false)));
	textures.insert(std::make_pair("texture_wall_normal", TextureLoader::Get()->GetTexture("Textures/PBR/wall/normal.png", false, GL_LINEAR, false)));
	textures.insert(std::make_pair("texture_lego",        TextureLoader::Get()->GetTexture("Textures/lego.png", false, GL_LINEAR, false)));
	textures.insert(std::make_pair("texture_plank",       TextureLoader::Get()->GetTexture("Textures/texture_plank.png", false, GL_LINEAR, false)));
}

void SceneJolt::SetupMeshes()
{
	Mesh* cube = new Mesh();
	cube->Create(&MeshData::vertices[0], &MeshData::indices[0], MeshData::vertexCount, MeshData::indexCount);
	meshes.insert(std::make_pair("cube", cube));

	Block* block_floor = new Block(glm::vec3(100.0f, 4.0f, 100.0f));
	meshes.insert(std::make_pair("block_floor", block_floor));

	Block* block_wall_1 = new Block(glm::vec3(100.0f, 20.0f, 4.0f));
	meshes.insert(std::make_pair("block_wall_1", block_wall_1));

	Block* block_wall_2 = new Block(glm::vec3(4.0f, 20.0f, 100.0f));
	meshes.insert(std::make_pair("block_wall_2", block_wall_2));

	Block* plank_1 = new Block(glm::vec3(1.0f, 1.0f, 12.0f));
	meshes.insert(std::make_pair("plank_1", plank_1));

	Block* plank_2 = new Block(glm::vec3(12.0f, 1.0f, 1.0f));
	meshes.insert(std::make_pair("plank_2", plank_2));

	Sphere* sphere = new Sphere();
	meshes.insert(std::make_pair("sphere", sphere));
}

void SceneJolt::SetupModels()
{
}

void SceneJolt::SetupPhysics()
{
	m_World = std::make_unique<PhysicsWorld>();
	m_World->SetGravity(glm::vec3(0.0f, (float)m_GravityIntensity, 0.0f));

	// Floor
	AddBox(glm::vec3(  0.0f,  0.0f,   0.0f), glm::vec3(50.0f, 2.0f, 50.0f), 0.0f, m_Bounciness);
	// Walls
	AddBox(glm::vec3(  0.0f, 12.0f, -52.0f), glm::vec3(50.0f, 10.0f, 2.0f), 0.0f, m_Bounciness);
	AddBox(glm::vec3(  0.0f, 12.0f,  52.0f), glm::vec3(50.0f, 10.0f, 2.0f), 0.0f, m_Bounciness);
	AddBox(glm::vec3(-52.0f, 12.0f,   0.0f), glm::vec3(2.0f, 10.0f, 50.0f), 0.0f, m_Bounciness);
	AddBox(glm::vec3( 52.0f, 12.0f,   0.0f), glm::vec3(2.0f, 10.0f, 50.0f), 0.0f, m_Bounciness);
	// Crates, resting on the floor (its top y = 2, plus their half size)
	m_Crates[0] = AddBox(glm::vec3( 40.0f, 5.0f,  40.0f), glm::vec3(3.0f), 20.0f, 0.2f);
	m_Crates[1] = AddBox(glm::vec3(-40.0f, 6.0f, -40.0f), glm::vec3(4.0f), 40.0f, 0.2f);

	// The tower starts on the floor (top y = 2), not inside it
	const float towerBase = 2.0f + 0.5f; // floor top + half plank thickness

	for (int i = 0; i < m_PlankFloors; i++)
	{
		m_Planks.push_back(AddBox(glm::vec3(-4.0f, towerBase + i * 2.0f + 0.0f,  0.0f), glm::vec3(0.5f, 0.5f, 6.0f), m_PlankMass, m_PlankBounciness));
		m_Planks.push_back(AddBox(glm::vec3( 4.0f, towerBase + i * 2.0f + 0.0f,  0.0f), glm::vec3(0.5f, 0.5f, 6.0f), m_PlankMass, m_PlankBounciness));
		m_Planks.push_back(AddBox(glm::vec3( 0.0f, towerBase + i * 2.0f + 1.0f, -4.0f), glm::vec3(6.0f, 0.5f, 0.5f), m_PlankMass, m_PlankBounciness));
		m_Planks.push_back(AddBox(glm::vec3( 0.0f, towerBase + i * 2.0f + 1.0f,  4.0f), glm::vec3(6.0f, 0.5f, 0.5f), m_PlankMass, m_PlankBounciness));
	}

	// The bodies were added one by one: the broad phase rebuilt as one tree, as after loading a level
	m_World->GetSystem().OptimizeBroadPhase();

	Log::GetLogger()->info("SceneJolt: {0} bodies", m_World->GetBodyCount());
}

JPH::BodyID SceneJolt::AddBox(glm::vec3 position, glm::vec3 halfExtents, float mass, float bounciness)
{
	return AddBody(new JPH::BoxShape(ToJolt(halfExtents)), position, mass, bounciness);
}

JPH::BodyID SceneJolt::AddSphere(glm::vec3 position, float radius, float mass, float bounciness)
{
	return AddBody(new JPH::SphereShape(radius), position, mass, bounciness);
}

// mass 0: a static body (as in Bullet). The shape is reference counted: the body keeps it
JPH::BodyID SceneJolt::AddBody(const JPH::Shape* shape, glm::vec3 position, float mass, float bounciness)
{
	const bool dynamic = mass > 0.0f;
	JPH::BodyCreationSettings settings(shape, ToJolt(position), JPH::Quat::sIdentity(),
		dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
		dynamic ? ObjectLayers::Dynamic : ObjectLayers::Static);
	settings.mRestitution = bounciness;
	settings.mFriction = 0.5f; // Bullet's default (Jolt's is 0.2): the tower of planks stands as it did
	if (dynamic)
	{
		// The given mass; the inertia from the shape (Jolt would take the mass from the shape's volume, density 1000)
		settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		settings.mMassPropertiesOverride.mMass = mass;
	}
	return m_World->AddBody(settings);
}

void SceneJolt::Fire()
{
	if (!m_FireEnabled) return;
	if ((int)m_Spheres.size() >= m_SphereCountMax) return;

	glm::vec3 position = m_Camera->GetPosition() + glm::vec3(0.0f, -1.0f, 0.0f) + m_Camera->GetFront() * 2.0f;
	JPH::BodyID sphere = AddSphere(position, m_SphereRadius, m_SphereMass, m_Bounciness);
	if (sphere.IsInvalid()) return;
	m_Spheres.push_back(sphere);

	// The shot: an impulse (mass times velocity) at its center
	m_World->GetBodyInterface().AddImpulse(sphere, ToJolt(m_Camera->GetDirection() * m_FireIntensity));
}

void SceneJolt::FixedUpdate(float fixedStep)
{
	m_World->Step(fixedStep);
}

void SceneJolt::Update(float timestep, Window* mainWindow)
{
	Scene::Update(timestep, mainWindow);

	// `timestep` is the absolute time since startup, not the frame time: the frame time is the difference to the previous
	// frame's (the game loop makes fixed steps of it, and counts a long one, after a hitch or a breakpoint, as 0.25 s)
	float currentTime = Timer::Get()->GetCurrentTimestamp(); // updated by Scene::Update above
	float deltaTime = m_LastPhysicsTimestamp < 0.0f ? 0.0f : std::max(currentTime - m_LastPhysicsTimestamp, 0.0f);
	m_LastPhysicsTimestamp = currentTime;

	if (mainWindow->getMouseButtons()[GLFW_MOUSE_BUTTON_LEFT])
	{
		if (currentTime - m_LastTimestep > m_FireCooldown)
		{
			Fire();
			m_LastTimestep = currentTime;
		}
	}

	m_World->SetGravity(glm::vec3(0.0f, (float)m_GravityIntensity, 0.0f));

	{
		Profiler profiler("SceneJolt::PhysicsSteps");
		m_GameLoop.Frame(deltaTime, this);
		m_ProfilerResults.insert(std::make_pair(profiler.GetName(), profiler.Stop()));
	}

	// The orange light follows the latest sphere
	if (!m_Spheres.empty())
	{
		LightManager::pointLights[0].SetPosition(m_World->GetBodyPosition(m_Spheres.back()));
	}
}

void SceneJolt::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	// The Settings panel docked at the left of the window while nothing is docked there (imgui.ini, shared with the other
	// scenes, may have no place for it: it would float over the scene). Moved or undocked later, it stays where it is put
	ImGuiDockNode* dockspace = m_DockspaceID ? ImGui::DockBuilderGetNode(m_DockspaceID) : nullptr;
	if (dockspace && dockspace->IsLeafNode() && !m_SettingsDocked)
	{
		ImGuiID central = 0;
		const ImGuiID left = ImGui::DockBuilderSplitNode(m_DockspaceID, ImGuiDir_Left, 0.28f, nullptr, &central);
		ImGui::DockBuilderDockWindow("Settings", left);
		ImGui::DockBuilderFinish(m_DockspaceID);
	}
	m_SettingsDocked = true; // checked once: the user's layout from here on

	ImGui::Begin("Settings");
	{
		glm::vec3 lightDirection = LightManager::directionalLight.GetDirection();

		// Point light for the fired spheres
		glm::vec3 PL0_Position = LightManager::pointLights[0].GetPosition();
		glm::vec3 PL0_Color = LightManager::pointLights[0].GetColor();
		float PL0_AmbIntensity = LightManager::pointLights[0].GetAmbientIntensity();
		float PL0_DiffIntensity = LightManager::pointLights[0].GetDiffuseIntensity();

		ImGui::SliderFloat3("DirLight Direction", glm::value_ptr(lightDirection), -1.0f, 1.0f);
		ImGui::ColorEdit3("PL0 Color", glm::value_ptr(PL0_Color));
		ImGui::SliderFloat3("PL0 Position", glm::value_ptr(PL0_Position), -20.0f, 20.0f);
		ImGui::SliderFloat("PL0 Amb Intensity", &PL0_AmbIntensity, -20.0f, 20.0f);
		ImGui::SliderFloat("PL0 Diff Intensity", &PL0_DiffIntensity, -20.0f, 20.0f);
		ImGui::SliderInt("Gravity Intensity", &m_GravityIntensity, -10, 10);
		ImGui::SliderFloat("Bounciness", &m_Bounciness, 0.0f, 2.0f);
		ImGui::Checkbox("Fire Enabled", &m_FireEnabled);
		ImGui::SliderFloat("Fire Intensity", &m_FireIntensity, 0.0f, m_FireIntensityMax);
		ImGui::SliderFloat("Tiling Factor", &m_TilingFactor, 0.0f, 2.0f);
		ImGui::Text("Spheres: %d/%d", (int)m_Spheres.size(), m_SphereCountMax);
		bool wireframeEnabled = IsWireframeEnabled();
		ImGui::Checkbox("Wireframe Enabled [R]", &wireframeEnabled);
		SetWireframeEnabled(wireframeEnabled);

		// Jolt's state: bodies asleep stay out of the step until something touches them
		ImGui::Separator();
		ImGui::Text("Bodies: %u (%u awake)", m_World->GetBodyCount(), m_World->GetActiveBodyCount());
		ImGui::Text("Fixed steps: %llu", (unsigned long long)m_GameLoop.GetFixedStepCount());

		// print profiler results
		ImGui::Separator();
		ImGui::Text("Profiler results:");
		for (auto& profilerResult : m_ProfilerResults)
		{
			ImGui::Text("%.2fms %s", profilerResult.second, profilerResult.first.c_str());
		}
		m_ProfilerResults.clear();

		LightManager::directionalLight.SetDirection(lightDirection);

		LightManager::pointLights[0].SetColor(PL0_Color);
		LightManager::pointLights[0].SetAmbientIntensity(PL0_AmbIntensity);
		LightManager::pointLights[0].SetDiffuseIntensity(PL0_DiffIntensity);
	}
	ImGui::End();
}

void SceneJolt::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
	glm::mat4 model;
	for (JPH::BodyID sphere : m_Spheres)
	{
		model = m_World->GetBodyTransform(sphere); // position and rotation of the body
		model = glm::scale(model, glm::vec3(m_SphereRadius));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["silver_diffuse"]->Bind(textureSlots["diffuse"]);
		textures["silver_normal"]->Bind(textureSlots["normal"]);
		if (passType == "main")
		{
			materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		}
		meshes["sphere"]->Render();
	}

	// Crates (the cube mesh is 2 units wide: scaled by twice their half size)
	const float crateScales[2] = { 6.0f, 8.0f };
	for (int i = 0; i < 2; i++)
	{
		model = m_World->GetBodyTransform(m_Crates[i]);
		model = glm::scale(model, glm::vec3(crateScales[i]));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["crate_diffuse"]->Bind(textureSlots["diffuse"]);
		textures["crate_normal"]->Bind(textureSlots["normal"]);
		if (passType == "main")
		{
			materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		}
		meshes["cube"]->Render();
	}

	for (size_t i = 0; i < m_Planks.size(); i++)
	{
		model = m_World->GetBodyTransform(m_Planks[i]);
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_plank"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		if (passType == "main")
		{
			materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		}

		if (i % 4 == 0 || i % 4 == 1)
			meshes["plank_1"]->Render();
		else
			meshes["plank_2"]->Render();
	}

	if (passType == "main")
	{
		shaders["main"]->Bind();
		shaders["main"]->SetFloat("tilingFactor", m_TilingFactor);

		/* Floor */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_lego"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["block_floor"]->Render();

		/* Wall 1 */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 12.0f, -52.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_lego"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["block_wall_1"]->Render();

		/* Wall 2 */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 12.0f, 52.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_lego"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["block_wall_1"]->Render();

		/* Wall 3 */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-52.0f, 12.0f, 0.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_lego"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["block_wall_2"]->Render();

		/* Wall 4 */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(52.0f, 12.0f, 0.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["texture_lego"]->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["block_wall_2"]->Render();
	}
}

SceneJolt::~SceneJolt()
{
	m_World.reset(); // its bodies, before the process shuts Jolt down
}
