#include "VulkanTestLayer.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanSwapChain.h"
#include "Hazel/Renderer/Renderer2D.h"

#include "HazelLegacy/Platform/Vulkan/VulkanRendererHazelLegacy.h"
#include "HazelLegacy/Renderer/MeshHazelLegacy.h"

#include "Core/Application.h"
#include "HazelVulkan/ExampleVertex.h"


namespace Hazel {

	Ref<SceneHazelLegacy> VulkanTestLayer::s_Scene;
	SceneHierarchyPanelHazelLegacy* VulkanTestLayer::s_SceneHierarchyPanel;
	ContentBrowserPanel* VulkanTestLayer::s_ContentBrowserPanel;
	MaterialEditorPanel* VulkanTestLayer::s_MaterialEditorPanel;

	struct SceneRendererData
	{
		const HazelScene* ActiveScene = nullptr;

		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;
			Environment SceneEnvironment;
			float SkyboxLod;
			LightEnvironment SceneLightEnvironment;
		} SceneData;

		// Resources
		Ref<Pipeline> GeometryPipeline;
		Ref<Pipeline> CompositePipeline;
		Ref<Pipeline> SkyboxPipeline;
		Ref<Pipeline> GridPipeline;
		Ref<HazelMaterial> SkyboxMaterial;

		Ref<RenderPass> GeoPass;

		struct DrawCommand
		{
			Ref<MeshHazelLegacy> Mesh;
			Ref<HazelMaterial> Material;
			glm::mat4 Transform;
		};

		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> SelectedMeshDrawList;

		// Grid
		Ref<HazelMaterial> GridMaterial;

		// SceneRendererOptions Options; // moved to VulkanRenderer
	};

	static SceneRendererData s_Data;

	std::vector<Ref<MeshHazelLegacy>> VulkanTestLayer::s_Meshes;

	VulkanTestLayer::VulkanTestLayer()
		: m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	VulkanTestLayer::VulkanTestLayer(const std::string& name)
		: MoravaLayer(name), m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	VulkanTestLayer::~VulkanTestLayer() {}

	void VulkanTestLayer::OnAttach()
	{
		s_Scene = Hazel::Ref<Hazel::SceneHazelLegacy>::Create();
		s_SceneHierarchyPanel = new Hazel::SceneHierarchyPanelHazelLegacy(s_Scene);
		s_ContentBrowserPanel = new Hazel::ContentBrowserPanel();
		s_MaterialEditorPanel = new MaterialEditorPanel();
		s_Meshes.push_back(Ref<MeshHazelLegacy>::Create("Models/Cerberus/CerberusMaterials.fbx"));
	}

	void VulkanTestLayer::OnDetach() {}

	void VulkanTestLayer::OnUpdate(Timestep ts)
	{
		m_Camera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), (float)VulkanRendererHazelLegacy::GetViewportWidth(), (float)VulkanRendererHazelLegacy::GetViewportHeight(), 0.01f, 1000.0f));

		m_Camera.OnUpdate(ts);

		for (Ref<MeshHazelLegacy> mesh : s_Meshes)
		{
			VulkanRendererHazelLegacy::SubmitMeshTemp(mesh); // the method should be removed from VulkanRenderer
		}

		VulkanRendererHazelLegacy::MapUniformBuffersVTL(s_Meshes[0], m_Camera);
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

	void VulkanTestLayer::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) {}

	void VulkanTestLayer::OnRender(::Window* mainWindow, ::Scene* scene)
	{
		// VulkanRenderer::Draw(scene->GetCamera());
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	/**** BEGIN this version of the OnAttach method is outdated ****
	void VulkanTestLayer::OnAttachOld()
	{
		s_Scene = Hazel::Ref<Hazel::HazelScene>::Create();

		s_SceneHierarchyPanel = new Hazel::SceneHierarchyPanel(s_Scene);

		s_ContentBrowserPanel = new Hazel::ContentBrowserPanel();

		s_MaterialEditorPanel = new MaterialEditorPanel();

		// m_Meshes.push_back(Ref<MeshHazelLegacy>::Create("Models/Gladiator/Gladiator.fbx"));
		// m_Meshes.push_back(Ref<MeshHazelLegacy>::Create("Models/Hazel/TestSceneVulkan.fbx"));
		// m_Meshes.push_back(Ref<MeshHazelLegacy>::Create("Models/Hazel/Sphere1m.fbx"));
		s_Meshes.push_back(Ref<MeshHazelLegacy>::Create("Models/Cerberus/CerberusMaterials.fbx"));

		/**** BEGIN the code that is not used anymore ****
		RenderPassSpecification renderPassSpec;
		HazelFramebufferSpecification framebufferSpec;
		framebufferSpec.DebugName = "GeoPassFramebufferSpec";
		framebufferSpec.Width = 1280;
		framebufferSpec.Height = 720;
		renderPassSpec.TargetFramebuffer = HazelFramebuffer::Create(framebufferSpec);
		s_Data.GeoPass = RenderPass::Create(renderPassSpec);

		// Geometry pipeline
		{
			HazelFramebufferSpecification spec;
			Ref<HazelFramebuffer> framebuffer = HazelFramebuffer::Create(spec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");
			pipelineSpecification.RenderPass = s_Data.GeoPass;
			pipelineSpecification.DebugName = "PBR-Static";
			s_Data.GeometryPipeline = Pipeline::Create(pipelineSpecification);
		}
		/**** END the code that is not used anymore ****
	}
	/**** END this version of the OnAttach method is outdated ****/

	/**** BEGIN moved to VulkanRenderer ****
	SceneRendererOptions& VulkanTestLayer::GetOptions()
	{
		return s_Data.Options;
	}
	/**** END moved to VulkanRenderer ****/

	/**** BEGIN code moved from VulkanTestLayer to VulkanRenderer ****
	void VulkanTestLayer::MapUniformBuffersVTL(const glm::vec4& clearColor, const EditorCamera& camera)
	{
		// Temporary code
		s_Data.SceneData.SceneCamera.Camera = camera;
		auto mesh = s_Meshes[0];

		HazelRenderer::GetRendererAPI()->BeginRenderPass(s_Data.GeoPass);

		auto viewProjection = s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix() * s_Data.SceneData.SceneCamera.ViewMatrix;
		// glm::vec3 cameraPosition = glm::inverse(s_Data.SceneData.SceneCamera.ViewMatrix)[3];
		glm::vec3 cameraPosition = camera.GetPosition();

		// float skyboxLod = s_Data.ActiveScene->GetSkyboxLod();
		// HazelRenderer::Submit([viewProjection, cameraPosition]() {});
		{
			auto inverseVP = glm::inverse(viewProjection);
			// auto shader = s_Data.GridMaterial->GetShader().As<VulkanShader>();
			// void* ubPtr = shader->MapUniformBuffer(0);
			struct ViewProj
			{
				glm::mat4 ViewProjection;
				glm::mat4 InverseViewProjection;
			};
			ViewProj viewProj;
			viewProj.ViewProjection = viewProjection;
			viewProj.InverseViewProjection = inverseVP;
			// memcpy(ubPtr, &viewProj, sizeof(ViewProj));
			// shader->UnmapUniformBuffer(0);

			// shader = s_Data.SkyboxMaterial->GetShader().As<VulkanShader>();
			// ubPtr = shader->MapUniformBuffer(0);
			// memcpy(ubPtr, &viewProj, sizeof(ViewProj));
			// shader->UnmapUniformBuffer(0);

			// shader = HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static").As<VulkanShader>();
			// ubPtr = shader->MapUniformBuffer(0);
			// memcpy(ubPtr, &viewProj, sizeof(ViewProj));
			// shader->UnmapUniformBuffer(0);

			Ref<VulkanShader> shader = mesh->GetMeshShader().As<VulkanShader>();

			{
				void* ubPtr = shader->MapUniformBuffer(0, 0);
				glm::mat4 viewProj = camera.GetViewProjection();
				memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
				shader->UnmapUniformBuffer(0, 0);
			}

			struct Light
			{
				glm::vec3 Direction;
				float Padding = 0.0f;
				glm::vec3 Radiance;
				float Multiplier;
			};

			struct UB
			{
				Light lights;
				glm::vec3 u_CameraPosition;
				// glm::vec4 u_AlbedoColorUB;
			};

			UB ub;
			ub.lights =
			{
				{ 0.5f, 0.5f, 0.5f },
				0.0f,
				{ 1.0f, 1.0f, 1.0f },
				1.0f
			};

			ub.lights.Direction = VulkanRenderer::GetLightDirectionTemp();
			ub.u_CameraPosition = cameraPosition;
			// ub.u_AlbedoColorUB = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

			// Log::GetLogger()->info("Light Direction: {0}, {1}, {2}", ub.lights.Direction.x, ub.lights.Direction.y, ub.lights.Direction.z);

			void* ubPtr = shader->MapUniformBuffer(1, 0);
			memcpy(ubPtr, &ub, sizeof(UB));
			shader->UnmapUniformBuffer(1, 0);
		}
	}
	/**** END code moved from VulkanTestLayer to VulkanRenderer ****/

	/**** BEGIN we don't need this method anymore, the only important part is mapping uniform buffers ****
	 **** SceneRenderer::GeometryPass in Hazel Vulkan branch
	void VulkanTestLayer::GeometryPass(const glm::vec4& clearColor, const EditorCamera& camera)
	{
		MapUniformBuffersVTL(clearColor, camera);

		// Skybox
		// s_Data.SkyboxMaterial->Set("u_Uniforms.TextureLod", s_Data.SceneData.SkyboxLod);
		// s_Data.SkyboxMaterial->Set("u_Texture", s_Data.SceneData.SceneEnvironment.RadianceMap);
		// VulkanRenderer::SubmitFullscreenQuadStatic(s_Data.SkyboxPipeline, s_Data.SkyboxMaterial);

		// RenderEntities
		for (auto& dc : s_Data.DrawList)
		{
			HazelRenderer::GetRendererAPI()->RenderMesh(s_Data.GeometryPipeline, dc.Mesh, dc.Transform);
		}

		for (auto& dc : s_Data.SelectedMeshDrawList)
		{
			HazelRenderer::GetRendererAPI()->RenderMesh(s_Data.GeometryPipeline, dc.Mesh, dc.Transform);
		}

		// Grid
		if (VulkanRenderer::GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f));
			// VulkanRenderer::RenderQuadStatic(s_Data.GridPipeline, s_Data.GridMaterial, transform);
		}

		if (VulkanRenderer::GetOptions().ShowBoundingBoxes)
		{
			Renderer2D::BeginScene(viewProjection, true);
			for (auto& dc : s_Data.DrawList)
			{
				HazelRenderer::DrawAABB(dc.Mesh, dc.Transform);
			}
			Renderer2D::EndScene();
		}

		HazelRenderer::GetRendererAPI()->EndRenderPass();

#if 0
		/**** BEGIN the old VulkanTestLayer code, belongs to SceneRenderer::GeometryPass in Vulkan branch, here VulkanTestLayer::GeometryPass ****

		// HazelRenderer::Submit([=]() mutable {});
		{
			Ref<VulkanContext> context = Ref<VulkanContext>(Application::Get()->GetWindow()->GetRenderContext());
			Ref<VulkanShader> shader = mesh->GetMeshShader().As<VulkanShader>();
			VulkanSwapChain& swapChain = context->GetSwapChain();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			// Set clear values for all framebuffer attachments with loadOp set to clear
			// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
			VkClearValue clearValues[2];
			clearValues[0].color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
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

			// Skybox
			float skyboxLod = s_Data.ActiveScene->GetSkyboxLod();
			s_Data.SkyboxMaterial->Set("u_Uniforms.TextureLod", skyboxLod);
			s_Data.SkyboxMaterial->Set("u_Texture", s_Data.SceneData.SceneEnvironment.RadianceMap);
			VulkanRenderer::SubmitFullscreenQuadStatic(s_Data.SkyboxPipeline, s_Data.SkyboxMaterial);

			// RenderEntities
			for (auto& dc : s_Data.DrawList)
			{
				VulkanRenderer::RenderMeshStatic(dc.Mesh, dc.Transform);
			}

			// Grid
			if (GetOptions().ShowGrid)
			{
				const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f));
				VulkanRenderer::RenderQuadStatic(s_Data.GridPipeline, s_Data.GridMaterial, transform);
			}

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

			/**** END the old VulkanTestLayer code, belongs to SceneRenderer::GeometryPass in Vulkan branch, here VulkanTestLayer::GeometryPass ****
		}
#endif

	}
	/**** END we don't need this method anymore, the only important part is mapping uniform buffers ****/

	/**** BEGIN Vulkan ImGuizmo ****
	void VulkanTestLayer::UpdateImGuizmo()
	{
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
	}
	/**** END Vulkan ImGuizmo ****/

	/**** BEGIN this longer version of the OnImGuiRender method is no longer needed ****
	void VulkanTestLayer::OnImGuiRenderOld(::Window* mainWindow, ::Scene* scene)
	{
		/**** BEGIN Back to Vulkan // Hazel Live (17.02.2021) ****

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
		/**** END Vulkan ImGui Render Pass ****
	}
	/**** END this longer version of the OnImGuiRender method is no longer needed ****/

}
