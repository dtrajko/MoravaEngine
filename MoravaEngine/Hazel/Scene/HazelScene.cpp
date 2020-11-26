#include "HazelScene.h"
#include "Components.h"
#include "Entity.h"
#include "../Renderer/HazelMesh.h"
#include "../Renderer/SceneRenderer.h"

#include <glm/glm.hpp>

#include <string>


namespace Hazel {

	static const std::string DefaultEntityName = "Entity";

	HazelScene::HazelScene()
	{
	}

	void HazelScene::SetEnvironment(const Environment& environment)
	{
		m_Environment = environment;
		SetSkybox(std::shared_ptr<Hazel::HazelTextureCube>(environment.RadianceMap));
	}

	void HazelScene::SetSkybox(const Ref<Hazel::HazelTextureCube>& skybox)
	{
		m_SkyboxTexture = skybox;
		m_ShaderSkybox->setInt("u_Texture", skybox.get()->GetID());
	}

	//	void HazelScene::AddEntity(Entity entity)
	//	{
	//		m_Entities.push_back(entity);
	//	}

	void HazelScene::OnEntitySelected(Entity entity)
	{
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
		Entity entity = Entity(m_Registry.create(), this);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		// NoECS
		entity.SetName(entityName);
		// AddEntity(entity);

		Log::GetLogger()->debug("CreateEntity name = '{0}'", name);

		return entity;
	}

	void HazelScene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void HazelScene::OnUpdate(float ts)
	{
		//	ECS Update all entities
		//	for (auto entity : m_Entities)
		//	{
		//		auto mesh = entity.GetMesh();
		//		if (mesh) {
		//			mesh->OnUpdate(ts, false);
		//		}
		//	}

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
				cameraComponent.Camera.SetViewportSize(width, height);
			}
		}
	}

	HazelScene::~HazelScene()
	{
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

		//	No ECS
		//	for (Entity entity : m_Entities)
		//	{
		//		delete entity;
		//	}
	}

	template<typename T>
	void HazelScene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}

	template<>
	void HazelScene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void HazelScene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void HazelScene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
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

}
