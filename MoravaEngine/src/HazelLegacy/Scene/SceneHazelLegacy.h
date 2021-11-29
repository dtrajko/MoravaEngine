#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Editor/EditorCamera.h"
#include "Hazel/Renderer/SceneEnvironment.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/SceneCamera.h"

#include "HazelLegacy/Renderer/MaterialHazelLegacy.h"
#include "HazelLegacy/Renderer/TextureHazelLegacy.h"

#include "Shader/MoravaShader.h"

#include "entt.hpp"

#include <string>
#include <unordered_map>


namespace HazelLegacy {

	class SceneRendererHazelLegacy;
	struct TransformComponentHazelLegacy;

	struct DirLightHazelLegacy
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		float Multiplier = 1.0f;
	};

	struct DirectionalLightHazelLegacy
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Multiplier = 0.0f;

		// C++ only
		bool CastShadows = true;
	};

	struct PointLightHazelLegacy
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Multiplier = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float MinRadius = 0.001f;
		float Radius = 25.0f;
		float Falloff = 1.0f;
		float SourceSize = 0.1f;
		bool CastsShadows = true;
		char Padding[3]{ 0, 0, 0 };
	};

	struct LightEnvironmentLegacy
	{
		DirectionalLightHazelLegacy DirectionalLights[4];
		std::vector<PointLightHazelLegacy> PointLights;
	};

	class EntityHazelLegacy;
	class ScriptableEntity;
	using EntityMapHazelLegacy = std::unordered_map<UUID, entt::entity>;


	class SceneHazelLegacy : public Hazel::RefCounted
	{
	public:
		SceneHazelLegacy(const std::string& debugName = "Scene", bool isEditorScene = false);
		~SceneHazelLegacy();

		void Init();

		void OnUpdate(Hazel::Timestep ts);
		void OnRenderRuntime(Hazel::Ref<SceneRendererHazelLegacy> renderer, Hazel::Timestep ts);
		void OnRenderEditor(Hazel::Ref<SceneRendererHazelLegacy> renderer, Hazel::Timestep ts, const Hazel::EditorCamera& editorCamera);
		void OnRenderSimulation(Hazel::Ref<SceneRendererHazelLegacy> renderer, Hazel::Timestep ts, const Hazel::EditorCamera& editorCamera);
		void OnEvent(Event& e);

		// Runtime
		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationEnd();

		void SetViewportSize(uint32_t width, uint32_t height);

		void OnViewportResize(uint32_t width, uint32_t height);

		template<typename T>
		void OnComponentAdded(EntityHazelLegacy entity, T& component);

		inline void SetCamera(const Hazel::HazelCamera& camera) { m_Camera = camera; };
		inline Hazel::HazelCamera* GetCamera() { return &m_Camera; }

		void SetSkyboxMaterial(Hazel::Ref<MaterialHazelLegacy> skyboxMaterial) { m_SkyboxMaterial = skyboxMaterial; }
		Hazel::Ref<MaterialHazelLegacy> GetSkyboxMaterial() { return m_SkyboxMaterial; }

		void SetEnvironment(Hazel::Ref<Environment> environment);
		inline Hazel::Ref<Environment> GetEnvironment() { return m_Environment; }
		void SetSkybox(const Hazel::Ref<TextureCubeHazelLegacy>& skybox);

		DirLightHazelLegacy& GetLight() { return m_Light; }
		inline const DirLightHazelLegacy& GetLight() const { return m_Light; }
		inline void SetLight(DirLightHazelLegacy light) { m_Light = light; };

		EntityHazelLegacy GetMainCameraEntity();

		EntityHazelLegacy CreateEntity(const std::string& name = "");
		EntityHazelLegacy CreateEntity(const std::string& name, Hazel::Ref<SceneHazelLegacy> scene);
		EntityHazelLegacy CreateEntityWithID(UUID uuid, const std::string& name = "", bool runtimeMap = false);
		void DestroyEntity(EntityHazelLegacy entity);
		EntityHazelLegacy CloneEntity(EntityHazelLegacy entity);
		void DuplicateEntity(EntityHazelLegacy entity); // Cherno's version, same as CloneEntity

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<T>();
		}

		EntityHazelLegacy FindEntityByTag(const std::string& tag);
		EntityHazelLegacy FindEntityByUUID(UUID id);

		void ConvertToLocalSpace(EntityHazelLegacy entity);
		void ConvertToWorldSpace(EntityHazelLegacy entity);
		glm::mat4 GetTransformRelativeToParent(EntityHazelLegacy entity);
		glm::mat4 GetWorldSpaceTransformMatrix(EntityHazelLegacy entity);
		TransformComponentHazelLegacy GetWorldSpaceTransform(EntityHazelLegacy entity);

		void ParentEntity(EntityHazelLegacy entity, EntityHazelLegacy parent);
		void UnparentEntity(EntityHazelLegacy entity, bool convertToWorldSpace = true);

		const EntityMapHazelLegacy& GetEntityMap() const { return m_EntityIDMap; }

		// Temporary/experimental
		virtual void OnEntitySelected(EntityHazelLegacy entity);

		void CopyTo(Hazel::Ref<SceneHazelLegacy>& target); // Working on Hazel LIVE! #14

		UUID GetUUID() const { return m_SceneID; }

		static Hazel::Ref<SceneHazelLegacy> GetScene(UUID uuid);

		void SetPhysics2DGravity(float gravity);
		float GetPhysics2DGravity() const;

		// Editor-specific
		void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }

		// Obsolete methods
		inline void SetSkyboxLod(float LOD) { m_SkyboxLod = LOD; }
		float& GetSkyboxLod() { return m_SkyboxLod; }
		float GetSkyboxLod() const { return m_SkyboxLod; }

	public:
		UUID m_SceneID;
		entt::entity m_SceneEntity;
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		EntityMapHazelLegacy m_EntityIDMap;

	private:
		HazelCamera m_Camera;
		DirLightHazelLegacy m_Light;
		float m_LightMultiplier = 0.3f;

		// MoravaShader* m_ShaderSkybox;

		Hazel::Ref<Environment> m_Environment;
		float m_EnvironmentIntensity = 1.0f;
		Hazel::Ref<Hazel::TextureCubeHazelLegacy> m_SkyboxTexture;
		MoravaShader* m_ShaderSkybox;

		Hazel::Ref<HazelMaterial> m_SkyboxMaterial;

		entt::entity m_SelectedEntity;

		EntityHazelLegacy* m_PhysicsBodyEntityBuffer = nullptr;

		std::string m_DebugName;

		LightEnvironmentLegacy m_LightEnvironment;

		EntityHazelLegacy* m_Physics2DBodyEntityBuffer = nullptr;

		std::vector<std::function<void()>> m_PostUpdateQueue;

		Hazel::Ref<Hazel::Renderer2D> m_SceneRenderer2D;

		float m_SkyboxLod = 1.0f;
		bool m_IsPlaying = false;
		bool m_ShouldSimulate = false;

		friend class EntityHazelLegacy;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
		friend class SceneHierarchyPanelHazelLegacy;

		// friend void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		// friend void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity);

	};

}
