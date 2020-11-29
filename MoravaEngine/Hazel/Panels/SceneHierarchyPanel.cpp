#include "SceneHierarchyPanel.h"

#include "../Renderer/HazelMesh.h"
#include "../Script/ScriptEngine.h"

#include "../../Math.h"
#include "../../ImGuiWrapper.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

#include <vector>

// TODO:
// - Eventually change imgui node IDs to be entity/asset GUID

namespace Hazel
{
	SceneHierarchyPanel::SceneHierarchyPanel(HazelScene* scene)
	{
		SetContext(scene);
	}

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{
	}

	void SceneHierarchyPanel::SetContext(HazelScene* scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
		if (m_SelectionContext && false)
		{
			//	Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMap();
			UUID selectedEntityID = m_SelectionContext.GetUUID();

			if (entityMap.find(selectedEntityID) != entityMap.end()) {
				m_SelectionContext = entityMap.at(selectedEntityID);
			}
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");

		if (m_Context)
		{
			uint32_t entityCount = 0;
			uint32_t meshCount = 0;

			m_Context->m_Registry.each([&](auto entity)
				{
					DrawEntityNode(Entity(entity, m_Context));
				});

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			{
				m_SelectionContext = {};
				m_CurrentlySelectedTransform = glm::mat4(1.0f);
			}

			// Right-click on blank space
			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
				{
					m_Context->CreateEntity("Empty Entity");
				}
				ImGui::EndPopup();
			}

			ImGui::End();

			ImGui::Begin("Properties");

			//	if (m_SelectionContext.HasComponent<Hazel::TagComponent>())
			//	{
			//		DrawComponents(m_SelectionContext);
			//	}

			ImGui::End();

#if TODO
			ImGui::Begin("Mesh Debug");
			if (ImGui::CollapsingHeader(mesh->m_FilePath.c_str()))
			{
				if (mesh->m_IsAnimated)
				{
					if (ImGui::CollapsingHeader("Animation"))
					{
						if (ImGui::Button(mesh->m_AnimationPlaying ? "Pause" : "Play"))
							mesh->m_AnimationPlaying = !mesh->m_AnimationPlaying;

						ImGui::SliderFloat("##AnimationTime", &mesh->m_AnimationTime, 0.0f, (float)mesh->m_Scene->mAnimations[0]->mDuration);
						ImGui::DragFloat("Time Scale", &mesh->m_TimeMultiplier, 0.05f, 0.0f, 10.0f);
					}
				}
			}
			ImGui::End();
#endif
		}
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		// ImGui::Text("%s", tag.c_str());

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
			m_CurrentlySelectedTransform = entity.GetComponent<TransformComponent>().GetTransform();

			m_Context->OnEntitySelected(entity);
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
			{
				entityDeleted = true;
			}
			ImGui::EndPopup();
		}

		if (opened)
		{
			ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_SpanAvailWidth;
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)((uint32_t)entity + 1000), flags, tag.c_str());
			if (opened) {
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		if (entityDeleted) {
			m_Context->DestroyEntity(entity);
			//	if (m_SelectionContext == entity) {
			//		m_SelectionContext = {};
			//		m_CurrentlySelectedTransform = glm::mat4(1.0f);
			//	}
		}
	}

	void SceneHierarchyPanel::DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID)
	{
		static char imguiName[128];
		memset(imguiName, 0, 128);
		sprintf(imguiName, "Mesh##%d", imguiMeshID++);

		// Mesh Hierarchy
		if(ImGui::TreeNode(imguiName))
		{
			auto rootNode = mesh->GetSceneAssimp()->mRootNode;
			MeshNodeHierarchy(mesh, rootNode, glm::mat4(1.0f), 0);
			ImGui::TreePop();
		}
	}

	// NoECS version
	void SceneHierarchyPanel::MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 localTransform = Math::Mat4FromAssimpMat4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t meshIndex = node->mMeshes[i];
			((HazelMesh*)mesh)->GetSubmeshes()[meshIndex].Transform = transform;
		}

		if (ImGui::TreeNode(node->mName.C_Str()))
		{
			{
				auto [translation, rotation, scale] = Math::GetTransformDecomposition(transform);
				ImGui::Text("World Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}
			{
				auto [translation, rotation, scale] = Math::GetTransformDecomposition(localTransform);
				ImGui::Text("Local Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}

			for (uint32_t i = 0; i < node->mNumChildren; i++) {
				MeshNodeHierarchy(mesh, node->mChildren[i], transform, level + 1);
			}

			ImGui::TreePop();
		}
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string name, Entity entity, UIFunction uiFunction)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGuiTreeNodeFlags treeNodeFlags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_AllowItemOverlap |
			ImGuiTreeNodeFlags_FramePadding;

		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4.0f, 4.0f });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar();
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);

			ImGui::PushFont(boldFont);
			if (ImGui::Button("...", ImVec2{ lineHeight, lineHeight })) {
				ImGui::OpenPopup("ComponentSettings");
			}
			ImGui::PopFont();

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove Component")) {
					removeComponent = true;
				}

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent) {
				entity.RemoveComponent<T>();
			}
		}
	}

	static void EndPropertyGrid()
	{
		ImGui::Columns(1);
		ImGuiWrapper::PopID();
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		ImGui::AlignTextToFramePadding();

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			// Camera0
			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, sizeof(buffer), tag.c_str());
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}

			ImGui::Separator();
		}

		DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
		{
			ImGuiWrapper::DrawVec3Control("Translation", component.Translation, 0.0f, 100.0f);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			ImGuiWrapper::DrawVec3Control("Rotation", rotation, 0.0f, 100.0f);
			component.Rotation = glm::radians(rotation);
			ImGuiWrapper::DrawVec3Control("Scale", component.Scale, 1.0f, 100.0f);
		});

		DrawComponent<MeshComponent>("Mesh", entity, [](auto& component)
		{
			if (component.Mesh) {
				ImGui::InputText("File Path", (char*)component.Mesh->GetFilePath().c_str(), 256, ImGuiInputTextFlags_ReadOnly);
			}
			else {
				ImGui::InputText("File Path", (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
			}
		});

		DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
		{
			auto& camera = component.Camera;

			ImGui::Checkbox("Primary", &component.Primary);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];

			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjectionType((SceneCamera::ProjectionType)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFOV = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if (ImGui::DragFloat("Vertical FOV", &verticalFOV))
					camera.SetPerspectiveVerticalFOV(glm::radians(verticalFOV));

				float nearClip = camera.GetPerspectiveNearClip();
				if (ImGui::DragFloat("Near", &nearClip))
					camera.SetPerspectiveNearClip(nearClip);

				float farClip = camera.GetPerspectiveFarClip();
				if (ImGui::DragFloat("Far", &farClip))
					camera.SetPerspectiveFarClip(farClip);
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (ImGui::DragFloat("Size", &orthoSize))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if (ImGui::DragFloat("Near", &orthoNear))
					camera.SetOrthographicNearClip(orthoNear);

				float orthoFar = camera.GetOrthographicFarClip();
				if (ImGui::DragFloat("Far", &orthoFar))
					camera.SetOrthographicFarClip(orthoFar);

				ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			}
		});

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
		});

		DrawComponent<ScriptComponent>("Script", entity, [=](auto& component)
		{
			ImGuiWrapper::BeginPropertyGrid();
			ImGuiWrapper::Property("Module Name", component.ModuleName.c_str());

			// Public Fields
			auto& fieldMap = ScriptEngine::GetFieldMap();
			if (fieldMap.find(component.ModuleName) != fieldMap.end())
			{
				auto& publicFields = fieldMap.at(component.ModuleName);
				for (auto& field : publicFields)
				{
					switch (field.Type)
					{
					case FieldType::Int:
					{
						int value = field.GetValue<int>();
						if (ImGuiWrapper::Property(field.Name.c_str(), value))
						{
							field.SetValue(value);
						}
						break;
					}
					case FieldType::Float:
					{
						float value = field.GetValue<float>();
						if (ImGuiWrapper::Property(field.Name.c_str(), value, 0.2f))
						{
							field.SetValue(value);
						}
					}
					case FieldType::Vec2:
					{
						glm::vec2 value = field.GetValue<glm::vec2>();
						if (ImGuiWrapper::Property(field.Name.c_str(), value, 0.2f))
						{
							field.SetValue(value);
						}
					}
					}
				}
			}
			EndPropertyGrid();

			if (ImGui::Button("Run Script"))
			{
				ScriptEngine::OnCreateEntity(entity);
			}
		});

		{
			if (ImGui::Button("Add Component")) {
				ImGui::OpenPopup("AddComponent");
			}

			if (ImGui::BeginPopup("AddComponent"))
			{
				if (ImGui::MenuItem("Mesh")) {
					m_SelectionContext.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Script")) {
					m_SelectionContext.AddComponent<ScriptComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Camera")) {
					m_SelectionContext.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Sprite Renderer")) {
					m_SelectionContext.AddComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
	}
}
