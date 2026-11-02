// ModelH2M's ImGui windows (its node hierarchy, animation controls): compiled into the editor (see ModelH2M.cpp for the
// rest of the class)

#include "ModelH2M.h"

#include "Core/Log.h"
#include "Core/Math.h"
#include "Core/Util.h"

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <imgui.h>


namespace H2M
{

	void ModelH2M::ImGuiNodeHierarchy(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 localTransform = Math::Mat4FromAssimpMat4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t meshIndex = node->mMeshes[i];
			m_Meshes[meshIndex]->NodeName = node->mName.C_Str();
			m_Meshes[meshIndex]->Transform = transform;
		}

		if (ImGui::TreeNode(node->mName.C_Str()))
		{
			auto [translation, rotation, scale] = Math::GetTransformDecomposition(transform);
			glm::vec3 rotationVec3 = glm::degrees(glm::eulerAngles(rotation));
			ImGui::Text("World Transform");
			ImGui::Text("  Translation: %.2f %.2f %.2f", translation.x, translation.y, translation.z);
			ImGui::Text("  Rotation:    %.2f %.2f %.2f", rotationVec3.x, rotationVec3.y, rotationVec3.z);
			ImGui::Text("  Scale:       %.2f %.2f %.2f", scale.x, scale.y, scale.z);

			/****
			{
				auto [translation, rotation, scale] = Math::GetTransformDecomposition(localTransform);
				glm::vec3 rotationVec3 = glm::degrees(glm::eulerAngles(rotation));
				ImGui::Text("Local Transform");
				ImGui::Text("  Translation: %.2f %.2f %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Rotation:    %.2f %.2f %.2f", rotationVec3.x, rotationVec3.y, rotationVec3.z);
				ImGui::Text("  Scale:       %.2f %.2f %.2f", scale.x, scale.y, scale.z);
			}
			****/

			for (uint32_t i = 0; i < node->mNumChildren; i++)
			{
				ImGuiNodeHierarchy(node->mChildren[i], transform, level + 1);
			}

			ImGui::TreePop();
		}
	}

	void ModelH2M::OnImGuiRender(uint32_t id, bool* p_open)
	{
		if (!m_Scene)
		{
			Log::GetLogger()->error("Mesh: Scene not initialized!");
			return;
		}

		// Mesh Hierarchy
		ImGui::Begin("Mesh Hierarchy", p_open);
		ImGuiNodeHierarchy(m_Scene->mRootNode, glm::mat4(1.0f), 0);
		ImGui::End();

		ImGui::Begin("Mesh Debug", p_open);
		std::string meshFileName = std::to_string(id) + ". " + Util::GetFileNameFromFullPath(m_FilePath);
		if (ImGui::CollapsingHeader(meshFileName.c_str()))
		{
			if (m_IsAnimated && m_Scene->mAnimations)
			{
				ImGui::Indent(10.0f);
				if (ImGui::CollapsingHeader("Animation"))
				{
					if (ImGui::Button(m_AnimationPlaying ? "Pause" : "Play"))
						m_AnimationPlaying = !m_AnimationPlaying;

					ImGui::SliderFloat("##AnimationTime", &m_AnimationTime, 0.0f, (float)m_Scene->mAnimations[0]->mDuration);
					ImGui::DragFloat("Time Scale", &m_TimeMultiplier, 0.05f, 0.0f, 100.0f);
				}
				ImGui::Unindent(10.0f);
			}
		}
		ImGui::End();
	}

}
