#include "HazelScene.h"
#include "Components.h"
#include "Entity.h"
#include "ScriptableEntity.h"
#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Script/ScriptEngine.h"

#include "Core/Math.h"
#include "EnvMap/EnvMapSceneRenderer.h"

#include <glm/glm.hpp>

// Box2D
#include <box2d/box2d.h>

#include <string>
#include <unordered_map>


namespace Hazel {

	static const std::string DefaultEntityName = "Entity";

	std::unordered_map<UUID, HazelScene*> s_ActiveScenes;
	std::unordered_map<UUID, Entity> s_EntityIDMap;
	std::unordered_map<UUID, Entity> s_RuntimeEntityIDMap;
	std::unordered_map<UUID, Entity>* s_ScriptEntityIDMap = &s_EntityIDMap;

	struct SceneComponent
	{
		UUID SceneID;
	};

	// TODO: move to physics file
	class ContactListener : public b2ContactListener
	{
	public:
		virtual void BeginContact(b2Contact* contact) override
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

			// TODO: improve these if checks
			if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
			{
				ScriptEngine::OnCollision2DBegin(a);
			}

			if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName)) {
				ScriptEngine::OnCollision2DBegin(b);
			}
		}

		virtual void EndContact(b2Contact* contact) override
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

			// TODO: improve these if checks
			if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName)) {
				ScriptEngine::OnCollision2DEnd(a);
			}

			if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName)) {
				ScriptEngine::OnCollision2DEnd(b);
			}
		}

		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override
		{
			B2_NOT_USED(contact);
			B2_NOT_USED(oldManifold);
		}

		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override
		{
			B2_NOT_USED(contact);
			B2_NOT_USED(impulse);
		}
	};

	static ContactListener s_Box2DContactListener;

	struct Box2DWorldComponent
	{
		std::unique_ptr<b2World> World;
	};

	static void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		HazelScene* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;
		HZ_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		ScriptEngine::InitScriptEntity(Entity{ scene->m_EntityIDMap.at(entityID), scene });
	}

	static void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		HazelScene* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;

		HZ_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		ScriptEngine::OnScriptComponentDestroyed(sceneID, entityID);
	}

	HazelScene::HazelScene(const std::string& debugName, bool isEditorScene)
		: m_DebugName(debugName)
	{
		m_Registry.on_construct<ScriptComponent>().connect<&OnScriptComponentConstruct>();
		m_Registry.on_destroy<ScriptComponent>().connect<&OnScriptComponentDestroy>();

		m_SceneEntity = m_Registry.create();
		m_Registry.emplace<SceneComponent>(m_SceneEntity, m_SceneID);

		// TODO: Obviously not necessary in all cases
		Box2DWorldComponent& b2dWorld = m_Registry.emplace<Box2DWorldComponent>(m_SceneEntity, std::make_unique<b2World>(b2Vec2{ 0.0f, -9.81f }));
		b2dWorld.World->SetContactListener(&s_Box2DContactListener);

		s_ActiveScenes[m_SceneID] = this;

		Init();
	}

	HazelScene::~HazelScene()
	{
		m_Registry.on_destroy<ScriptComponent>().disconnect();

		m_Registry.clear();
		s_ActiveScenes.erase(m_SceneID);
		ScriptEngine::OnSceneDestruct(m_SceneID);

		// Destroy scripts
		{
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
				{
					// TODO: Move to Scene::OnSceneStop
					if (nsc.Instance)
					{
						nsc.Instance->OnDestroy();
						nsc.DestroyScript(&nsc);
					}
				});
		}
	}

	void HazelScene::Init()
	{
		if (Hazel::RendererAPI::Current() == Hazel::RendererAPIType::Vulkan) {
			// auto skyboxShader = HazelShader::Create("assets/shaders/Renderer2D.glsl");

			// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Skybox.glsl");
			// auto skyboxShader = HazelRenderer::GetShaderLibrary()->Get("Skybox"); // Spir-V method // Pre-load shaders in order to use Get
			// m_SkyboxMaterial = HazelMaterial::Create(skyboxShader);
			// m_SkyboxMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);
		}
		else {
			auto skyboxShader = Shader::Create("Shaders/Hazel/Skybox.vs", "Shaders/Hazel/Skybox.fs");
			m_SkyboxMaterial = Material::Create(skyboxShader);
			m_SkyboxMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);
		}
	}

	// Merge OnUpdate/Render into one function?
	void HazelScene::OnUpdate(Timestep ts)
	{
		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& box2DWorld = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;
		int32_t velocityIterations = 6;
		int32_t positionIterations = 2;
		box2DWorld->Step(ts, velocityIterations, positionIterations);

		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& tc = e.Transform();
				auto& rb2d = e.GetComponent<RigidBody2DComponent>();
				b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);

				auto& position = body->GetPosition();
				auto [translation, rotationQuat, scale] = Math::GetTransformDecomposition(tc.GetTransform());
				glm::vec3 rotation = glm::eulerAngles(rotationQuat);

				tc.GetTransform() = glm::translate(glm::mat4(1.0f), { position.x, position.y, tc.GetTransform()[3].z }) *
					glm::toMat4(glm::quat({ rotation.x, rotation.y, body->GetAngle() })) *
					glm::scale(glm::mat4(1.0f), scale);
			}
		}

		//	ECS Update all entities
		auto view = m_Registry.view<ScriptComponent>();
		for (auto entity : view)
		{
			UUID entityID = m_Registry.get<IDComponent>(entity).ID;
			Entity e = { entity, this };
			if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName)) {
				ScriptEngine::OnUpdateEntity(m_SceneID, entityID, ts);
			}
		}

		m_Registry.view<MeshComponent>().each([=](auto entity, auto& mc)
			{
				auto mesh = mc.Mesh;
				if (mesh) {
					mesh->OnUpdate(ts, false);
				}
			});

		SceneRenderer::BeginScene(this, { m_Camera, m_Camera.GetViewMatrix() });

		// Render entities
		m_Registry.view<MeshComponent>().each([=](auto entity, auto& mc)
		{
			// TODO: Should we render (logically)
			EnvMapSceneRenderer::SubmitEntity(Entity{ entity, this });
		});

		SceneRenderer::EndScene();

		// Update scripts
		{
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
				{
					// TODO: Move to Scene::OnScenePlay
					if (!nsc.Instance)
					{
						nsc.Instance = nsc.InstantiateScript();
						nsc.Instance->m_Entity = Entity{ entity, this };
						nsc.Instance->OnCreate();
					}

					nsc.Instance->OnUpdate(ts);
				});
		}

		// Render 2D
		HazelCamera* mainCamera = nullptr;
		glm::mat4 cameraTransform = glm::mat4(1.0f);
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();

			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transform.GetTransform();
					break;
				}
			}
		}

		if (mainCamera)
		{
			// Renderer2D::BeginScene(mainCamera->GetProjection(), *cameraTransform);

			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				// Renderer2D::DrawQuad(transform, sprite.Color);
			}

			// Renderer2D::EndScene();
		}
	}

	void HazelScene::OnRenderRuntime(Timestep ts)
	{
	}

	void HazelScene::OnRenderEditor(Timestep ts, const EditorCamera& editorCamera)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		if (Hazel::RendererAPI::Current() == Hazel::RendererAPIType::Vulkan)
		{
			m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

			auto group = m_Registry.group<MeshComponent>(entt::get<TransformComponent>);

			SceneRenderer::BeginScene(this, { editorCamera, editorCamera.GetViewMatrix() });
			for (auto entity : group)
			{
				auto [meshComponent, transformComponent] = group.get<MeshComponent, TransformComponent>(entity);
				if (meshComponent.Mesh)
				{
					meshComponent.Mesh->OnUpdate(ts, false);

					// TODO: Should we render (logically)

					if (m_SelectedEntity == entity) {
						SceneRenderer::SubmitSelectedMesh(meshComponent, transformComponent);
					}
					else {
						SceneRenderer::SubmitMesh(meshComponent, transformComponent);
					}
				}
			}
			SceneRenderer::EndScene();
		}
		/////////////////////////////////////////////////////////////////////
	}

	void HazelScene::SetEnvironment(const Environment& environment)
	{
		m_Environment = environment;
		SetSkybox(environment.RadianceMap);
	}

	void HazelScene::SetSkybox(const Ref<HazelTextureCube>& skybox)
	{
		m_SkyboxTexture = skybox;
		m_ShaderSkybox->setInt("u_Texture", skybox.Raw()->GetID());
	}

	Entity HazelScene::GetMainCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& comp = view.get<CameraComponent>(entity);
			if (comp.Primary)
				return { entity, this };
		}
		return {};
	}

	void HazelScene::OnEntitySelected(Entity entity)
	{
		// TODO...
	}

	Entity HazelScene::CreateEntity(const std::string& name, Ref<HazelScene> scene)
	{
		Entity entity = CreateEntity(name);
		entity.m_Scene = scene.Raw();

		return entity;
	}

	Entity HazelScene::CreateEntity(const std::string& name)
	{
		const std::string& entityName = name.empty() ? DefaultEntityName : name;

		// ECS
		auto entity = Entity{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();

		entity.AddComponent<TransformComponent>(glm::vec3(0.0f)); // glm::mat4(1.0f)

		// auto& tag = entity.AddComponent<TagComponent>();
		// tag.Tag = name.empty() ? "Entity" : name;
		entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);

		Log::GetLogger()->debug("CreateEntity name = '{0}'", name);

		s_EntityIDMap[idComponent.ID] = entity;
		return entity;
	}

	Entity HazelScene::CreateEntityWithID(UUID uuid, const std::string& name, bool runtimeMap)
	{
		auto entity = Entity{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = uuid;

		entity.AddComponent<TransformComponent>(glm::vec3(0.0f)); // glm::mat4(1.0f)

		if (!name.empty()) {
			entity.AddComponent<TagComponent>(name);
		}
		// entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);

		auto& entityMap = runtimeMap ? s_RuntimeEntityIDMap : s_EntityIDMap;

		Log::GetLogger()->debug("CreateEntityWithID uuid = '{0}', name = '{1}'", uuid, name);

		// HZ_CORE_ASSERT(entityMap.find(uuid) == entityMap.end());
		entityMap[uuid] = entity;
		return entity;
	}

	void HazelScene::DestroyEntity(Entity entity)
	{
		if (entity.HasComponent<ScriptComponent>())
		{
			ScriptEngine::OnScriptComponentDestroyed(m_SceneID, entity.GetUUID());
		}

		m_Registry.destroy(entity.m_EntityHandle);
	}

	template<typename T>
	static void CopyComponent(entt::registry& srcRegistry, entt::registry& dstRegistry, std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto components = srcRegistry.view<T>();
		for (auto srcEntity : components)
		{
			entt::entity destEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

			auto& srcComponent = srcRegistry.get<T>(srcEntity);
			auto& destComponent = dstRegistry.emplace_or_replace<T>(destEntity, srcComponent);
		}
	}

	template<typename T>
	static void CopyComponentIfExists(entt::entity dst, entt::entity src, entt::registry& registry)
	{
		if (registry.has<T>(src))
		{
			auto& srcComponent = registry.get<T>(src);
			registry.emplace_or_replace<T>(dst, srcComponent);
		}
	}

	void HazelScene::DuplicateEntity(Entity entity)
	{
		Entity newEntity;
		if (entity.HasComponent<TagComponent>()) {
			newEntity = CreateEntity(entity.GetComponent<TagComponent>().Tag);
		}
		else {
			newEntity = CreateEntity();
		}

		CopyComponentIfExists<TransformComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MeshComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<DirectionalLightComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SkyLightComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<ScriptComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CameraComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MaterialComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
	}

	Entity HazelScene::FindEntityByTag(const std::string& tag)
	{
		// TODO: If this becomes used often, consider indexing by tag
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			auto& candidate = view.get<TagComponent>(entity).Tag;
			if (candidate == tag)
			{
				return Entity(entity, this);
			}
		}

		return Entity{};
	}

	/**
	 * Working on Hazel LIVE! #14
	 * 
	 * Copy to runtime
	 */
	void HazelScene::CopyTo(Ref<HazelScene>& target)
	{
		// Environment
		target->m_Light = m_Light;
		target->m_LightMultiplier = m_LightMultiplier;

		target->m_Environment = m_Environment;
		target->m_SkyboxTexture = m_SkyboxTexture;
		target->m_SkyboxMaterial = m_SkyboxMaterial;
		target->m_SkyboxLod = m_SkyboxLod;

		std::unordered_map<UUID, entt::entity> enttMap;
		auto idComponents = m_Registry.view<IDComponent>();
		for (auto entity : idComponents)
		{
			auto uuid = m_Registry.get<IDComponent>(entity).ID;
			Entity e = target->CreateEntityWithID(uuid, "Entity", true);
			enttMap[uuid] = e.m_EntityHandle;
		}

		CopyComponent<TagComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TransformComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MeshComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<DirectionalLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SkyLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<ScriptComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CameraComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SpriteRendererComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RigidBody2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<BoxCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CircleCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MaterialComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<NativeScriptComponent>(target->m_Registry, m_Registry, enttMap);

		const auto& entityInstanceMap = ScriptEngine::GetEntityInstanceMap();
		if (entityInstanceMap.find(target->GetUUID()) != entityInstanceMap.end()) {
			ScriptEngine::CopyEntityScriptData(target->GetUUID(), m_SceneID);
		}

		target->SetPhysics2DGravity(GetPhysics2DGravity());
	}

	Ref<HazelScene> HazelScene::GetScene(UUID uuid)
	{
		if (s_ActiveScenes.find(uuid) != s_ActiveScenes.end()) {
			return s_ActiveScenes.at(uuid);
		}

		return {};
	}

	void HazelScene::SetPhysics2DGravity(float gravity)
	{
		m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->SetGravity({ 0.0f, gravity });
	}

	float HazelScene::GetPhysics2DGravity() const
	{
		return m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->GetGravity().y;
	}

	Entity HazelScene::CloneEntity(Entity entity)
	{
		Entity entityClone = Entity(m_Registry.create(), this);

		if (entity.HasComponent<IDComponent>()) {
			entityClone.AddComponent<IDComponent>(entity.GetComponent<IDComponent>());
		}

		if (entity.HasComponent<TagComponent>()) {
			entityClone.AddComponent<TagComponent>(entity.GetComponent<TagComponent>());
		}

		if (entity.HasComponent<TransformComponent>()) {
			entityClone.AddComponent<TransformComponent>(entity.GetComponent<TransformComponent>());
		}

		if (entity.HasComponent<MeshComponent>()) {
			entityClone.AddComponent<MeshComponent>(entity.GetComponent<MeshComponent>());
		}

		if (entity.HasComponent<ScriptComponent>()) {
			entityClone.AddComponent<ScriptComponent>(entity.GetComponent<ScriptComponent>());
		}

		if (entity.HasComponent<CameraComponent>()) {
			entityClone.AddComponent<CameraComponent>(entity.GetComponent<CameraComponent>());
		}

		if (entity.HasComponent<SpriteRendererComponent>()) {
			entityClone.AddComponent<SpriteRendererComponent>(entity.GetComponent<SpriteRendererComponent>());
		}

		if (entity.HasComponent<RigidBody2DComponent>()) {
			entityClone.AddComponent<RigidBody2DComponent>(entity.GetComponent<RigidBody2DComponent>());
		}

		if (entity.HasComponent<BoxCollider2DComponent>()) {
			entityClone.AddComponent<BoxCollider2DComponent>(entity.GetComponent<BoxCollider2DComponent>());
		}

		if (entity.HasComponent<CircleCollider2DComponent>()) {
			entityClone.AddComponent<CircleCollider2DComponent>(entity.GetComponent<CircleCollider2DComponent>());
		}

		if (entity.HasComponent<MaterialComponent>()) {
			entityClone.AddComponent<MaterialComponent>(entity.GetComponent<MaterialComponent>());
		}

		if (entity.HasComponent<DirectionalLightComponent>()) {
			entityClone.AddComponent<DirectionalLightComponent>(entity.GetComponent<DirectionalLightComponent>());
		}

		if (entity.HasComponent<SkyLightComponent>()) {
			entityClone.AddComponent<SkyLightComponent>(entity.GetComponent<SkyLightComponent>());
		}

		if (entity.HasComponent<NativeScriptComponent>()) {
			entityClone.AddComponent<NativeScriptComponent>(entity.GetComponent<NativeScriptComponent>());
		}

		Log::GetLogger()->warn("Method HazelScene::CopyEntity implemented poorly [Tag: '{0}']", entity.GetComponent<TagComponent>().Tag);

		return entityClone;
	}

	void HazelScene::OnEvent(Event& e)
	{
	}

	void HazelScene::OnRuntimeStart()
	{
		s_ScriptEntityIDMap = &s_RuntimeEntityIDMap;

		auto view = m_Registry.view<ScriptComponent>();
		for (auto entity : view) {
			Entity e = { entity, this };
			if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName)) { // TODO implemented method
				ScriptEngine::InstantiateEntityClass({ entity, this });
			}
		}

		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& world = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;
		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			m_PhysicsBodyEntityBuffer = new Entity[view.size()];
			uint32_t physicsBodyEntityBufferIndex = 0;
			for (auto entity : view)
			{
				Entity e = { entity, this };
				UUID entityID = e.GetComponent<IDComponent>().ID;
				auto& tc = e.Transform();
				auto& rigidBody2D = m_Registry.get<RigidBody2DComponent>(entity);

				b2BodyDef bodyDef;
				if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Static) {
					bodyDef.type = b2_staticBody;
				}
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Dynamic) {
					bodyDef.type = b2_dynamicBody;
				}
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Kinematic) {
					bodyDef.type = b2_kinematicBody;
				}
				bodyDef.position.Set(tc.GetTransform()[3].x, tc.GetTransform()[3].y);

				auto [translation, rotationQuat, scale] = Math::GetTransformDecomposition(tc.GetTransform());
				glm::vec3 rotation = glm::eulerAngles(rotationQuat);
				bodyDef.angle = rotation.z;

				b2Body* body = world->CreateBody(&bodyDef);
				body->SetFixedRotation(rigidBody2D.FixedRotation);
				Entity* entityStorage = &m_PhysicsBodyEntityBuffer[physicsBodyEntityBufferIndex++];
				*entityStorage = e;
				body->GetUserData().pointer = (uintptr_t)entityStorage;
				rigidBody2D.RuntimeBody = body;
			}
		}

		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& boxCollider2D = m_Registry.get<BoxCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					HZ_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2PolygonShape polygonShape;
					polygonShape.SetAsBox(boxCollider2D.Size.x, boxCollider2D.Size.y);

					// TODO:
					b2FixtureDef fixtureDef;
					fixtureDef.shape = &polygonShape;
					fixtureDef.density = boxCollider2D.Density;
					fixtureDef.friction = boxCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		{
			auto view = m_Registry.view<CircleCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& circleCollider2D = m_Registry.get<CircleCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					HZ_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2CircleShape circleShape;
					circleShape.m_radius = circleCollider2D.Radius;

					// TODO:
					b2FixtureDef fixtureDef;
					fixtureDef.shape = &circleShape;
					fixtureDef.density = 1.0f;
					fixtureDef.friction = 1.0f;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		m_IsPlaying = true;
	}

	void HazelScene::OnRuntimeStop()
	{
		delete[] m_PhysicsBodyEntityBuffer;
		m_IsPlaying = false;

		s_ScriptEntityIDMap = &s_EntityIDMap;
	}

	void HazelScene::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	void HazelScene::OnViewportResize(uint32_t width, uint32_t height)
	{
		SetViewportSize(width, height);

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio) {
				cameraComponent.Camera.SetViewportSize((float)width, (float)height);
			}
		}
	}

	template<typename T>
	void HazelScene::OnComponentAdded(Entity entity, T& component)
	{
		// static_assert(false);
	}

	template<>
	void HazelScene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<MeshComponent>(Entity entity, MeshComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		component.Camera.SetViewportSize((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	template<>
	void HazelScene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<RigidBody2DComponent>(Entity entity, RigidBody2DComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<MaterialComponent>(Entity entity, MaterialComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<DirectionalLightComponent>(Entity entity, DirectionalLightComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<PointLightComponent>(Entity entity, PointLightComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<SpotLightComponent>(Entity entity, SpotLightComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<SkyLightComponent>(Entity entity, SkyLightComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

}
