#include "RendererDeferredOGL.h"

#include "Core/Application.h"


RendererDeferredOGL::RendererDeferredOGL()
{
	SetShaders();
	SetupTextureSlots();
	SetupTextures();
	SetupMeshes();
}

void RendererDeferredOGL::Init(Scene* scene)
{
}

void RendererDeferredOGL::SetShaders()
{
	m_ShaderGeometryPass = Hazel::Ref<Shader>::Create("Shaders/OGLdev/tutorial35/geometry_pass.vs", "Shaders/OGLdev/tutorial35/geometry_pass.fs");
	s_Shaders.insert(std::make_pair("geometry_pass", m_ShaderGeometryPass.Raw()));
	Log::GetLogger()->info("RendererDeferredOGL: m_ShaderGeometryPass compiled [programID={0}]", m_ShaderGeometryPass->GetProgramID());

	m_ShaderForwardBasic = Hazel::Ref<Shader>::Create("Shaders/OGLdev/tutorial35/forward_basic.vs", "Shaders/OGLdev/tutorial35/forward_basic.fs");
	s_Shaders.insert(std::make_pair("forward_basic", m_ShaderForwardBasic.Raw()));
	Log::GetLogger()->info("RendererDeferredOGL: m_ShaderForwardBasic compiled [programID={0}]", m_ShaderForwardBasic->GetProgramID());
}

void RendererDeferredOGL::SetupTextureSlots()
{
	m_TextureSlot_Diffuse = 1;
}

void RendererDeferredOGL::SetupTextures()
{
	ResourceManager::LoadTexture("crate_diffuse", "Textures/crate.png");
	ResourceManager::LoadTexture("crate_normal", "Textures/crateNormal.png");
}

void RendererDeferredOGL::SetupMeshes()
{
	m_MeshBlock = Hazel::Ref<Block>::Create(glm::vec3(1.0f, 1.0f, 1.0f));
}

void RendererDeferredOGL::Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	// Forward rendering
	ForwardPass(mainWindow, scene, projectionMatrix);

	// Deferred rendering
	// GeometryPass(scene, projectionMatrix);
	// LightPass(mainWindow);
}

void RendererDeferredOGL::ForwardPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	m_ShaderForwardBasic->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	m_ShaderForwardBasic->setMat4("gWVP", projectionMatrix * scene->GetCamera()->GetViewMatrix() * model);
	m_ShaderForwardBasic->setMat4("gWorld", model);
	m_ShaderForwardBasic->setInt("gColorMap", m_TextureSlot_Diffuse);
	m_ShaderForwardBasic->Validate();

	ResourceManager::GetTexture("crate_diffuse")->Bind(m_TextureSlot_Diffuse);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_MeshBlock->Render();

	m_ShaderForwardBasic->Unbind();
}

void RendererDeferredOGL::GeometryPass(Scene* scene, glm::mat4 projectionMatrix)
{
	m_gbuffer.BindForWriting();

	m_ShaderGeometryPass->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	m_ShaderGeometryPass->setMat4("gWVP", projectionMatrix * scene->GetCamera()->GetViewMatrix() * model);
	m_ShaderGeometryPass->setMat4("gWorld", model);
	m_ShaderGeometryPass->setInt("gColorMap", m_TextureSlot_Diffuse);
	m_ShaderGeometryPass->Validate();

	ResourceManager::GetTexture("crate_diffuse")->Bind(m_TextureSlot_Diffuse);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_MeshBlock->Render();

	m_ShaderGeometryPass->Unbind();
}

void RendererDeferredOGL::LightPass(Window* mainWindow)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_gbuffer.BindForReading();

	GLint WINDOW_WIDTH = Application::Get()->GetWindow()->GetWidth();
	GLint WINDOW_HEIGHT = Application::Get()->GetWindow()->GetHeight();

	GLsizei HalfWidth = (GLsizei)(WINDOW_WIDTH / 2.0f);
	GLsizei HalfHeight = (GLsizei)(WINDOW_HEIGHT / 2.0f);

	m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_POSITION);
	glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_DIFFUSE);
	glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		0, HalfHeight, HalfWidth, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
	glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		HalfWidth, HalfHeight, WINDOW_WIDTH, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	m_gbuffer.SetReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_TEXCOORD);
	glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		HalfWidth, 0, WINDOW_WIDTH, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

RendererDeferredOGL::~RendererDeferredOGL()
{
}
