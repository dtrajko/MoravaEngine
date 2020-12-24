#include "SceneHierarchyPanel.h"

#include "../Renderer/HazelMesh.h"
#include "../Script/ScriptEngine.h"
#include "../ImGui/ImGui.h"

#include "../../Math.h"
#include "../../ImGuiWrapper.h"
#include "../../EntitySelection.h"
#include "../../Application.h"

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
		EntitySelection::s_SelectionContext = {};
		if (EntitySelection::s_SelectionContext.size() && false)
		{
			//	Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMap();
			UUID selectedEntityID = EntitySelection::s_SelectionContext[0].Entity.GetUUID();

			if (entityMap.find(selectedEntityID) != entityMap.end()) {
				EntitySelection::s_SelectionContext.push_back(SelectedSubmesh{ Entity{ entityMap.at(selectedEntityID), m_Context }, nullptr, 0 });
			}
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		EntitySelection::s_SelectionContext.clear();

		if (entity.HasComponent<MeshComponent>())
		{
			// if MeshComponent is available in entity
			auto mesh = entity.GetComponent<MeshComponent>().Mesh;
			for (auto& submesh : mesh->GetSubmeshes())
			{
				EntitySelection::s_SelectionContext.push_back(SelectedSubmesh{ entity, &submesh, 0 });
			}
		}
		else
		{
			// if MeshComponent is not available in entity
			EntitySelection::s_SelectionContext.push_back(SelectedSubmesh{ entity, nullptr, 0 });
		}
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
				EntitySelection::s_SelectionContext = {};
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

			if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity.HasComponent<Hazel::TagComponent>())
			{
				DrawComponents(EntitySelection::s_SelectionContext[0].Entity);
			}

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

		ImGuiTreeNodeFlags flags = ((EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());

		if (ImGui::IsItemClicked())
		{
			// EntitySelection::s_SelectionContext.push_back(SelectedSubmesh{ entity, nullptr, 0 });

			Log::GetLogger()->debug("ImGui::IsItemClicked: entity.Tag '{0}'", entity.GetComponent<Hazel::TagComponent>().Tag);

			SetSelected(entity);
			m_Context->OnEntitySelected(entity);
		}

		bool entityDeleted = false;
		bool entityCloned = false;

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
			{
				entityDeleted = true;
			}

			if (ImGui::MenuItem("Clone Entity"))
			{
				entityCloned = true;
			}

			ImGui::EndPopup();
		}

		if (opened) {

			DrawEntitySubmeshes(entity);

			ImGui::TreePop();
		}

		if (entityDeleted) {
			m_Context->DestroyEntity(entity);
			if (EntitySelection::s_SelectionContext[0].Entity == entity) {
				EntitySelection::s_SelectionContext = {};
			}

			m_EntityDeletedCallback(entity);
		}

		if (entityCloned) {
			m_Context->CloneEntity(entity);
			if (EntitySelection::s_SelectionContext[0].Entity == entity) {
				EntitySelection::s_SelectionContext = {};
			}
		}
	}

	void SceneHierarchyPanel::DrawEntitySubmeshes(Entity entity)
	{
		if (entity.HasComponent<Hazel::MeshComponent>())
		{
			auto mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;

			for (auto& submesh : mesh->GetSubmeshes())
			{
				bool selected = false;
				for (auto selection : EntitySelection::s_SelectionContext)
				{
					if (selection.Mesh && selection.Mesh->NodeName == submesh.NodeName)
					{
						selected = true;
						break;
					}
				}

				ImGuiTreeNodeFlags flags = (selected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
				bool opened = ImGui::TreeNodeEx((void*)(uint64_t)((uint32_t)entity + 1000 + submesh.BaseIndex + 1), flags, submesh.MeshName.c_str());

				if (ImGui::IsItemClicked())
				{
					EntitySelection::s_SelectionContext.clear();
					EntitySelection::s_SelectionContext.push_back(SelectedSubmesh{ entity, &submesh, 0 });
				}

				if (opened) {
					ImGui::Text("MeshName: ");
					ImGui::SameLine();
					ImGui::Text(submesh.MeshName.c_str());

					ImGui::Text("NodeName: ");
					ImGui::SameLine();
					ImGui::Text(submesh.NodeName.c_str());

					ImGui::Text("MaterialIndex: ");
					ImGui::SameLine();
					ImGui::Text(std::to_string(submesh.MaterialIndex).c_str());

					// ...
					ImGui::TreePop();
				}
			}
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

		auto id = entity.GetComponent<IDComponent>().ID;

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

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
			ImGuiWrapper::DrawVec3Control("Translation", component.Translation, 0.0f, 80.0f);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			ImGuiWrapper::DrawVec3Control("Rotation", rotation, 0.0f, 80.0f);
			component.Rotation = glm::radians(rotation);
			ImGuiWrapper::DrawVec3Control("Scale", component.Scale, 1.0f, 80.0f);
		});

		DrawComponent<MeshComponent>("Mesh", entity, [](MeshComponent& mc)
		{
				ImGui::Columns(3);
				ImGui::SetColumnWidth(0, 100);
				ImGui::SetColumnWidth(1, 300);
				ImGui::SetColumnWidth(2, 40);
				ImGui::Text("File Path");
				ImGui::NextColumn();
				ImGui::PushItemWidth(-1);
				if (mc.Mesh) {
					ImGui::InputText("##meshfilepath", (char*)mc.Mesh->GetFilePath().c_str(), 256, ImGuiInputTextFlags_ReadOnly);
				}
				else {
					ImGui::InputText("##meshfilepath", (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
				}
				ImGui::PopItemWidth();
				ImGui::NextColumn();
				if (ImGui::Button("...##openmesh"))
				{
					std::string file = Application::Get()->OpenFile();
					if (!file.empty()) {
						// mc.Mesh = Ref<HazelMesh>::Create(file); // TODO: HazelMesh constructor
					}
				}
				ImGui::Columns(1);
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

		DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& sc) mutable
		{
			UI::BeginPropertyGrid();
			std::string oldName = sc.ModuleName;

			if (UI::Property("Module Name", sc.ModuleName, ScriptEngine::ModuleExists(sc.ModuleName))) // TODO: no live edit
			{
				// Shutdown old script
				if (ScriptEngine::ModuleExists(oldName)) {
					ScriptEngine::ShutdownScriptEntity(entity, oldName);
				}

				if (ScriptEngine::ModuleExists(sc.ModuleName)) {
					ScriptEngine::InitScriptEntity(entity);
				}
			}
			
			// Public Fields
			if (ScriptEngine::ModuleExists(sc.ModuleName))
			{
				EntityInstanceData& entityInstanceData = ScriptEngine::GetEntityInstanceData(entity.GetSceneUUID(), id);
				auto& moduleFieldMap = entityInstanceData.ModuleFieldMap;
				if (moduleFieldMap.find(sc.ModuleName) != moduleFieldMap.end())
				{
					auto& publicFields = moduleFieldMap.at(sc.ModuleName);
					for (auto& [name, field] : publicFields)
					{
						bool isRuntime = m_Context->m_IsPlaying && field.IsRuntimeAvailable();

						for (auto& field : publicFields)
						{
							switch (field.second.Type)
							{
							case FieldType::Int:
							{
								int value = field.second.GetStoredValue<int>();
								if (ImGuiWrapper::Property(field.second.Name.c_str(), value))
								{
									field.second.SetStoredValue(value);
								}
								break;
							}
							case FieldType::Float:
							{
								float value = field.second.GetStoredValue<float>();
								if (ImGuiWrapper::Property(field.second.Name.c_str(), value, 0.2f))
								{
									field.second.SetStoredValue(value);
								}
								break;
							}
							case FieldType::Vec2:
							{
								glm::vec2 value = field.second.GetStoredValue<glm::vec2>();
								if (ImGuiWrapper::Property(field.second.Name.c_str(), value, 0.2f))
								{
									field.second.SetStoredValue(value);
								}
								break;
							}
							case FieldType::Vec3:
							{
								glm::vec3 value = isRuntime ? field.second.GetRuntimeValue<glm::vec3>() : field.second.GetStoredValue<glm::vec3>();
								if (UI::Property(field.second.Name.c_str(), value, 0.2f))
								{
									if (isRuntime) {
										field.second.SetRuntimeValue(value);
									} else {
										field.second.SetStoredValue(value);
									}
								}
								break;
							}
							case FieldType::Vec4:
							{
								glm::vec4 value = isRuntime ? field.second.GetRuntimeValue<glm::vec4>() : field.second.GetStoredValue<glm::vec4>();
								if (UI::Property(field.second.Name.c_str(), value, 0.2f))
								{
									if (isRuntime) {
										field.second.SetRuntimeValue(value);
									} else {
										field.second.SetStoredValue(value);
									}
								}
								break;
							}
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
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Script")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<ScriptComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Camera")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Sprite Renderer")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
	}
}
