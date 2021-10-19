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

#include "Hazel/Editor/SceneHierarchyPanel.h"
#include "Hazel/Renderer/HazelRenderer.h"

#include "Material/MaterialLibrary.h"
#include "Editor/MaterialEditorPanel.h"

// ImGui includes
#if !defined(IMGUI_IMPL_API)
#define IMGUI_IMPL_API
#endif
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

// temporary DX11 objects
static Hazel::Ref<DX11VertexBuffer> s_VertexBufferCube;
static Hazel::Ref<DX11IndexBuffer> s_IndexBufferCube;

static Hazel::Ref<DX11VertexBuffer> s_VertexBufferQuad;
static Hazel::Ref<DX11IndexBuffer> s_IndexBufferQuad;

static Hazel::Ref<Hazel::Pipeline> s_PipelineIlluminated;
static Hazel::Ref<Hazel::Pipeline> s_PipelineUnlit;

static DX11ConstantBufferLayout s_ConstantBufferLayout;
static DX11ConstantBufferLayout s_ConstantBufferLayoutDeferred;

static Hazel::Ref<DX11ConstantBuffer> s_ConstantBuffer;

static glm::vec3 s_LightPosition;
static glm::vec3 s_LightDirection;

static Hazel::Ref<DX11Texture2D> s_RenderTarget;
static Hazel::Ref<DX11Texture2D> s_DepthStencil;

static bool s_DeferredRenderingEnabled = true;

/**** BEGIN variables from the EnvMapEditorLayer ****/

static EventCooldown s_ResizeViewport = { 0.0f, 1.0f };

static Hazel::Ref<Hazel::HazelTexture2D> s_CheckerboardTexture;

static Hazel::Ref<EnvMapMaterial> s_DefaultMaterial;
static Hazel::Ref<EnvMapMaterial> s_LightMaterial;

static Hazel::Ref<MoravaFramebuffer> s_RenderFramebuffer;
static Hazel::Ref<MoravaFramebuffer> s_PostProcessingFramebuffer;

/**** BEGIN variables from Scene.cpp ****/

// ImGuizmo
static glm::mat4* s_ImGuizmoTransform = nullptr;
static Hazel::Submesh* s_SelectedSubmesh;
static SelectionMode s_SelectionMode = SelectionMode::Entity;

/**** END variables from Scene.cpp ****/


struct Viewport
{
	int X;
	int Y;
	int Width;
	int Height;
	int MouseX;
	int MouseY;
};

static Viewport s_ImGuiViewport;
static Viewport s_ImGuiViewportEnvMap;
static bool s_ViewportPanelMouseOver = false;
static bool s_ViewportPanelFocused = false;
static glm::vec2 s_ImGuiViewportMain;


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

	s_ViewportWidth = Application::Get()->GetWindow()->GetWidth(); // should we use framebufferSpec.Width?
	s_ViewportHeight = Application::Get()->GetWindow()->GetHeight(); // should we use framebufferSpec.Height?

	if (s_DeferredRenderingEnabled)
	{
		s_RenderTarget->Release();
		s_DepthStencil->Release();

		s_RenderTarget = Hazel::Ref<DX11Texture2D>::Create(glm::vec2((float)s_ViewportWidth, (float)s_ViewportHeight), DX11Texture2D::Type::RenderTarget);
		s_DepthStencil = Hazel::Ref<DX11Texture2D>::Create(glm::vec2((float)s_ViewportWidth, (float)s_ViewportHeight), DX11Texture2D::Type::DepthStencil);
	}

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

	CreateCube();

	if (s_DeferredRenderingEnabled)
	{
		CreateQuad(); // to render a render target framebuffer to
	}

	s_ConstantBuffer = Hazel::Ref<DX11ConstantBuffer>::Create(&s_ConstantBufferLayout, sizeof(DX11ConstantBufferLayout));

	// Create a render target texture and a depth stencil texture
	s_ViewportWidth = Application::Get()->GetWindow()->GetWidth(); // should we use framebufferSpec.Width?
	s_ViewportHeight = Application::Get()->GetWindow()->GetHeight(); // should we use framebufferSpec.Height?

	if (s_DeferredRenderingEnabled)
	{
		s_RenderTarget = Hazel::Ref<DX11Texture2D>::Create(glm::vec2((float)s_ViewportWidth, (float)s_ViewportHeight), DX11Texture2D::Type::RenderTarget);
		s_DepthStencil = Hazel::Ref<DX11Texture2D>::Create(glm::vec2((float)s_ViewportWidth, (float)s_ViewportHeight), DX11Texture2D::Type::DepthStencil);
	}

	/**** END DirectX 11 Init (from DX11TestLayer::OnAttach) ****/

	/**** BEGIN the code from the EnvMapEditorLayer constructor ****/

	// Create a default material
	s_DefaultMaterial = MaterialLibrary::CreateDefaultMaterial("MAT_DEF");
	MaterialLibrary::AddEnvMapMaterial(s_DefaultMaterial->GetUUID(), s_DefaultMaterial);

	EnvMapEditorLayer::s_DefaultMaterial = s_DefaultMaterial; // try to remove this dependency in SceneHierarchyPanel::DrawComponents

	// Create the light material
	s_LightMaterial = MaterialLibrary::CreateDefaultMaterial("MAT_LIGHT");
	// Load Hazel/Renderer/HazelTexture
	s_LightMaterial->GetAlbedoInput().TextureMap = ResourceManager::LoadHazelTexture2D("Textures/light_bulb.png");
	s_LightMaterial->GetAlbedoInput().UseTexture = true;
	MaterialLibrary::AddEnvMapMaterial(s_LightMaterial->GetUUID(), s_LightMaterial);

	s_CheckerboardTexture = ResourceManager::LoadHazelTexture2D("Textures/Hazel/Checkerboard.png");

	s_ImGuizmoTransform = nullptr;

	s_ResizeViewport = { 0.0f, 1.0f };

	SetupRenderFramebuffer();

	/****END the code from the EnvMapEditorLayer constructor ****/
}

void DX11Renderer::Shutdown()
{
}

void DX11Renderer::SelectEntity(Hazel::Entity e)
{
	Log::GetLogger()->info("DX11Renderer::SelectEntity called!");
}

void DX11Renderer::OnEntityDeleted(Hazel::Entity e)
{
	if (EntitySelection::s_SelectionContext.size())
	{
		if (EntitySelection::s_SelectionContext[0].Entity == e) {
			EntitySelection::s_SelectionContext.clear();
			EnvMapSharedData::s_EditorScene->SetSelectedEntity({});
		}
	}

	Log::GetLogger()->info("DX11Renderer::OnEntityDeleted called!");
}

void DX11Renderer::SetupRenderFramebuffer()
{
	uint32_t width = Application::Get()->GetWindow()->GetWidth();
	uint32_t height = Application::Get()->GetWindow()->GetHeight();

	// Main render target
	s_RenderFramebuffer = MoravaFramebuffer::Create(width, height);
	s_RenderFramebuffer->AddColorAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
	s_RenderFramebuffer->AddDepthAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Depth);
	s_RenderFramebuffer->Generate(width, height);
	
	// Post-processing framebuffer
	s_PostProcessingFramebuffer = MoravaFramebuffer::Create(width, height);
	s_PostProcessingFramebuffer->AddColorAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
	s_PostProcessingFramebuffer->Generate(width, height);
}

void DX11Renderer::CreateCube()
{
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
		// -------------- POSITION XYZ --- NORMAL XYZ ----------------- TANGENT XYZ ---- BINORMAL XYZ --- TEXCOORD UV

		// front side
		DX11VertexLayout{ positionList[0], glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[1], glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[2], glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[3], glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// back side
		DX11VertexLayout{ positionList[4], glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[5], glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[6], glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[7], glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// top side
		DX11VertexLayout{ positionList[1], glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[6], glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[5], glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[2], glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// bottom side
		DX11VertexLayout{ positionList[7], glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[0], glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[3], glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[4], glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// right side
		DX11VertexLayout{ positionList[3], glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[2], glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[5], glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[4], glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
		// left side
		DX11VertexLayout{ positionList[7], glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[6], glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[1], glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[0], glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
	};

	// temporary DX11 objects and data structures
	uint32_t vertexStride = sizeof(DX11VertexLayout);
	uint32_t vertexCount = ARRAYSIZE(vertexList);
	s_VertexBufferCube = Hazel::Ref<DX11VertexBuffer>::Create(vertexList, vertexStride, vertexCount);

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
	s_IndexBufferCube = Hazel::Ref<DX11IndexBuffer>::Create(indexList, (uint32_t)(indexCount * sizeof(uint32_t)));
}

void DX11Renderer::CreateQuad()
{
	glm::vec3 positionList[] =
	{
		// POSITION      
		// ------- X ---- Y ---- Z
		// Front face
		glm::vec3(-1.0f, -1.0f, 0.0f), // POS0
		glm::vec3(-1.0f,  1.0f, 0.0f), // POS1
		glm::vec3( 1.0f,  1.0f, 0.0f), // POS2
		glm::vec3( 1.0f, -1.0f, 0.0f), // POS3
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
		// -------------- POSITION XYZ --- NORMAL XYZ ----------------- TANGENT XYZ ---- BINORMAL XYZ --- TEXCOORD UV
		DX11VertexLayout{ positionList[0], glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[1], },
		DX11VertexLayout{ positionList[1], glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[0], },
		DX11VertexLayout{ positionList[2], glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[2], },
		DX11VertexLayout{ positionList[3], glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f), glm::vec3(0.0f), texcoordList[3], },
	};

	// temporary DX11 objects and data structures
	uint32_t vertexStride = sizeof(DX11VertexLayout);
	uint32_t vertexCount = ARRAYSIZE(vertexList);
	s_VertexBufferQuad = Hazel::Ref<DX11VertexBuffer>::Create(vertexList, vertexStride, vertexCount);

	uint32_t indexList[] =
	{
		// Front side
		0,  1,  2, // First triangle
		2,  3,  0, // Second triangle
	};

	uint32_t indexCount = ARRAYSIZE(indexList);
	s_IndexBufferQuad = Hazel::Ref<DX11IndexBuffer>::Create(indexList, (uint32_t)(indexCount * sizeof(uint32_t)));
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

	// Load constant buffer content to each material (moved from the Update method)
	for (size_t m = 0; m < DX11TestLayer::s_ListMaterials.size(); m++)
	{
		DX11TestLayer::s_ListMaterials[m]->SetData(&s_ConstantBufferLayout, sizeof(DX11ConstantBufferLayout));
	}
}

// TODO: Temporary method until composite rendering is enabled
void DX11Renderer::Draw(Hazel::HazelCamera* camera)
{
	Update(); // Ideally, it should be called separately, before the Draw() method

	DrawToFramebuffer(camera); // Forward rendering, or the geometry stage of deferred rendering

	if (s_DeferredRenderingEnabled)
	{
		ClearRenderTargetColorSwapChain(0.2f, 0.2f, 0.2f, 1.0f); // dark gray
		ClearDepthStencilSwapChain();

		// We don't need this because we use ImGui to display the viewport
		// DrawToScreen(camera); // Deferred rendering (basic, no G-Buffer)
	}
}

void DX11Renderer::RenderImGui()
{
	// BEGIN DirectX 11 ImGui Render Pass
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

		if (DX11TestLayer::s_ShowWindowSceneHierarchy)
		{
			DX11TestLayer::s_SceneHierarchyPanel->OnImGuiRender(&DX11TestLayer::s_ShowWindowSceneHierarchy);
		}

		if (DX11TestLayer::s_ShowWindowAssetManager)
		{
			DX11TestLayer::s_ContentBrowserPanel->OnImGuiRender(&DX11TestLayer::s_ShowWindowAssetManager);
		}

		if (DX11TestLayer::s_ShowWindowMaterialEditor)
		{
			DX11TestLayer::s_MaterialEditorPanel->OnImGuiRender(&DX11TestLayer::s_ShowWindowMaterialEditor);
		}

		if (DX11TestLayer::s_ShowWindowMaterialEditor)
		{
			DisplaySubmeshMaterialSelector(&DX11TestLayer::s_ShowWindowMaterialEditor);
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

			ImGui::Image((void*)(intptr_t)s_RenderTarget->m_ShaderResourceViewDX11, ImVec2{ viewportPanelSize.x, viewportPanelSize.y});

			UpdateImGuizmo();

			auto windowSize = ImGui::GetWindowSize();
			ImVec2 minBound = ImGui::GetWindowPos();

			minBound.x += viewportOffset.x;
			// minBound.y += viewportOffset.y;

			ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
			DX11TestLayer::s_ViewportBounds[0] = { minBound.x, minBound.y };
			DX11TestLayer::s_ViewportBounds[1] = { maxBound.x, maxBound.y };

			DX11TestLayer::s_AllowViewportCameraEvents = ImGui::IsMouseHoveringRect(minBound, maxBound); // EditorLayer
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	// END DirectX 11 ImGui Render Pass
}

void DX11Renderer::DisplaySubmeshMaterialSelector(bool* p_open)
{
	/////////////////////////////////////////////////////////
	//// SELECTION
	/////////////////////////////////////////////////////////
	ImGui::Begin("Selection", p_open);
	{
		ImGui::Text("Selection Mode: ");
		ImGui::SameLine();
		const char* label = s_SelectionMode == SelectionMode::Entity ? "Entity" : "Mesh";
		if (ImGui::Button(label))
		{
			s_SelectionMode = s_SelectionMode == SelectionMode::Entity ? SelectionMode::SubMesh : SelectionMode::Entity;
		}

		std::string entityTag = "N/A";
		std::string meshName = "N/A";
		SubmeshUUID submeshUUID = "N/A";
		Hazel::Entity* entity = nullptr;

		if (EntitySelection::s_SelectionContext.size())
		{
			SelectedSubmesh selectedSubmesh = EntitySelection::s_SelectionContext[0];

			entity = &selectedSubmesh.Entity;
			entityTag = selectedSubmesh.Entity.GetComponent<Hazel::TagComponent>().Tag;
			meshName = (selectedSubmesh.Mesh) ? selectedSubmesh.Mesh->MeshName : "N/A";
			submeshUUID = MaterialLibrary::GetSubmeshUUID(entity, selectedSubmesh.Mesh);
		}

		ImGui::Text("Selected Entity: ");
		ImGui::SameLine();
		ImGui::Text(entityTag.c_str());

		ImGui::Text("Selected Mesh: ");
		ImGui::SameLine();
		ImGui::Text(meshName.c_str());

		// Drop down for selecting a material for a specific submesh
		std::vector<std::string> materialNameStrings;
		int index = 0;
		for (auto& material : MaterialLibrary::s_EnvMapMaterials) {
			materialNameStrings.push_back(material.second->GetName());
		}

		std::string submeshMaterialName = materialNameStrings.size() ? materialNameStrings[0] : "N/A";

		MaterialUUID materialUUID;
		if (MaterialLibrary::s_SubmeshMaterialUUIDs.find(submeshUUID) != MaterialLibrary::s_SubmeshMaterialUUIDs.end()) {
			materialUUID = MaterialLibrary::s_SubmeshMaterialUUIDs.at(submeshUUID);
		}
		int selectedMaterial = -1;

		if (ImGui::BeginCombo("Material", submeshMaterialName.c_str()))
		{
			size_t emm_index = 0;
			for (auto emm_it = MaterialLibrary::s_EnvMapMaterials.begin(); emm_it != MaterialLibrary::s_EnvMapMaterials.end(); emm_it++)
			{
				bool is_selected = (submeshMaterialName == materialNameStrings[emm_index]);
				if (ImGui::Selectable(materialNameStrings.at(emm_index).c_str(), is_selected))
				{
					submeshMaterialName = materialNameStrings[emm_index];
					materialUUID = emm_it->second->GetUUID();
					if (meshName != "N/A" && submeshMaterialName != "N/A" && submeshUUID != "N/A")
					{
						auto sm_it = MaterialLibrary::s_SubmeshMaterialUUIDs.find(submeshUUID);
						if (sm_it != MaterialLibrary::s_SubmeshMaterialUUIDs.end()) {
							sm_it->second = materialUUID;
							Log::GetLogger()->debug("s_SubmeshMaterialUUIDs UPDATE [ SubmeshUUID: '{0}' => MaterialUUID: '{1}', Items: {2} ]",
								submeshUUID, materialUUID, MaterialLibrary::s_SubmeshMaterialUUIDs.size());
							break;
						}
						else {
							MaterialLibrary::s_SubmeshMaterialUUIDs.insert(std::make_pair(submeshUUID, materialUUID));
							Log::GetLogger()->debug("s_SubmeshMaterialUUIDs INSERT [ SubmeshUUID: '{0}' => MaterialUUID: '{1}', Items: {2} ]",
								submeshUUID, materialUUID, MaterialLibrary::s_SubmeshMaterialUUIDs.size());
						}
					}
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
				emm_index++;
			}
			ImGui::EndCombo();
		}
	}
	ImGui::End();
}

void DX11Renderer::DrawComponent(const std::string name)
{
	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	ImGuiTreeNodeFlags treeNodeFlags =
		ImGuiTreeNodeFlags_DefaultOpen |
		ImGuiTreeNodeFlags_Framed |
		ImGuiTreeNodeFlags_SpanAvailWidth |
		ImGuiTreeNodeFlags_AllowItemOverlap |
		ImGuiTreeNodeFlags_FramePadding;

	// auto& component = entity.GetComponent<T>();
	ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4.0f, 4.0f });
	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImGui::Separator();
	bool open = ImGui::TreeNodeEx((void*)typeid(0).hash_code(), treeNodeFlags, name.c_str());
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
		// UIFunction content
		{
			glm::vec3 componentTranslation = glm::vec3(0.0f); // component.Translation
			glm::vec3 componentRotation = glm::vec3(0.0f); // component.Rotation
			glm::vec3 componentScale = glm::vec3(1.0f); // component.Scale

			ImGuiWrapper::DrawVec3Control("Translation", componentTranslation, 0.0f, 80.0f);
			glm::vec3 rotation = glm::degrees(componentRotation);
			ImGuiWrapper::DrawVec3Control("Rotation", rotation, 0.0f, 80.0f);
			// component.Rotation = glm::radians(rotation);
			ImGuiWrapper::DrawVec3Control("Scale", componentScale, 1.0f, 80.0f);
		}

		ImGui::TreePop();
	}

	if (removeComponent) {
		// TODO: remove component
	}
}

void DX11Renderer::UpdateImGuizmo()
{
	// ImGuizmo
	if (DX11TestLayer::s_ImGuizmoType != -1 && EntitySelection::s_SelectionContext.size())
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
		bool snap = DX11TestLayer::s_LeftControlKeyPressed; // ImGuizmo snap enabled
		float snapValue = 5.0f; // Snap to 0.5m for translation/scale
		// Snap to 45 degrees for rotation
		if (DX11TestLayer::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE)
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
				glm::value_ptr(DX11TestLayer::GetCamera()->GetViewMatrix()),
				glm::value_ptr(DX11TestLayer::GetCamera()->GetProjectionMatrix()),
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

void DX11Renderer::DrawEntityNode(const std::string name)
{
	// const char* name = "Unnamed Entity";

	ImGuiTreeNodeFlags flags =
		// ImGuiTreeNodeFlags_Selected |
		ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_SpanAvailWidth;
	bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)0, flags, name.c_str());

	if (ImGui::IsItemClicked())
	{
		Log::GetLogger()->debug("Entity Node clicked!");
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

		// DrawEntitySubmeshes(entity);

		ImGui::TreePop();
	}

	if (entityDeleted) {
		// TODO: delete the entity
	}

	if (entityCloned) {
		// TODO: clone the entity
	}
}

void DX11Renderer::DrawToScreen(Hazel::HazelCamera* camera)
{
	ClearRenderTargetColorSwapChain(0.2f, 0.2f, 0.2f, 1.0f); // dark gray
	ClearDepthStencilSwapChain();

	DX11Context::Get()->SetRasterizerState(DX11CullMode::Front);

	uint32_t viewportWidth = Application::Get()->GetWindow()->GetWidth();
	uint32_t viewportHeight = Application::Get()->GetWindow()->GetHeight();
	DX11Context::Get()->SetViewportSize(viewportWidth, viewportHeight);
	DX11TestLayer::GetCamera()->SetViewportSize((float)viewportWidth, (float)viewportHeight);

	Hazel::Ref<DX11Shader> dx11ShaderUnlit = s_PipelineUnlit->GetSpecification().Shader.As<DX11Shader>();

	dx11ShaderUnlit->GetVertexShader()->Bind();
	dx11ShaderUnlit->GetPixelShader()->Bind();

	s_VertexBufferQuad->Bind();
	s_IndexBufferQuad->Bind();
	s_PipelineUnlit->Bind();

	/**** BEGIN constant buffer ****/

	s_ConstantBufferLayoutDeferred = s_ConstantBufferLayout;

	// World/Model/Transform matrix
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(1.0f, -1.0f, 1.0f)); // flip the texture vertically

	// decrease the quad size little bit so we can visually recognize the deferred rendering mode
	model = glm::scale(model, glm::vec3(0.99f, 0.98f, 1.0f));

	s_ConstantBufferLayoutDeferred.Model = model;
	s_ConstantBufferLayoutDeferred.View = glm::mat4(1.0f);
	s_ConstantBufferLayoutDeferred.Projection = glm::mat4(1.0f);
	s_ConstantBuffer->Update(&s_ConstantBufferLayoutDeferred);

	/**** END constant buffer ****/

	dx11ShaderUnlit->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
	dx11ShaderUnlit->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

	std::vector<Hazel::Ref<Hazel::HazelTexture>> textures;
	Hazel::Ref<Hazel::HazelTexture2D> textureDiffuse = s_RenderTarget;
	Hazel::Ref<Hazel::HazelTexture2D> textureNormal = ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
	textures.push_back(textureDiffuse.As<DX11Texture2D>());
	textures.push_back(textureNormal.As<DX11Texture2D>());
	dx11ShaderUnlit->GetVertexShader()->SetTextures(textures);
	dx11ShaderUnlit->GetPixelShader()->SetTextures(textures);

	uint32_t startVertexIndex = 0;
	uint32_t startIndexLocation = 0;
	DX11Renderer::DrawIndexedTriangleList(s_IndexBufferQuad->GetIndexCount(), startVertexIndex, startIndexLocation);
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

	DX11Context::Get()->SetRasterizerState(DX11CullMode::None);

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

	dx11Mesh->GetVertexBuffer()->Bind();
	pipeline->Bind();
	dx11Mesh->GetIndexBuffer()->Bind();

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

void DX11Renderer::SubmitFullscreenQuad(Hazel::Ref<Hazel::Pipeline> pipeline, Hazel::Ref<Hazel::HazelMaterial> material)
{
	// Hazel::HazelRenderer::Submit([]() {});
}

void DX11Renderer::SetSceneEnvironment(Hazel::Ref<Hazel::Environment> environment, Hazel::Ref<Hazel::HazelImage2D> shadow)
{
}

std::pair<Hazel::Ref<Hazel::HazelTextureCube>, Hazel::Ref<Hazel::HazelTextureCube>> DX11Renderer::CreateEnvironmentMap(const std::string& filepath)
{
	return std::pair<Hazel::Ref<Hazel::HazelTextureCube>, Hazel::Ref<Hazel::HazelTextureCube>>();
}

void DX11Renderer::RenderMesh(Hazel::Ref<Hazel::Pipeline> pipeline, Hazel::Ref<Hazel::HazelMesh> mesh, const glm::mat4& transform)
{
}

void DX11Renderer::RenderMeshWithoutMaterial(Hazel::Ref<Hazel::Pipeline> pipeline, Hazel::Ref<Hazel::HazelMesh> mesh, const glm::mat4& transform)
{
}

void DX11Renderer::RenderQuad(Hazel::Ref<Hazel::Pipeline> pipeline, Hazel::Ref<Hazel::HazelMaterial> material, const glm::mat4& transform)
{
}

Hazel::RendererCapabilities& DX11Renderer::GetCapabilities()
{
	return Hazel::RendererCapabilities{};
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

void DX11Renderer::ResizeViewport(glm::vec2 viewportPanelSize, Hazel::Ref<MoravaFramebuffer> renderFramebuffer)
{
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

void DX11Renderer::DrawToFramebuffer(Hazel::HazelCamera* camera)
{
	/**** BEGIN DirectX 11 rendering ****/

	if (s_DeferredRenderingEnabled)
	{
		/**** BEGIN Deferred rendering (basic, no G-Buffer) ****/
		ClearRenderTargetColor(s_RenderTarget, 0.1f, 0.2f, 0.4f, 1.0f); // dark blue
		ClearDepthStencil(s_DepthStencil);
		// Redirect rendering from swapchain (window) to framebuffer render targets
		SetRenderTarget(s_RenderTarget, s_DepthStencil);
		/**** END Deferred rendering (basic, no G-Buffer) ****/
	}
	else
	{
		/**** BEGIN Forward rendering ****/
		ClearRenderTargetColorSwapChain(0.6f, 0.2f, 0.8f, 1.0f); // magenta
		ClearDepthStencilSwapChain();
		/**** END Forward rendering ****/
	}

	DX11Context::Get()->SetRasterizerState(DX11CullMode::None);

	Hazel::Ref<DX11Shader> dx11Shader = s_PipelineIlluminated->GetSpecification().Shader.As<DX11Shader>();

	uint32_t viewportWidth = Application::Get()->GetWindow()->GetWidth();
	uint32_t viewportHeight = Application::Get()->GetWindow()->GetHeight();
	DX11Context::Get()->SetViewportSize(viewportWidth, viewportHeight);
	DX11TestLayer::GetCamera()->SetViewportSize((float)viewportWidth, (float)viewportHeight);

	s_LightPosition.x = 0.0f;
	s_LightPosition.y = sin(Timer::Get()->GetCurrentTimestamp() * 0.5f) * 290.0f;
	s_LightPosition.z = cos(Timer::Get()->GetCurrentTimestamp() * 0.5f) * 290.0f;

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
		skyboxTransform = glm::scale(skyboxTransform, glm::vec3(300.0f));
		skyboxTransform = glm::rotate(skyboxTransform, glm::radians(Timer::Get()->GetCurrentTimestamp() * 10.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// World/Model/Transform matrix
		s_ConstantBufferLayout.Model = skyboxTransform;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11ShaderUnlit->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11ShaderUnlit->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<Hazel::HazelTexture>> textures;
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
		s_PipelineIlluminated->Bind(); // TODO: DX11TestLayer::s_Mesh->GetPipeline()->Bind();
		indexBuffer->Bind();

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, s_LightPosition);
		model = glm::scale(model, glm::vec3(10.0f));

		s_ConstantBufferLayout.Model = model;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		std::vector<Hazel::Ref<Hazel::HazelTexture>> textures;
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

	// Rendering ECS entities (meshes)
	RenderMeshesECS();

	RenderImGui();
}

// the code is taken from EnvMapSceneRenderer::GeometryPass()
void DX11Renderer::RenderMeshesECS()
{
	// s_PipelineIlluminated->Bind();

	Hazel::Ref<DX11Shader> dx11Shader = s_PipelineIlluminated->GetSpecification().Shader.As<DX11Shader>();

	auto meshEntities = DX11TestLayer::s_SceneHierarchyPanel->GetContext()->GetAllEntitiesWith<Hazel::MeshComponent>();

	// Render all entities with mesh component
	if (meshEntities.size())
	{
		for (auto entt : meshEntities)
		{
			Hazel::Entity entity = { entt, DX11TestLayer::s_Scene.Raw() };
			auto& meshComponent = entity.GetComponent<Hazel::MeshComponent>();

			if (meshComponent.Mesh)
			{
				glm::mat4 entityTransform = glm::mat4(1.0f);
				if (entity && entity.HasComponent<Hazel::TransformComponent>()) {
					entityTransform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
				}

				RenderObject renderObject = {};
				renderObject.Mesh = meshComponent.Mesh;
				renderObject.PipelineType = RenderObject::PipelineType::Light;
				renderObject.Transform = entityTransform;
				renderObject.Entity = entity;

				RenderMesh(renderObject);
			}
		}
	}
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
	// Hazel::Ref<DX11Pipeline> dx11Pipeline = renderObject.Mesh->GetPipeline().As<DX11Pipeline>();

	dx11MeshVB->Bind();
	pipeline->Bind();
	dx11meshIB->Bind();

	for (Hazel::Submesh submesh : renderObject.Mesh->GetSubmeshes())
	{
		// World/Model/Transform matrix
		s_ConstantBufferLayout.Model = renderObject.Transform;
		s_ConstantBuffer->Update(&s_ConstantBufferLayout);

		dx11Shader->GetVertexShader()->BindConstantBuffer(s_ConstantBuffer);
		dx11Shader->GetPixelShader()->BindConstantBuffer(s_ConstantBuffer);

		// Render Submesh
		// load submesh materials for each specific submesh from the s_EnvMapMaterials list
		Hazel::Ref<EnvMapMaterial> envMapMaterial = Hazel::Ref<EnvMapMaterial>();
		std::string materialUUID;

		if (renderObject.Entity)
		{
			materialUUID = MaterialLibrary::GetSubmeshMaterialUUID(renderObject.Mesh, submesh, &renderObject.Entity);
			if (MaterialLibrary::s_EnvMapMaterials.find(materialUUID) != MaterialLibrary::s_EnvMapMaterials.end())
			{
				envMapMaterial = MaterialLibrary::s_EnvMapMaterials.at(materialUUID);

				renderObject.Textures.clear();
				if (envMapMaterial->GetAlbedoInput().TextureMap)
				{
					renderObject.Textures.push_back(envMapMaterial->GetAlbedoInput().TextureMap);
				}

				if (envMapMaterial->GetNormalInput().TextureMap)
				{
					renderObject.Textures.push_back(envMapMaterial->GetNormalInput().TextureMap);
				}
			}
		}

		// Load textures for submesh material
		std::vector<Hazel::Ref<Hazel::HazelTexture>> textures = {};

		if (renderObject.Textures.size() < 1)
		{
			renderObject.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/default_material_albedo.png"));
		}
		if (renderObject.Textures.size() < 2)
		{
			renderObject.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/normal_map_default.png"));
		}

		textures.push_back(renderObject.Textures.at(0).As<DX11Texture2D>()); // Albedo Map
		textures.push_back(renderObject.Textures.at(1).As<DX11Texture2D>()); // Normal Map

		dx11Shader->GetVertexShader()->SetTextures(textures);
		dx11Shader->GetPixelShader()->SetTextures(textures);

		// DX11Renderer::DrawTriangleStrip(s_VertexBuffer->GetVertexCount(), startVertexIndex);
		DX11Renderer::DrawIndexedTriangleList(submesh.IndexCount, submesh.BaseVertex, submesh.BaseIndex);
	}
}
