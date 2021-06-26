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
#include "DX11IndexBuffer.h"
#include "DX11TestLayer.h"

#include "Hazel/Renderer/HazelRenderer.h"

// ImGui includes
#if !defined(IMGUI_IMPL_API)
#define IMGUI_IMPL_API
#endif
#include "imgui.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "ImGuizmo.h"


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

// temporary DX11 objects
static Hazel::Ref<DX11VertexBuffer> s_VertexBuffer;
static Hazel::Ref<DX11IndexBuffer> s_IndexBuffer;
static Hazel::Ref<Hazel::Pipeline> s_Pipeline;
static Hazel::Ref<DX11ConstantBuffer> s_ConstantBuffer;


void DX11Renderer::SubmitMesh(const Hazel::Ref<Hazel::HazelMesh>& mesh)
{
	// Temporary code - populate selected submesh
	std::vector<Hazel::Submesh> submeshes = mesh->GetSubmeshes();
	s_SelectedSubmesh = &submeshes.at(0);

	s_Meshes.push_back(mesh);
}

void DX11Renderer::OnResize(uint32_t width, uint32_t height)
{
	// HazelRenderer::Submit([=]() {});
	// auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
}

void DX11Renderer::Init()
{
	/**** BEGIN DirectX 11 Init (from DX11TestLayer::OnAttach) ****/

	Hazel::HazelFramebufferTextureSpecification framebufferTextureSpecification;
	framebufferTextureSpecification.Format = Hazel::HazelImageFormat::RGBA;

	std::vector<Hazel::HazelFramebufferTextureSpecification> framebufferTextureSpecifications;
	framebufferTextureSpecifications.push_back(framebufferTextureSpecification);

	Hazel::HazelFramebufferAttachmentSpecification framebufferAttachmentSpecification{};
	framebufferAttachmentSpecification.Attachments = framebufferTextureSpecifications;

	Hazel::HazelFramebufferSpecification framebufferSpecification{};
	framebufferSpecification.ClearColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	framebufferSpecification.DebugName = "DX11 Framebuffer specification";
	framebufferSpecification.Width = Application::Get()->GetWindow()->GetWidth();
	framebufferSpecification.Height = Application::Get()->GetWindow()->GetHeight();
	framebufferSpecification.NoResize = false;
	framebufferSpecification.Samples = 1; // TODO: for mipmaps? what is the optimal number?
	framebufferSpecification.Scale = 1.0f;
	framebufferSpecification.SwapChainTarget = true; // render to screen or to offscreen render target
	framebufferSpecification.Attachments = framebufferAttachmentSpecification;

	Hazel::RenderPassSpecification renderPassSpecification{};
	renderPassSpecification.DebugName = "DX11 Render Pass specificartion";
	renderPassSpecification.TargetFramebuffer = Hazel::HazelFramebuffer::Create(framebufferSpecification);

	MoravaShaderSpecification moravaShaderSpecification;
	moravaShaderSpecification.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
	// moravaShaderSpecification.VertexShaderPath = "Shaders/HLSL/DirLightBumpVS.hlsl";
	// moravaShaderSpecification.PixelShaderPath = "Shaders/HLSL/DirLightBumpPS.hlsl";
	moravaShaderSpecification.VertexShaderPath = "Shaders/HLSL/BasicVertexShader.hlsl";
	moravaShaderSpecification.PixelShaderPath = "Shaders/HLSL/BasicPixelShader.hlsl";
	moravaShaderSpecification.ForceCompile = false;

	Hazel::PipelineSpecification pipelineSpecification{};
	pipelineSpecification.DebugName = "DX11 Pipeline specification";
	pipelineSpecification.Layout = Hazel::VertexBufferLayout{};
	pipelineSpecification.RenderPass = Hazel::RenderPass::Create(renderPassSpecification);
	pipelineSpecification.Shader = MoravaShader::Create(moravaShaderSpecification);

	s_Pipeline = Hazel::Pipeline::Create(pipelineSpecification);

	DX11VertexLayout vertexList[] =
	{
		// ----------------- POSITION XYZ --------- TEXCOORD UV --- NORMAL XYZ ----------- TANGENT XYZ --------- BINORMAL XYZ
		DX11VertexLayout{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { -0.8f, -0.8f, 0.0f }, { 1.0f, 1.0f, 1.0f }, }, // VERTEX #0
		DX11VertexLayout{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { -0.8f,  0.8f, 0.0f }, { 1.0f, 0.0f, 1.0f }, }, // VERTEX #1
		DX11VertexLayout{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, {  0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 1.0f }, }, // VERTEX #2
		DX11VertexLayout{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, {  0.8f,  0.8f, 0.0f }, { 0.0f, 0.0f, 1.0f }, }, // VERTEX #3

		DX11VertexLayout{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, {  0.8f,  0.8f, 0.0f }, { 0.0f, 0.0f, 1.0f }, }, // VERTEX #4
		DX11VertexLayout{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, {  0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 1.0f }, }, // VERTEX #5
		DX11VertexLayout{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { -0.8f,  0.8f, 0.0f }, { 1.0f, 0.0f, 1.0f }, }, // VERTEX #6
		DX11VertexLayout{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { -0.8f, -0.8f, 0.0f }, { 1.0f, 1.0f, 1.0f }, }, // VERTEX #7
	};

	// temporary DX11 objects and data structures
	uint32_t vertexStride = sizeof(DX11VertexLayout);
	uint32_t vertexCount = ARRAYSIZE(vertexList);
	s_VertexBuffer = Hazel::Ref<DX11VertexBuffer>::Create(vertexList, vertexStride, vertexCount, pipelineSpecification.Shader);

	uint32_t indexList[] =
	{
		// Front side
		0, 1, 2, // First triangle
		2, 3, 0, // Second triangle
		// Back side
		4, 5, 6,
		6, 7, 4,
		// Top side
		1, 6, 5,
		5, 2, 1,
		// Bottom side
		7, 0, 3,
		3, 4, 7,
		// Right side
		3, 2, 5,
		5, 4, 3,
		// Left side
		7, 6, 1,
		1, 0, 7,
	};

	uint32_t indexCount = ARRAYSIZE(indexList);
	s_IndexBuffer = Hazel::Ref<DX11IndexBuffer>::Create(indexList, indexCount);

	DX11ConstantBufferLayout constantBufferLayout;
	constantBufferLayout.Model = glm::mat4(1.0f);
	constantBufferLayout.View = glm::mat4(1.0f);
	constantBufferLayout.Projection = glm::mat4(1.0f);
	constantBufferLayout.Time = 0;
	s_ConstantBuffer = Hazel::Ref<DX11ConstantBuffer>::Create(&constantBufferLayout, sizeof(DX11ConstantBufferLayout));

	/**** BEGIN DirectX 11 Init (from DX11TestLayer::OnAttach) ****/

	/****
	Hazel::HazelRenderer::Submit([=]() {});

	{
		Hazel::HazelFramebufferSpecification spec;
		spec.Width = s_ViewportWidth;
		spec.Height = s_ViewportHeight;

		s_Framebuffer = Hazel::HazelFramebuffer::Create(spec);
		s_Framebuffer->AddResizeCallback([](Hazel::Ref<Hazel::HazelFramebuffer> framebuffer) {
			Hazel::HazelRenderer::Submit([framebuffer]() mutable {});
		});

		Hazel::PipelineSpecification pipelineSpecification;
		pipelineSpecification.Layout = {
			{ Hazel::ShaderDataType::Float3, "a_Position" },
			{ Hazel::ShaderDataType::Float3, "a_Normal" },
			{ Hazel::ShaderDataType::Float3, "a_Tangent" },
			{ Hazel::ShaderDataType::Float3, "a_Binormal" },
			{ Hazel::ShaderDataType::Float2, "a_TexCoord" },
		};
		pipelineSpecification.Shader = Hazel::HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");

		Hazel::RenderPassSpecification renderPassSpec;
		renderPassSpec.TargetFramebuffer = s_Framebuffer;
		pipelineSpecification.RenderPass = Hazel::RenderPass::Create(renderPassSpec);
		s_MeshPipeline = Hazel::Pipeline::Create(pipelineSpecification);
	}

	{
		Hazel::HazelFramebufferSpecification spec;
		spec.SwapChainTarget = true;
		Hazel::Ref<Hazel::HazelFramebuffer> framebuffer = Hazel::HazelFramebuffer::Create(spec);

		Hazel::PipelineSpecification pipelineSpecification;
		pipelineSpecification.Layout = {
			{ Hazel::ShaderDataType::Float3, "a_Position" },
			{ Hazel::ShaderDataType::Float3, "a_Normal" },
			{ Hazel::ShaderDataType::Float3, "a_Tangent" },
			{ Hazel::ShaderDataType::Float3, "a_Binormal" },
			{ Hazel::ShaderDataType::Float2, "a_TexCoord" },
		};
		pipelineSpecification.Shader = Hazel::HazelRenderer::GetShaderLibrary()->Get("Texture");

		Hazel::RenderPassSpecification renderPassSpec;
		renderPassSpec.TargetFramebuffer = framebuffer;
		pipelineSpecification.RenderPass = Hazel::RenderPass::Create(renderPassSpec);
		s_CompositePipeline = Hazel::Pipeline::Create(pipelineSpecification);
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

	s_QuadVertexBuffer = Hazel::VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
	uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
	s_QuadIndexBuffer = Hazel::IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

	Hazel::HazelRenderer::Submit([=]() {});
	{
		auto shader = s_CompositePipeline->GetSpecification().Shader;
		auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
		auto DX11Device = DX11Context::Get()->GetDX11Device();
	}

	Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
	****/
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
	// Hazel::HazelRenderer::Submit([=]() mutable {});

	// Log::GetLogger()->info("Timer::GetCurrentTimestamp: {0}", Timer::Get()->GetCurrentTimestamp());

	/**** BEGIN DirectX 11 rendering ****/
	ClearRenderTargetColor(0.1f, 0.2f, 0.4f, 1.0f);

	DX11Context::Get()->SetRasterizerState(DX11CullMode::None);

	Hazel::Ref<DX11Shader> dx11Shader = s_Pipeline->GetSpecification().Shader.As<DX11Shader>();

	dx11Shader->GetVertexShader()->Bind();
	dx11Shader->GetPixelShader()->Bind();

	uint32_t viewportWidth = Application::Get()->GetWindow()->GetWidth();
	uint32_t viewportHeight = Application::Get()->GetWindow()->GetHeight();
	DX11Context::Get()->SetViewportSize(viewportWidth, viewportHeight);
	DX11TestLayer::GetCamera()->SetViewportSize((float)viewportWidth, (float)viewportHeight);

	// View matrix (Camera)
	glm::mat4 view = DX11TestLayer::GetCamera()->GetViewMatrix();
	glm::mat4 projection = DX11TestLayer::GetCamera()->GetProjectionMatrix();

	// BEGIN render mesh #1
	{
		s_VertexBuffer->Bind();
		s_IndexBuffer->Bind();
		s_Pipeline->Bind();

		// World/Model/Transform matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		// model = glm::scale(model, glm::vec3(1.0f));

		DX11ConstantBufferLayout constantBufferLayout;
		constantBufferLayout.Model = model;
		constantBufferLayout.View = view;
		constantBufferLayout.Projection = projection;
		constantBufferLayout.Time = (uint32_t)(Timer::Get()->GetCurrentTimestamp() * 1000.0f);
		// Log::GetLogger()->info("s_ConstantBufferLayout.Time: {0}", constantBufferLayout.Time);
		s_ConstantBuffer->Update(&constantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		uint32_t startVertexIndex = 0;
		uint32_t startIndexLocation = 0;
		// DX11Renderer::DrawTriangleStrip(s_VertexBuffer->GetVertexCount(), startVertexIndex);
		DX11Renderer::DrawIndexedTriangleList(s_IndexBuffer->GetIndexCount(), startVertexIndex, startIndexLocation);
	}
	// END render mesh #1

	// BEGIN render mesh #2
	{
		s_VertexBuffer->Bind();
		s_IndexBuffer->Bind();
		s_Pipeline->Bind();

		// World/Model/Transform matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		// model = glm::scale(model, glm::vec3(1.0f));

		DX11ConstantBufferLayout constantBufferLayout;
		constantBufferLayout.Model = model;
		constantBufferLayout.View = view;
		constantBufferLayout.Projection = projection;
		constantBufferLayout.Time = (uint32_t)(Timer::Get()->GetCurrentTimestamp() * 1000.0f);
		s_ConstantBuffer->Update(&constantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		uint32_t startVertexIndex = 0;
		uint32_t startIndexLocation = 0;
		// DX11Renderer::DrawTriangleStrip(s_VertexBuffer->GetVertexCount(), startVertexIndex);
		DX11Renderer::DrawIndexedTriangleList(s_IndexBuffer->GetIndexCount(), startVertexIndex, startIndexLocation);
	}
	// END render mesh #2

	// BEGIN render mesh #3
	{
		s_VertexBuffer->Bind();
		s_IndexBuffer->Bind();
		s_Pipeline->Bind();

		// World/Model/Transform matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		// model = glm::scale(model, glm::vec3(1.0f));

		DX11ConstantBufferLayout constantBufferLayout;
		constantBufferLayout.Model = model;
		constantBufferLayout.View = view;
		constantBufferLayout.Projection = projection;
		constantBufferLayout.Time = (uint32_t)(Timer::Get()->GetCurrentTimestamp() * 1000.0f);
		s_ConstantBuffer->Update(&constantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		uint32_t startVertexIndex = 0;
		uint32_t startIndexLocation = 0;
		// DX11Renderer::DrawTriangleStrip(s_VertexBuffer->GetVertexCount(), startVertexIndex);
		DX11Renderer::DrawIndexedTriangleList(s_IndexBuffer->GetIndexCount(), startVertexIndex, startIndexLocation);
	}
	// END render mesh #3

	for (auto& mesh : s_Meshes)
	{
		RenderMesh(mesh, camera);
	}

	s_Meshes.clear();

	//	Log::GetLogger()->info("Elapsed time: {0}, delta time: {1}, App Window size: [{2}x{3}]",
	//		Timer::Get()->GetCurrentTimestamp(), Timer::Get()->GetDeltaTime(),
	//		Application::Get()->GetWindow()->GetWidth(), Application::Get()->GetWindow()->GetHeight());

	/**** END DirectX 11 rendering ****/

	/****
	Hazel::HazelRenderer::Submit([=](){});

	Hazel::Ref<DX11Context> dx11Context = DX11Context::Get();

	// Begin Render Pass

	// BEGIN ImGui Render Pass
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
		ImGui_ImplDX11_RenderDrawData(main_draw_data);

		// End Vulkan Command Buffer
	}
	// END ImGui Render Pass

	// Vulkan CmdExecuteCommands
	// Vulkan CmdEndRenderPass
	****/
}

void DX11Renderer::BeginFrame()
{
	// Hazel::HazelRenderer::Submit([]() {});
	// Hazel::Ref<DX11Context> context = DX11Context::Get();

	// TODO
}

void DX11Renderer::EndFrame()
{
	// Hazel::HazelRenderer::Submit([]() {});
}

void DX11Renderer::BeginRenderPass(const Hazel::Ref<Hazel::RenderPass>& renderPass)
{
	// Hazel::HazelRenderer::Submit([]() {});

	BeginFrame();

	// TODO
}

void DX11Renderer::EndRenderPass()
{
	// Hazel::HazelRenderer::Submit([]() {});
}

void DX11Renderer::SubmitFullscreenQuad(Hazel::Ref<Hazel::Pipeline> pipeline, Hazel::Ref<Material> material)
{
	// Hazel::HazelRenderer::Submit([]() {});
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

// BEGIN methods from DX11Context
void DX11Renderer::ClearRenderTargetColor(float red, float green, float blue, float alpha)
{
	FLOAT clear_color[] = { red, green, blue, alpha };

	std::shared_ptr<DX11SwapChain> dx11SwapChain = DX11Context::Get()->GetSwapChain();

	DX11Context::Get()->GetDX11DeviceContext()->ClearRenderTargetView(dx11SwapChain->GetRenderTargetView(), clear_color);
	DX11Context::Get()->GetDX11DeviceContext()->ClearDepthStencilView(dx11SwapChain->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	DX11Context::Get()->GetDX11DeviceContext()->OMSetRenderTargets(1, &dx11SwapChain->m_DX11RenderTargetView, DX11Context::Get()->GetSwapChain()->GetDepthStencilView());
}

void DX11Renderer::ClearRenderTargetColor(Hazel::Ref<DX11Texture2D> renderTarget, float red, float green, float blue, float alpha)
{
	if (renderTarget->GetType() != DX11Texture2D::Type::RenderTarget) return;
	FLOAT clear_color[] = { red, green, blue, alpha };
	DX11Context::Get()->GetDX11DeviceContext()->ClearRenderTargetView(renderTarget->GetRenderTargetView(), clear_color);
}

void DX11Renderer::ClearDepthStencil()
{
	std::shared_ptr<DX11SwapChain> dx11SwapChain = DX11Context::Get()->GetSwapChain();

	DX11Context::Get()->GetDX11DeviceContext()->ClearDepthStencilView(dx11SwapChain->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void DX11Renderer::ClearDepthStencil(Hazel::Ref<DX11Texture2D> depthStencil)
{
	if (depthStencil->GetType() != DX11Texture2D::Type::DepthStencil) return;
	DX11Context::Get()->GetDX11DeviceContext()->ClearDepthStencilView(depthStencil->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void DX11Renderer::DrawTriangleList(uint32_t vertexCount, uint32_t startVertexIndex)
{
	DX11Context::Get()->GetDX11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DX11Context::Get()->GetDX11DeviceContext()->Draw((UINT)vertexCount, (UINT)startVertexIndex);
}

void DX11Renderer::DrawIndexedTriangleList(uint32_t indexCount, uint32_t startVertexIndex, uint32_t startIndexLocation)
{
	DX11Context::Get()->GetDX11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DX11Context::Get()->GetDX11DeviceContext()->DrawIndexed((UINT)indexCount, (UINT)startIndexLocation, (UINT)startVertexIndex);
}

void DX11Renderer::DrawTriangleStrip(uint32_t vertexCount, uint32_t startVertexIndex)
{
	DX11Context::Get()->GetDX11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	DX11Context::Get()->GetDX11DeviceContext()->Draw((UINT)vertexCount, (UINT)startVertexIndex);
}
