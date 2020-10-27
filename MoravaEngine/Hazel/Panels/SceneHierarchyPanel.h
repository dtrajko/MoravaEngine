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
		SceneHierarchyPanel(HazelScene* scene);
		~SceneHierarchyPanel();

		void SetContext(HazelScene* scene);
		void OnImGuiRender();

	private:
		// ECS (Scene::m_Registry)
		void DrawComponents(Entity entity);
		void DrawEntityNodeECS(Entity entity);
		void OnImGuiRenderECS();

		// NoECS (Scene::m_Entities)
		// void OnImGuiRenderNoECS();
		void DrawEntityNodeNoECS(Entity entity, uint32_t& imguiEntityID, uint32_t& imguiMeshID);
		void DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level);

	private:
		HazelScene* m_Context;
		Entity m_SelectionContext;

	};

}
