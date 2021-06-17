#include "DX11Renderer.h"

#include "imgui.h"

#include "DX11.h"
#include "DX11Context.h"
#include "DX11Framebuffer.h"
#include "DX11IndexBuffer.h"
#include "DX11Pipeline.h"
#include "DX11Shader.h"
#include "DX11Texture2D.h"
#include "DX11VertexBuffer.h"
#include "Hazel/Renderer/HazelRenderer.h"

#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
// #include "backends/imgui_impl_vulkan_with_textures.h"

#include "../../ImGuizmo/ImGuizmo.h"


static Hazel::Ref<Hazel::HazelFramebuffer> s_Framebuffer;
static Hazel::Ref<Hazel::Pipeline> s_MeshPipeline;
static Hazel::Ref<Hazel::Pipeline> s_CompositePipeline;
static Hazel::Ref<Hazel::VertexBuffer> s_QuadVertexBuffer;
static Hazel::Ref<Hazel::IndexBuffer> s_QuadIndexBuffer;
static ImTextureID s_TextureID;
static uint32_t s_ViewportWidth = 1280;
static uint32_t s_ViewportHeight = 720;

static std::vector<Hazel::Ref<Hazel::HazelMesh>> s_Meshes;

static Hazel::Submesh* s_SelectedSubmesh;
static glm::mat4* s_Transform_ImGuizmo = nullptr;

struct VulkanRendererData
{
	VkCommandBuffer ActiveCommandBuffer = nullptr;
};

static VulkanRendererData s_Data;


void DX11Renderer::SubmitMesh(const Hazel::Ref<Hazel::HazelMesh>& mesh)
{
	// Temporary code - populate selected submesh
	std::vector<Hazel::Submesh> submeshes = mesh->GetSubmeshes();
	s_SelectedSubmesh = &submeshes.at(0);

	s_Meshes.push_back(mesh);
}

void DX11Renderer::OnResize(uint32_t width, uint32_t height)
{
	// HazelRenderer::Submit([=]() {
	// });
	{
		auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;

		// auto DX11Device = DX11Context::GetCurrentDevice()->GetDX11Device();
	}
}

void DX11Renderer::Init()
{
	// HazelRenderer::Submit([=]() {
	// });
	{
	}

	{
		Hazel::HazelFramebufferSpecification spec;
		spec.Width = s_ViewportWidth;
		spec.Height = s_ViewportHeight;
		// TODO:	s_Framebuffer = Hazel::HazelFramebuffer::Create(spec);
		// TODO:	s_Framebuffer->AddResizeCallback([](Hazel::Ref<Hazel::HazelFramebuffer> framebuffer) {
		// TODO:		// HazelRenderer::Submit([framebuffer]() mutable
		// TODO:		// {
		// TODO:		// });
		// TODO:		{
		// TODO:		}
		// TODO:	});

		Hazel::PipelineSpecification pipelineSpecification;
		pipelineSpecification.Layout = {
			{ Hazel::ShaderDataType::Float3, "a_Position" },
			{ Hazel::ShaderDataType::Float3, "a_Normal" },
			{ Hazel::ShaderDataType::Float3, "a_Tangent" },
			{ Hazel::ShaderDataType::Float3, "a_Binormal" },
			{ Hazel::ShaderDataType::Float2, "a_TexCoord" },
		};
		// TODO: pipelineSpecification.Shader = Hazel::HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");

		Hazel::RenderPassSpecification renderPassSpec;
		renderPassSpec.TargetFramebuffer = s_Framebuffer;
		// TODO: pipelineSpecification.RenderPass = Hazel::RenderPass::Create(renderPassSpec);
		// TODO: s_MeshPipeline = Hazel::Pipeline::Create(pipelineSpecification);
	}

	{
		Hazel::HazelFramebufferSpecification spec;
		spec.SwapChainTarget = true;
		// TODO: Hazel::Ref<Hazel::HazelFramebuffer> framebuffer = Hazel::HazelFramebuffer::Create(spec);

		Hazel::PipelineSpecification pipelineSpecification;
		pipelineSpecification.Layout = {
			{ Hazel::ShaderDataType::Float3, "a_Position" },
			// { ShaderDataType::Float3, "a_Normal" },
			// { ShaderDataType::Float3, "a_Tangent" },
			// { ShaderDataType::Float3, "a_Binormal" },
			{ Hazel::ShaderDataType::Float2, "a_TexCoord" },
		};
		// TODO: pipelineSpecification.Shader = Hazel::HazelRenderer::GetShaderLibrary()->Get("Texture");

		Hazel::RenderPassSpecification renderPassSpec;
		// TODO: renderPassSpec.TargetFramebuffer = framebuffer;
		// TODO: pipelineSpecification.RenderPass = Hazel::RenderPass::Create(renderPassSpec);
		// TODO: s_CompositePipeline = Hazel::Pipeline::Create(pipelineSpecification);
	}

	// Create fullscreen quad
	float x = -1;
	float y = -1;
	float width = 2;
	float height = 2;

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
	};

	QuadVertex* data = new QuadVertex[4];

	data[0].Position = glm::vec3(x, y, 0.1f);
	data[0].TexCoord = glm::vec2(0, 0);

	data[1].Position = glm::vec3(x + width, y, 0.1f);
	data[1].TexCoord = glm::vec2(1, 0);

	data[2].Position = glm::vec3(x + width, y + height, 0.1f);
	data[2].TexCoord = glm::vec2(1, 1);

	data[3].Position = glm::vec3(x, y + height, 0.1f);
	data[3].TexCoord = glm::vec2(0, 1);

	// TODO: s_QuadVertexBuffer = Hazel::VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
	uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
	// TODO: s_QuadIndexBuffer = Hazel::IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

	// HazelRenderer::Submit([=]()
	// {
	// });
	{
		// TODO: auto shader = s_CompositePipeline->GetSpecification().Shader;
		// TODO: auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;

		// TODO: auto DX11Device = DX11Context::GetCurrentDevice()->GetDX11Device();

		auto vulkanFB = s_Framebuffer;
	}

	Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
}

static void RenderMesh(Hazel::Ref<Hazel::HazelMesh> mesh, Hazel::HazelCamera* camera) // TODO: remove the HazelCamera parameter
{
	// TODO

	auto& submeshes = mesh->GetSubmeshes();
	for (Hazel::Submesh& submesh : submeshes)
	{
		// TODO: Draw Indexed
	}
}

static void CompositeRenderPass(VkCommandBufferInheritanceInfo& inheritanceInfo)
{
	Hazel::Ref<DX11Context> dx11Context = Hazel::Ref<DX11Context>(Application::Get()->GetWindow()->GetRenderContext());
	// DX11SwapChain& swapChain = context->GetSwapChain();

	// TODO
}

// TODO: Temporary method until composite rendering is enabled
void DX11Renderer::Draw(Hazel::HazelCamera* camera)
{
	// HazelRenderer::Submit([=]() mutable
	// {
	// });
	{
		Hazel::Ref<DX11Context> dx11Context = DX11Context::Get();
		// DX11SwapChain& swapChain = context->GetSwapChain();

		dx11Context->ClearRenderTargetColor(1.0f, 0.4f, 0.0f, 1.0f);

		for (auto& mesh : s_Meshes)
		{
			RenderMesh(mesh, camera);
		}

		s_Meshes.clear();
	}

	// HazelRenderer::Submit([=]()
	// {
	// });
	{
		Hazel::Ref<DX11Context> context = DX11Context::Get();
		// DX11SwapChain& swapChain = context->GetSwapChain();

		{
			// Begin Render Pass

			/**** BEGIN ImGui Render Pass ****
			{
				// ImGui Dockspace
				bool p_open = true;
				ShowExampleAppDockSpace(&p_open);

				ImGui::Begin("Scene Hierarchy");
				ImGui::End();

				// TEMP: Render Viewport
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGui::Begin("Viewport");
				auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
				auto viewportSize = ImGui::GetContentRegionAvail();
				ImGui::Image(s_TextureID, viewportSize, { 0, 1 }, { 1, 0 });

				if (s_ViewportWidth != viewportSize.x || s_ViewportHeight != viewportSize.y)
				{
					s_ViewportWidth = (uint32_t)viewportSize.x;
					s_ViewportHeight = (uint32_t)viewportSize.y;
					// s_Framebuffer->Resize(s_ViewportWidth, s_ViewportHeight, true);
				}

				Window* mainWindow = Application::Get()->GetWindow();
				UpdateImGuizmo(mainWindow, camera);

				ImGui::End();
				ImGui::PopStyleVar();

				// TODO: Move to VulkanImGuiLayer
				// Rendering
				ImGui::Render();

				ImDrawData* main_draw_data = ImGui::GetDrawData();
				// ImGui_ImplVulkan_RenderDrawData(main_draw_data, s_ImGuiCommandBuffer);

				// End Vulkan Command Buffer
			}
			**** END ImGui Render Pass ****/

			// Vulkan CmdExecuteCommands
			// Vulkan CmdEndRenderPass
		}
	}
}

void DX11Renderer::BeginFrame()
{
	//	HazelRenderer::Submit([]()
	//	{
	//	});
	{
		Hazel::Ref<DX11Context> context = DX11Context::Get();
		// DX11SwapChain& swapChain = context->GetSwapChain();

		// TODO
	}
}

void DX11Renderer::EndFrame()
{
	//	HazelRenderer::Submit([]()
	//	{
	//	});
	{
	}
}

void DX11Renderer::BeginRenderPass(const Hazel::Ref<Hazel::RenderPass>& renderPass)
{
	//	HazelRenderer::Submit([renderPass]()
	//	{
	//	});
	{
		BeginFrame();

		// TODO
	}
}

void DX11Renderer::EndRenderPass()
{
	//	HazelRenderer::Submit([]()
	//	{
	//	});
	{
		vkCmdEndRenderPass(s_Data.ActiveCommandBuffer);
		s_Data.ActiveCommandBuffer = nullptr;
	}
}

void DX11Renderer::SubmitFullscreenQuad(Hazel::Ref<Hazel::Pipeline> pipeline, Hazel::Ref<Material> material)
{
	//	HazelRenderer::Submit([]()
	//	{
	//	});
	{
	}
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Docking, DockSpace / ShowExampleAppDockSpace()
//-----------------------------------------------------------------------------

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void DX11Renderer::ShowExampleAppDockSpace(bool* p_open)
{
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", p_open, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	else
	{
		// ShowDockingDisabledMessage();
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Docking"))
		{
			// Disabling fullscreen would allow the window to be moved to the front of other windows,
			// which we can't undo at the moment without finer window depth/z control.
			//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

			if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
			if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
			if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
			if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
			if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
			ImGui::Separator();
			if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
				*p_open = false;
			ImGui::EndMenu();
		}
		/****
		HelpMarker(
			"When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n\n"
			" > if io.ConfigDockingWithShift==false (default):" "\n"
			"   drag windows from title bar to dock" "\n"
			" > if io.ConfigDockingWithShift==true:" "\n"
			"   drag windows from anywhere and hold Shift to dock" "\n\n"
			"This demo app has nothing to do with it!" "\n\n"
			"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)." "\n\n"
			"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
			"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)"
		);
		****/

		ImGui::EndMenuBar();
	}

	ImGui::End();
}

void DX11Renderer::UpdateImGuizmo(Window* mainWindow, Hazel::HazelCamera* camera)
{
	// BEGIN ImGuizmo

	// ImGizmo switching modes
	if (Input::IsKeyPressed(Key::D1))
		Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

	if (Input::IsKeyPressed(Key::D2))
		Scene::s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;

	if (Input::IsKeyPressed(Key::D3))
		Scene::s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;

	if (Input::IsKeyPressed(Key::D4))
		Scene::s_ImGuizmoType = -1;

	// ImGuizmo
	if (Scene::s_ImGuizmoType != -1)
	{
		float rw = (float)ImGui::GetWindowWidth();
		float rh = (float)ImGui::GetWindowHeight();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

		if (s_SelectedSubmesh != nullptr) {
			s_Transform_ImGuizmo = &s_SelectedSubmesh->Transform; // Connect to model transform
		}

		// Snapping
		bool snap = Input::IsKeyPressed(Key::LeftControl);
		float snapValue = 1.0f; // Snap to 0.5m for translation/scale
		// Snap to 45 degrees for rotation
		if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE) {
			snapValue = 45.0f;
		}

		float snapValues[3] = { snapValue, snapValue, snapValue };

		if (s_Transform_ImGuizmo != nullptr) // TODO: specify display criteria here
		{
			ImGuizmo::Manipulate(
				glm::value_ptr(camera->GetViewMatrix()),
				glm::value_ptr(camera->GetProjectionMatrix()),
				(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
				ImGuizmo::LOCAL,
				glm::value_ptr(*s_Transform_ImGuizmo),
				nullptr,
				snap ? snapValues : nullptr);
		}
	}
	// END ImGuizmo
}

uint32_t DX11Renderer::GetViewportWidth()
{
	return s_ViewportWidth;
}

uint32_t DX11Renderer::GetViewportHeight()
{
	return s_ViewportHeight;
}
