#include "SceneHierarchyPanel.h"

#include "../Renderer/HazelMesh.h"
#include "../Script/ScriptEngine.h"
#include "../ImGui/ImGui.h"

#include "../../Math.h"
#include "../../ImGuiWrapper.h"
#include "../../EntitySelection.h"
#include "../../Application.h"
#include "../../EnvironmentMap.h"

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

	void SceneHierarchyPanel::SetContext(const Ref<HazelScene>& scene)
	{
		m_Context = scene;
		EntitySelection::s_SelectionContext = {};
		if (EntitySelection::s_SelectionContext.size() && false)
		{
			//	Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMap();
			UUID selectedEntityID = EntitySelection::s_SelectionContext[0].Entity.GetUUID();

			if (entityMap.find(selectedEntityID) != entityMap.end()) {
				EntitySelection::s_SelectionContext.push_back(SelectedSubmesh({ entityMap.at(selectedEntityID), nullptr, 0 }));
			}
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		EntitySelection::s_SelectionContext.clear();

		if (entity.HasComponent<MeshComponent>())
		{
			// if MeshComponent is available in entity
			auto& meshComponent = entity.GetComponent<MeshComponent>();
			if (meshComponent.Mesh)
			{
				for (auto& submesh : meshComponent.Mesh->GetSubmeshes())
				{
					EntitySelection::s_SelectionContext.push_back(SelectedSubmesh{ entity, &submesh, 0 });
				}
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
				Entity e(entity, m_Context.Raw());
				if (e.HasComponent<IDComponent>()) {
					DrawEntityNode(e);
				}
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
		const char* name = "Unnamed Entity";
		if (entity.HasComponent<TagComponent>()) {
			name = entity.GetComponent<TagComponent>().Tag.c_str();
		}
		// ImGui::Text("%s", tag.c_str());

		ImGuiTreeNodeFlags flags = ((EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, name);

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

			char buffer[256];
			memset(buffer, 0, 256);
			memcpy(buffer, tag.c_str(), tag.length());
			ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);
			if (ImGui::InputText("##Tag", buffer, 256))
			{
				tag = std::string(buffer);
			}
			ImGui::PopItemWidth();

			ImGui::Separator();
		}

		// ID
		ImGui::SameLine();
		ImGui::TextDisabled("%llx", id);
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 textSize = ImGui::CalcTextSize("Add Component");
		ImGui::SameLine(contentRegionAvailable.x - (textSize.x + GImGui->Style.FramePadding.y));

		if (ImGui::Button("Add Component")) {
			ImGui::OpenPopup("AddComponentPanel");
		}

		if (ImGui::BeginPopup("AddComponentPanel"))
		{
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<CameraComponent>())
			{
				if (ImGui::Button("Camera"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<MeshComponent>())
			{
				if (ImGui::Button("Mesh"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<DirectionalLightComponent>())
			{
				if (ImGui::Button("Directional Light"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<DirectionalLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<SkyLightComponent>())
			{
				if (ImGui::Button("Sky Light"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SkyLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<ScriptComponent>())
			{
				if (ImGui::Button("Script"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<ScriptComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<SpriteRendererComponent>())
			{
				if (ImGui::Button("Sprite Renderer"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<RigidBody2DComponent>())
			{
				if (ImGui::Button("Rigidbody 2D"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<RigidBody2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<BoxCollider2DComponent>())
			{
				if (ImGui::Button("Box Collider 2D"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<BoxCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<CircleCollider2DComponent>())
			{
				if (ImGui::Button("Circle Collider 2D"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<CircleCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<MaterialComponent>())
			{
				if (ImGui::Button("Material"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<MaterialComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
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
			ImGui::SetColumnWidth(0, 70.0f);
			ImGui::SetColumnWidth(1, 180.0f);
			ImGui::SetColumnWidth(2, 30.0f);
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
					// mc.Mesh = Ref<HazelMesh>::Create(file);
					mc.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create(file, nullptr, nullptr, false);
					EnvironmentMap::LoadEnvMapMaterials(mc.Mesh);
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
				float verticalFOV = camera.GetPerspectiveVerticalFOV();
				if (ImGui::DragFloat("Vertical FOV", &verticalFOV))
					camera.SetPerspectiveVerticalFOV(verticalFOV);

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

		DrawComponent<DirectionalLightComponent>("Directional Light", entity, [](DirectionalLightComponent& dlc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyColor("Radiance", dlc.Radiance);
			UI::Property("Intensity", dlc.Intensity);
			UI::Property("Cast Shadows", dlc.CastShadows);
			UI::Property("Soft Shadows", dlc.SoftShadows);
			UI::Property("Source Size", dlc.LightSize);
			UI::EndPropertyGrid();
		});

		DrawComponent<SkyLightComponent>("Sky Light", entity, [](SkyLightComponent& slc)
		{
			ImGui::Columns(3);
			ImGui::SetColumnWidth(0, 70.0f);
			ImGui::SetColumnWidth(1, 180.0f);
			ImGui::SetColumnWidth(2, 30.0f);
			ImGui::Text("File Path");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (!slc.SceneEnvironment.FilePath.empty())
				ImGui::InputText("##envfilepath", (char*)slc.SceneEnvironment.FilePath.c_str(), 256, ImGuiInputTextFlags_ReadOnly);
			else
				ImGui::InputText("##envfilepath", (char*)"Empty", 256, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopItemWidth();
			ImGui::NextColumn();
			if (ImGui::Button("...##openenv"))
			{
				std::string file = Application::Get()->OpenFile("*.hdr");
				if (!file.empty())
					slc.SceneEnvironment = Environment::Load(file);
			}
			ImGui::Columns(1);

			UI::BeginPropertyGrid();
			UI::Property("Intensity", slc.Intensity, 0.01f, 0.0f, 5.0f);
			UI::EndPropertyGrid();
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

		DrawComponent<RigidBody2DComponent>("Rigidbody 2D", entity, [](RigidBody2DComponent& rb2dc)
		{
			// Rigidbody2D Type
			const char* rb2dTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			const char* currentType = rb2dTypeStrings[(int)rb2dc.BodyType];
			if (ImGui::BeginCombo("Type", currentType))
			{
				for (int type = 0; type < 3; type++)
				{
					bool is_selected = (currentType == rb2dTypeStrings[type]);
					if (ImGui::Selectable(rb2dTypeStrings[type], is_selected))
					{
						currentType = rb2dTypeStrings[type];
						rb2dc.BodyType = (RigidBody2DComponent::Type)type;
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (rb2dc.BodyType == RigidBody2DComponent::Type::Dynamic)
			{
				UI::BeginPropertyGrid();
				UI::Property("Fixed Rotation", rb2dc.FixedRotation);
				UI::EndPropertyGrid();
			}
		});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& bc2dc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Offset", bc2dc.Offset);
			UI::Property("Size", bc2dc.Size);
			UI::Property("Density", bc2dc.Density);
			UI::Property("Friction", bc2dc.Friction);

			UI::EndPropertyGrid();
		});

		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& cc2dc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Offset", cc2dc.Offset);
			UI::Property("Radius", cc2dc.Radius);
			UI::Property("Density", cc2dc.Density);
			UI::Property("Friction", cc2dc.Friction);

			UI::EndPropertyGrid();
		});

		DrawComponent <MaterialComponent > ("Material", entity, [](MaterialComponent& mc)
			{
				UI::BeginPropertyGrid();

				UI::Property("Name", mc.Name);

				// ImGuiWrapper::DrawMaterialUI(nullptr, mc.Name, m_CheckerboardTexture);

				UI::EndPropertyGrid();
			});

		/****
		{
			ImGui::Separator();

			if (ImGui::Button("Add Component")) {
				ImGui::OpenPopup("AddComponent");
			}

			if (ImGui::BeginPopup("AddComponent"))
			{
				if (ImGui::MenuItem("Camera")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Mesh")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Script")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<ScriptComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Sprite Renderer")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Rigidbody 2D")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<RigidBody2DComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Box Collider 2D")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<BoxCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Circle Collider 2D")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<CircleCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Directional Light")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<DirectionalLightComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Sky Light")) {
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SkyLightComponent>();
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
		****/
	}
}
