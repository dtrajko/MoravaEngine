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

	class Entity;

	class HazelScene
	{

	public:
		HazelScene();
		~HazelScene();

		void OnUpdate(float ts);
		void OnViewportResize(uint32_t width, uint32_t height);

		inline entt::registry* GetRegistry() { return &m_Registry; };
		inline const std::vector<Entity*>& GetEntities() { return m_Entities; };

		void SetCamera(const HazelCamera& camera);
		void SetEnvironment(const Environment& environment);
		void SetSkybox(const Ref<Hazel::HazelTextureCube>& skybox);

		void AddEntity(Entity* entity);
		Entity* CreateEntity(const std::string& name = "");

	private:
		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		// ECS
		entt::registry m_Registry;
		std::vector<Entity*> m_Entities;

		HazelCamera m_Camera;
		Environment m_Environment;
		Ref<Hazel::HazelTextureCube> m_SkyboxTexture;
		Shader* m_ShaderSkybox;

	};

}
