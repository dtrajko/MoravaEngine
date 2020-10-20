#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "../Core/Base.h"
#include "../Scene/Entity.h"
#include "../Scene/HazelScene.h"
#include "../Scene/Components.h"

#include "../../Log.h"


struct aiNode;

namespace Hazel
{

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(HazelScene* context);
		~SceneHierarchyPanel();

		void SetContext(HazelScene* context);
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
		HazelScene* m_Context;
		Entity m_SelectionContext;

	};

}
