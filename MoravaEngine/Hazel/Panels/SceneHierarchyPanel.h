#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "../Core/Base.h"
#include "../Scene/Entity.h"
#include "../../Scene.h"
#include "../../Log.h"


namespace Hazel
{

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(Scene* context);
		~SceneHierarchyPanel();

		void SetContext(Scene* context);
		void OnImGuiRender();

	private:
		// ECS (Scene::m_Registry)
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
		void OnImGuiRenderECS();

		// NoECS (Scene::m_Entities)
		void OnImGuiRenderNoECS();
		void DrawEntityNode(Entity* entity, uint32_t& imguiEntityID, uint32_t& imguiMeshID);
		void DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level);

	private:
		Scene* m_Context;
		Entity m_SelectionContext;

	};

}
