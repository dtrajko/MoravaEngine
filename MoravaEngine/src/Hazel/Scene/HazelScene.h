#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Components.h"
#include "SceneCamera.h"

#include "Hazel/Core/Ref.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Editor/EditorCamera.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/SceneEnvironment.h"

#include "Shader/MoravaShader.h"

#include "entt.hpp"

#include <string>
#include <unordered_map>


namespace Hazel {

	struct HazelLight
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;
		float Multiplier = 1.0f;
	};

	struct HazelDirectionalLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Multiplier = 0.0f;

		// C++ only
		bool CastShadows = true;
	};

	struct HazelPointLight
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

	struct LightEnvironment
	{
		HazelDirectionalLight DirectionalLights[4];
		std::vector<HazelPointLight> PointLights;
	};

	class Entity;
	class ScriptableEntity;
	using EntityMap = std::unordered_map<UUID, entt::entity>;


	class HazelScene : public RefCounted
	{
	public:
		HazelScene(const std::string& debugName = "Scene", bool isEditorScene = false);
		~HazelScene();

		void Init();

		void OnUpdate(Timestep ts);
		void OnRenderRuntime(Timestep ts);
		void OnRenderEditor(Timestep ts, const EditorCamera& editorCamera);
		void OnEvent(Event& e);

		// Runtime
		void OnRuntimeStart();
		void OnRuntimeStop();

		void SetViewportSize(uint32_t width, uint32_t height);

		void OnViewportResize(uint32_t width, uint32_t height);

		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		inline void SetCamera(const HazelCamera& camera) { m_Camera = camera; };
		inline HazelCamera* GetCamera() { return &m_Camera; }

		void SetSkyboxMaterial(Ref<HazelMaterial> skyboxMaterial) { m_SkyboxMaterial = skyboxMaterial; }
		Ref<HazelMaterial> GetSkyboxMaterial() { return m_SkyboxMaterial; }

		void SetEnvironment(const Environment& environment);
		inline const Environment& GetEnvironment() const { return m_Environment; }
		void SetSkybox(const Ref<HazelTextureCube>& skybox);

		HazelLight& GetLight() { return m_Light; }
		inline const HazelLight& GetLight() const { return m_Light; }
		inline void SetLight(HazelLight light) { m_Light = light; };

		Entity GetMainCameraEntity();

		inline void SetSkyboxLod(float LOD) { m_SkyboxLod = LOD; }
		float& GetSkyboxLod() { return m_SkyboxLod; }
		float GetSkyboxLod() const { return m_SkyboxLod; }

		Entity CreateEntity(const std::string& name = "");
		Entity CreateEntity(const std::string& name, Ref<HazelScene> scene);
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "", bool runtimeMap = false);
		void DestroyEntity(Entity entity);
		Entity CloneEntity(Entity entity);
		void DuplicateEntity(Entity entity); // Cherno's version, same as CloneEntity

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<T>();
		}

		Entity FindEntityByTag(const std::string& tag);
		Entity FindEntityByUUID(UUID id);

		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);
		glm::mat4 GetTransformRelativeToParent(Entity entity);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		TransformComponent GetWorldSpaceTransform(Entity entity);

		void ParentEntity(Entity entity, Entity parent);
		void UnparentEntity(Entity entity, bool convertToWorldSpace = true);

		const EntityMap& GetEntityMap() const { return m_EntityIDMap; }
		
		// Temporary/experimental
		virtual void OnEntitySelected(Entity entity);

		void CopyTo(Ref<HazelScene>& target); // Working on Hazel LIVE! #14

		UUID GetUUID() const { return m_SceneID; }

		static Ref<HazelScene> GetScene(UUID uuid);

		void SetPhysics2DGravity(float gravity);
		float GetPhysics2DGravity() const;

		// Editor-specific
		void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }

	public:
		UUID m_SceneID;
		entt::entity m_SceneEntity;
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		EntityMap m_EntityIDMap;

	private:

		HazelCamera m_Camera;
		HazelLight m_Light;
		float m_LightMultiplier = 0.3f;

		Environment m_Environment;
		Ref<Hazel::HazelTextureCube> m_SkyboxTexture;
		MoravaShader* m_ShaderSkybox;

		Ref<HazelMaterial> m_SkyboxMaterial;

		entt::entity m_SelectedEntity;

		Entity* m_PhysicsBodyEntityBuffer = nullptr;

		float m_SkyboxLod = 1.0f;
		bool m_IsPlaying = false;

		std::string m_DebugName;

		LightEnvironment m_LightEnvironment;

		friend class Entity;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;

		// friend void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		// friend void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity);

	};

}
