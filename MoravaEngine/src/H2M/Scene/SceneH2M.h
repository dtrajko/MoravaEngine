/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "ComponentsH2M.h"
#include "SceneCameraH2M.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Core/UUID_H2M.h"
#include "H2M/Editor/EditorCameraH2M.h"
#include "H2M/Renderer/MaterialH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/SceneEnvironmentH2M.h"

#include "Shader/MoravaShader.h"

#include "entt.hpp"

#include <string>
#include <unordered_map>


namespace H2M
{

	struct LightH2M
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;
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
		float Falloff = 1.f;
		float SourceSize = 0.1f;
		bool CastsShadows = true;
		char Padding[3]{ 0, 0, 0 };
	};

	struct LightEnvironmentH2M
	{
		DirectionalLightH2M DirectionalLights[4];
		std::vector<PointLightH2M> PointLights;
	};

	class EntityH2M;
	using EntityMapH2M = std::unordered_map<UUID_H2M, entt::entity>;


	class SceneH2M : public RefCountedH2M
	{
	public:
		SceneH2M(const std::string& debugName = "SceneH2M", bool isEditorScene = false);
		~SceneH2M();

		void Init();

		void OnUpdate(TimestepH2M ts);
		void OnRenderRuntime(TimestepH2M ts);
		void OnRenderEditor(TimestepH2M ts, const EditorCameraH2M& editorCamera);
		void OnEvent(EventH2M& e);

		// Runtime
		void OnRuntimeStart();
		void OnRuntimeStop();

		void SetViewportSize(uint32_t width, uint32_t height);

		void OnViewportResize(uint32_t width, uint32_t height);

		template<typename T>
		void OnComponentAdded(EntityH2M entity, T& component);

		inline void SetCamera(const CameraH2M& camera) { m_Camera = camera; };
		inline CameraH2M* GetCamera() { return &m_Camera; }

		void SetSkyboxMaterial(RefH2M<MaterialH2M> skyboxMaterial) { m_SkyboxMaterial = skyboxMaterial; }
		RefH2M<MaterialH2M> GetSkyboxMaterial() { return m_SkyboxMaterial; }

		void SetEnvironment(const EnvironmentH2M& environment);
		inline const EnvironmentH2M& GetEnvironment() const { return m_Environment; }
		void SetSkybox(const RefH2M<TextureCubeH2M>& skybox);

		LightH2M& GetLight() { return m_Light; }
		inline const LightH2M& GetLight() const { return m_Light; }
		inline void SetLight(LightH2M light) { m_Light = light; };

		EntityH2M GetMainCameraEntity();

		inline void SetSkyboxLod(float LOD) { m_SkyboxLod = LOD; }
		float& GetSkyboxLod() { return m_SkyboxLod; }
		float GetSkyboxLod() const { return m_SkyboxLod; }

		EntityH2M CreateEntity(const std::string& name = "");
		EntityH2M CreateEntity(const std::string& name, RefH2M<SceneH2M> scene);
		EntityH2M CreateEntityWithID(UUID_H2M uuid, const std::string& name = "", bool runtimeMap = false);
		void DestroyEntity(EntityH2M entity);
		EntityH2M CloneEntity(EntityH2M entity);
		void DuplicateEntity(EntityH2M entity); // Cherno's version, same as CloneEntity

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<T>();
		}

		EntityH2M FindEntityByTag(const std::string& tag);
		EntityH2M FindEntityByUUID(UUID_H2M id);

		void ConvertToLocalSpace(EntityH2M entity);
		void ConvertToWorldSpace(EntityH2M entity);
		glm::mat4 GetTransformRelativeToParent(EntityH2M entity);
		glm::mat4 GetWorldSpaceTransformMatrix(EntityH2M entity);
		TransformComponentH2M GetWorldSpaceTransform(EntityH2M entity);

		void ParentEntity(EntityH2M entity, EntityH2M parent);
		void UnparentEntity(EntityH2M entity, bool convertToWorldSpace = true);

		const EntityMapH2M& GetEntityMapH2M() const { return m_EntityIDMap; }
		
		// Temporary/experimental
		virtual void OnEntitySelected(EntityH2M entity);

		void CopyTo(RefH2M<SceneH2M>& target); // Working on Hazel LIVE! #14

		UUID_H2M GetUUID() const { return m_SceneID; }

		static RefH2M<SceneH2M> GetScene(UUID_H2M uuid);

		void SetPhysics2DGravity(float gravity);
		float GetPhysics2DGravity() const;

		// Editor-specific
		void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }

	public:
		UUID_H2M m_SceneID;
		entt::entity m_SceneEntity;
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		EntityMapH2M m_EntityIDMap;

	private:
		CameraH2M m_Camera;
		LightH2M m_Light;
		float m_LightMultiplier = 0.3f;

		EnvironmentH2M m_Environment;
		RefH2M<TextureCubeH2M> m_SkyboxTexture;
		MoravaShader* m_ShaderSkybox;

		RefH2M<MaterialH2M> m_SkyboxMaterial;

		entt::entity m_SelectedEntity;

		EntityH2M* m_PhysicsBodyEntityBuffer = nullptr;

		float m_SkyboxLod = 1.0f;
		bool m_IsPlaying = false;

		std::string m_DebugName;

		LightEnvironmentH2M m_LightEnvironment;

		friend class EntityH2M;
		friend class SceneRendererH2M;
		friend class SceneHierarchyPanelH2M;

		// friend void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		// friend void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity);

	};

}
