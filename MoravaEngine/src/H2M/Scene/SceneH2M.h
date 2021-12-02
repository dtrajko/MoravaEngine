#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Core/UUID.h"
#include "H2M/Editor/EditorCamera.h"
#include "H2M/Renderer/SceneEnvironment.h"
#include "H2M/Renderer/Renderer2D.h"
#include "H2M/Scene/Components.h"
#include "H2M/Scene/SceneCamera.h"

#include "H2M/Renderer/MaterialH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include "Shader/MoravaShader.h"

#include "entt.hpp"

#include <string>
#include <unordered_map>


namespace H2M {

	class SceneRendererH2M;
	struct TransformComponentH2M;

	struct DirLightH2M
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		float Multiplier = 1.0f;
	};

	struct DirectionalLightH2M
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Multiplier = 0.0f;

		// C++ only
		bool CastShadows = true;
	};

	struct PointLightH2M
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
		DirectionalLightH2M DirectionalLights[4];
		std::vector<PointLightH2M> PointLights;
	};

	class EntityH2M;
	class ScriptableEntity;
	using EntityMapH2M = std::unordered_map<UUID, entt::entity>;


	class SceneH2M : public H2M::RefCounted
	{
	public:
		SceneH2M(const std::string& debugName = "Scene", bool isEditorScene = false);
		~SceneH2M();

		void Init();

		void OnUpdate(H2M::Timestep ts);
		void OnRenderRuntime(RefH2M<SceneRendererH2M> renderer, H2M::Timestep ts);
		void OnRenderEditor(RefH2M<SceneRendererH2M> renderer, H2M::Timestep ts, const H2M::EditorCamera& editorCamera);
		void OnRenderSimulation(RefH2M<SceneRendererH2M> renderer, H2M::Timestep ts, const H2M::EditorCamera& editorCamera);
		void OnEvent(Event& e);

		// Runtime
		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationEnd();

		void SetViewportSize(uint32_t width, uint32_t height);

		void OnViewportResize(uint32_t width, uint32_t height);

		template<typename T>
		void OnComponentAdded(EntityH2M entity, T& component);

		inline void SetCamera(const H2M::CameraH2M& camera) { m_Camera = camera; };
		inline H2M::CameraH2M* GetCamera() { return &m_Camera; }

		void SetSkyboxMaterial(RefH2M<MaterialH2M> skyboxMaterial) { m_SkyboxMaterial = skyboxMaterial; }
		RefH2M<MaterialH2M> GetSkyboxMaterial() { return m_SkyboxMaterial; }

		void SetEnvironment(RefH2M<Environment> environment);
		inline RefH2M<Environment> GetEnvironment() { return m_Environment; }
		void SetSkybox(const RefH2M<TextureCubeH2M>& skybox);

		DirLightH2M& GetLight() { return m_Light; }
		inline const DirLightH2M& GetLight() const { return m_Light; }
		inline void SetLight(DirLightH2M light) { m_Light = light; };

		EntityH2M GetMainCameraEntity();

		EntityH2M CreateEntity(const std::string& name = "");
		EntityH2M CreateEntity(const std::string& name, RefH2M<SceneH2M> scene);
		EntityH2M CreateEntityWithID(UUID uuid, const std::string& name = "", bool runtimeMap = false);
		void DestroyEntity(EntityH2M entity);
		EntityH2M CloneEntity(EntityH2M entity);
		void DuplicateEntity(EntityH2M entity); // Cherno's version, same as CloneEntity

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<T>();
		}

		EntityH2M FindEntityByTag(const std::string& tag);
		EntityH2M FindEntityByUUID(UUID id);

		void ConvertToLocalSpace(EntityH2M entity);
		void ConvertToWorldSpace(EntityH2M entity);
		glm::mat4 GetTransformRelativeToParent(EntityH2M entity);
		glm::mat4 GetWorldSpaceTransformMatrix(EntityH2M entity);
		TransformComponentH2M GetWorldSpaceTransform(EntityH2M entity);

		void ParentEntity(EntityH2M entity, EntityH2M parent);
		void UnparentEntity(EntityH2M entity, bool convertToWorldSpace = true);

		const EntityMapH2M& GetEntityMap() const { return m_EntityIDMap; }

		// Temporary/experimental
		virtual void OnEntitySelected(EntityH2M entity);

		void CopyTo(RefH2M<SceneH2M>& target); // Working on Hazel LIVE! #14

		UUID GetUUID() const { return m_SceneID; }

		static RefH2M<SceneH2M> GetScene(UUID uuid);

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

		EntityMapH2M m_EntityIDMap;

	private:
		HazelCamera m_Camera;
		DirLightH2M m_Light;
		float m_LightMultiplier = 0.3f;

		// MoravaShader* m_ShaderSkybox;

		RefH2M<Environment> m_Environment;
		float m_EnvironmentIntensity = 1.0f;
		RefH2M<H2M::TextureCubeH2M> m_SkyboxTexture;
		MoravaShader* m_ShaderSkybox;

		RefH2M<HazelMaterial> m_SkyboxMaterial;

		entt::entity m_SelectedEntity;

		EntityH2M* m_PhysicsBodyEntityBuffer = nullptr;

		std::string m_DebugName;

		LightEnvironmentLegacy m_LightEnvironment;

		EntityH2M* m_Physics2DBodyEntityBuffer = nullptr;

		std::vector<std::function<void()>> m_PostUpdateQueue;

		RefH2M<H2M::Renderer2D> m_SceneRenderer2D;

		float m_SkyboxLod = 1.0f;
		bool m_IsPlaying = false;
		bool m_ShouldSimulate = false;

		friend class EntityH2M;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
		friend class SceneHierarchyPanelH2M;

		// friend void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		// friend void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity);

	};

}
