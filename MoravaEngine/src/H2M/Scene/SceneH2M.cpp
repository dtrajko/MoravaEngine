/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "SceneH2M.h"

#include "H2M/Core/BaseH2M.h"
#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"
#include "H2M/Renderer/SceneRendererVulkanH2M.h"
#include "H2M/Scene/EntityH2M.h"
#include "H2M/Scene/ScriptableEntityH2M.h"

#include "Core/Math.h"
#include "EnvMap/EnvMapSceneRenderer.h"

#include <glm/glm.hpp>

// Box2D
#include <box2d/box2d.h>
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>

#include <string>
#include <unordered_map>


namespace H2M
{

	static const std::string DefaultEntityName = "Entity";

	std::unordered_map<UUID_H2M, SceneH2M*> s_ActiveScenes;
	std::unordered_map<UUID_H2M, EntityH2M> s_EntityIDMap;
	std::unordered_map<UUID_H2M, EntityH2M> s_RuntimeEntityIDMap;
	std::unordered_map<UUID_H2M, EntityH2M>* s_ScriptEntityIDMap = &s_EntityIDMap;

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
	}

	static void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity)
	{
	}

	static b2BodyType Rigidbody2DTypeToBox2DBody(Rigidbody2DComponentH2M::BodyType bodyType)
	{
		switch (bodyType)
		{
			case Rigidbody2DComponentH2M::BodyType::Static:		return b2_staticBody;
			case Rigidbody2DComponentH2M::BodyType::Dynamic:	return b2_dynamicBody;
			case Rigidbody2DComponentH2M::BodyType::Kinematic:	return b2_kinematicBody;
		}
		H2M_CORE_ASSERT(false, "Unknown body type!");
		return b2_staticBody;
	}

	template<typename T>
	static void CopyComponent(entt::registry& srcRegistry, entt::registry& dstRegistry, std::unordered_map<UUID_H2M, entt::entity>& enttMap)
	{
		auto components = srcRegistry.view<T>();
		for (auto srcEntity : components)
		{
			entt::entity destEntity = enttMap.at(srcRegistry.get<IDComponentH2M>(srcEntity).ID);

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

	template<typename Component>
	static void CopyComponentHazel2D(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID_H2M, entt::entity>& enttMap)
	{
		auto view = src.view<Component>();
		for (auto e : view)
		{
			UUID_H2M uuid = src.get<IDComponentH2M>(e).ID;
			H2M_CORE_ASSERT(enttMap.find(uuid) != enttMap.end());
			entt::entity dstEnttID = enttMap.at(uuid);

			auto& component = src.get<Component>(e);
			dst.emplace_or_replace<Component>(dstEnttID, component);
		}
	}

	template<typename Component>
	static void CopyComponentIfExistsHazel2D(EntityH2M dst, EntityH2M src)
	{
		if (src.HasComponent<Component>())
		{
			dst.AddOrReplaceComponent<Component>();
		}
	}

	SceneH2M::SceneH2M(const std::string& debugName, bool isEditorScene, bool initalize)
		: m_DebugName(debugName)
	{
		m_SceneEntity = m_Registry.create();

		// TODO: Obviously not necessary in all cases

		// s_ActiveScenes[m_SceneID] = this;

		Init();
	}

	SceneH2M::~SceneH2M()
	{
		m_Registry.clear();
		// s_ActiveScenes.erase(m_SceneID);
	}

	RefH2M<SceneH2M> SceneH2M::Copy(RefH2M<SceneH2M> other)
	{
		RefH2M<SceneH2M> newScene = RefH2M<SceneH2M>::Create();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID_H2M, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponentH2M>();
		for (auto e : idView)
		{
			UUID_H2M uuid = srcSceneRegistry.get<IDComponentH2M>(e).ID;
			const auto& name = srcSceneRegistry.get<TagComponentH2M>(e).Tag;
			EntityH2M newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponentH2M and TagComponentH2M)
		CopyComponentHazel2D<RelationshipComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<TransformComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<MeshComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<CameraComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<NativeScriptComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<MaterialComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<DirectionalLightComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<PointLightComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<SpotLightComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<SkyLightComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<SpriteRendererComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<CircleRendererComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<Rigidbody2DComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponentHazel2D<BoxCollider2DComponentH2M>(dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	void SceneH2M::DuplicateEntity(EntityH2M entity)
	{
		EntityH2M newEntity;
		if (entity.HasComponent<TagComponentH2M>())
		{
			newEntity = CreateEntity(entity.GetComponent<TagComponentH2M>().Tag);
		}
		else
		{
			newEntity = CreateEntity();
		}

		CopyComponentIfExists<RelationshipComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<TransformComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MeshComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CameraComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<NativeScriptComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MaterialComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<DirectionalLightComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<PointLightComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SpotLightComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SkyLightComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SpriteRendererComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CircleRendererComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<Rigidbody2DComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<BoxCollider2DComponentH2M>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
	}

	/**
	 * Working on Hazel LIVE! #14
	 *
	 * Copy to runtime
	 */
	void SceneH2M::CopyTo(RefH2M<SceneH2M>& target)
	{
		// Environment
		target->m_Light = m_Light;
		target->m_LightMultiplier = m_LightMultiplier;

		target->m_Environment = m_Environment;
		target->m_SkyboxTexture = m_SkyboxTexture;
		target->m_SkyboxMaterial = m_SkyboxMaterial;
		target->m_SkyboxLod = m_SkyboxLod;

		std::unordered_map<UUID_H2M, entt::entity> enttMap;
		auto idComponents = m_Registry.view<IDComponentH2M>();
		for (auto entity : idComponents)
		{
			auto uuid = m_Registry.get<IDComponentH2M>(entity).ID;
			EntityH2M e = target->CreateEntityWithUUID(uuid, "Entity", true);
		}

		CopyComponent<TagComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RelationshipComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TransformComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MeshComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CameraComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<NativeScriptComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MaterialComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<DirectionalLightComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<PointLightComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SpotLightComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SkyLightComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SpriteRendererComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CircleRendererComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<Rigidbody2DComponentH2M>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<BoxCollider2DComponentH2M>(target->m_Registry, m_Registry, enttMap);

		target->SetPhysics2DGravity(GetPhysics2DGravity());
	}

	EntityH2M SceneH2M::CreateEntity(const std::string& name, RefH2M<SceneH2M> scene)
	{
		EntityH2M entity = CreateEntity(name);
		entity.m_Scene = scene.Raw();

		return entity;
	}

	EntityH2M SceneH2M::CreateEntity(const std::string& name)
	{
#if 1 // New -- not working for point/spot lights in EnvMapEditorLayer

		return CreateEntityWithUUID(UUID_H2M(), name);

#else // Old -- working for point/spot lights in EnvMapEditorLayer

		const std::string& entityName = name.empty() ? DefaultEntityName : name;

		// ECS
		auto entity = EntityH2M{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponentH2M>();

		entity.AddComponent<TransformComponentH2M>(glm::vec3(0.0f)); // glm::mat4(1.0f)

		// auto& tag = entity.AddComponent<TagComponentH2M>();
		// tag.Tag = name.empty() ? "Entity" : name;
		entity.AddComponent<TagComponentH2M>(name.empty() ? "Entity" : name);

		Log::GetLogger()->debug("CreateEntity name = '{0}'", name);

		s_EntityIDMap[idComponent.ID] = entity;
		return entity;

#endif
	}

	EntityH2M SceneH2M::CreateEntityWithUUID(UUID_H2M uuid, const std::string& name, bool runtimeMap)
	{
		const std::string& entityName = name.empty() ? DefaultEntityName : name;

		// ECS
		auto entity = EntityH2M{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponentH2M>();
		idComponent.ID = uuid;

		entity.AddComponent<TransformComponentH2M>(glm::vec3(0.0f)); // glm::mat4(1.0f)

		entity.AddComponent<TagComponentH2M>(name);

		Log::GetLogger()->debug("CreateEntityWithID uuid = '{0}', name = '{1}'", uuid, name);

		// H2M_CORE_ASSERT(EntityMapH2M.find(uuid) == EntityMapH2M.end());
		auto& entityMapH2M = runtimeMap ? s_RuntimeEntityIDMap : s_EntityIDMap;
		entityMapH2M[uuid] = entity;
		return entity;
	}

	void SceneH2M::DestroyEntity(EntityH2M entity)
	{
		m_Registry.destroy(entity.m_EntityHandle);
	}

	void SceneH2M::OnRuntimeStart()
	{
		b2Vec2 gravity = b2Vec2{ 0.0f, -9.81f };
		m_PhysicsWorld = new b2World(gravity);

		auto view = m_Registry.view<Rigidbody2DComponentH2M>();
		for (auto e : view)
		{
			EntityH2M entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponentH2M>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponentH2M>();

			b2BodyDef bodyDef;
			bodyDef.type = Rigidbody2DTypeToBox2DBody(rb2d.Type);
			bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
			bodyDef.angle = transform.Rotation.z;

			b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rb2d.FixedRotation);
			rb2d.RuntimeBody = body;

			if (entity.HasComponent<BoxCollider2DComponentH2M>())
			{
				auto& bc2d = entity.GetComponent<BoxCollider2DComponentH2M>();

				b2PolygonShape boxShape;
				boxShape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &boxShape;
				fixtureDef.density = bc2d.Density;
				fixtureDef.friction = bc2d.Friction;
				fixtureDef.restitution = bc2d.Restitution;
				fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}
		}

		// -----------------------------------------
		m_IsPlaying = true;
		s_ScriptEntityIDMap = &s_RuntimeEntityIDMap;

		Log::GetLogger()->info("SceneH2M::OnRuntimeStart");
	}

	void SceneH2M::OnRuntimeStop()
	{
		delete m_PhysicsWorld;
		m_PhysicsWorld = nullptr;

		// -----------------------------------------
		m_IsPlaying = false;
		delete[] m_PhysicsBodyEntityBuffer;

		s_ScriptEntityIDMap = &s_EntityIDMap;

		Log::GetLogger()->info("SceneH2M::OnRuntimeStop");
	}

	void SceneH2M::OnSimulationStart()
	{
		// TODO
		Log::GetLogger()->info("SceneH2M::OnSimulationStart");
	}

	void SceneH2M::OnSimulationEnd()
	{
		// TODO
		Log::GetLogger()->info("SceneH2M::OnSimulationEnd");
	}

	void SceneH2M::Init()
	{
		MoravaShaderSpecification moravaShaderSpec;

		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::OpenGL:
				moravaShaderSpec.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
				moravaShaderSpec.VertexShaderPath = "Shaders/Hazel/Skybox.vs";
				moravaShaderSpec.FragmentShaderPath = "Shaders/Hazel/Skybox.fs";
				break;
			case RendererAPITypeH2M::Vulkan:
				moravaShaderSpec.ShaderType = MoravaShaderSpecification::ShaderType::HazelShader;
				moravaShaderSpec.HazelShaderPath = "Resources/Shaders/Skybox.glsl";
				break;
			case RendererAPITypeH2M::DX11:
				moravaShaderSpec.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
				moravaShaderSpec.VertexShaderPath = "Shaders/HLSL/UnlitVertexShader.hlsl";
				moravaShaderSpec.PixelShaderPath = "Shaders/HLSL/UnlitPixelShader.hlsl";
				break;
		}
		moravaShaderSpec.ForceCompile = false;
		auto skyboxShader = MoravaShader::Create(moravaShaderSpec);

		m_SkyboxMaterial = Material::Create(skyboxShader);
		m_SkyboxMaterial->SetFlag(MaterialFlagH2M::DepthTest, false);

		// auto skyboxShader = HazelShader::Create("Resources/Shaders/Renderer2D.glsl");
		// HazelRenderer::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
		// auto skyboxShader = HazelRenderer::GetShaderLibrary()->Get("Skybox"); // Spir-V method // Pre-load shaders in order to use Get
		// m_SkyboxMaterial = HazelMaterial::Create(skyboxShader);
		// m_SkyboxMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);
	}

	// TODO: replaced by OnUpdateRuntime/OnUpdateEditor, should be removed
	// Merge OnUpdate/Render into one function?
	void SceneH2M::OnUpdate(TimestepH2M ts)
	{
		// Box2D physics
		int32_t velocityIterations = 6;
		int32_t positionIterations = 2;

		{
			auto view = m_Registry.view<Rigidbody2DComponentH2M>();
			for (auto entity : view)
			{
				EntityH2M e = { entity, this };
				auto& tc = e.Transform();
				auto& rb2d = e.GetComponent<Rigidbody2DComponentH2M>();
				b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);

				auto& position = body->GetPosition();
				auto [translation, rotationQuat, scale] = Math::GetTransformDecomposition(tc.GetTransform());
				glm::vec3 rotation = glm::eulerAngles(rotationQuat);

				tc.GetTransform() = glm::translate(glm::mat4(1.0f), { position.x, position.y, tc.GetTransform()[3].z }) *
					glm::toMat4(glm::quat({ rotation.x, rotation.y, body->GetAngle() })) *
					glm::scale(glm::mat4(1.0f), scale);
			}
		}

		m_Registry.view<MeshComponentH2M>().each([=](auto entity, auto& mc)
			{
				auto mesh = mc.Mesh;
				if (mesh) {
					mesh->OnUpdate(ts, false);
				}
			});

		SceneRendererH2M::BeginScene(this, { m_Camera, m_Camera.GetViewMatrix() });

		// Render entities
		m_Registry.view<MeshComponentH2M>().each([=](auto entity, auto& mc)
		{
			// TODO: Should we render (logically)
			EnvMapSceneRenderer::SubmitEntity(EntityH2M{ entity, this });
		});

		SceneRendererH2M::EndScene();

		// Render 2D
		CameraH2M* mainCamera = nullptr;
		glm::mat4 cameraTransform = glm::mat4(1.0f);
		{
			auto view = m_Registry.view<TransformComponentH2M, CameraComponentH2M>();

			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponentH2M, CameraComponentH2M>(entity);

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

			auto group = m_Registry.group<TransformComponentH2M>(entt::get<SpriteRendererComponentH2M>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponentH2M, SpriteRendererComponentH2M>(entity);

				// Renderer2D::DrawQuad(transform, sprite.Color);
			}

			// Renderer2D::EndScene();
		}
	}

	void SceneH2M::OnUpdateRuntime(TimestepH2M ts)
	{
		// Update scripts
		{
			//	m_Registry.view<NativeScriptComponentH2M>().each([=](auto entity, auto& nsc)
			//	{
			//		// TODO: Move to scene::OnScenePlay
			//		if (!nsc.Instance)
			//		{
			//			nsc.Instance = nsc.InstantiateScript();
			//			nsc.Instance->m_Entity = EntityH2M{ entity, this };
			//			nsc.Instance->OnCreate();
			//		}
			//	
			//		nsc.Instance->OnUpdate(ts);
			//	});
		}

		// Physics 2D
		{
			const int32_t velocityIterations = 6;
			const int32_t positionIterations = 2;
			m_PhysicsWorld->Step(ts, velocityIterations, positionIterations);

			// Retrieve transform from Box2D
			auto view = m_Registry.view<Rigidbody2DComponentH2M>();
			for (auto e : view)
			{
				EntityH2M entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponentH2M>();
				auto& rb2d = entity.GetComponent<Rigidbody2DComponentH2M>();

				b2Body* body = (b2Body*)rb2d.RuntimeBody;
				const auto& position = body->GetPosition();
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			}
		}

		// Render 2D
		CameraH2M* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponentH2M, CameraComponentH2M>();
			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponentH2M, CameraComponentH2M>(entity);

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
			// Renderer2D_H2M::BeginScene(*mainCamera, cameraTransform);

			auto group = m_Registry.group<TransformComponentH2M>(entt::get<SpriteRendererComponentH2M>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponentH2M, SpriteRendererComponentH2M>(entity);

				Renderer2D_H2M::DrawSprite(transform.GetTransform(), sprite, (int)entity);
			}

			// Renderer2D_H2M::EndScene();
		}
	}

	void SceneH2M::OnUpdateEditor(TimestepH2M ts, EditorCameraH2M& camera)
	{
		// Renderer2D_H2M::BeginScene(camera);

		auto group = m_Registry.group<TransformComponentH2M>(entt::get<SpriteRendererComponentH2M>);
		for (auto entity : group)
		{
			auto [transform, sprite] = group.get<TransformComponentH2M, SpriteRendererComponentH2M>(entity);

			Renderer2D_H2M::DrawSprite(transform.GetTransform(), sprite, (int)entity);
		}

		// Renderer2D_H2M::EndScene();
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

	void SceneH2M::GetPrimaryCameraEntity()
	{
	}

	void SceneH2M::OnRenderRuntime(RefH2M<SceneRendererH2M> renderer, TimestepH2M ts)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		EntityH2M cameraEntity = GetMainCameraEntity();
		if (!cameraEntity) return;

		// Process camera entity
		glm::mat4 cameraViewMatrix = glm::inverse(cameraEntity.GetComponent<TransformComponentH2M>().Transform);
		H2M_CORE_ASSERT(cameraEntity, "Scene does not contain any cameras!");
		SceneCameraH2M& camera = cameraEntity.GetComponent<CameraComponentH2M>();
		camera.SetViewportSize((float)m_ViewportWidth, (float)m_ViewportHeight);

		// Process lights
		{
			m_LightEnvironment = LightEnvironmentH2M();
			auto lights = m_Registry.group<DirectionalLightComponentH2M>(entt::get<TransformComponentH2M>);
			uint32_t directionalLightIndex = 0;
			for (auto entity : lights)
			{
				auto [transformComponent, lightComponent] = lights.get<TransformComponentH2M, DirectionalLightComponentH2M>(entity);
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
			m_Environment = EnvironmentH2M();
			auto lights = m_Registry.group<SkyLightComponentH2M>(entt::get<TransformComponentH2M>);
			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponentH2M, SkyLightComponentH2M>(entity);
				m_Environment = skyLightComponent.SceneEnvironment;
				SetSkybox(m_Environment.RadianceMap);
			}
		}

		//	m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

		//	auto group = m_Registry.group<MeshComponentH2M>(entt::get<TransformComponentH2M>);
		//	renderer->SetScene(this);
		//	renderer->BeginScene({ camera, cameraViewMatrix, 0.1f, 1000.0f, 45.0f }); //TODO: real values
		//	for (auto entity : group)
		//	{
		//		auto [transformComponent, meshComponent] = group.get<TransformComponentH2M, MeshComponentH2M>(entity);
		//		if (meshComponent.Mesh /* && !meshComponent.Mesh->IsFlagSet(AssetFlag::Missing) */)
		//		{
		//			meshComponent.Mesh->OnUpdate(ts, false);
		//			EntityH2M e = EntityH2M(entity, this);
		//			glm::mat4 transform = GetTransformRelativeToParent(e);
		//	
		//			if (e.HasComponent<RigidBodyComponentH2M>())
		//			{
		//				transform = e.Transform().GetTransform();
		//			}
		//	
		//			// TODO: Should we render (logically)
		//			renderer->SubmitMesh(meshComponent.Mesh, meshComponent.MaterialTable, transform);
		//		}
		//	}
		//	renderer->EndScene();
	}

	void SceneH2M::OnRenderEditor(RefH2M<SceneRendererH2M>, TimestepH2M ts, const EditorCameraH2M& editorCamera)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
		{
			m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

			auto group = m_Registry.group<MeshComponentH2M>(entt::get<TransformComponentH2M>);
			SceneRendererVulkanH2M::BeginScene(this, { editorCamera, editorCamera.GetViewMatrix() });
			for (auto entity : group)
			{
				auto& [meshComponent, transformComponent] = group.get<MeshComponentH2M, TransformComponentH2M>(entity);
				if (meshComponent.Mesh)
				{
					meshComponent.Mesh->OnUpdate(ts);

					// TODO: Should we render (logically)

					if (m_SelectedEntity == entity)
					{
						SceneRendererVulkanH2M::SubmitSelectedMesh(meshComponent, transformComponent);
					}
					else {
						SceneRendererVulkanH2M::SubmitMesh(meshComponent, transformComponent);
					}
				}
			}
			SceneRendererVulkanH2M::EndScene();

			// the following code replaces the VulkanRenderer::Draw() method
			// VulkanRenderer::SetCamera((HazelCamera)editorCamera); // s_Data.SceneData.SceneCamera.Camera = *camera;
			// VulkanRenderer::GeometryPass();
			// VulkanRenderer::CompositePass();
		}
		else
		{
			// Process lights
			{
				m_LightEnvironment = LightEnvironmentH2M();
				auto lights = m_Registry.group<DirectionalLightComponentH2M>(entt::get<TransformComponentH2M>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : lights)
				{
					auto [transformComponent, lightComponent] = lights.get<TransformComponentH2M, DirectionalLightComponentH2M>(entity);
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
				m_Environment = EnvironmentH2M();
				auto lights = m_Registry.group<SkyLightComponentH2M>(entt::get<TransformComponentH2M>);
				for (auto entity : lights)
				{
					auto [transformComponent, skyLightComponent] = lights.get<TransformComponentH2M, SkyLightComponentH2M>(entity);
					m_Environment = skyLightComponent.SceneEnvironment;
					SetSkybox(m_Environment.RadianceMap);
				}
			}

			if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
			{
				m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

				auto group = m_Registry.group<MeshComponentH2M>(entt::get<TransformComponentH2M>);

				SceneRendererH2M::BeginScene(this, { editorCamera, editorCamera.GetViewMatrix() });
				for (auto entity : group)
				{
					auto [meshComponent, transformComponent] = group.get<MeshComponentH2M, TransformComponentH2M>(entity);
					if (meshComponent.Mesh)
					{
						meshComponent.Mesh->OnUpdate(ts, false);

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

	void SceneH2M::SetEnvironment(const EnvironmentH2M& environment)
	{
		m_Environment = environment;
		SetSkybox(environment.RadianceMap);
	}

	void SceneH2M::SetSkybox(const RefH2M<TextureCubeH2M>& skybox)
	{
		m_SkyboxTexture = skybox;

		if (m_ShaderSkybox != nullptr)
		{
			m_ShaderSkybox->SetInt("u_Texture", skybox.Raw()->GetID());
		}

		m_SkyboxMaterial->Set("u_Texture", skybox);
	}

	EntityH2M SceneH2M::GetMainCameraEntity()
	{
		auto view = m_Registry.view<CameraComponentH2M>();
		for (auto entity : view)
		{
			auto& comp = view.get<CameraComponentH2M>(entity);
			if (comp.Primary)
			{
				return { entity, this };
			}
		}
		return {};
	}

	EntityH2M SceneH2M::FindEntityByUUID(UUID_H2M id)
	{
		auto view = m_Registry.view<IDComponentH2M>();
		for (auto entity : view)
		{
			auto& idComponent = m_Registry.get<IDComponentH2M>(entity);
			if (idComponent.ID == id)
				return EntityH2M(entity, this);
		}

		return EntityH2M{};
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

	glm::mat4 SceneH2M::GetTransformRelativeToParent(EntityH2M entity)
	{
		glm::mat4 transform(1.0f);

		EntityH2M parent = FindEntityByUUID(entity.GetParentUUID());
		if (parent)
			transform = GetTransformRelativeToParent(parent);

		return transform * entity.Transform().GetTransform();
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

	RefH2M<SceneH2M> SceneH2M::GetScene(UUID_H2M uuid)
	{
		if (s_ActiveScenes.find(uuid) != s_ActiveScenes.end()) {
			return s_ActiveScenes.at(uuid);
		}

		return {};
	}

	void SceneH2M::SetPhysics2DGravity(float gravity)
	{
		// TODO
	}

	float SceneH2M::GetPhysics2DGravity() const
	{
		return float{};
	}

	EntityH2M SceneH2M::CloneEntity(EntityH2M entity)
	{
		EntityH2M entityClone = EntityH2M(m_Registry.create(), this);

		if (entity.HasComponent<IDComponentH2M>()) {
			entityClone.AddComponent<IDComponentH2M>(entity.GetComponent<IDComponentH2M>());
		}

		if (entity.HasComponent<TagComponentH2M>()) {
			entityClone.AddComponent<TagComponentH2M>(entity.GetComponent<TagComponentH2M>());
		}

		if (entity.HasComponent<RelationshipComponentH2M>()) {
			entityClone.AddComponent<RelationshipComponentH2M>(entity.GetComponent<RelationshipComponentH2M>());
		}

		if (entity.HasComponent<TransformComponentH2M>()) {
			entityClone.AddComponent<TransformComponentH2M>(entity.GetComponent<TransformComponentH2M>());
		}

		if (entity.HasComponent<MeshComponentH2M>()) {
			entityClone.AddComponent<MeshComponentH2M>(entity.GetComponent<MeshComponentH2M>());
		}

		if (entity.HasComponent<CameraComponentH2M>()) {
			entityClone.AddComponent<CameraComponentH2M>(entity.GetComponent<CameraComponentH2M>());
		}

		if (entity.HasComponent<NativeScriptComponentH2M>()) {
			entityClone.AddComponent<NativeScriptComponentH2M>(entity.GetComponent<NativeScriptComponentH2M>());
		}

		if (entity.HasComponent<MaterialComponentH2M>()) {
			entityClone.AddComponent<MaterialComponentH2M>(entity.GetComponent<MaterialComponentH2M>());
		}

		if (entity.HasComponent<DirectionalLightComponentH2M>()) {
			entityClone.AddComponent<DirectionalLightComponentH2M>(entity.GetComponent<DirectionalLightComponentH2M>());
		}

		if (entity.HasComponent<PointLightComponentH2M>()) {
			entityClone.AddComponent<PointLightComponentH2M>(entity.GetComponent<PointLightComponentH2M>());
		}

		if (entity.HasComponent<SpotLightComponentH2M>()) {
			entityClone.AddComponent<SpotLightComponentH2M>(entity.GetComponent<SpotLightComponentH2M>());
		}

		if (entity.HasComponent<SkyLightComponentH2M>()) {
			entityClone.AddComponent<SkyLightComponentH2M>(entity.GetComponent<SkyLightComponentH2M>());
		}

		if (entity.HasComponent<SpriteRendererComponentH2M>()) {
			entityClone.AddComponent<SpriteRendererComponentH2M>(entity.GetComponent<SpriteRendererComponentH2M>());
		}

		if (entity.HasComponent<CircleRendererComponentH2M>()) {
			entityClone.AddComponent<CircleRendererComponentH2M>(entity.GetComponent<CircleRendererComponentH2M>());
		}

		if (entity.HasComponent<Rigidbody2DComponentH2M>()) {
			entityClone.AddComponent<Rigidbody2DComponentH2M>(entity.GetComponent<Rigidbody2DComponentH2M>());
		}

		if (entity.HasComponent<BoxCollider2DComponentH2M>()) {
			entityClone.AddComponent<BoxCollider2DComponentH2M>(entity.GetComponent<BoxCollider2DComponentH2M>());
		}

		Log::GetLogger()->warn("Method SceneH2M::CopyEntity implemented poorly [Tag: '{0}']", entity.GetComponent<TagComponentH2M>().Tag);

		return entityClone;
	}

	void SceneH2M::OnEvent(EventH2M& e)
	{
	}

	void SceneH2M::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	template<typename T>
	void SceneH2M::OnComponentAdded(EntityH2M entity, T& component)
	{
		// static_assert(false);
	}

	template<>
	void SceneH2M::OnComponentAdded<IDComponentH2M>(EntityH2M entity, IDComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<TagComponentH2M>(EntityH2M entity, TagComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<TransformComponentH2M>(EntityH2M entity, TransformComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<MeshComponentH2M>(EntityH2M entity, MeshComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<CameraComponentH2M>(EntityH2M entity, CameraComponentH2M& component)
	{
		component.Camera.SetViewportSize((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	template<>
	void SceneH2M::OnComponentAdded<SpriteRendererComponentH2M>(EntityH2M entity, SpriteRendererComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<CircleRendererComponentH2M>(EntityH2M entity, CircleRendererComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<MaterialComponentH2M>(EntityH2M entity, MaterialComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<DirectionalLightComponentH2M>(EntityH2M entity, DirectionalLightComponentH2M& component)
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

	template<>
	void SceneH2M::OnComponentAdded<Rigidbody2DComponentH2M>(EntityH2M entity, Rigidbody2DComponentH2M& component)
	{
	}

	template<>
	void SceneH2M::OnComponentAdded<BoxCollider2DComponentH2M>(EntityH2M entity, BoxCollider2DComponentH2M& component)
	{
	}

}
