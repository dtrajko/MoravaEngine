#pragma once

#include "../Core/UUID.h"
#include "../Core/Ref.h"
#include "../Renderer/HazelTexture.h"
#include "../Renderer/HazelMaterial.h"
#include "../Scene/SceneCamera.h"
#include "../Renderer/EditorCamera.h"

#include "../../Shader.h"

#include "entt.hpp"

#include <string>
#include <unordered_map>


namespace Hazel {

	struct Environment
	{
		Ref<Hazel::HazelTextureCube> RadianceMap;
		Ref<Hazel::HazelTextureCube> IrradianceMap;

		static Environment Load(const std::string& filepath) {}; // TODO
	};

	struct HazelLight
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;
		float Multiplier = 1.0f;
	};

	class Entity;
	class ScriptableEntity;

	class HazelScene : public RefCounted
	{

	public:
		HazelScene() = default;
		HazelScene(const std::string& debugName);
		~HazelScene();

		void Init();

		void OnUpdate(float ts);
		void OnRenderRuntime(Timestep ts);
		void OnRenderEditor(Timestep ts, const EditorCamera& editorCamera);
		void OnEvent(Event& e);

		// Runtime
		void OnRuntimeStart();
		void OnRuntimeStop();
		void OnViewportResize(uint32_t width, uint32_t height);

		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		inline void SetCamera(const HazelCamera& camera) { m_Camera = camera; };
		inline HazelCamera* GetCamera() { return &m_Camera; }

		void SetSkyboxMaterial(Ref<HazelMaterialInstance> skyboxMaterial) { m_SkyboxMaterial = skyboxMaterial; }
		Ref<HazelMaterialInstance> GetSkyboxMaterial() { return m_SkyboxMaterial; }

		void SetEnvironment(const Environment& environment);
		inline Environment GetEnvironment() { return m_Environment; }

		inline void SetLight(HazelLight light) { m_Light = light; };
		inline HazelLight GetLight() { return m_Light; }

		void SetSkybox(const Ref<HazelTextureCube>& skybox);

		inline void SetSkyboxLOD(float LOD) { m_SkyboxLOD = LOD; }
		float& GetSkyboxLOD() { return m_SkyboxLOD; }

		Entity CreateEntity(const std::string& name = "");
		Entity CreateEntity(const std::string& name, const HazelScene& scene);
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "", bool runtimeMap = false);
		void DestroyEntity(Entity entity);
		Entity CloneEntity(Entity entity);
		void DuplicateEntity(Entity entity); // Cherno's version, same as CloneEntity

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<T>();
		}

		std::unordered_map<UUID, entt::entity> GetEntityMap() const;

		// Temporary/experimental
		virtual void OnEntitySelected(Entity entity);

		void CopyTo(Ref<HazelScene>& target); // Working on Hazel LIVE! #14

		UUID GetUUID() const { return m_SceneID; }

		static Ref<HazelScene> GetScene(UUID uuid);

		// Editor-specific
		void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }

	public:
		UUID m_SceneID;
		entt::entity m_SceneEntity;
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

	private:
		std::unordered_map<UUID, entt::entity> m_EntityIDMap;

		HazelCamera m_Camera;
		HazelLight m_Light;
		float m_LightMultiplier = 0.3f;

		Environment m_Environment;
		Ref<Hazel::HazelTextureCube> m_SkyboxTexture;
		Shader* m_ShaderSkybox;

		Ref<HazelMaterialInstance> m_SkyboxMaterial;

		entt::entity m_SelectedEntity;

		float m_SkyboxLOD = 1.0f;
		bool m_IsPlaying = false;

		std::string m_DebugName;

		friend class SceneHierarchyPanel;

		friend void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		friend void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity);

	};

}
