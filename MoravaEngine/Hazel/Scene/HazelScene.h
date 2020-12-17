#pragma once

#include "../Core/UUID.h"
#include "../Core/Ref.h"
#include "../Renderer/HazelTexture.h"
#include "../Renderer/HazelMaterial.h"
#include "../Scene/SceneCamera.h"

#include "../../Shader.h"

#include "entt.hpp"

#include <string>


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

	using EntityMap = std::unordered_map<UUID, Entity>;

	class HazelScene : public RefCounted
	{

	public:
		HazelScene() = default;
		HazelScene(const std::string& debugName);
		~HazelScene();

		void Init();

		void OnUpdate(float ts);
		void OnEvent(Event& e);
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

		Entity CreateEntity(const std::string& name);
		Entity CreateEntity(const std::string& name, const HazelScene& scene);
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "", bool runtimeMap = false);
		void DestroyEntity(Entity entity);
		Entity CloneEntity(Entity entity);

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<T>();
		}

		const EntityMap& GetEntityMap() const { return m_EntityIDMap; }

		// Temporary/experimental
		virtual void OnEntitySelected(Entity entity);

		void CopyTo(Ref<HazelScene>& target); // Working on Hazel LIVE! #14

	public:
		// ECS
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

	private:
		EntityMap m_EntityIDMap;

		HazelCamera m_Camera;
		HazelLight m_Light;
		float m_LightMultiplier = 0.3f;

		Environment m_Environment;
		Ref<Hazel::HazelTextureCube> m_SkyboxTexture;
		Shader* m_ShaderSkybox;

		Ref<HazelMaterialInstance> m_SkyboxMaterial;

		float m_SkyboxLOD = 1.0f;

		std::string m_DebugName;

		entt::entity m_SceneEntity;
		UUID m_SceneID;

	};

}
