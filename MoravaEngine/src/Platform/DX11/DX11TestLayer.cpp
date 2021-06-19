#include "DX11TestLayer.h"

#include "Platform/DX11/DX11Context.h"
// #include "Platform/DX11/DX11VertexBuffer.h"
// #include "Platform/DX11/DX11IndexBuffer.h"
// #include "Platform/DX11/DX11Shader.h"
#include "Platform/DX11/DX11SwapChain.h"
#include "Platform/DX11/DX11Renderer.h"
#include "Platform/DX11/DX11Shader.h"

#include "Core/Application.h"


DX11TestLayer::DX11TestLayer()
	: m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
{
}

DX11TestLayer::DX11TestLayer(const std::string& name)
	: Layer(name), m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
{
}

DX11TestLayer::~DX11TestLayer()
{
}

void DX11TestLayer::OnAttach()
{
	Hazel::HazelFramebufferTextureSpecification framebufferTextureSpecification;
	framebufferTextureSpecification.Format = Hazel::HazelImageFormat::RGBA;

	std::vector<Hazel::HazelFramebufferTextureSpecification> framebufferTextureSpecifications;
	framebufferTextureSpecifications.push_back(framebufferTextureSpecification);

	Hazel::HazelFramebufferAttachmentSpecification framebufferAttachmentSpecification{};
	framebufferAttachmentSpecification.Attachments = framebufferTextureSpecifications;

	Hazel::HazelFramebufferSpecification framebufferSpecification{};
	framebufferSpecification.ClearColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
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
	moravaShaderSpecification.VertexShaderPath = "Shaders/HLSL/DirectionalLightVertexShader.hlsl";
	moravaShaderSpecification.PixelShaderPath = "Shaders/HLSL/DirectionalLightPixelShader.hlsl";
	moravaShaderSpecification.ForceCompile = false;

	Hazel::PipelineSpecification pipelineSpecification{};
	pipelineSpecification.DebugName = "DX11 Pipeline specification";
	pipelineSpecification.Layout = Hazel::VertexBufferLayout{};
	pipelineSpecification.RenderPass = Hazel::RenderPass::Create(renderPassSpecification);
	pipelineSpecification.Shader = MoravaShader::Create(moravaShaderSpecification);

	m_Pipeline = Hazel::Pipeline::Create(pipelineSpecification);

	// m_Meshes.push_back(Hazel::Ref<Hazel::HazelMesh>::Create("Models/Cerberus/CerberusMaterials.fbx"));
}

void DX11TestLayer::OnDetach()
{
}

void DX11TestLayer::OnUpdate(Hazel::Timestep ts)
{
	m_Camera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), (float)DX11Renderer::GetViewportWidth(), (float)DX11Renderer::GetViewportHeight(), 0.01f, 1000.0f));

	// TODO: m_Camera.OnUpdate(ts);

	glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	Render(clearColor, m_Camera);
	for (Hazel::Ref<Hazel::HazelMesh> mesh : m_Meshes)
	{
		DX11Renderer::SubmitMesh(mesh);
	}
}

void DX11TestLayer::OnImGuiRender(Window* mainWindow, Scene* scene)
{
}

void DX11TestLayer::OnEvent(Event& event)
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

void DX11TestLayer::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
}

void DX11TestLayer::OnRender(Window* mainWindow)
{
	// RendererBasic::Clear(1.0f, 0.0f, 1.0f, 1.0f);
}

void DX11TestLayer::Render(const glm::vec4& clearColor, const Hazel::EditorCamera& camera)
{
	if (!m_Meshes.size()) return;

	auto mesh = m_Meshes[0];
	// HazelRenderer::Submit([=]() mutable
	// {
	// });
	{
		Hazel::Ref<DX11Context> context = Hazel::Ref<DX11Context>(Application::Get()->GetWindow()->GetRenderContext());
		// TODO:	Hazel::Ref<DX11Shader> shader = mesh->GetMeshShader().As<DX11Shader>();
		// TODO:	DX11SwapChain& swapChain = context->GetSwapChain();

		// TODO:	{
		// TODO:		// uniform buffer binding 0 uniform Camera
		// TODO:		void* ubPtr = shader->MapUniformBuffer(0);
		// TODO:		glm::mat4 proj = glm::perspectiveFov(glm::radians(45.0f), (float)swapChain.GetWidth(), (float)swapChain.GetHeight(), 0.1f, 1000.0f);
		// TODO:		// glm::mat4 view = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 4.0f)));
		// TODO:		glm::mat4 viewProj = proj * camera.GetViewMatrix();  // Runtime camera
		// TODO:		// glm::mat4 viewProj = m_Camera.GetViewProjection(); // Editor camera
		// TODO:		memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
		// TODO:		shader->UnmapUniformBuffer(0);
		// TODO:	}
	}
}
