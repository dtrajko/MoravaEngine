#pragma once

#include "Entity.h"
#include "../Renderer/HazelTexture.h"
#include "../Scene/SceneCamera.h"
#include "../Renderer/HazelMaterial.h"

#include "../../Shader.h"

#include "entt.hpp"

#include <string>


namespace Hazel {

	struct Environment
	{
		Hazel::HazelTextureCube* RadianceMap;
		Hazel::HazelTextureCube* IrradianceMap;
	};

	struct HazelLight
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;

		float Multiplier = 1.0f;
	};

	class Entity;

	class HazelScene
	{

	public:
		HazelScene();
		~HazelScene();

		Entity* CreateEntity(const std::string& name, bool ecs);
		Entity* CreateEntityECS(const std::string& name);
		Entity* CreateEntityNoECS(const std::string& name);
		void DestroyEntity(Entity entity);

		void OnUpdate(float ts);
		void OnViewportResize(uint32_t width, uint32_t height);

		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		void AddEntity(Entity* entity);

		// inline entt::registry* GetRegistry() { return &m_Registry; };
		inline std::vector<Entity*>* GetEntities() { return &m_Entities; };

		inline void SetCamera(const HazelCamera& camera) { m_Camera = camera; };
		inline HazelCamera* GetCamera() { return &m_Camera; }

		void SetSkyboxMaterial(Ref<HazelMaterialInstance> skyboxMaterial) { m_SkyboxMaterial = skyboxMaterial; }
		Ref<HazelMaterialInstance> GetSkyboxMaterial() { return m_SkyboxMaterial; }

		void SetEnvironment(const Environment& environment);
		inline Environment GetEnvironment() { return m_Environment; }

		inline void SetLight(HazelLight light) { m_Light = light; };
		inline HazelLight GetLight() { return m_Light; }

		void SetSkybox(const Ref<Hazel::HazelTextureCube>& skybox);

		inline void SetSkyboxLOD(float LOD) { m_SkyboxLOD = LOD; }
		float& GetSkyboxLOD() { return m_SkyboxLOD; }

	public:
		// ECS
		entt::registry m_Registry;

	private:
		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		// NoECS
		std::vector<Entity*> m_Entities;

		HazelCamera m_Camera;

		HazelLight m_Light;
		float m_LightMultiplier = 0.3f;

		Environment m_Environment;
		Ref<Hazel::HazelTextureCube> m_SkyboxTexture;
		Shader* m_ShaderSkybox;

		Ref<HazelMaterialInstance> m_SkyboxMaterial;

		float m_SkyboxLOD = 1.0f;

	};

}
