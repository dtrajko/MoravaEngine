#include "HazelScene.h"
#include "Components.h"
#include "Entity.h"

#include <glm/glm.hpp>

#include <string>


namespace Hazel {


	HazelScene::HazelScene()
	{
	}

	Entity HazelScene::CreateEntity(const std::string& name)
	{ 
		Entity entity = { GetRegistry()->create(), this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		return entity;
	}

	void HazelScene::OnUpdate(float ts)
	{
		// Update scripts
		{
			GetRegistry()->view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
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
		Camera* mainCamera = nullptr;
		glm::mat4* cameraTransform = nullptr;
		{
			auto view = GetRegistry()->view<TransformComponent, CameraComponent>();

			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = &transform.Transform;
					break;
				}
			}
		}

		if (mainCamera)
		{
			// Renderer2D::BeginScene(mainCamera->GetProjection(), *cameraTransform);

			auto group = GetRegistry()->group<TransformComponent>(entt::get<SpriteRendererComponent>);
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
		auto view = GetRegistry()->view<CameraComponent>();
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
			GetRegistry()->view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
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
}