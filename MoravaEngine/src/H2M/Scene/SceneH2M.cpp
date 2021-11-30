#include "SceneH2M.h"

#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"
#include "H2M/Scene/ComponentsH2M.h"
#include "H2M/Scene/EntityH2M.h"

#include "Core/Math.h"
#include "EnvMap/EnvMapSceneRenderer.h"

#include <glm/glm.hpp>

// Box2D
#include <box2d/box2d.h>

#include <string>
#include <unordered_map>


namespace H2M {

	static const std::string DefaultEntityName = "EntityH2M";

	std::unordered_map<UUID, SceneH2M*> s_ActiveScenes;
	std::unordered_map<UUID, EntityH2M> s_EntityIDMap;
	std::unordered_map<UUID, EntityH2M> s_RuntimeEntityIDMap;
	std::unordered_map<UUID, EntityH2M>* s_ScriptEntityIDMap = &s_EntityIDMap;

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
			EntityH2M& a = *(EntityH2M*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			EntityH2M& b = *(EntityH2M*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;
		}

		virtual void EndContact(b2Contact* contact) override
		{
			EntityH2M& a = *(EntityH2M*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			EntityH2M& b = *(EntityH2M*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;
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

		SceneH2M* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;
		HZ_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		// ScriptEngine::InitScriptEntity(Entity{ scene->m_EntityIDMap.at(entityID), scene });
	}

	static void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		SceneH2M* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;

		HZ_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		ScriptEngine::OnScriptComponentDestroyed(sceneID, entityID);
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

	SceneH2M::SceneH2M(const std::string& debugName, bool isEditorScene)
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

	SceneH2M::~SceneH2M()
	{
		m_Registry.on_destroy<ScriptComponent>().disconnect();

		m_Registry.clear();
		s_ActiveScenes.erase(m_SceneID);
		ScriptEngine::OnSceneDestruct(m_SceneID);
	}

	void SceneH2M::Init()
	{
		MoravaShaderSpecification moravaShaderSpec;

		switch (H2M::RendererAPIH2M::Current())
		{
		case H2M::RendererAPIH2MType::OpenGL:
			moravaShaderSpec.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
			moravaShaderSpec.VertexShaderPath = "Shaders/Hazel/Skybox.vs";
			moravaShaderSpec.FragmentShaderPath = "Shaders/Hazel/Skybox.fs";
			break;
		case H2M::RendererAPIH2MType::Vulkan:
			moravaShaderSpec.ShaderType = MoravaShaderSpecification::ShaderType::HazelShader;
			moravaShaderSpec.HazelShaderPath = "Resources/Shaders/Skybox.glsl";
			break;
		case H2M::RendererAPIH2MType::DX11:
			moravaShaderSpec.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
			moravaShaderSpec.VertexShaderPath = "Shaders/HLSL/UnlitVertexShader.hlsl";
			moravaShaderSpec.PixelShaderPath = "Shaders/HLSL/UnlitPixelShader.hlsl";
			break;
		}
		moravaShaderSpec.ForceCompile = false;
		auto skyboxShader = MoravaShader::Create(moravaShaderSpec);

		m_SkyboxMaterial = Material::Create(skyboxShader);
		m_SkyboxMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);

		// auto skyboxShader = HazelShader::Create("Resources/Shaders/Renderer2D.glsl");
		// HazelRenderer::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
		// auto skyboxShader = HazelRenderer::GetShaderLibrary()->Get("Skybox"); // Spir-V method // Pre-load shaders in order to use Get
		// m_SkyboxMaterial = HazelMaterial::Create(skyboxShader);
		// m_SkyboxMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);
	}

	// Merge OnUpdate/Render into one function?
	void SceneH2M::OnUpdate(Timestep ts)
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
				EntityH2M e = { entity, this };
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
			EntityH2M e = { entity, this };
			if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName)) {
				// ScriptEngine::OnUpdateEntity(e, ts);
			}
		}

		m_Registry.view<MeshComponentH2M>().each([=](auto entity, auto& mc)
			{
				auto mesh = mc.Mesh;
				if (mesh) {
					mesh->OnUpdate(ts);
				}
			});

		SceneRendererH2M::BeginScene(nullptr, { m_Camera, m_Camera.GetViewMatrix() });

		// Render entities
		m_Registry.view<MeshComponentH2M>().each([=](auto entity, auto& mc)
			{
				// TODO: Should we render (logically)
				EnvMapSceneRenderer::SubmitEntity(EntityH2M{ entity, this });
			});

		SceneRendererH2M::EndScene();

		// Render 2D
		HazelCamera* mainCamera = nullptr;
		glm::mat4 cameraTransform = glm::mat4(1.0f);
		{
			auto view = m_Registry.view<TransformComponentH2M, CameraComponent>();

			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponentH2M, CameraComponent>(entity);

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
			// Renderer2D::BeginScene(mainCamera->GetProjectionMatrix(), *cameraTransform);

			auto group = m_Registry.group<TransformComponentH2M>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponentH2M, SpriteRendererComponent>(entity);

				// Renderer2D::DrawQuad(transform, sprite.Color);
			}

			// Renderer2D::EndScene();
		}
	}

	void SceneH2M::OnRenderRuntime(Ref<SceneRendererH2M> renderer, Timestep ts)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		EntityH2M cameraEntity = GetMainCameraEntity();
		if (!cameraEntity) return;

		// Process camera entity
		glm::mat4 cameraViewMatrix = glm::inverse(cameraEntity.GetComponent<TransformComponentH2M>().Transform);
		HZ_CORE_ASSERT(cameraEntity, "Scene does not contain any cameras!");
		SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>();
		camera.SetViewportSize((float)m_ViewportWidth, (float)m_ViewportHeight);

		// Process lights
		{
			m_LightEnvironment = LightEnvironmentLegacy();
			auto lights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponentH2M>);
			uint32_t directionalLightIndex = 0;
			for (auto entity : lights)
			{
				auto [transformComponent, lightComponent] = lights.get<TransformComponentH2M, DirectionalLightComponent>(entity);
				glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
				m_LightEnvironment.DirectionalLights[directionalLightIndex++] =
				{
					direction,
					lightComponent.Radiance,
					1.0f,
					lightComponent.CastShadows,
				};
			}
		}

		// TODO: only one sky light at the moment!
		{
			m_Environment = Ref<Environment>::Create();
			auto lights = m_Registry.group<SkyLightComponentH2M>(entt::get<TransformComponentH2M>);
			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponentH2M, SkyLightComponentH2M>(entity);
				// m_Environment = skyLightComponent.SceneEnvironment;
				SetSkybox(m_Environment->RadianceMap);
			}
		}

		m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

		auto group = m_Registry.group<MeshComponentH2M>(entt::get<TransformComponentH2M>);
		// renderer->SetScene(this);
		// renderer->BeginScene({ camera, cameraViewMatrix, 0.1f, 1000.0f, 45.0f }); //TODO: real values
		for (auto entity : group)
		{
			auto [transformComponent, meshComponent] = group.get<TransformComponentH2M, MeshComponentH2M>(entity);
			if (meshComponent.Mesh /* && !meshComponent.Mesh->IsFlagSet(AssetFlag::Missing) */)
			{
				meshComponent.Mesh->OnUpdate(ts);
				EntityH2M e = EntityH2M(entity, this);
				glm::mat4 transform = GetTransformRelativeToParent(e);

				//	if (e.HasComponent<RigidBodyComponent>())
				//		transform = e.Transform().GetTransform();

				// TODO: Should we render (logically)
				// renderer->SubmitMesh(meshComponent.Mesh, meshComponent.MaterialTable, transform);
			}
		}

		// renderer->EndScene();
	}

	void SceneH2M::OnRenderEditor(Ref<SceneRendererH2M> renderer, Timestep ts, const EditorCamera& editorCamera)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		if (RendererAPI::Current() == RendererAPIType::Vulkan)
		{
			m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

			auto group = m_Registry.group<MeshComponentH2M>(entt::get<TransformComponentH2M>);
			SceneRendererH2M::BeginScene(nullptr, { editorCamera, editorCamera.GetViewMatrix() });
			for (auto entity : group)
			{
				auto& [meshComponent, transformComponent] = group.get<MeshComponentH2M, TransformComponentH2M>(entity);
				if (meshComponent.Mesh)
				{
					meshComponent.Mesh->OnUpdate(ts);

					// TODO: Should we render (logically)

					if (m_SelectedEntity == entity)
					{
						SceneRendererH2M::SubmitSelectedMesh(meshComponent, transformComponent);
					}
					else {
						SceneRendererH2M::SubmitMesh(meshComponent, transformComponent);
					}
				}
			}
			SceneRendererH2M::EndScene();

			// the following code replaces the VulkanRenderer::Draw() method
			// VulkanRenderer::SetCamera((HazelCamera)editorCamera); // s_Data.SceneData.SceneCamera.Camera = *camera;
			// VulkanRenderer::GeometryPass();
			// VulkanRenderer::CompositePass();
		}
		else
		{
			// Process lights
			{
				m_LightEnvironment = LightEnvironmentLegacy();
				auto lights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponentH2M>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : lights)
				{
					auto [transformComponent, lightComponent] = lights.get<TransformComponentH2M, DirectionalLightComponent>(entity);
					glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
					m_LightEnvironment.DirectionalLights[directionalLightIndex++] =
					{
						direction,
						lightComponent.Radiance,
						1.0f,
						lightComponent.CastShadows,
					};
				}
			}

			// TODO: only one sky light at the moment!
			{
				m_Environment = Ref<Environment>::Create();
				auto lights = m_Registry.group<SkyLightComponentH2M>(entt::get<TransformComponentH2M>);
				for (auto entity : lights)
				{
					auto [transformComponent, skyLightComponent] = lights.get<TransformComponentH2M, SkyLightComponentH2M>(entity);
					// m_Environment = skyLightComponent.SceneEnvironment;
					SetSkybox(m_Environment->RadianceMap);
				}
			}

			if (H2M::RendererAPIH2M::Current() == H2M::RendererAPIH2MType::Vulkan)
			{
				m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

				auto group = m_Registry.group<MeshComponentH2M>(entt::get<TransformComponentH2M>);

				SceneRendererH2M::BeginScene(nullptr, { editorCamera, editorCamera.GetViewMatrix() });
				for (auto entity : group)
				{
					auto [meshComponent, transformComponent] = group.get<MeshComponentH2M, TransformComponentH2M>(entity);
					if (meshComponent.Mesh)
					{
						meshComponent.Mesh->OnUpdate(ts);

						// TODO: Should we render (logically)

						if (m_SelectedEntity == entity) {
							SceneRendererH2M::SubmitSelectedMesh(meshComponent, transformComponent);
						}
						else {
							SceneRendererH2M::SubmitMesh(meshComponent, transformComponent);
						}
					}
				}
				SceneRendererH2M::EndScene();
			}
		}
	}

	void SceneH2M::OnRenderSimulation(Ref<SceneRendererH2M> renderer, Timestep ts, const EditorCamera& editorCamera)
	{
		Log::GetLogger()->warn("SceneH2M::OnRenderSimulation method not yet implemented!");
	}

	void SceneH2M::SetEnvironment(Ref<Environment> environment)
	{
		m_Environment = environment;
		SetSkybox(environment->RadianceMap);
	}

	void SceneH2M::SetSkybox(const Ref<TextureCubeH2M>& skybox)
	{
		m_SkyboxTexture = skybox;
		m_ShaderSkybox->SetInt("u_Texture", skybox.Raw()->GetID());

		m_SkyboxMaterial->Set("u_Texture", skybox);
	}

	EntityH2M SceneH2M::GetMainCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& comp = view.get<CameraComponent>(entity);
			if (comp.Primary) {
				return { entity, this };
			}
		}
		return {};
	}

	EntityH2M SceneH2M::FindEntityByUUID(UUID id)
	{
		auto view = m_Registry.view<IDComponent>();
		for (auto entity : view)
		{
			auto& idComponent = m_Registry.get<IDComponent>(entity);
			if (idComponent.ID == id)
				return EntityH2M(entity, this);
		}

		return EntityH2M{};
	}

	glm::mat4 SceneH2M::GetTransformRelativeToParent(EntityH2M entity)
	{
		glm::mat4 transform(1.0f);

		EntityH2M parent = FindEntityByUUID(entity.GetParentUUID());
		if (parent)
			transform = GetTransformRelativeToParent(parent);

		return transform * entity.Transform().GetTransform();
	}

	/**
	 * Working on Hazel LIVE! #14
	 *
	 * Copy to runtime
	 */
	void SceneH2M::CopyTo(Ref<SceneH2M>& target)
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
			EntityH2M e = target->CreateEntityWithID(uuid, "Entity", true);
			enttMap[uuid] = e.m_EntityHandle;
		}

		CopyComponent<DirectionalLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<ScriptComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SpriteRendererComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RigidBody2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<BoxCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CircleCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TagComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TransformComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CameraComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MeshComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MaterialComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SkyLightComponentH2M>(target->m_Registry, m_Registry, enttMap);

		const auto& entityInstanceMap = ScriptEngine::GetEntityInstanceMap();
		if (entityInstanceMap.find(target->GetUUID()) != entityInstanceMap.end()) {
			ScriptEngine::CopyEntityScriptData(target->GetUUID(), m_SceneID);
		}

		target->SetPhysics2DGravity(GetPhysics2DGravity());
	}

	void SceneH2M::ConvertToLocalSpace(EntityH2M entity)
	{
		EntityH2M parent = FindEntityByUUID(entity.GetParentUUID());

		if (!parent)
			return;

		auto& transform = entity.Transform();
		glm::mat4 parentTransform = GetWorldSpaceTransformMatrix(parent);

		glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();
		Math::DecomposeTransform(localTransform, transform.Translation, transform.Rotation, transform.Scale);
	}

	void SceneH2M::ConvertToWorldSpace(EntityH2M entity)
	{
		EntityH2M parent = FindEntityByUUID(entity.GetParentUUID());

		if (!parent)
			return;

		glm::mat4 transform = GetTransformRelativeToParent(entity);
		auto& entityTransform = entity.Transform();
		Math::DecomposeTransform(transform, entityTransform.Translation, entityTransform.Rotation, entityTransform.Scale);
	}

	glm::mat4 SceneH2M::GetWorldSpaceTransformMatrix(EntityH2M entity)
	{
		glm::mat4 transform = entity.Transform().GetTransform();

		while (EntityH2M parent = FindEntityByUUID(entity.GetParentUUID()))
		{
			transform = parent.Transform().GetTransform() * transform;
			entity = parent;
		}

		return transform;
	}

	// TODO: Definitely cache this at some point
	TransformComponentH2M SceneH2M::GetWorldSpaceTransform(EntityH2M entity)
	{
		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		TransformComponentH2M transformComponent;

		Math::DecomposeTransform(transform, transformComponent.Translation, transformComponent.Rotation, transformComponent.Scale);

		glm::quat rotationQuat = glm::quat(transformComponent.Rotation);
		transformComponent.Up = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 1.0f, 0.0f)));
		transformComponent.Right = glm::normalize(glm::rotate(rotationQuat, glm::vec3(1.0f, 0.0f, 0.0f)));
		transformComponent.Forward = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 0.0f, -1.0f)));

		return transformComponent;
	}

	void SceneH2M::ParentEntity(EntityH2M entity, EntityH2M parent)
	{
		if (parent.IsDescendantOf(entity))
		{
			UnparentEntity(parent);

			EntityH2M newParent = FindEntityByUUID(entity.GetParentUUID());
			if (newParent)
			{
				UnparentEntity(entity);
				ParentEntity(parent, newParent);
			}
		}
		else
		{
			EntityH2M previousParent = FindEntityByUUID(entity.GetParentUUID());

			if (previousParent)
				UnparentEntity(entity);
		}

		entity.SetParentUUID(parent.GetUUID());
		parent.Children().push_back(entity.GetUUID());

		ConvertToLocalSpace(entity);
	}

	void SceneH2M::UnparentEntity(EntityH2M entity, bool convertToWorldSpace)
	{
		EntityH2M parent = FindEntityByUUID(entity.GetParentUUID());
		if (!parent)
			return;

		auto& parentChildren = parent.Children();
		parentChildren.erase(std::remove(parentChildren.begin(), parentChildren.end(), entity.GetUUID()), parentChildren.end());

		if (convertToWorldSpace)
			ConvertToWorldSpace(entity);

		entity.SetParentUUID(0);
	}

	void SceneH2M::OnEntitySelected(EntityH2M entity)
	{
		// TODO...
	}

	EntityH2M SceneH2M::CreateEntity(const std::string& name, Ref<SceneH2M> scene)
	{
		EntityH2M entity = CreateEntity(name);
		entity.m_Scene = scene.Raw();

		return entity;
	}

	EntityH2M SceneH2M::CreateEntity(const std::string& name)
	{
		const std::string& entityName = name.empty() ? DefaultEntityName : name;

		// ECS
		auto entity = EntityH2M{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();

		entity.AddComponent<TransformComponentH2M>(glm::vec3(0.0f)); // glm::mat4(1.0f)

		// auto& tag = entity.AddComponent<TagComponentH2M>();
		// tag.Tag = name.empty() ? "Entity" : name;
		entity.AddComponent<TagComponentH2M>(name.empty() ? "Entity" : name);

		Log::GetLogger()->debug("CreateEntity name = '{0}'", name);

		s_EntityIDMap[idComponent.ID] = entity;
		return entity;
	}

	EntityH2M SceneH2M::CreateEntityWithID(UUID uuid, const std::string& name, bool runtimeMap)
	{
		auto entity = EntityH2M{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = uuid;

		entity.AddComponent<TransformComponentH2M>(glm::vec3(0.0f)); // glm::mat4(1.0f)

		if (!name.empty()) {
			entity.AddComponent<TagComponentH2M>(name);
		}
		// entity.AddComponent<TagComponentH2M>(name.empty() ? "Entity" : name);

		auto& entityMap = runtimeMap ? s_RuntimeEntityIDMap : s_EntityIDMap;

		Log::GetLogger()->debug("CreateEntityWithID uuid = '{0}', name = '{1}'", uuid, name);

		// HZ_CORE_ASSERT(entityMap.find(uuid) == entityMap.end());
		entityMap[uuid] = entity;
		return entity;
	}

	void SceneH2M::DestroyEntity(EntityH2M entity)
	{
		if (entity.HasComponent<ScriptComponent>())
		{
			ScriptEngine::OnScriptComponentDestroyed(m_SceneID, entity.GetUUID());
		}

		m_Registry.destroy(entity.m_EntityHandle);
	}

	void SceneH2M::DuplicateEntity(EntityH2M entity)
	{
		EntityH2M newEntity;
		if (entity.HasComponent<TagComponentH2M>()) {
			newEntity = CreateEntity(entity.GetComponent<TagComponentH2M>().Tag);
		}
		else {
			newEntity = CreateEntity();
		}

		CopyComponentIfExists<DirectionalLightComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<ScriptComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<TransformComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CameraComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MeshComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MaterialComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SkyLightComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
	}

	EntityH2M SceneH2M::FindEntityByTag(const std::string& tag)
	{
		// TODO: If this becomes used often, consider indexing by tag
		auto view = m_Registry.view<TagComponentH2M>();
		for (auto entity : view)
		{
			auto& candidate = view.get<TagComponentH2M>(entity).Tag;
			if (candidate == tag)
			{
				return EntityH2M(entity, this);
			}
		}

		return EntityH2M{};
	}

	Ref<SceneH2M> SceneH2M::GetScene(UUID uuid)
	{
		if (s_ActiveScenes.find(uuid) != s_ActiveScenes.end()) {
			return s_ActiveScenes.at(uuid);
		}

		return {};
	}

	void SceneH2M::SetPhysics2DGravity(float gravity)
	{
		m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->SetGravity({ 0.0f, gravity });
	}

	float SceneH2M::GetPhysics2DGravity() const
	{
		return m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->GetGravity().y;
	}

	EntityH2M SceneH2M::CloneEntity(EntityH2M entity)
	{
		EntityH2M entityClone = EntityH2M(m_Registry.create(), this);

		if (entity.HasComponent<IDComponent>()) {
			entityClone.AddComponent<IDComponent>(entity.GetComponent<IDComponent>());
		}

		if (entity.HasComponent<TagComponentH2M>()) {
			entityClone.AddComponent<TagComponentH2M>(entity.GetComponent<TagComponentH2M>());
		}

		if (entity.HasComponent<ScriptComponent>()) {
			entityClone.AddComponent<ScriptComponent>(entity.GetComponent<ScriptComponent>());
		}

		if (entity.HasComponent<CameraComponentH2M>()) {
			entityClone.AddComponent<CameraComponentH2M>(entity.GetComponent<CameraComponentH2M>());
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

		////////////////////////////////////////
		//// ComponentsH2M

		if (entity.HasComponent<TransformComponentH2M>()) {
			entityClone.AddComponent<TransformComponentH2M>(entity.GetComponent<TransformComponentH2M>());
		}

		if (entity.HasComponent<MeshComponentH2M>()) {
			entityClone.AddComponent<MeshComponentH2M>(entity.GetComponent<MeshComponentH2M>());
		}

		if (entity.HasComponent<SkyLightComponentH2M>()) {
			entityClone.AddComponent<SkyLightComponentH2M>(entity.GetComponent<SkyLightComponentH2M>());
		}

		if (entity.HasComponent<MaterialComponentH2M>()) {
			entityClone.AddComponent<MaterialComponentH2M>(entity.GetComponent<MaterialComponentH2M>());
		}

		Log::GetLogger()->warn("Method SceneH2M::CopyEntity implemented poorly [Tag: '{0}']", entity.GetComponent<TagComponentH2M>().Tag);

		return entityClone;
	}

	void SceneH2M::OnEvent(Event& e)
	{
	}

	void SceneH2M::OnRuntimeStart()
	{
		s_ScriptEntityIDMap = &s_RuntimeEntityIDMap;

		auto view = m_Registry.view<ScriptComponent>();
		for (auto entity : view) {
			EntityH2M e = { entity, this };
			if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName)) { // TODO implemented method
				// ScriptEngine::InstantiateEntityClass({ entity, this });
			}
		}

		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& world = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;
		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			m_PhysicsBodyEntityBuffer = new EntityH2M[view.size()];
			uint32_t physicsBodyEntityBufferIndex = 0;
			for (auto entity : view)
			{
				EntityH2M e = { entity, this };
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
				EntityH2M* entityStorage = &m_PhysicsBodyEntityBuffer[physicsBodyEntityBufferIndex++];
				*entityStorage = e;
				body->GetUserData().pointer = (uintptr_t)entityStorage;
				rigidBody2D.RuntimeBody = body;
			}
		}

		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				EntityH2M e = { entity, this };
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
				EntityH2M e = { entity, this };
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

	void SceneH2M::OnRuntimeStop()
	{
		delete[] m_PhysicsBodyEntityBuffer;
		m_IsPlaying = false;

		s_ScriptEntityIDMap = &s_EntityIDMap;
	}

	void SceneH2M::OnSimulationStart()
	{
		Log::GetLogger()->warn("SceneH2M::OnSimulationStart method not yet implemented!");
	}

	void SceneH2M::OnSimulationEnd()
	{
		Input::SetCursorMode(CursorMode::Normal);

		delete[] m_Physics2DBodyEntityBuffer;
		Physics::DestroyScene();

		m_ShouldSimulate = false;
	}

	void SceneH2M::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	void SceneH2M::OnViewportResize(uint32_t width, uint32_t height)
	{
		SetViewportSize(width, height);

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponentH2M>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponentH2M>(entity);
			if (!cameraComponent.FixedAspectRatio) {
				cameraComponent.Camera.SetViewportSize((float)width, (float)height);
			}
		}
	}

	template<typename T>
	void SceneH2M::OnComponentAdded(EntityH2M entity, T& component)
	{
		// static_assert(false);
	}

	template<>
	void SceneH2M::OnComponentAdded<IDComponent>(EntityH2M entity, IDComponent& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<ScriptComponent>(EntityH2M entity, ScriptComponent& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<CameraComponentH2M>(EntityH2M entity, CameraComponentH2M& component)
	{
		component.Camera.SetViewportSize((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	template<>
	void SceneH2M::OnComponentAdded<SpriteRendererComponent>(EntityH2M entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<RigidBody2DComponent>(EntityH2M entity, RigidBody2DComponent& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<BoxCollider2DComponent>(EntityH2M entity, BoxCollider2DComponent& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<CircleCollider2DComponent>(EntityH2M entity, CircleCollider2DComponent& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<DirectionalLightComponent>(EntityH2M entity, DirectionalLightComponent& component)
	{
	}

	/////////////////////////////////////////
	//// ComponentsH2M

	template<>
	void SceneH2M::OnComponentAdded<TagComponentH2M>(EntityH2M entity, TagComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<MeshComponentH2M>(EntityH2M entity, MeshComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<TransformComponentH2M>(EntityH2M entity, TransformComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<MaterialComponentH2M>(EntityH2M entity, MaterialComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<PointLightComponentH2M>(EntityH2M entity, PointLightComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<SpotLightComponentH2M>(EntityH2M entity, SpotLightComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<SkyLightComponentH2M>(EntityH2M entity, SkyLightComponentH2M& component)
	{
	}

}
