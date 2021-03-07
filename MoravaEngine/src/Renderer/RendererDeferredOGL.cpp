#include "RendererDeferredOGL.h"

#include "Core/Application.h"


RendererDeferredOGL::RendererDeferredOGL()
{
}

void RendererDeferredOGL::Init(Scene* scene)
{
	SetShaders();
}

void RendererDeferredOGL::SetShaders()
{
	m_ShaderGeometryPass = Hazel::Ref<Shader>::Create("Shaders/OGLdev/tutorial35/geometry_pass.vs", "Shaders/OGLdev/tutorial35/geometry_pass.fs");
	s_Shaders.insert(std::make_pair("geometry_pass", m_ShaderGeometryPass.Raw()));
	printf("RendererDeferredOGL: m_ShaderGeometryPass compiled [programID=%d]\n", m_ShaderGeometryPass->GetProgramID());
}

void RendererDeferredOGL::Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	GeometryPass();
	LightPass();

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererDeferredOGL::GeometryPass()
{
	m_gbuffer.BindForWriting();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Pipeline

}

void RendererDeferredOGL::LightPass()
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
