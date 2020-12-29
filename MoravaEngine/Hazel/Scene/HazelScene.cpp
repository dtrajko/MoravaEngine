#include "HazelScene.h"
#include "Components.h"
#include "Entity.h"
#include "../Renderer/HazelMesh.h"
#include "../Renderer/SceneRenderer.h"
#include "../Script/ScriptEngine.h"
#include "ScriptableEntity.h"

#include "../../Math.h"

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

	struct Box2DWorldComponent
	{
		std::unique_ptr<b2World> World;
	};

	void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		HazelScene* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;
		HZ_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		ScriptEngine::InitScriptEntity(Entity{ scene->m_EntityIDMap.at(entityID), scene });
	}

	HazelScene::HazelScene(const std::string& debugName)
		: m_DebugName(debugName)
	{
		m_Registry.on_construct<ScriptComponent>().connect<&OnScriptComponentConstruct>();

		m_SceneEntity = m_Registry.create();
		m_Registry.emplace<SceneComponent>(m_SceneEntity, m_SceneID);

		// TODO: Obviously not necessary in all cases
		m_Registry.emplace<Box2DWorldComponent>(m_SceneEntity, std::make_unique<b2World>(b2Vec2{ 0.0f, -9.81f }));

		s_ActiveScenes[m_SceneID] = this;

		Init();
	}

	void HazelScene::Init()
	{
		Log::GetLogger()->error("HazelScene::Init method not implemented yet!");
	}

	// Merge OnUpdate/Render into one function?
	void HazelScene::OnUpdate(float ts)
	{
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

		SceneRenderer::BeginScene(this);

		// Render entities
		m_Registry.view<MeshComponent>().each([=](auto entity, auto& mc)
			{
				// TODO: Should we render (logically)
				SceneRenderer::SubmitEntity(Entity{ entity, this });
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
		glm::mat4* cameraTransform = nullptr;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();

			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = &transform.GetTransform();
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

	void HazelScene::OnEntitySelected(Entity entity)
	{
		// TODO...
	}

	Entity HazelScene::CreateEntity(const std::string& name, const HazelScene& scene)
	{
		Entity entity = CreateEntity(name);
		entity.m_Scene = this;

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

		//	if (!name.empty()) {
		//		entity.AddComponent<TagComponent>(name);
		//	}
		entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);

		auto& entityMap = runtimeMap ? s_RuntimeEntityIDMap : s_EntityIDMap;

		Log::GetLogger()->debug("CreateEntityWithID uuid = '{0}', name = '{1}'", uuid, name);

		HZ_CORE_ASSERT(entityMap.find(uuid) == entityMap.end());
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
			auto& destComponent = dstRegistry.emplace<T>(destEntity, srcComponent);
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
		target->m_SkyboxLOD = m_SkyboxLOD;

		std::unordered_map<UUID, entt::entity> enttMap;
		auto idComponents = m_Registry.view<IDComponent>();
		for (auto entity : idComponents)
		{
			auto uuid = m_Registry.get<IDComponent>(entity).ID;
			Entity e = target->CreateEntityWithID(uuid, "Entity");
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
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();
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
				bodyDef.position.Set(transform[3].x, transform[3].y);

				auto [translation, rotationQuat, scale] = Math::GetTransformDecomposition(transform);
				glm::vec3 rotation = glm::eulerAngles(rotationQuat);
				bodyDef.angle = rotation.z;
				rigidBody2D.RuntimeBody = world->CreateBody(&bodyDef);
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
					fixtureDef.density = 1.0f;
					fixtureDef.friction = 1.0f;
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
		s_ScriptEntityIDMap = &s_EntityIDMap;

		m_IsPlaying = false;
	}

	void HazelScene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

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

	HazelScene::~HazelScene()
	{
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

	template<typename T>
	void HazelScene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
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
	void HazelScene::OnComponentAdded<DirectionalLightComponent>(Entity entity, DirectionalLightComponent& component)
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
