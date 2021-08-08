#include "VulkanTestLayer.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanSwapChain.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"

#include "Core/Application.h"
#include "HazelVulkan/ExampleVertex.h"


namespace Hazel {

	Ref<HazelScene> VulkanTestLayer::s_Scene;
	SceneHierarchyPanel* VulkanTestLayer::s_SceneHierarchyPanel;
	ContentBrowserPanel* VulkanTestLayer::s_ContentBrowserPanel;
	MaterialEditorPanel* VulkanTestLayer::s_MaterialEditorPanel;


	VulkanTestLayer::VulkanTestLayer()
		: m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	VulkanTestLayer::VulkanTestLayer(const std::string& name)
		: MoravaLayer(name), m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	VulkanTestLayer::~VulkanTestLayer()
	{
	}

	void VulkanTestLayer::OnAttach()
	{
		s_Scene = Hazel::Ref<Hazel::HazelScene>::Create();

		s_SceneHierarchyPanel = new Hazel::SceneHierarchyPanel(s_Scene);

		s_ContentBrowserPanel = new Hazel::ContentBrowserPanel();

		s_MaterialEditorPanel = new MaterialEditorPanel();

		m_Meshes.push_back(Ref<HazelMesh>::Create("Models/Cerberus/CerberusMaterials.fbx"));
		// m_Meshes.push_back(Ref<HazelMesh>::Create("Models/Hazel/Sphere1m.fbx"));
		// m_Meshes.push_back(Ref<HazelMesh>::Create("Models/Gladiator/Gladiator.fbx"));
	}

	void VulkanTestLayer::OnDetach()
	{
	}

	void VulkanTestLayer::OnUpdate(Timestep ts)
	{
		m_Camera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), (float)VulkanRenderer::GetViewportWidth(), (float)VulkanRenderer::GetViewportHeight(), 0.01f, 1000.0f));

		m_Camera.OnUpdate(ts);

		glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		Render(clearColor, m_Camera);
		for (Ref<HazelMesh> mesh : m_Meshes)
		{
			VulkanRenderer::SubmitMesh(mesh);
		}
	}

	void VulkanTestLayer::OnImGuiRender(::Window* mainWindow, ::Scene* scene)
	{
		/**** BEGIN Back to Vulkan // Hazel Live (17.02.2021) ****/

		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		// m_SceneHierarchyPanel.OnImGuiRender();

		style.WindowMinSize.x = minWinSizeX;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImVec2 viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		uint64_t textureID = 0; // m_Framebuffer->GetColorAttachmentRendererID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ viewportSize.x, viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		ImGui::End();
		ImGui::PopStyleVar();

		// ImGui::End();

		/**** END Back to Vulkan // Hazel Live (17.02.2021) ****/

		/**** BEGIN Vulkan ImGui Render Pass ****
		{
			s_ImGuiViewportMain.x = ImGui::GetMainViewport()->GetWorkPos().x;
			s_ImGuiViewportMain.y = ImGui::GetMainViewport()->GetWorkPos().y;

			// ImGui Dockspace
			bool p_open = true;

			ShowExampleAppDockSpace(&p_open);

			//	ImGui::ShowDemoWindow(&p_open);
			//	
			//	Window* mainWindow = Application::Get()->GetWindow();
			//	UpdateImGuizmo(mainWindow, camera);
			//	
			//	// Rendering
			//	ImGui::Render();
			//	ImDrawData* main_draw_data = ImGui::GetDrawData();
			//	ImGui_ImplDX11_RenderDrawData(main_draw_data);

			if (VulkanTestLayer::s_ShowWindowSceneHierarchy)
			{
				VulkanTestLayer::s_SceneHierarchyPanel->OnImGuiRender(&VulkanTestLayer::s_ShowWindowSceneHierarchy);
			}

			if (VulkanTestLayer::s_ShowWindowAssetManager)
			{
				VulkanTestLayer::s_ContentBrowserPanel->OnImGuiRender(&VulkanTestLayer::s_ShowWindowAssetManager);
			}

			if (VulkanTestLayer::s_ShowWindowMaterialEditor)
			{
				VulkanTestLayer::s_MaterialEditorPanel->OnImGuiRender(&VulkanTestLayer::s_ShowWindowMaterialEditor);
			}

			if (VulkanTestLayer::s_ShowWindowMaterialEditor)
			{
				DisplaySubmeshMaterialSelector(&VulkanTestLayer::s_ShowWindowMaterialEditor);
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
			ImGui::Begin("Viewport");
			{
				s_ViewportPanelMouseOver = ImGui::IsWindowHovered();
				s_ViewportPanelFocused = ImGui::IsWindowFocused();

				ImGuiWrapper::SetViewportEnabled(true);
				ImGuiWrapper::SetViewportHovered(s_ViewportPanelMouseOver);
				ImGuiWrapper::SetViewportFocused(s_ViewportPanelFocused);

				auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
				auto viewportSize = ImGui::GetContentRegionAvail();

				ImVec2 screen_pos = ImGui::GetCursorScreenPos();

				s_ImGuiViewport.X = (int)(ImGui::GetWindowPos().x - s_ImGuiViewportMain.x);
				s_ImGuiViewport.Y = (int)(ImGui::GetWindowPos().y - s_ImGuiViewportMain.y);
				s_ImGuiViewport.Width = (int)ImGui::GetWindowWidth();
				s_ImGuiViewport.Height = (int)ImGui::GetWindowHeight();
				s_ImGuiViewport.MouseX = (int)ImGui::GetMousePos().x;
				s_ImGuiViewport.MouseY = (int)ImGui::GetMousePos().y;

				glm::vec2 viewportPanelSize = glm::vec2(viewportSize.x, viewportSize.y);

				ImGui::Image((void*)(intptr_t)s_RenderTarget->m_ShaderResourceViewDX11, ImVec2 { viewportPanelSize.x, viewportPanelSize.y });

				UpdateImGuizmo();

				auto windowSize = ImGui::GetWindowSize();
				ImVec2 minBound = ImGui::GetWindowPos();

				minBound.x += viewportOffset.x;
				// minBound.y += viewportOffset.y;

				ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
				VulkanTestLayer::s_ViewportBounds[0] = { minBound.x, minBound.y };
				VulkanTestLayer::s_ViewportBounds[1] = { maxBound.x, maxBound.y };

				VulkanTestLayer::s_AllowViewportCameraEvents = ImGui::IsMouseHoveringRect(minBound, maxBound); // EditorLayer
			}
			ImGui::End();
			ImGui::PopStyleVar();
		}
		/**** END Vulkan ImGui Render Pass ****/
	}

	void VulkanTestLayer::UpdateImGuizmo()
	{
		/**** BEGIN Vulkan ImGuizmo ****
		if (VulkanTestLayer::s_ImGuizmoType != -1 && EntitySelection::s_SelectionContext.size())
		{
			float rw = (float)ImGui::GetWindowWidth();
			float rh = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

			SelectedSubmesh selectedSubmesh = EntitySelection::s_SelectionContext[0];

			// Entity transform
			auto& transformComponent = selectedSubmesh.Entity.GetComponent<Hazel::TransformComponent>();
			glm::mat4 entityTransform = transformComponent.GetTransform();

			// Snapping
			bool snap = VulkanTestLayer::s_LeftControlKeyPressed; // ImGuizmo snap enabled
			float snapValue = 5.0f; // Snap to 0.5m for translation/scale
			// Snap to 45 degrees for rotation
			if (VulkanTestLayer::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE)
			{
				snapValue = 45.0f;
			}
			float snapValues[3] = { snapValue, snapValue, snapValue };

			if (s_SelectionMode == SelectionMode::Entity || !selectedSubmesh.Mesh)
			{
				ImGuizmo::Manipulate(
					glm::value_ptr(DX11TestLayer::GetCamera()->GetViewMatrix()),
					glm::value_ptr(DX11TestLayer::GetCamera()->GetProjectionMatrix()),
					DX11TestLayer::s_ImGuizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(entityTransform),
					nullptr,
					snap ? snapValues : nullptr);

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 translation, rotation, scale;
					Math::DecomposeTransform(entityTransform, translation, rotation, scale);

					glm::vec3 deltaRotation = rotation - transformComponent.Rotation;
					transformComponent.Translation = translation;
					transformComponent.Rotation += deltaRotation;
					transformComponent.Scale = scale;
				}
			}
			else if (s_SelectionMode == SelectionMode::SubMesh)
			{
				auto aabb = selectedSubmesh.Mesh->BoundingBox;

				glm::vec3 aabbCenterOffset = glm::vec3(
					aabb.Min.x + ((aabb.Max.x - aabb.Min.x) / 2.0f),
					aabb.Min.y + ((aabb.Max.y - aabb.Min.y) / 2.0f),
					aabb.Min.z + ((aabb.Max.z - aabb.Min.z) / 2.0f)
				);

				glm::mat4 submeshTransform = selectedSubmesh.Mesh->Transform;
				submeshTransform = glm::translate(submeshTransform, aabbCenterOffset);
				glm::mat4 transformBase = entityTransform * submeshTransform;

				ImGuizmo::Manipulate(
					glm::value_ptr(VulkanTestLayer::GetCamera()->GetViewMatrix()),
					glm::value_ptr(VulkanTestLayer::GetCamera()->GetProjectionMatrix()),
					DX11TestLayer::s_ImGuizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(transformBase),
					nullptr,
					snap ? snapValues : nullptr);

				if (ImGuizmo::IsUsing())
				{
					submeshTransform = glm::inverse(entityTransform) * transformBase;
					submeshTransform = glm::translate(submeshTransform, -aabbCenterOffset);
					selectedSubmesh.Mesh->Transform = submeshTransform;
				}
			}
		}
		/**** END Vulkan ImGuizmo ****/
	}

	void VulkanTestLayer::OnEvent(Event& event)
	{
		m_Camera.OnEvent(event);

		if (event.GetEventType() == EventType::WindowResize)
		{
			WindowResizeEvent& e = (WindowResizeEvent&)event;
			if (e.GetWidth() != 0 && e.GetHeight() != 0)
			{
				m_Camera.SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());
				m_Camera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), (float)e.GetWidth(), (float)e.GetHeight(), 0.1f, 10000.0f));
			}
		}
	}

	void VulkanTestLayer::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
	{
	}

	void VulkanTestLayer::OnRender(::Window* mainWindow, ::Scene* scene)
	{
		VulkanRenderer::Draw(scene->GetCamera());
	}

	void VulkanTestLayer::Render(const glm::vec4& clearColor, const EditorCamera& camera)
	{
		auto mesh = m_Meshes[0];
		// HazelRenderer::Submit([=]() mutable
		// {
		// });
		{
			Ref<VulkanContext> context = Ref<VulkanContext>(Application::Get()->GetWindow()->GetRenderContext());
			Ref<VulkanShader> shader = mesh->GetMeshShader().As<VulkanShader>();
			VulkanSwapChain& swapChain = context->GetSwapChain();

#if 0
			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			// Set clear values for all framebuffer attachments with loadOp set to clear
			// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
			VkClearValue clearValues[2];
			clearValues[0].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
			clearValues[1].depthStencil = { 1.0f, 0 };

			uint32_t width = swapChain.GetWidth();
			uint32_t height = swapChain.GetHeight();

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;

			// Set target frame buffer
			renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();
#endif

			{
				// uniform buffer binding 0 uniform Camera
				void* ubPtr = shader->MapUniformBuffer(0);
				// glm::mat4 proj = glm::perspectiveFov(glm::radians(45.0f), (float)swapChain.GetWidth(), (float)swapChain.GetHeight(), 0.1f, 1000.0f);
				glm::mat4 proj = glm::perspectiveFov(glm::radians(45.0f), (float)VulkanRenderer::GetViewportWidth(), (float)VulkanRenderer::GetViewportHeight(), 0.1f, 1000.0f);
				// glm::mat4 view = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 4.0f)));
				glm::mat4 viewProj = proj * camera.GetViewMatrix();  // Runtime camera
				// glm::mat4 viewProj = m_Camera.GetViewProjection(); // Editor camera
				memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
				shader->UnmapUniformBuffer(0);
			}

			{
				struct Light
				{
					glm::vec3 Direction;
					glm::vec3 Radiance;
					float Multiplier;
				};

				struct UB
				{
					Light lights;
					glm::vec3 u_CameraPosition;
				};

				UB ub;
				ub.lights = {
					{ 0.5f, 0.5f, 0.5f },
					{ 1.0f, 1.0f, 1.0f },
					1.0f,
				};
				ub.u_CameraPosition = camera.GetPosition();

				void* ubPtr = shader->MapUniformBuffer(1);
				memcpy(ubPtr, &ub, sizeof(UB));
				shader->UnmapUniformBuffer(1);
			}

#if 0
			{
				VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
				VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));

				// Start the first sub pass specified in our default render pass setup by the base class
				// This will clear the color and depth attachment
				vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				// Update dynamic viewport state
				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = (float)height;
				viewport.height = -(float)height;
				viewport.width = (float)width;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(drawCommandBuffer, 0, 1, &viewport);

				// Update dynamic scissor state
				VkRect2D scissor = {};
				scissor.extent.width = width;
				scissor.extent.height = height;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				vkCmdSetScissor(drawCommandBuffer, 0, 1, &scissor);

				// VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

				// DRAW GEO HERE

				vkCmdEndRenderPass(drawCommandBuffer);

				// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
				// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

				VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
			}
#endif
		}
	}
}
