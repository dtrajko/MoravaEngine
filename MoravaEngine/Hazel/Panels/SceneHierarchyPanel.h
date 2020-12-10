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
		void SetSelected(Entity entity);

		void OnImGuiRender();

	private:
		void DrawEntityNode(Entity entity);
		void DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level);
		void DrawComponents(Entity entity);

	private:
		HazelScene* m_Context;

	public:
		glm::mat4 m_CurrentlySelectedTransform = glm::mat4(1.0f);

	};

}
