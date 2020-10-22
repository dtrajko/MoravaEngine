#pragma once

#include "Entity.h"
#include "../Renderer/HazelTexture.h"
#include "../Scene/SceneCamera.h"

#include "../../Shader.h"

#include "entt.hpp"

#include <string>


namespace Hazel {

	struct Environment
	{
		Hazel::HazelTextureCube* RadianceMap;
		Hazel::HazelTextureCube* IrradianceMap;
	};

	struct Light
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

		void OnUpdate(float ts);
		void OnViewportResize(uint32_t width, uint32_t height);

		inline entt::registry* GetRegistry() { return &m_Registry; };
		inline std::vector<Entity*>* GetEntities() { return &m_Entities; };

		void SetCamera(const HazelCamera& camera);
		void SetEnvironment(const Environment& environment);
		void SetSkybox(const Ref<Hazel::HazelTextureCube>& skybox);

		void AddEntity(Entity* entity);
		Entity* CreateEntity(const std::string& name = "");

		inline void SetSkyboxLOD(float LOD) { m_SkyboxLOD = LOD; }
		float& GetSkyboxLOD() { return m_SkyboxLOD; }

	private:
		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		// ECS
		entt::registry m_Registry;
		std::vector<Entity*> m_Entities;

		float m_SkyboxLOD = 1.0f;

		HazelCamera m_Camera;
		Environment m_Environment;
		Ref<Hazel::HazelTextureCube> m_SkyboxTexture;
		Shader* m_ShaderSkybox;

	};

}
