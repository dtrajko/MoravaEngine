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
#include "DX11VertexLayout.h"

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

static std::vector<RenderObject> s_RenderObjects;

static Hazel::Submesh* s_SelectedSubmesh;
static glm::mat4* s_Transform_ImGuizmo = nullptr;

// temporary DX11 objects
static Hazel::Ref<DX11VertexBuffer> s_VertexBuffer;
static Hazel::Ref<DX11IndexBuffer> s_IndexBuffer;

static Hazel::Ref<Hazel::Pipeline> s_PipelineIlluminated;
static Hazel::Ref<Hazel::Pipeline> s_PipelineUnlit;

static Hazel::Ref<DX11ConstantBuffer> s_ConstantBuffer;
static DX11ConstantBufferLayout s_ConstantBufferLayout;

static glm::vec3 s_LightPosition;
static glm::vec3 s_LightDirection;

static Hazel::Ref<DX11Texture2D> s_RenderTarget;
static Hazel::Ref<DX11Texture2D> s_DepthStencil;


void DX11Renderer::SubmitMesh(RenderObject renderObject)
{
	// Temporary code - populate selected submesh
	std::vector<Hazel::Submesh> submeshes = renderObject.Mesh->GetSubmeshes();
	s_SelectedSubmesh = &submeshes.at(0);

	s_RenderObjects.push_back(renderObject);
}

void DX11Renderer::OnResize(uint32_t width, uint32_t height)
{
	Log::GetLogger()->info("DX11Renderer::OnResize({0}, {1})", width, height);

	s_RenderTarget->Release();
	s_DepthStencil->Release();

	s_ViewportWidth = Application::Get()->GetWindow()->GetWidth(); // should we use framebufferSpec.Width?
	s_ViewportHeight = Application::Get()->GetWindow()->GetHeight(); // should we use framebufferSpec.Height?

	s_RenderTarget = Hazel::Ref<DX11Texture2D>::Create(glm::vec2((float)s_ViewportWidth, (float)s_ViewportHeight), DX11Texture2D::Type::RenderTarget);
	s_DepthStencil = Hazel::Ref<DX11Texture2D>::Create(glm::vec2((float)s_ViewportWidth, (float)s_ViewportHeight), DX11Texture2D::Type::DepthStencil);

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

	Hazel::HazelFramebufferSpecification framebufferSpec{};
	framebufferSpec.ClearColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	framebufferSpec.DebugName = "DX11 Framebuffer specification";
	framebufferSpec.Width = Application::Get()->GetWindow()->GetWidth();
	framebufferSpec.Height = Application::Get()->GetWindow()->GetHeight();
	framebufferSpec.NoResize = false;
	framebufferSpec.Samples = 1; // TODO: for mipmaps? what is the optimal number?
	framebufferSpec.Scale = 1.0f;
	framebufferSpec.SwapChainTarget = true; // render to screen or to offscreen render target
	framebufferSpec.Attachments = framebufferAttachmentSpecification;

	Hazel::RenderPassSpecification renderPassSpecification{};
	renderPassSpecification.DebugName = "DX11 Render Pass specificartion";
	renderPassSpecification.TargetFramebuffer = Hazel::HazelFramebuffer::Create(framebufferSpec);

	Hazel::PipelineSpecification pipelineSpecIlluminated{};
	pipelineSpecIlluminated.DebugName = "DX11 Pipeline specification";
	pipelineSpecIlluminated.Layout = Hazel::VertexBufferLayout{};
	pipelineSpecIlluminated.RenderPass = Hazel::RenderPass::Create(renderPassSpecification);

	MoravaShaderSpecification moravaShaderSpecIlluminated;
	moravaShaderSpecIlluminated.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
	moravaShaderSpecIlluminated.VertexShaderPath = "Shaders/HLSL/DirLightVertexShader.hlsl";
	moravaShaderSpecIlluminated.PixelShaderPath = "Shaders/HLSL/DirLightPixelShader.hlsl";
	moravaShaderSpecIlluminated.ForceCompile = false;
	pipelineSpecIlluminated.Shader = ResourceManager::CreateOrLoadShader(moravaShaderSpecIlluminated);

	s_PipelineIlluminated = Hazel::Pipeline::Create(pipelineSpecIlluminated);

	// ---- BEGIN Unlit Pipeline ----
	Hazel::PipelineSpecification pipelineSpecUnlit;
	pipelineSpecUnlit.DebugName = "DX11 Unlit Pipeline specification";
	pipelineSpecUnlit.Layout = Hazel::VertexBufferLayout{};
	pipelineSpecUnlit.RenderPass = Hazel::RenderPass::Create(renderPassSpecification);

	MoravaShaderSpecification moravaShaderSpecUnlit;
	moravaShaderSpecUnlit.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
	moravaShaderSpecUnlit.VertexShaderPath = "Shaders/HLSL/UnlitVertexShader.hlsl";
	moravaShaderSpecUnlit.PixelShaderPath = "Shaders/HLSL/UnlitPixelShader.hlsl";
	moravaShaderSpecUnlit.ForceCompile = false;
	pipelineSpecUnlit.Shader = ResourceManager::CreateOrLoadShader(moravaShaderSpecUnlit);

	s_PipelineUnlit = Hazel::Pipeline::Create(pipelineSpecUnlit);
	// ---- END Unlit Pipeline ----

	glm::vec3 positionList[] =
	{
		// POSITION      
		// ------- X ---- Y ---- Z
		// Front face
		glm::vec3(-0.5f, -0.5f, -0.5f), // POS0
		glm::vec3(-0.5f,  0.5f, -0.5f), // POS1
		glm::vec3( 0.5f,  0.5f, -0.5f), // POS2
		glm::vec3( 0.5f, -0.5f, -0.5f), // POS3

		// Back face
		glm::vec3( 0.5f, -0.5f,  0.5f), // POS4
		glm::vec3( 0.5f,  0.5f,  0.5f), // POS5
		glm::vec3(-0.5f,  0.5f,  0.5f), // POS6
		glm::vec3(-0.5f, -0.5f,  0.5f), // POS7
	};

	glm::vec2 texcoordList[] =
	{
		// TEXCOORD
		// ------ U --- V
		glm::vec2(0.0f, 0.0f),
		glm::vec2(0.0f, 1.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(1.0f, 1.0f),
	};

	DX11VertexLayout vertexList[] =
	{
		// ----------------- POSITION XYZ ---------- NORMAL XYZ ----------- TANGENT XYZ ---------- BINORMAL XYZ -------- TEXCOORD UV
		// DX11VertexLayout{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }, { -0.8f, -0.8f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, }, // VERTEX #0
		// DX11VertexLayout{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { -0.8f,  0.8f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, }, // VERTEX #1
		// DX11VertexLayout{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, {  0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, }, // VERTEX #2
		// DX11VertexLayout{ {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f }, {  0.8f,  0.8f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, }, // VERTEX #3

		// DX11VertexLayout{ {  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f }, {  0.8f,  0.8f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, }, // VERTEX #4
		// DX11VertexLayout{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, {  0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, }, // VERTEX #5
		// DX11VertexLayout{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { -0.8f,  0.8f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, }, // VERTEX #6
		// DX11VertexLayout{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f }, { -0.8f, -0.8f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, }, // VERTEX #7

		// front side
		DX11VertexLayout{ positionList[0], glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[1], glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[2], glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[3], glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// back side
		DX11VertexLayout{ positionList[4], glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[5], glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[6], glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[7], glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// top side
		DX11VertexLayout{ positionList[1], glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[6], glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[5], glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[2], glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// bottom side
		DX11VertexLayout{ positionList[7], glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[0], glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[3], glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[4], glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// right side
		DX11VertexLayout{ positionList[3], glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[2], glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[5], glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[4], glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// left side
		DX11VertexLayout{ positionList[7], glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[6], glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[1], glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[0], glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
	};

	// temporary DX11 objects and data structures
	uint32_t vertexStride = sizeof(DX11VertexLayout);
	uint32_t vertexCount = ARRAYSIZE(vertexList);
	s_VertexBuffer = Hazel::Ref<DX11VertexBuffer>::Create(vertexList, vertexStride, vertexCount);

	uint32_t indexList[] =
	{
		// Front side
		 0,  1,  2, // First triangle
		 2,  3,  0, // Second triangle
		// Back side
		 4,  5,  6,
		 6,  7,  4,
		// Top side
		 8,  9, 10,
		10, 11,  8,
		// Bottom side
		12, 13, 14,
		14, 15, 12,
		// Right side
		16, 17, 18,
		18, 19, 16,
		// Left side
		20, 21, 22,
		22, 23, 20,
	};

	uint32_t indexCount = ARRAYSIZE(indexList);
	s_IndexBuffer = Hazel::Ref<DX11IndexBuffer>::Create(indexList, (uint32_t)(indexCount * sizeof(uint32_t)));

	s_ConstantBuffer = Hazel::Ref<DX11ConstantBuffer>::Create(&s_ConstantBufferLayout, sizeof(DX11ConstantBufferLayout));

	// Create a render target texture and a depth stencil texture
	s_ViewportWidth = Application::Get()->GetWindow()->GetWidth(); // should we use framebufferSpec.Width?
	s_ViewportHeight = Application::Get()->GetWindow()->GetHeight(); // should we use framebufferSpec.Height?

	s_RenderTarget = Hazel::Ref<DX11Texture2D>::Create(glm::vec2((float)s_ViewportWidth, (float)s_ViewportHeight), DX11Texture2D::Type::RenderTarget);
	s_DepthStencil = Hazel::Ref<DX11Texture2D>::Create(glm::vec2((float)s_ViewportWidth, (float)s_ViewportHeight), DX11Texture2D::Type::DepthStencil);

	/**** END DirectX 11 Init (from DX11TestLayer::OnAttach) ****/

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

static void CompositeRenderPass(VkCommandBufferInheritanceInfo& inheritanceInfo)
{
	Hazel::Ref<DX11Context> dx11Context = Application::Get()->GetWindow()->GetRenderContext().As<DX11Context>();
	// DX11SwapChain& swapChain = context->GetSwapChain();

	// TODO
}

void DX11Renderer::Update()
{
	s_ViewportWidth = 1280;
	s_ViewportHeight = 720;

	// Load constant buffer content to each material
	for (size_t m = 0; m < DX11TestLayer::s_ListMaterials.size(); m++)
	{
		DX11TestLayer::s_ListMaterials[m]->SetData(&s_ConstantBufferLayout, sizeof(DX11ConstantBufferLayout));
	}
}

// TODO: Temporary method until composite rendering is enabled
void DX11Renderer::Draw(Hazel::HazelCamera* camera)
{
	Update(); // Ideally, it should be called separately, before the Draw() method

	// Hazel::HazelRenderer::Submit([=]() mutable {});

	// Log::GetLogger()->info("Timer::GetCurrentTimestamp: {0}", Timer::Get()->GetCurrentTimestamp());

	/**** BEGIN DirectX 11 rendering ****/
	ClearRenderTargetColorSwapChain(0.1f, 0.2f, 0.4f, 1.0f);
	ClearDepthStencilSwapChain();

	DX11Context::Get()->SetRasterizerState(DX11CullMode::None);

	// Redirect rendering from swapchain (window) to framebuffer render targets
	// SetRenderTarget(s_RenderTarget, s_DepthStencil);

	Hazel::Ref<DX11Shader> dx11Shader = s_PipelineIlluminated->GetSpecification().Shader.As<DX11Shader>();

	uint32_t viewportWidth = Application::Get()->GetWindow()->GetWidth();
	uint32_t viewportHeight = Application::Get()->GetWindow()->GetHeight();
	DX11Context::Get()->SetViewportSize(viewportWidth, viewportHeight);
	DX11TestLayer::GetCamera()->SetViewportSize((float)viewportWidth, (float)viewportHeight);

	s_LightPosition.x = 0.0f;
	s_LightPosition.y = sin(Timer::Get()->GetCurrentTimestamp() * 0.5f) * 58.0f;
	s_LightPosition.z = cos(Timer::Get()->GetCurrentTimestamp() * 0.5f) * 58.0f;

	s_LightDirection = glm::normalize(s_LightPosition - glm::vec3(0.0f, 6.0f, 0.0f));

	s_ConstantBufferLayout.Projection = DX11TestLayer::GetCamera()->GetProjectionMatrix();
	s_ConstantBufferLayout.View = DX11TestLayer::GetCamera()->GetViewMatrix();
	s_ConstantBufferLayout.LightDirection = s_LightDirection;
	s_ConstantBufferLayout.CameraPosition = DX11TestLayer::GetCamera()->GetPosition();
	s_ConstantBufferLayout.LightPosition = s_LightPosition;
	s_ConstantBufferLayout.LightRadius = 1000.0f;
	s_ConstantBufferLayout.Time = (uint32_t)(Timer::Get()->GetCurrentTimestamp() * 1000.0f);

	// BEGIN render skybox
	{
		Hazel::Ref<DX11Shader> dx11ShaderUnlit = s_PipelineUnlit->GetSpecification().Shader.As<DX11Shader>();

		dx11ShaderUnlit->GetVertexShader()->Bind();
		dx11ShaderUnlit->GetPixelShader()->Bind();

		Hazel::Ref<DX11VertexBuffer> skyboxVB = DX11TestLayer::s_SkyboxSphere->GetVertexBuffer().As<DX11VertexBuffer>();
		Hazel::Ref<DX11IndexBuffer> skyboxIB = DX11TestLayer::s_SkyboxSphere->GetIndexBuffer().As<DX11IndexBuffer>();
		skyboxVB->Bind();
		skyboxIB->Bind();
		s_PipelineUnlit->Bind();

		glm::mat4 skyboxTransform = glm::mat4(1.0f);
		skyboxTransform = glm::scale(skyboxTransform, glm::vec3(60.0f));
		skyboxTransform = glm::rotate(skyboxTransform, glm::radians(Timer::Get()->GetCurrentTimestamp() * 10.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// World/Model/Transform matrix
		s_ConstantBufferLayout.Model = skyboxTransform;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11ShaderUnlit->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11ShaderUnlit->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<DX11Texture2D>> textures;
		Hazel::Ref<Hazel::HazelTexture2D> textureDiffuse = ResourceManager::LoadHazelTexture2D("Textures/PardCode/umhlanga_sunrise_4k.jpg");
		Hazel::Ref<Hazel::HazelTexture2D> textureNormal = ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
		textures.push_back(textureDiffuse.As<DX11Texture2D>());
		textures.push_back(textureNormal.As<DX11Texture2D>());

		dx11ShaderUnlit->GetVertexShader()->SetTextures(textures);
		dx11ShaderUnlit->GetPixelShader()->SetTextures(textures);

		uint32_t startVertexIndex = 0;
		uint32_t startIndexLocation = 0;
		DX11Renderer::DrawIndexedTriangleList(skyboxIB->GetIndexCount(), startVertexIndex, startIndexLocation);
	}
	// END Render skybox

	// BEGIN render Mesh Light Source
	{
		dx11Shader->GetVertexShader()->Bind();
		dx11Shader->GetPixelShader()->Bind();

		Hazel::Ref<DX11VertexBuffer> vertexBuffer = DX11TestLayer::s_MeshLight->GetVertexBuffer().As<DX11VertexBuffer>();
		Hazel::Ref<DX11IndexBuffer> indexBuffer = DX11TestLayer::s_MeshLight->GetIndexBuffer().As<DX11IndexBuffer>();
		vertexBuffer->Bind();
		indexBuffer->Bind();
		s_PipelineIlluminated->Bind(); // TODO: DX11TestLayer::s_Mesh->GetPipeline()->Bind();

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, s_LightPosition);
		model = glm::scale(model, glm::vec3(2.0f));

		s_ConstantBufferLayout.Model = model;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<DX11Texture2D>> textures;
		Hazel::Ref<Hazel::HazelTexture2D> textureDiffuse = ResourceManager::LoadHazelTexture2D("Textures/PardCode/gold.png");
		Hazel::Ref<Hazel::HazelTexture2D> textureNormal = ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");

		textures.push_back(textureDiffuse.As<DX11Texture2D>());
		textures.push_back(textureNormal.As<DX11Texture2D>());

		dx11Shader->GetVertexShader()->SetTextures(textures);
		dx11Shader->GetPixelShader()->SetTextures(textures);

		uint32_t indexCount = indexBuffer->GetIndexCount();
		uint32_t startVertexIndex = 0;
		uint32_t startIndexLocation = 0;
		DX11Renderer::DrawIndexedTriangleList(indexCount, startVertexIndex, startIndexLocation);
	}
	// END render Mesh Light Source

	// BEGIN render mesh #1
	{
		s_VertexBuffer->Bind();
		s_IndexBuffer->Bind();
		s_PipelineIlluminated->Bind();

		// World/Model/Transform matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-4.0f, 2.0f, 4.0f));
		model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(2.0f));

		s_ConstantBufferLayout.Model = model;
		// Log::GetLogger()->info("s_ConstantBufferLayout.Time: {0}", constantBufferLayout.Time);
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<DX11Texture2D>> textures;
		Hazel::Ref<Hazel::HazelTexture2D> textureDiffuse = ResourceManager::LoadHazelTexture2D("Textures/PardCode/wood.jpg");
		Hazel::Ref<Hazel::HazelTexture2D> textureNormal = ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
		textures.push_back(textureDiffuse.As<DX11Texture2D>());
		textures.push_back(textureNormal.As<DX11Texture2D>());

		dx11Shader->GetVertexShader()->SetTextures(textures);
		dx11Shader->GetPixelShader()->SetTextures(textures);

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
		s_PipelineIlluminated->Bind();

		// World/Model/Transform matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 2.0f, 4.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(2.0f));

		s_ConstantBufferLayout.Model = model;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<DX11Texture2D>> textures;
		Hazel::Ref<Hazel::HazelTexture2D> textureDiffuse = ResourceManager::LoadHazelTexture2D("Textures/PardCode/wood.jpg");
		Hazel::Ref<Hazel::HazelTexture2D> textureNormal = ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
		textures.push_back(textureDiffuse.As<DX11Texture2D>());
		textures.push_back(textureNormal.As<DX11Texture2D>());

		dx11Shader->GetVertexShader()->SetTextures(textures);
		dx11Shader->GetPixelShader()->SetTextures(textures);

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
		s_PipelineIlluminated->Bind();

		// World/Model/Transform matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(4.0f, 2.0f, 4.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * 40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(2.0f));

		s_ConstantBufferLayout.Model = model;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<DX11Texture2D>> textures;
		Hazel::Ref<Hazel::HazelTexture2D> textureDiffuse = ResourceManager::LoadHazelTexture2D("Textures/PardCode/wood.jpg");
		Hazel::Ref<Hazel::HazelTexture2D> textureNormal = ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
		textures.push_back(textureDiffuse.As<DX11Texture2D>());
		textures.push_back(textureNormal.As<DX11Texture2D>());

		dx11Shader->GetVertexShader()->SetTextures(textures);
		dx11Shader->GetPixelShader()->SetTextures(textures);

		uint32_t startVertexIndex = 0;
		uint32_t startIndexLocation = 0;
		// DX11Renderer::DrawTriangleStrip(s_VertexBuffer->GetVertexCount(), startVertexIndex);
		DX11Renderer::DrawIndexedTriangleList(s_IndexBuffer->GetIndexCount(), startVertexIndex, startIndexLocation);
	}
	// END render mesh #3

	// BEGIN render DX11Mesh
	{
		Hazel::Ref<DX11VertexBuffer> dx11VertexBuffer = DX11TestLayer::s_Mesh->GetVertexBuffer().As<DX11VertexBuffer>();
		Hazel::Ref<DX11IndexBuffer> dx11IndexBuffer = DX11TestLayer::s_Mesh->GetIndexBuffer().As<DX11IndexBuffer>();
		dx11VertexBuffer->Bind();
		dx11IndexBuffer->Bind();
		s_PipelineIlluminated->Bind(); // TODO: DX11TestLayer::s_Mesh->GetPipeline()->Bind();

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 4.5f, 4.0f));
		model = glm::rotate(model, glm::radians(Timer::Get()->GetCurrentTimestamp() * -40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f));

		s_ConstantBufferLayout.Model = model;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<DX11Texture2D>> textures;
		Hazel::Ref<Hazel::HazelTexture2D> textureDiffuse = ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg");
		Hazel::Ref<Hazel::HazelTexture2D> textureNormal = ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg");
		textures.push_back(textureDiffuse.As<DX11Texture2D>());
		textures.push_back(textureNormal.As<DX11Texture2D>());

		dx11Shader->GetVertexShader()->SetTextures(textures);
		dx11Shader->GetPixelShader()->SetTextures(textures);

		uint32_t indexCount = dx11IndexBuffer->GetIndexCount();
		uint32_t startVertexIndex = 0;
		uint32_t startIndexLocation = 0;
		DX11Renderer::DrawIndexedTriangleList(indexCount, startVertexIndex, startIndexLocation);
	}
	// END render DX11Mesh

	// BEGIN render meshes without materials
	for (auto& renderObject : s_RenderObjects)
	{
		RenderMesh(renderObject);
	}

	s_RenderObjects.clear();
	// END render meshes without materials

	// BEGIN render meshes with materials
	for (RenderObject renderObjectWithMaterials : DX11TestLayer::s_RenderObjectsWithMaterials)
	{
		RenderMeshDX11(renderObjectWithMaterials, DX11TestLayer::s_ListMaterials);
	}
	// END render meshes with materials


	// BEGIN DirectX 11 ImGui Render Pass
	{
		// ImGui Dockspace
		bool p_open = true;

		// ShowExampleAppDockSpace(&p_open);
		// ImGui::ShowDemoWindow(&p_open);

		// Window* mainWindow = Application::Get()->GetWindow();
		// UpdateImGuizmo(mainWindow, camera);

		// Rendering
		// ImGui::Render();
		// ImDrawData* main_draw_data = ImGui::GetDrawData();
		// ImGui_ImplDX11_RenderDrawData(main_draw_data);
	}
	// END DirectX 11 ImGui Render Pass

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

void DX11Renderer::RenderMesh(RenderObject renderObject)
{
	Hazel::Ref<Hazel::Pipeline> pipeline;

	if (renderObject.PipelineType == RenderObject::PipelineType::Light)
	{
		pipeline = s_PipelineIlluminated;
	}
	else if (renderObject.PipelineType == RenderObject::PipelineType::Unlit)
	{
		pipeline = s_PipelineUnlit;
	}

	Hazel::Ref<DX11Shader> dx11Shader = pipeline->GetSpecification().Shader.As<DX11Shader>();

	dx11Shader->GetVertexShader()->Bind();
	dx11Shader->GetPixelShader()->Bind();

	Hazel::Ref<DX11VertexBuffer> dx11MeshVB = renderObject.Mesh->GetVertexBuffer().As<DX11VertexBuffer>();
	Hazel::Ref<DX11IndexBuffer> dx11meshIB = renderObject.Mesh->GetIndexBuffer().As<DX11IndexBuffer>();
	dx11MeshVB->Bind();
	dx11meshIB->Bind();
	// Hazel::Ref<DX11Pipeline> dx11Pipeline = renderObject.Mesh->GetPipeline().As<DX11Pipeline>();
	pipeline->Bind();

	uint32_t textureIndex = 0;
	for (Hazel::Submesh submesh : renderObject.Mesh->GetSubmeshes())
	{
		// World/Model/Transform matrix
		s_ConstantBufferLayout.Model = renderObject.Transform;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<DX11Texture2D>> textures;
		textures.push_back(renderObject.Textures.at(textureIndex++).As<DX11Texture2D>()); // Albedo Map
		textures.push_back(renderObject.Textures.at(textureIndex++).As<DX11Texture2D>()); // Normal Map

		dx11Shader->GetVertexShader()->SetTextures(textures);
		dx11Shader->GetPixelShader()->SetTextures(textures);

		// DX11Renderer::DrawTriangleStrip(s_VertexBuffer->GetVertexCount(), startVertexIndex);
		DX11Renderer::DrawIndexedTriangleList(submesh.IndexCount, submesh.BaseVertex, submesh.BaseIndex);
	}
}

void DX11Renderer::RenderMeshDX11(RenderObject renderObject, const std::vector<Hazel::Ref<DX11Material>>& listMaterials)
{
	// Drawing meshes with materials

	// TODO: consider using this high level API, perhaps in a separate method https://youtu.be/cQnPUhEy6MU?t=450
	// for each (material in materials)
	//   1. SetMaterial(material)
	//   2. SetVertexBuffer(mesh->GetVertexBuffer())
	//   3. SetIndexBuffer(mesh->GetIndexBuffer())
	//   4. DrawIndexedTriangleList(slit.NumIndices, 0, slot.StartIndex)

	Hazel::Ref<Hazel::Pipeline> pipeline;

	if (renderObject.PipelineType == RenderObject::PipelineType::Light)
	{
		pipeline = s_PipelineIlluminated;
	}
	else if (renderObject.PipelineType == RenderObject::PipelineType::Unlit)
	{
		pipeline = s_PipelineUnlit;
	}

	Hazel::Ref<DX11Mesh> dx11Mesh = renderObject.MeshDX11;

	// Hazel::Ref<DX11VertexBuffer> dx11VertexBuffer = dx11Mesh->GetVertexBuffer().As<DX11VertexBuffer>();
	// Hazel::Ref<DX11IndexBuffer> dx11IndexBuffer = dx11Mesh->GetIndexBuffer().As<DX11IndexBuffer>();
	// dx11VertexBuffer->Bind();
	// dx11IndexBuffer->Bind();

	dx11Mesh->GetVertexBuffer()->Bind();
	dx11Mesh->GetIndexBuffer()->Bind();

	pipeline->Bind();

	for (size_t m = 0; m < dx11Mesh->GetNumMaterialSlots(); m++)
	{
		if (m >= listMaterials.size()) break;

		DX11MaterialSlot materialSlot = dx11Mesh->GetMaterialSlot((uint32_t)m);

		Hazel::Ref<DX11Material> material = listMaterials[m];

		material->Bind();

		// World/Model/Transform matrix
		s_ConstantBufferLayout.Model = renderObject.Transform;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		DX11Renderer::DrawIndexedTriangleList((uint32_t)materialSlot.NumIndices, 0, (uint32_t)materialSlot.StartIndex);
	}
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
void DX11Renderer::ClearRenderTargetColorSwapChain(float red, float green, float blue, float alpha)
{
	DX11Context::Get()->GetSwapChain()->ClearRenderTargetColor(red, green, blue, alpha);
}

void DX11Renderer::ClearRenderTargetColor(Hazel::Ref<DX11Texture2D> renderTarget, float red, float green, float blue, float alpha)
{
	if (renderTarget->GetType() != DX11Texture2D::Type::RenderTarget) return;
	FLOAT clear_color[] = { red, green, blue, alpha };
	DX11Context::Get()->GetDX11DeviceContext()->ClearRenderTargetView(renderTarget->GetRenderTargetViewDX11(), clear_color);
}

void DX11Renderer::ClearDepthStencilSwapChain()
{
	std::shared_ptr<DX11SwapChain> dx11SwapChain = DX11Context::Get()->GetSwapChain();

	DX11Context::Get()->GetDX11DeviceContext()->ClearDepthStencilView(dx11SwapChain->GetDepthStencilViewDX11(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void DX11Renderer::ClearDepthStencil(Hazel::Ref<DX11Texture2D> depthStencil)
{
	if (depthStencil->GetType() != DX11Texture2D::Type::DepthStencil) return;
	DX11Context::Get()->GetDX11DeviceContext()->ClearDepthStencilView(depthStencil->GetDepthStencilViewDX11(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void DX11Renderer::SetRenderTarget(Hazel::Ref<DX11Texture2D> renderTarget, Hazel::Ref<DX11Texture2D> depthStencil)
{
	if (renderTarget->GetType() != DX11Texture2D::Type::RenderTarget) return;
	if (depthStencil->GetType() != DX11Texture2D::Type::DepthStencil) return;

	DX11Context::Get()->GetDX11DeviceContext()->OMSetRenderTargets(1, &renderTarget->m_RenderTargetViewDX11, depthStencil->m_DepthStencilViewDX11);
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
