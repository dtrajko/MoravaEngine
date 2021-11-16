#include "SceneHazelLegacy.h"

#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/ScriptableEntity.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Physics/3D/Physics.h"

#include "HazelLegacy/Renderer/RendererAPIHazelLegacy.h"
#include "HazelLegacy/Renderer/SceneRendererHazelLegacy.h"
#include "HazelLegacy/Scene/ComponentsHazelLegacy.h"
#include "HazelLegacy/Scene/EntityHazelLegacy.h"

#include "Core/Math.h"
#include "EnvMap/EnvMapSceneRenderer.h"

#include <glm/glm.hpp>

// Box2D
#include <box2d/box2d.h>

#include <string>
#include <unordered_map>


namespace Hazel {

	static const std::string DefaultEntityName = "EntityHazelLegacy";

	std::unordered_map<UUID, SceneHazelLegacy*> s_ActiveScenes;
	std::unordered_map<UUID, EntityHazelLegacy> s_EntityIDMap;
	std::unordered_map<UUID, EntityHazelLegacy> s_RuntimeEntityIDMap;
	std::unordered_map<UUID, EntityHazelLegacy>* s_ScriptEntityIDMap = &s_EntityIDMap;

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
				// ScriptEngine::OnCollision2DBegin(a, b);
			}

			if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName)) {
				// ScriptEngine::OnCollision2DBegin(b, a);
			}
		}

		virtual void EndContact(b2Contact* contact) override
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

			// TODO: improve these if checks
			if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName)) {
				// ScriptEngine::OnCollision2DEnd(a, b);
			}

			if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName)) {
				// ScriptEngine::OnCollision2DEnd(b, a);
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

		SceneHazelLegacy* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;
		HZ_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		// ScriptEngine::InitScriptEntity(Entity{ scene->m_EntityIDMap.at(entityID), scene });
	}

	static void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		SceneHazelLegacy* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;

		HZ_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		ScriptEngine::OnScriptComponentDestroyed(sceneID, entityID);
	}

	template<typename T>
	static void CopyComponent(entt::registry& srcRegistry, entt::registry& dstRegistry, std::unordered_map<UUID, entt::entity>& enttMap)
	{
		//	auto components = srcRegistry.view<T>();
		//	for (auto srcEntity : components)
		//	{
		//		entt::entity destEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);
		//	
		//		auto& srcComponent = srcRegistry.get<T>(srcEntity);
		//		auto& destComponent = dstRegistry.emplace_or_replace<T>(destEntity, srcComponent);
		//	}
	}

	template<typename T>
	static void CopyComponentIfExists(entt::entity dst, entt::entity src, entt::registry& registry)
	{
		if (registry.has<T>(src))
		{
			// auto& srcComponent = registry.get<T>(src);
			// registry.emplace_or_replace<T>(dst, srcComponent);
		}
	}

	SceneHazelLegacy::SceneHazelLegacy(const std::string& debugName, bool isEditorScene)
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

	SceneHazelLegacy::~SceneHazelLegacy()
	{
		m_Registry.on_destroy<ScriptComponent>().disconnect();

		m_Registry.clear();
		s_ActiveScenes.erase(m_SceneID);
		ScriptEngine::OnSceneDestruct(m_SceneID);
	}

	void SceneHazelLegacy::Init()
	{
		MoravaShaderSpecification moravaShaderSpec;

		switch (Hazel::RendererAPIHazelLegacy::Current())
		{
		case Hazel::RendererAPITypeHazelLegacy::OpenGL:
			moravaShaderSpec.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
			moravaShaderSpec.VertexShaderPath = "Shaders/Hazel/Skybox.vs";
			moravaShaderSpec.FragmentShaderPath = "Shaders/Hazel/Skybox.fs";
			break;
		case Hazel::RendererAPITypeHazelLegacy::Vulkan:
			moravaShaderSpec.ShaderType = MoravaShaderSpecification::ShaderType::HazelShader;
			moravaShaderSpec.HazelShaderPath = "Resources/Shaders/Skybox.glsl";
			break;
		case Hazel::RendererAPITypeHazelLegacy::DX11:
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
	void SceneHazelLegacy::OnUpdate(Timestep ts)
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
				EntityHazelLegacy e = { entity, this };
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
			EntityHazelLegacy e = { entity, this };
			if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName)) {
				// ScriptEngine::OnUpdateEntity(e, ts);
			}
		}

		m_Registry.view<MeshComponentHazelLegacy>().each([=](auto entity, auto& mc)
			{
				auto mesh = mc.Mesh;
				if (mesh) {
					mesh->OnUpdate(ts);
				}
			});

		SceneRendererHazelLegacy::BeginScene(nullptr, { m_Camera, m_Camera.GetViewMatrix() });

		// Render entities
		// m_Registry.view<MeshComponentHazelLegacy>().each([=](auto entity, auto& mc) {});
		{
			// TODO: Should we render (logically)
			// EnvMapSceneRenderer::SubmitEntity(EntityHazelLegacy{ entity, this });
		}

		SceneRendererHazelLegacy::EndScene();

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
			// Renderer2D::BeginScene(mainCamera->GetProjectionMatrix(), *cameraTransform);

			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				// Renderer2D::DrawQuad(transform, sprite.Color);
			}

			// Renderer2D::EndScene();
		}
	}

	void SceneHazelLegacy::OnRenderRuntime(Ref<SceneRenderer> renderer, Timestep ts)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		EntityHazelLegacy cameraEntity = GetMainCameraEntity();
		if (!cameraEntity) return;

		// Process camera entity
		glm::mat4 cameraViewMatrix = glm::inverse(cameraEntity.GetComponent<TransformComponent>().Transform);
		HZ_CORE_ASSERT(cameraEntity, "Scene does not contain any cameras!");
		SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>();
		camera.SetViewportSize((float)m_ViewportWidth, (float)m_ViewportHeight);

		// Process lights
		{
			m_LightEnvironment = LightEnvironmentLegacy();
			auto lights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
			uint32_t directionalLightIndex = 0;
			for (auto entity : lights)
			{
				auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);
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
			//	auto lights = m_Registry.group<SkyLightComponent>(entt::get<TransformComponent>);
			//	for (auto entity : lights)
			//	{
			//		auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
			//		// m_Environment = skyLightComponent.SceneEnvironment;
			//		SetSkybox(m_Environment->RadianceMap);
			//	}
		}

		m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

		auto group = m_Registry.group<MeshComponentHazelLegacy>(entt::get<TransformComponent>);
		// renderer->SetScene(this);
		// renderer->BeginScene({ camera, cameraViewMatrix, 0.1f, 1000.0f, 45.0f }); //TODO: real values
		for (auto entity : group)
		{
			auto [transformComponent, meshComponent] = group.get<TransformComponent, MeshComponentHazelLegacy>(entity);
			if (meshComponent.Mesh /* && !meshComponent.Mesh->IsFlagSet(AssetFlag::Missing) */)
			{
				meshComponent.Mesh->OnUpdate(ts);
				EntityHazelLegacy e = EntityHazelLegacy(entity, this);
				glm::mat4 transform = GetTransformRelativeToParent(e);

				//	if (e.HasComponent<RigidBodyComponent>())
				//		transform = e.Transform().GetTransform();

				// TODO: Should we render (logically)
				// renderer->SubmitMesh(meshComponent.Mesh, meshComponent.MaterialTable, transform);
			}
		}

		// renderer->EndScene();
	}

	void SceneHazelLegacy::OnRenderEditor(Ref<SceneRenderer> renderer, Timestep ts, const EditorCamera& editorCamera)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		if (RendererAPI::Current() == RendererAPIType::Vulkan)
		{
			m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

			auto group = m_Registry.group<MeshComponentHazelLegacy>(entt::get<TransformComponent>);
			SceneRendererHazelLegacy::BeginScene(nullptr, { editorCamera, editorCamera.GetViewMatrix() });
			for (auto entity : group)
			{
				auto& [meshComponent, transformComponent] = group.get<MeshComponentHazelLegacy, TransformComponent>(entity);
				if (meshComponent.Mesh)
				{
					meshComponent.Mesh->OnUpdate(ts);

					// TODO: Should we render (logically)

					if (m_SelectedEntity == entity)
					{
						SceneRendererHazelLegacy::SubmitSelectedMesh(meshComponent, transformComponent);
					}
					else {
						SceneRendererHazelLegacy::SubmitMesh(meshComponent, transformComponent);
					}
				}
			}
			SceneRendererHazelLegacy::EndScene();

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
				auto lights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : lights)
				{
					auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);
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
				//	auto lights = m_Registry.group<SkyLightComponent>(entt::get<TransformComponent>);
				//	for (auto entity : lights)
				//	{
				//		auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
				//		// m_Environment = skyLightComponent.SceneEnvironment;
				//		SetSkybox(m_Environment->RadianceMap);
				//	}
			}

			if (Hazel::RendererAPI::Current() == Hazel::RendererAPIType::Vulkan)
			{
				m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

				auto group = m_Registry.group<MeshComponentHazelLegacy>(entt::get<TransformComponent>);

				SceneRendererHazelLegacy::BeginScene(nullptr, { editorCamera, editorCamera.GetViewMatrix() });
				for (auto entity : group)
				{
					auto [meshComponent, transformComponent] = group.get<MeshComponentHazelLegacy, TransformComponent>(entity);
					if (meshComponent.Mesh)
					{
						meshComponent.Mesh->OnUpdate(ts);

						// TODO: Should we render (logically)

						if (m_SelectedEntity == entity) {
							SceneRendererHazelLegacy::SubmitSelectedMesh(meshComponent, transformComponent);
						}
						else {
							SceneRendererHazelLegacy::SubmitMesh(meshComponent, transformComponent);
						}
					}
				}
				SceneRendererHazelLegacy::EndScene();
			}
		}
	}

	void SceneHazelLegacy::OnRenderSimulation(Ref<SceneRenderer> renderer, Timestep ts, const EditorCamera& editorCamera)
	{
		Log::GetLogger()->warn("SceneHazelLegacy::OnRenderSimulation method not yet implemented!");
	}

	void SceneHazelLegacy::SetEnvironment(Ref<Environment> environment)
	{
		m_Environment = environment;
		SetSkybox(environment->RadianceMap);
	}

	void SceneHazelLegacy::SetSkybox(const Ref<HazelTextureCube>& skybox)
	{
		m_SkyboxTexture = skybox;
		m_ShaderSkybox->SetInt("u_Texture", skybox.Raw()->GetID());

		m_SkyboxMaterial->Set("u_Texture", skybox);
	}

	EntityHazelLegacy SceneHazelLegacy::GetMainCameraEntity()
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

	EntityHazelLegacy SceneHazelLegacy::FindEntityByUUID(UUID id)
	{
		auto view = m_Registry.view<IDComponent>();
		for (auto entity : view)
		{
			auto& idComponent = m_Registry.get<IDComponent>(entity);
			if (idComponent.ID == id)
				return EntityHazelLegacy(entity, this);
		}

		return EntityHazelLegacy{};
	}

	glm::mat4 SceneHazelLegacy::GetTransformRelativeToParent(EntityHazelLegacy entity)
	{
		glm::mat4 transform(1.0f);

		EntityHazelLegacy parent = FindEntityByUUID(entity.GetParentUUID());
		if (parent)
			transform = GetTransformRelativeToParent(parent);

		return transform * entity.Transform().GetTransform();
	}

	/**
	 * Working on Hazel LIVE! #14
	 *
	 * Copy to runtime
	 */
	void SceneHazelLegacy::CopyTo(Ref<SceneHazelLegacy>& target)
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
			EntityHazelLegacy e = target->CreateEntityWithID(uuid, "Entity", true);
			enttMap[uuid] = e.m_EntityHandle;
		}

		CopyComponent<TagComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TransformComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MeshComponentHazelLegacy>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<DirectionalLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SkyLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<ScriptComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CameraComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SpriteRendererComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RigidBody2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<BoxCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CircleCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MaterialComponent>(target->m_Registry, m_Registry, enttMap);

		const auto& entityInstanceMap = ScriptEngine::GetEntityInstanceMap();
		if (entityInstanceMap.find(target->GetUUID()) != entityInstanceMap.end()) {
			ScriptEngine::CopyEntityScriptData(target->GetUUID(), m_SceneID);
		}

		target->SetPhysics2DGravity(GetPhysics2DGravity());
	}

	void SceneHazelLegacy::ConvertToLocalSpace(EntityHazelLegacy entity)
	{
		EntityHazelLegacy parent = FindEntityByUUID(entity.GetParentUUID());

		if (!parent)
			return;

		auto& transform = entity.Transform();
		glm::mat4 parentTransform = GetWorldSpaceTransformMatrix(parent);

		glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();
		Math::DecomposeTransform(localTransform, transform.Translation, transform.Rotation, transform.Scale);
	}

	void SceneHazelLegacy::ConvertToWorldSpace(EntityHazelLegacy entity)
	{
		EntityHazelLegacy parent = FindEntityByUUID(entity.GetParentUUID());

		if (!parent)
			return;

		glm::mat4 transform = GetTransformRelativeToParent(entity);
		auto& entityTransform = entity.Transform();
		Math::DecomposeTransform(transform, entityTransform.Translation, entityTransform.Rotation, entityTransform.Scale);
	}

	glm::mat4 SceneHazelLegacy::GetWorldSpaceTransformMatrix(EntityHazelLegacy entity)
	{
		glm::mat4 transform = entity.Transform().GetTransform();

		while (EntityHazelLegacy parent = FindEntityByUUID(entity.GetParentUUID()))
		{
			transform = parent.Transform().GetTransform() * transform;
			entity = parent;
		}

		return transform;
	}

	// TODO: Definitely cache this at some point
	TransformComponent SceneHazelLegacy::GetWorldSpaceTransform(EntityHazelLegacy entity)
	{
		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		TransformComponent transformComponent;

		Math::DecomposeTransform(transform, transformComponent.Translation, transformComponent.Rotation, transformComponent.Scale);

		glm::quat rotationQuat = glm::quat(transformComponent.Rotation);
		transformComponent.Up = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 1.0f, 0.0f)));
		transformComponent.Right = glm::normalize(glm::rotate(rotationQuat, glm::vec3(1.0f, 0.0f, 0.0f)));
		transformComponent.Forward = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 0.0f, -1.0f)));

		return transformComponent;
	}

	void SceneHazelLegacy::ParentEntity(EntityHazelLegacy entity, EntityHazelLegacy parent)
	{
		if (parent.IsDescendantOf(entity))
		{
			UnparentEntity(parent);

			EntityHazelLegacy newParent = FindEntityByUUID(entity.GetParentUUID());
			if (newParent)
			{
				UnparentEntity(entity);
				ParentEntity(parent, newParent);
			}
		}
		else
		{
			EntityHazelLegacy previousParent = FindEntityByUUID(entity.GetParentUUID());

			if (previousParent)
				UnparentEntity(entity);
		}

		entity.SetParentUUID(parent.GetUUID());
		parent.Children().push_back(entity.GetUUID());

		ConvertToLocalSpace(entity);
	}

	void SceneHazelLegacy::UnparentEntity(EntityHazelLegacy entity, bool convertToWorldSpace)
	{
		EntityHazelLegacy parent = FindEntityByUUID(entity.GetParentUUID());
		if (!parent)
			return;

		auto& parentChildren = parent.Children();
		parentChildren.erase(std::remove(parentChildren.begin(), parentChildren.end(), entity.GetUUID()), parentChildren.end());

		if (convertToWorldSpace)
			ConvertToWorldSpace(entity);

		entity.SetParentUUID(0);
	}

	void SceneHazelLegacy::OnEntitySelected(EntityHazelLegacy entity)
	{
		// TODO...
	}

	EntityHazelLegacy SceneHazelLegacy::CreateEntity(const std::string& name, Ref<SceneHazelLegacy> scene)
	{
		EntityHazelLegacy entity = CreateEntity(name);
		entity.m_Scene = scene.Raw();

		return entity;
	}

	EntityHazelLegacy SceneHazelLegacy::CreateEntity(const std::string& name)
	{
		const std::string& entityName = name.empty() ? DefaultEntityName : name;

		// ECS
		auto entity = EntityHazelLegacy{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();

		entity.AddComponent<TransformComponent>(glm::vec3(0.0f)); // glm::mat4(1.0f)

		// auto& tag = entity.AddComponent<TagComponent>();
		// tag.Tag = name.empty() ? "Entity" : name;
		entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);

		Log::GetLogger()->debug("CreateEntity name = '{0}'", name);

		s_EntityIDMap[idComponent.ID] = entity;
		return entity;
	}

	EntityHazelLegacy SceneHazelLegacy::CreateEntityWithID(UUID uuid, const std::string& name, bool runtimeMap)
	{
		auto entity = EntityHazelLegacy{ m_Registry.create(), this };
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

	void SceneHazelLegacy::DestroyEntity(EntityHazelLegacy entity)
	{
		if (entity.HasComponent<ScriptComponent>())
		{
			ScriptEngine::OnScriptComponentDestroyed(m_SceneID, entity.GetUUID());
		}

		m_Registry.destroy(entity.m_EntityHandle);
	}

	void SceneHazelLegacy::DuplicateEntity(EntityHazelLegacy entity)
	{
		EntityHazelLegacy newEntity;
		if (entity.HasComponent<TagComponent>()) {
			newEntity = CreateEntity(entity.GetComponent<TagComponent>().Tag);
		}
		else {
			newEntity = CreateEntity();
		}

		CopyComponentIfExists<TransformComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MeshComponentHazelLegacy>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<DirectionalLightComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		// CopyComponentIfExists<SkyLightComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<ScriptComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CameraComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MaterialComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
	}

	EntityHazelLegacy SceneHazelLegacy::FindEntityByTag(const std::string& tag)
	{
		// TODO: If this becomes used often, consider indexing by tag
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			auto& candidate = view.get<TagComponent>(entity).Tag;
			if (candidate == tag)
			{
				return EntityHazelLegacy(entity, this);
			}
		}

		return EntityHazelLegacy{};
	}

	Ref<SceneHazelLegacy> SceneHazelLegacy::GetScene(UUID uuid)
	{
		if (s_ActiveScenes.find(uuid) != s_ActiveScenes.end()) {
			return s_ActiveScenes.at(uuid);
		}

		return {};
	}

	void SceneHazelLegacy::SetPhysics2DGravity(float gravity)
	{
		m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->SetGravity({ 0.0f, gravity });
	}

	float SceneHazelLegacy::GetPhysics2DGravity() const
	{
		return m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->GetGravity().y;
	}

	EntityHazelLegacy SceneHazelLegacy::CloneEntity(EntityHazelLegacy entity)
	{
		EntityHazelLegacy entityClone = EntityHazelLegacy(m_Registry.create(), this);

		if (entity.HasComponent<IDComponent>()) {
			entityClone.AddComponent<IDComponent>(entity.GetComponent<IDComponent>());
		}

		if (entity.HasComponent<TagComponent>()) {
			entityClone.AddComponent<TagComponent>(entity.GetComponent<TagComponent>());
		}

		if (entity.HasComponent<TransformComponent>()) {
			entityClone.AddComponent<TransformComponent>(entity.GetComponent<TransformComponent>());
		}

		if (entity.HasComponent<MeshComponentHazelLegacy>()) {
			entityClone.AddComponent<MeshComponentHazelLegacy>(entity.GetComponent<MeshComponentHazelLegacy>());
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

		//	if (entity.HasComponent<SkyLightComponent>()) {
		//		entityClone.AddComponent<SkyLightComponent>(entity.GetComponent<SkyLightComponent>());
		//	}

		Log::GetLogger()->warn("Method SceneHazelLegacy::CopyEntity implemented poorly [Tag: '{0}']", entity.GetComponent<TagComponent>().Tag);

		return entityClone;
	}

	void SceneHazelLegacy::OnEvent(Event& e)
	{
	}

	void SceneHazelLegacy::OnRuntimeStart()
	{
		s_ScriptEntityIDMap = &s_RuntimeEntityIDMap;

		auto view = m_Registry.view<ScriptComponent>();
		for (auto entity : view) {
			EntityHazelLegacy e = { entity, this };
			if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName)) { // TODO implemented method
				// ScriptEngine::InstantiateEntityClass({ entity, this });
			}
		}

		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& world = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;
		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			m_PhysicsBodyEntityBuffer = new EntityHazelLegacy[view.size()];
			uint32_t physicsBodyEntityBufferIndex = 0;
			for (auto entity : view)
			{
				EntityHazelLegacy e = { entity, this };
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
				EntityHazelLegacy* entityStorage = &m_PhysicsBodyEntityBuffer[physicsBodyEntityBufferIndex++];
				*entityStorage = e;
				body->GetUserData().pointer = (uintptr_t)entityStorage;
				rigidBody2D.RuntimeBody = body;
			}
		}

		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				EntityHazelLegacy e = { entity, this };
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
				EntityHazelLegacy e = { entity, this };
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

	void SceneHazelLegacy::OnRuntimeStop()
	{
		delete[] m_PhysicsBodyEntityBuffer;
		m_IsPlaying = false;

		s_ScriptEntityIDMap = &s_EntityIDMap;
	}

	void SceneHazelLegacy::OnSimulationStart()
	{
		Log::GetLogger()->warn("SceneHazelLegacy::OnSimulationStart method not yet implemented!");
	}

	void SceneHazelLegacy::OnSimulationEnd()
	{
		Input::SetCursorMode(CursorMode::Normal);

		delete[] m_Physics2DBodyEntityBuffer;
		Physics::DestroyScene();

		m_ShouldSimulate = false;
	}

	void SceneHazelLegacy::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	void SceneHazelLegacy::OnViewportResize(uint32_t width, uint32_t height)
	{
		SetViewportSize(width, height);

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponentLegacy>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponentLegacy>(entity);
			if (!cameraComponent.FixedAspectRatio) {
				cameraComponent.Camera.SetViewportSize((float)width, (float)height);
			}
		}
	}

	template<typename T>
	void SceneHazelLegacy::OnComponentAdded(EntityHazelLegacy entity, T& component)
	{
		// static_assert(false);
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<IDComponent>(EntityHazelLegacy entity, IDComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<TagComponent>(EntityHazelLegacy entity, TagComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<TransformComponent>(EntityHazelLegacy entity, TransformComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<MeshComponent>(EntityHazelLegacy entity, MeshComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<MeshComponentHazelLegacy>(EntityHazelLegacy entity, MeshComponentHazelLegacy& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<ScriptComponent>(EntityHazelLegacy entity, ScriptComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<CameraComponent>(EntityHazelLegacy entity, CameraComponent& component)
	{
		component.Camera.SetViewportSize((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<SpriteRendererComponent>(EntityHazelLegacy entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<RigidBody2DComponent>(EntityHazelLegacy entity, RigidBody2DComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<BoxCollider2DComponent>(EntityHazelLegacy entity, BoxCollider2DComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<CircleCollider2DComponent>(EntityHazelLegacy entity, CircleCollider2DComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<MaterialComponent>(EntityHazelLegacy entity, MaterialComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<DirectionalLightComponent>(EntityHazelLegacy entity, DirectionalLightComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<PointLightComponent>(EntityHazelLegacy entity, PointLightComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<SpotLightLegacyComponent>(EntityHazelLegacy entity, SpotLightLegacyComponent& component)
	{
	}

	template<>
	void SceneHazelLegacy::OnComponentAdded<SkyLightComponent>(EntityHazelLegacy entity, SkyLightComponent& component)
	{
	}

}
