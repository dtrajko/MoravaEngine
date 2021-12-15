/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "SceneHierarchyPanelH2M.h"
#include "H2M/ImGui/ImGuiH2M.h"

// Morava
#include "Editor/EntitySelection.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Material/MaterialLibrary.h"


// TODO:
// - Eventually change imgui node IDs to be entity/asset GUID

namespace H2M
{
	SceneHierarchyPanelH2M::SceneHierarchyPanelH2M(RefH2M<SceneH2M> scene)
	{
		SetContext(scene);
	}

	SceneHierarchyPanelH2M::~SceneHierarchyPanelH2M()
	{
	}

	void SceneHierarchyPanelH2M::SetContext(RefH2M<SceneH2M> scene)
	{
		m_Context = scene;
		EntitySelection::s_SelectionContext = {};
		if (EntitySelection::s_SelectionContext.size() && false)
		{
			//	Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMapH2M();
			UUID_H2M selectedEntityID = EntitySelection::s_SelectionContext[0].Entity.GetUUID();

			if (entityMap.find(selectedEntityID) != entityMap.end()) {
				entt::entity entityID = entityMap.at(selectedEntityID);
				EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh({ EntityH2M(entityID, scene.Raw()), new SubmeshH2M(), 0 }));
			}
		}
	}

	void SceneHierarchyPanelH2M::SetSelected(EntityH2M entity)
	{
		if (entity.HasComponent<MeshComponentH2M>())
		{
			// if MeshComponent is available in entity
			auto& meshComponent = entity.GetComponent<MeshComponentH2M>();
			if (meshComponent.Mesh)
			{
				if (EnvMapEditorLayer::s_SelectionMode == SelectionMode::Entity)
				{
					EntitySelection::s_SelectionContext.clear();
					for (RefH2M<SubmeshH2M> submesh : meshComponent.Mesh->GetSubmeshes())
					{
						EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh{ entity, submesh, 0 });
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
			EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh{ entity, new SubmeshH2M(), 0 });
		}
	}

	void SceneHierarchyPanelH2M::OnImGuiRender(bool* p_open)
	{
		if (m_Context)
		{
			ImGui::Begin("Scene Hierarchy", p_open);
			{
				uint32_t entityCount = 0;
				uint32_t meshCount = 0;

				m_Context->m_Registry.each([&](auto entity)
					{
						EntityH2M e(entity, m_Context.Raw());
						if (e.HasComponent<IDComponentH2M>()) {
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
					if (ImGui::BeginMenu("Create"))
					{
						if (ImGui::MenuItem("Empty Entity"))
						{
							EntityH2M newEntity = m_Context->CreateEntity("Empty Entity");
							// SetSelected(newEntity);
						}

						if (ImGui::MenuItem("Mesh"))
						{
							EntityH2M newEntity = m_Context->CreateEntity("Mesh");
							SetSelected(newEntity);
							newEntity.AddComponent<MeshComponentH2M>();
							// EntitySelection::s_SelectionContext[0].Entity.AddComponent<MeshComponent>();
						}

						ImGui::Separator();

						if (ImGui::MenuItem("Directional Light"))
						{
							EntityH2M newEntity = m_Context->CreateEntity("Directional Light");
							// newEntity.AddComponent<DirectionalLightComponent>();
							// EntitySelection::s_SelectionContext[0].Entity.AddComponent<DirectionalLightComponent>();
							// SetSelected(newEntity);
						}

						if (ImGui::MenuItem("Sky Light"))
						{
							EntityH2M newEntity = m_Context->CreateEntity("Sky Light");
							// newEntity.AddComponent<SkyLightComponent>();
							// EntitySelection::s_SelectionContext[0].Entity.AddComponent<SkyLightComponent>();
							// SetSelected(newEntity);
						}

						ImGui::EndMenu();
					}
					ImGui::EndPopup();
				}
			}
			ImGui::End();

			ImGui::Begin("Properties");
			{
				if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity.HasComponent<TagComponentH2M>())
				{
					DrawComponents(EntitySelection::s_SelectionContext[0].Entity);
				}
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
		else
		{
			Log::GetLogger()->warn("SceneHierarchyPanel::OnImGuiRender: Context Undefined!");
		}
	}

	void SceneHierarchyPanelH2M::DrawEntityNode(EntityH2M entity)
	{
		const char* name = "Unnamed Entity";
		if (entity.HasComponent<TagComponentH2M>()) {
			name = entity.GetComponent<TagComponentH2M>().Tag.c_str();
		}
		// ImGui::Text("%s", tag.c_str());

		ImGuiTreeNodeFlags flags = ((EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, name);

		if (ImGui::IsItemClicked())
		{
			// EnvironmentMap::AddSubmeshToSelectionContext(SelectedSubmesh{ entity, nullptr, 0 });

			Log::GetLogger()->debug("ImGui::IsItemClicked: entity.Tag '{0}'", entity.GetComponent<TagComponentH2M>().Tag);

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

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity == entity)
			{
				EntitySelection::s_SelectionContext = {};
			}

			if (m_EntityDeletedCallback)
			{
				m_EntityDeletedCallback(entity);
			}
			else
			{
				Log::GetLogger()->warn("EntityDeletedCallback undefined!");
			}
		}

		if (entityCloned) {
			m_Context->CloneEntity(entity);
			if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Entity == entity) {
				EntitySelection::s_SelectionContext = {};
			}
		}
	}

	void SceneHierarchyPanelH2M::DrawEntitySubmeshes(EntityH2M entity)
	{
		if (!entity.HasComponent<MeshComponentH2M>()) return;
		if (!entity.GetComponent<MeshComponentH2M>().Mesh) return;

		auto mesh = entity.GetComponent<MeshComponentH2M>().Mesh;

		std::vector<RefH2M<SubmeshH2M>>& submeshes = mesh->GetSubmeshes();

		for (int i = 0; i < submeshes.size(); i++)
		{
			bool submeshSelected = false;
			for (auto selection : EntitySelection::s_SelectionContext)
			{
				if (selection.Mesh && selection.Mesh->MeshName == submeshes[i]->MeshName)
				{
					submeshSelected = true;
					break;
				}
			}

			ImGuiTreeNodeFlags flags = (submeshSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)((uint32_t)entity + 1000 + submeshes[i]->BaseIndex + i), flags, submeshes[i]->MeshName.c_str());

			if (ImGui::IsItemClicked())
			{
				EntitySelection::s_SelectionContext.clear();
				EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh{ entity, submeshes[i], 0 });

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
				ImGui::Text(submeshes[i]->MeshName.c_str());

				ImGui::Text("NodeName: ");
				ImGui::SameLine();
				ImGui::Text(submeshes[i]->NodeName.c_str());

				ImGui::Text("MaterialIndex: ");
				ImGui::SameLine();
				ImGui::Text(std::to_string(submeshes[i]->MaterialIndex).c_str());

				SubmeshUUID submeshUUID = MaterialLibrary::GetSubmeshUUID(entity, submeshes[i]);
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
				Log::GetLogger()->debug("SceneHierarchyPanel DeleteSubmesh('{0}')", submeshes[i]->MeshName);
				mesh->DeleteSubmesh(submeshes[i]);
			}

			if (submeshCloned && submeshSelected) {
				mesh->CloneSubmesh(submeshes[i]);
			}
		}
	}

	void SceneHierarchyPanelH2M::DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID)
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
	void SceneHierarchyPanelH2M::MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 localTransform = Math::Mat4FromAssimpMat4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t meshIndex = node->mMeshes[i];
			((MeshH2M*)mesh)->GetSubmeshes()[meshIndex]->Transform = transform;
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
	static void DrawComponent(const std::string name, EntityH2M entity, UIFunction uiFunction)
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

	void SceneHierarchyPanelH2M::DrawComponents(EntityH2M entity)
	{
		ImGui::AlignTextToFramePadding();

		auto id = entity.GetComponent<IDComponentH2M>().ID;

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		if (entity.HasComponent<TagComponentH2M>())
		{
			auto& tag = entity.GetComponent<TagComponentH2M>().Tag;

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
			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<CameraComponentH2M>())
			{
				if (ImGui::Button("Camera"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<CameraComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<MeshComponentH2M>())
			{
				if (ImGui::Button("Mesh"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<MeshComponentH2M>();
					//	if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<MaterialComponent>()) {
					//		EntitySelection::s_SelectionContext[0].Entity.AddComponent<MaterialComponent>();
					//	}
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<DirectionalLightComponentH2M>())
			{
				if (ImGui::Button("Directional Light"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<DirectionalLightComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<PointLightComponentH2M>())
			{
				if (ImGui::Button("Point Light"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<PointLightComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<SpotLightComponentH2M>())
			{
				if (ImGui::Button("Spot Light"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SpotLightComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<SkyLightComponentH2M>())
			{
				if (ImGui::Button("Sky Light"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SkyLightComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<SpriteRendererComponentH2M>())
			{
				if (ImGui::Button("Sprite Renderer"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<SpriteRendererComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<CircleRendererComponentH2M>())
			{
				if (ImGui::Button("Circle Renderer"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<CircleRendererComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<MaterialComponentH2M>())
			{
				if (ImGui::Button("Material"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<MaterialComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<Rigidbody2DComponentH2M>())
			{
				if (ImGui::Button("Rigidbody 2D"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<Rigidbody2DComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (!EntitySelection::s_SelectionContext[0].Entity.HasComponent<BoxCollider2DComponentH2M>())
			{
				if (ImGui::Button("Box Collider 2D"))
				{
					EntitySelection::s_SelectionContext[0].Entity.AddComponent<BoxCollider2DComponentH2M>();
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}

		DrawComponent<TransformComponentH2M>("Transform", entity, [](auto& component)
		{
			ImGuiWrapper::DrawVec3Control("Translation", component.Translation, 0.0f, 80.0f);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			ImGuiWrapper::DrawVec3Control("Rotation", rotation, 0.0f, 80.0f);
			component.Rotation = glm::radians(rotation);
			ImGuiWrapper::DrawVec3Control("Scale", component.Scale, 1.0f, 80.0f);
		});

		DrawComponent<MeshComponentH2M>("Mesh", entity, [=](MeshComponentH2M& mc)
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

			std::string meshFilepath = "";

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					std::wstring itemPath = std::wstring((const wchar_t*)payload->Data);
					size_t itemSize = payload->DataSize;
					Log::GetLogger()->debug("END DRAG & DROP FILE '{0}', size: {1}", Util::to_str(itemPath.c_str()).c_str(), itemSize);

					meshFilepath = std::string{ itemPath.begin(), itemPath.end() };
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (ImGui::Button("...##openmesh"))
			{
				meshFilepath = Application::Get()->OpenFile();
			}

			if (!meshFilepath.empty())
			{
				mc.Mesh = RefH2M<MeshH2M>::Create(meshFilepath, RefH2M<MoravaShader>(), RefH2M<MaterialH2M>(), false);

				auto materialDataVector = MaterialLibrary::s_MaterialData;
				for (auto materialData : materialDataVector) {
					Log::GetLogger()->debug("* * * * * SceneHierarchyPanel Material name: '{0}'", materialData->Name);
				}

				MaterialLibrary::SetMaterialsToSubmeshes(mc.Mesh, entity, EnvMapEditorLayer::s_DefaultMaterial);
			}

			ImGui::Columns(1);

			ImGui::Separator();

			// UI::Property("Cast Shadows", mc.CastShadows);
			// UI::Property("Receive Shadows", mc.ReceiveShadows);

			ImGui::Checkbox("Cast Shadows", &mc.CastShadows);
			ImGui::Checkbox("Receive Shadows", &mc.ReceiveShadows);
		});

		DrawComponent<CameraComponentH2M>("Camera", entity, [=](CameraComponentH2M& cc)
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
						cc.Camera.SetProjectionType((SceneCameraH2M::ProjectionType)type);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGuiWrapper::BeginPropertyGrid();
			// Perspective parameters
			if (cc.Camera.GetProjectionType() == SceneCameraH2M::ProjectionType::Perspective)
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
			else if (cc.Camera.GetProjectionType() == SceneCameraH2M::ProjectionType::Orthographic)
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

		DrawComponent<SpriteRendererComponentH2M>("Sprite Renderer", entity, [](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
		});

		DrawComponent<CircleRendererComponentH2M>("Circle Renderer", entity, [](auto& component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Thickness", &component.Thickness, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Fade", &component.Fade, 0.01f, 0.0f, 1.0f);
		});

		DrawComponent<DirectionalLightComponentH2M>("Directional Light", entity, [](DirectionalLightComponentH2M& dlc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyColor("Radiance", dlc.Radiance);
			UI::Property("Intensity",     dlc.Intensity);
			UI::Property("Cast Shadows",  dlc.CastShadows);
			UI::Property("Soft Shadows",  dlc.SoftShadows);
			UI::Property("Source Size",   dlc.LightSize);
			UI::EndPropertyGrid();
		});

		DrawComponent<PointLightComponentH2M>("Point Light", entity, [](PointLightComponentH2M& plc)
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

		DrawComponent<SpotLightComponentH2M>("Spot Light", entity, [](SpotLightComponentH2M& slc)
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

		DrawComponent<SkyLightComponentH2M>("Sky Light", entity, [](SkyLightComponentH2M& slc)
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
					slc.SceneEnvironment = EnvironmentH2M::Load(file);
			}
			ImGui::Columns(1);

			UI::BeginPropertyGrid();
			UI::Property("Intensity", slc.Intensity, 0.01f, 0.0f, 5.0f);
			UI::EndPropertyGrid();
		});

		DrawComponent<MaterialComponentH2M>("Material", entity, [=](MaterialComponentH2M& mc)
		{
			if (!mc.Material) {
				mc.Material = MaterialLibrary::AddNewMaterial("")->EnvMapMaterialRef;
				// std::string materialName = EnvMapEditorLayer::NewMaterialName();
				// mc.Material = EnvMapEditorLayer::CreateDefaultMaterial(materialName);
				MaterialLibrary::AddMaterialFromComponent(entity);
			}

			ImGuiWrapper::DrawMaterialUI(mc.Material, EnvMapEditorLayer::s_CheckerboardTexture);
		});

		DrawComponent<Rigidbody2DComponentH2M>("Rigidbody 2D", entity, [](Rigidbody2DComponentH2M& component)
		{
			// Rigidbody2D Type
			const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];
			if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
			{
				for (int i = 0; i < 3; i++)
				{
					bool isSelected = (currentBodyTypeString == bodyTypeStrings[i]);
					if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
					{
						currentBodyTypeString = bodyTypeStrings[i];
						component.Type = (Rigidbody2DComponentH2M::BodyType)i;
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
		});

		DrawComponent<BoxCollider2DComponentH2M>("Box Collider 2D", entity, [](BoxCollider2DComponentH2M& component)
		{
			// BoxCollider2D Type
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
			ImGui::DragFloat2("Size",   glm::value_ptr(component.Size));

			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
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
