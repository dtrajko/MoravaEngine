#include "RendererDeferredOGL.h"

#include "Core/Application.h"
#include "Mesh/QuadSSAO.h"
#include "Scene/SceneDeferredOGL.h"


RendererDeferredOGL::RendererDeferredOGL()
{
	SetShaders();
	SetupTextureSlots();
	SetupTextures();
	SetupMeshes();
}

RendererDeferredOGL::~RendererDeferredOGL()
{
}

void RendererDeferredOGL::Init(Scene* scene)
{
	CreateBuffers();
}

void RendererDeferredOGL::CreateBuffers()
{
	unsigned int WindowWidth = Application::Get()->GetWindow()->GetWidth();
	unsigned int WindowHeight = Application::Get()->GetWindow()->GetHeight();

	if (WindowWidth != m_WindowWidthOld || WindowHeight != m_WindowHeightOld)
	{
		m_FramebufferSSAO.Init(WindowWidth, WindowHeight);
		Log::GetLogger()->warn("Re-create SSAO framebuffer width: {0}, height: {1}", WindowWidth, WindowHeight);

		m_gbuffer.Init(WindowWidth, WindowHeight);
		Log::GetLogger()->warn("Re-create GBuffer width: {0}, height: {1}", WindowWidth, WindowHeight);

		m_WindowWidthOld = WindowWidth;
		m_WindowHeightOld = WindowHeight;
	}
}

void RendererDeferredOGL::SetShaders()
{
	m_ShaderForwardBasic = MoravaShader::Create("Shaders/OGLdev/tutorial35/forward_basic.vs", "Shaders/OGLdev/tutorial35/forward_basic.fs");
	RendererBasic::GetShaders().insert(std::make_pair("forward_basic", m_ShaderForwardBasic.Raw()));
	Log::GetLogger()->info("RendererDeferredOGL: m_ShaderForwardBasic compiled [programID={0}]", m_ShaderForwardBasic->GetProgramID());

	m_ShaderGeometryPass = MoravaShader::Create("Shaders/OGLdev/tutorial35/geometry_pass.vs", "Shaders/OGLdev/tutorial35/geometry_pass.fs");
	RendererBasic::GetShaders().insert(std::make_pair("geometry_pass", m_ShaderGeometryPass.Raw()));
	Log::GetLogger()->info("RendererDeferredOGL: m_ShaderGeometryPass compiled [programID={0}]", m_ShaderGeometryPass->GetProgramID());
}

void RendererDeferredOGL::SetupTextureSlots()
{
	m_TextureSlot_Diffuse = 1;
}

void RendererDeferredOGL::SetupTextures()
{
	ResourceManager::LoadTexture("crate_diffuse", "Textures/crate.png");
	ResourceManager::LoadTexture("crate_normal",  "Textures/crateNormal.png");
}

void RendererDeferredOGL::SetupMeshes()
{
	m_MeshBlock = H2M::RefH2M<Block>::Create(glm::vec3(1.0f, 1.0f, 1.0f));
}

void RendererDeferredOGL::ForwardPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	m_ShaderForwardBasic->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	m_ShaderForwardBasic->SetMat4("gWVP", projectionMatrix * scene->GetCamera()->GetViewMatrix() * model);
	// m_ShaderForwardBasic->SetMat4("gWorld", model);
	m_ShaderForwardBasic->SetInt("gColorMap", m_TextureSlot_Diffuse);
	m_ShaderForwardBasic->Validate();

	ResourceManager::GetTexture("crate_diffuse")->Bind(m_TextureSlot_Diffuse);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_MeshBlock->Render();

	m_ShaderForwardBasic->Unbind();
}

void RendererDeferredOGL::GeometryPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	m_gbuffer.BindForWriting();

	m_ShaderGeometryPass->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	m_ShaderGeometryPass->SetMat4("gWVP", projectionMatrix * scene->GetCamera()->GetViewMatrix() * model);
	m_ShaderGeometryPass->SetMat4("gWorld", model);
	m_ShaderGeometryPass->SetInt("gColorMap", m_TextureSlot_Diffuse);
	m_ShaderGeometryPass->Validate();

	ResourceManager::GetTexture("crate_diffuse")->Bind(m_TextureSlot_Diffuse);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_MeshBlock->Render();

	m_ShaderGeometryPass->Unbind();
}

void RendererDeferredOGL::LightPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_gbuffer.BindForReading();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLint WINDOW_WIDTH = Application::Get()->GetWindow()->GetWidth();
	GLint WINDOW_HEIGHT = Application::Get()->GetWindow()->GetHeight();

	SceneDeferredOGL* sceneOGL = (SceneDeferredOGL*)scene;

	if (sceneOGL->GetRenderTarget() == (int)SceneDeferredOGL::RenderTarget::Deferred_Position)
	{
		m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_POSITION);
	}
	else if (sceneOGL->GetRenderTarget() == (int)SceneDeferredOGL::RenderTarget::Deferred_Diffuse)
	{
		m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_DIFFUSE);
	}
	else if (sceneOGL->GetRenderTarget() == (int)SceneDeferredOGL::RenderTarget::Deferred_Normal)
	{
		m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
	}
	else if (sceneOGL->GetRenderTarget() == (int)SceneDeferredOGL::RenderTarget::Deferred_TexCoord)
	{
		m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_TEXCOORD);
	}
	else if (sceneOGL->GetRenderTarget() == (int)SceneDeferredOGL::RenderTarget::Deferred_SSAO)
	{
		m_FramebufferSSAO.BindForReading();
		m_FramebufferSSAO.SetReadBuffer();
	}

	glBlitFramebuffer(0, 0, m_gbuffer.GetWidth(), m_gbuffer.GetHeight(),
		0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void RendererDeferredOGL::BeginFrame()
{
}

void RendererDeferredOGL::WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	SceneDeferredOGL* sceneOGL = (SceneDeferredOGL*)scene;

	if (sceneOGL->GetRenderTarget() == (int)SceneDeferredOGL::RenderTarget::Forward)
	{
		// Forward rendering
		ForwardPass(mainWindow, scene, projectionMatrix);
	}
	else
	{
		// Deferred rendering
		CreateBuffers();

		GeometryPass(mainWindow, scene, projectionMatrix);
		LightPass(mainWindow, scene, projectionMatrix);
	}
}
