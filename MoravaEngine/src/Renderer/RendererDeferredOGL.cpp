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
	m_ShaderGeometryPass->Bind();

	m_gbuffer.BindForWriting();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// layout(location = 0) in vec3 Position;
	// layout(location = 1) in vec2 TexCoord;
	// layout(location = 2) in vec3 Normal;
	// uniform mat4 gWVP;           // vertex shader
	// uniform mat4 gWorld;         // vertex shader
	// uniform sampler2D gColorMap; // fragment shader

	glm::mat4 gWVP = glm::mat4(1.0f);
	m_ShaderGeometryPass->SetMat4("gWVP", gWVP);

	glm::mat4 gWorld = glm::mat4(1.0f);
	m_ShaderGeometryPass->SetMat4("gWorld", gWorld);

	int colorMapTextureSlot = 0;
	m_ShaderGeometryPass->setInt("gColorMap", colorMapTextureSlot);

	// Render a mesh
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
