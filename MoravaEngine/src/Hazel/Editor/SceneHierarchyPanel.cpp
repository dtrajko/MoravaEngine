#include "Hazel/Editor/SceneHierarchyPanel.h"

// Hazel
#include "Hazel/Script/ScriptEngine.h"

// Morava
#include "Editor/EntitySelection.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Material/MaterialLibrary.h"


// TODO:
// - Eventually change imgui node IDs to be entity/asset GUID

namespace Hazel
{
	SceneHierarchyPanel::SceneHierarchyPanel(Ref<HazelScene> scene)
	{
		SetContext(scene);
	}

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{
	}

	void SceneHierarchyPanel::SetContext(Ref<HazelScene> scene)
	{
		m_Context = scene;
		EntitySelection::s_SelectionContext = {};
		if (EntitySelection::s_SelectionContext.size() && false)
		{
			//	Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMap();
			UUID selectedEntityID = EntitySelection::s_SelectionContext[0].Entity.GetUUID();

			if (entityMap.find(selectedEntityID) != entityMap.end()) {
				entt::entity entityID = entityMap.at(selectedEntityID);
				EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh({ Entity(entityID, scene.Raw()) , new Hazel::Submesh(), 0 }));
			}
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		if (entity.HasComponent<MeshComponent>())
		{
			// if MeshComponent is available in entity
			auto& meshComponent = entity.GetComponent<MeshComponent>();
			if (meshComponent.Mesh)
			{
				if (EnvMapEditorLayer::s_SelectionMode == SelectionMode::Entity)
				{
					EntitySelection::s_SelectionContext.clear();
					for (auto& submesh : meshComponent.Mesh->GetSubmeshes())
					{
						EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh{ entity, &submesh, 0 });
					}
				}
				else if (EnvMapEditorLayer::s_SelectionMode == SelectionMode::SubMesh) {
					// Do nothing...
				}
				else {
					// Do nothing...
				}
			}
		}
		else
		{
			// if MeshComponent is not available in entity
			EntitySelection::s_SelectionContext.clear();
			EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh{ entity, new Hazel::Submesh(), 0 });
		}
	}

	void SceneHierarchyPanel::OnImGuiRender(bool* p_open)
	{
		ImGui::Begin("Scene Hierarchy", p_open);

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
			// EnvironmentMap::AddSubmeshToSelectionContext(SelectedSubmesh{ entity, nullptr, 0 });

			Log::GetLogger()->debug("ImGui::IsItemClicked: entity.Tag '{0}'", entity.GetComponent<Hazel::TagComponent>().Tag);

			SetSelected(entity);
			m_Context->OnEntitySelected(entity);

			EnvMapEditorLayer::s_SelectionMode = SelectionMode::Entity;
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
			if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity == entity) {
				EntitySelection::s_SelectionContext = {};
			}
			m_EntityDeletedCallback(entity);
		}

		if (entityCloned) {
			m_Context->CloneEntity(entity);
			if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity == entity) {
				EntitySelection::s_SelectionContext = {};
			}
		}
	}

	void SceneHierarchyPanel::DrawEntitySubmeshes(Entity entity)
	{
		if (!entity.HasComponent<Hazel::MeshComponent>()) return;
		if (!entity.GetComponent<Hazel::MeshComponent>().Mesh) return;

		auto mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;

		std::vector<Hazel::Submesh>& submeshes = mesh->GetSubmeshes();

		for (int i = 0; i < submeshes.size(); i++)
		{
			bool submeshSelected = false;
			for (auto selection : EntitySelection::s_SelectionContext)
			{
				if (selection.Mesh && selection.Mesh->MeshName == submeshes[i].MeshName)
				{
					submeshSelected = true;
					break;
				}
			}

			ImGuiTreeNodeFlags flags = (submeshSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)((uint32_t)entity + 1000 + submeshes[i].BaseIndex + i), flags, submeshes[i].MeshName.c_str());

			if (ImGui::IsItemClicked())
			{
				EntitySelection::s_SelectionContext.clear();
				EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh{ entity, &submeshes[i], 0 });

				EnvMapEditorLayer::s_SelectionMode = SelectionMode::SubMesh;
			}

			bool submeshDeleted = false;
			bool submeshCloned = false;

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete Submesh"))
				{
					submeshDeleted = true;
				}

				if (ImGui::MenuItem("Clone Submesh"))
				{
					submeshCloned = true;
				}

				ImGui::EndPopup();
			}

			if (opened) {
				ImGui::Text("MeshName: ");
				ImGui::SameLine();
				ImGui::Text(submeshes[i].MeshName.c_str());

				ImGui::Text("NodeName: ");
				ImGui::SameLine();
				ImGui::Text(submeshes[i].NodeName.c_str());

				ImGui::Text("MaterialIndex: ");
				ImGui::SameLine();
				ImGui::Text(std::to_string(submeshes[i].MaterialIndex).c_str());

				SubmeshUUID submeshUUID = MaterialLibrary::GetSubmeshUUID(&entity, &submeshes[i]);
				std::string materialUUID = "N/A";
				std::string materialName = "N/A";
				auto map_it = MaterialLibrary::s_SubmeshMaterialUUIDs.find(submeshUUID);
				if (MaterialLibrary::s_SubmeshMaterialUUIDs.find(submeshUUID) != MaterialLibrary::s_SubmeshMaterialUUIDs.end()) {
					materialUUID = map_it->second;
					materialName = map_it->first;
				}

				ImGui::Text("MaterialUUID: ");
				ImGui::SameLine();
				ImGui::Text(materialUUID.c_str());

				ImGui::Text("Material Name: ");
				ImGui::SameLine();
				ImGui::Text(materialName.c_str());

				// ...
				ImGui::TreePop();
			}

			if (submeshDeleted && submeshSelected) {
				Log::GetLogger()->debug("SceneHierarchyPanel DeleteSubmesh('{0}')", submeshes[i].MeshName);
				mesh->DeleteSubmesh(submeshes[i]);
			}

			if (submeshCloned && submeshSelected) {
				mesh->CloneSubmesh(submeshes[i]);
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
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 3.0f);

			ImGui::PushFont(boldFont);
			if (ImGui::Button("...", ImVec2{ lineHeight * 4.0f, lineHeight })) {
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
				// EntitySelection::s_SelectionContext = {};
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
					//	if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<MaterialComponent>()) {
					//		EntitySelection::s_SelectionContext[0].Entity.AddComponent<MaterialComponent>();
					//	}
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

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<PointLightComponent>())
			{
				if (ImGui::Button("Point Light"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<PointLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<SpotLightComponent>())
			{
				if (ImGui::Button("Spot Light"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SpotLightComponent>();
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

		DrawComponent<MeshComponent>("Mesh", entity, [=](MeshComponent& mc)
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
					mc.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create(file, Hazel::Ref<MoravaShader>(), Hazel::Ref<Hazel::HazelMaterial>(), false);

					auto materialDataVector = MaterialLibrary::s_MaterialData;
					for (auto materialData : materialDataVector) {
						Log::GetLogger()->debug("* * * * * SceneHierarchyPanel Material name: '{0}'", materialData->Name);
					}

					// MaterialLibrary::SetDefaultMaterialToSubmeshes(mc.Mesh, entity, EnvMapEditorLayer::s_DefaultMaterial);
					MaterialLibrary::SetMaterialsToSubmeshes(mc.Mesh, entity, EnvMapEditorLayer::s_DefaultMaterial);
				}
			}
			ImGui::Columns(1);
		});

		DrawComponent<CameraComponent>("Camera", entity, [=](CameraComponent& cc)
		{
			// Projection Type
			const char* projTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProj = projTypeStrings[(int)cc.Camera.GetProjectionType()];
			if (ImGui::BeginCombo("Projection", currentProj))
			{
				for (int type = 0; type < 2; type++)
				{
					bool is_selected = (currentProj == projTypeStrings[type]);
					if (ImGui::Selectable(projTypeStrings[type], is_selected))
					{
						currentProj = projTypeStrings[type];
						cc.Camera.SetProjectionType((SceneCamera::ProjectionType)type);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGuiWrapper::BeginPropertyGrid();
			// Perspective parameters
			if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFOV = cc.Camera.GetPerspectiveVerticalFOV();
				if (ImGuiWrapper::Property("Vertical FOV", verticalFOV)) {
					cc.Camera.SetPerspectiveVerticalFOV(verticalFOV);
				}

				float nearClip = cc.Camera.GetPerspectiveNearClip();
				if (ImGuiWrapper::Property("Near Clip", nearClip)) {
					cc.Camera.SetPerspectiveNearClip(nearClip);
				}

				float farClip = cc.Camera.GetPerspectiveFarClip();
				if (ImGuiWrapper::Property("Far Clip", farClip)) {
					cc.Camera.SetPerspectiveFarClip(farClip);
				}

				float exposure = cc.Camera.GetExposure();
				if (ImGuiWrapper::Property("Exposure", exposure, 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty)) {
					cc.Camera.SetExposure(exposure);
				}
			}

			// Orthographic parameters
			else if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = cc.Camera.GetOrthographicSize();
				if (ImGuiWrapper::Property("Size", orthoSize)) {
					cc.Camera.SetOrthographicSize(orthoSize);
				}

				float nearClip = cc.Camera.GetOrthographicNearClip();
				if (ImGuiWrapper::Property("Near Clip", nearClip)) {
					cc.Camera.SetOrthographicNearClip(nearClip);
				}

				float farClip = cc.Camera.GetOrthographicFarClip();
				if (ImGuiWrapper::Property("Far Clip", farClip)) {
					cc.Camera.SetOrthographicFarClip(farClip);
				}

				float exposure = cc.Camera.GetExposure();
				if (ImGuiWrapper::Property("Exposure", exposure, 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty)) {
					cc.Camera.SetExposure(exposure);
				}
			}

			EndPropertyGrid();
		});

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
		});

		DrawComponent<DirectionalLightComponent>("Directional Light", entity, [](DirectionalLightComponent& dlc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyColor("Radiance", dlc.Radiance);
			UI::Property("Intensity",     dlc.Intensity);
			UI::Property("Cast Shadows",  dlc.CastShadows);
			UI::Property("Soft Shadows",  dlc.SoftShadows);
			UI::Property("Source Size",   dlc.LightSize);
			UI::EndPropertyGrid();
		});

		DrawComponent<PointLightComponent>("Point Light", entity, [](PointLightComponent& plc)
		{
			UI::BeginPropertyGrid();
			UI::Property("Enabled",          plc.Enabled);
			UI::PropertyColor("Color",       plc.Color);
			// UI::Property("Position",         plc.Position);
			UI::Property("AmbientIntensity", plc.AmbientIntensity);
			UI::Property("DiffuseIntensity", plc.DiffuseIntensity);
			UI::Property("Constant",         plc.Constant);
			UI::Property("Linear",           plc.Linear);
			UI::Property("Exponent",         plc.Exponent);
			UI::Property("FarPlane",         plc.FarPlane);
			UI::EndPropertyGrid();
		});

		DrawComponent<SpotLightComponent>("Spot Light", entity, [](SpotLightComponent& slc)
		{
			UI::BeginPropertyGrid();
			UI::Property("Enabled",          slc.Enabled);
			UI::PropertyColor("Color",       slc.Color);
			// UI::Property("Position",         slc.Position);
			// UI::Property("Direction",        slc.Direction);
			UI::Property("AmbientIntensity", slc.AmbientIntensity);
			UI::Property("DiffuseIntensity", slc.DiffuseIntensity);
			UI::Property("Constant",         slc.Constant);
			UI::Property("Linear",           slc.Linear);
			UI::Property("Exponent",         slc.Exponent);
			UI::Property("Edge",             slc.Edge);
			UI::Property("EdgeProcessed",    slc.EdgeProcessed);
			UI::Property("FarPlane",         slc.FarPlane);
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

		DrawComponent <MaterialComponent > ("Material", entity, [=](MaterialComponent& mc)
			{
				if (!mc.Material) {
					mc.Material = MaterialLibrary::AddNewMaterial("")->EnvMapMaterialRef;
					// std::string materialName = EnvMapEditorLayer::NewMaterialName();
					// mc.Material = EnvMapEditorLayer::CreateDefaultMaterial(materialName);
					MaterialLibrary::AddMaterialFromComponent(entity);
				}

				ImGuiWrapper::DrawMaterialUI(mc.Material, EnvMapEditorLayer::s_CheckerboardTexture);
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
