#include "RendererDeferredOGL.h"

#include "Core/Application.h"
#include "LearnOpenGL/SphereJoey.h"


RendererDeferredOGL::RendererDeferredOGL()
{
	SetShaders();
}

void RendererDeferredOGL::Init(Scene* scene)
{
}

void RendererDeferredOGL::SetShaders()
{
	m_ShaderGeometryPass = Hazel::Ref<Shader>::Create("Shaders/OGLdev/tutorial35/geometry_pass.vs", "Shaders/OGLdev/tutorial35/geometry_pass.fs");
	s_Shaders.insert(std::make_pair("geometry_pass", m_ShaderGeometryPass.Raw()));
	Log::GetLogger()->info("RendererDeferredOGL: m_ShaderGeometryPass compiled [programID={0}]", m_ShaderGeometryPass->GetProgramID());

	m_ShaderForwardBasic = new Shader("Shaders/OGLdev/tutorial35/forward_basic.vs", "Shaders/OGLdev/tutorial35/forward_basic.fs");
	s_Shaders.insert(std::make_pair("forward_basic", m_ShaderForwardBasic));
	Log::GetLogger()->info("RendererDeferredOGL: m_ShaderForwardBasic compiled [programID={0}]", m_ShaderForwardBasic->GetProgramID());
}

void RendererDeferredOGL::RenderPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Shader* shaderForwardBasic = (Shader*)s_Shaders["forward_basic"];
	shaderForwardBasic->Bind();

	shaderForwardBasic->setMat4("model", glm::mat4(1.0f));
	shaderForwardBasic->setMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderForwardBasic->setMat4("projection", projectionMatrix);
	shaderForwardBasic->setVec3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderForwardBasic->setInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderForwardBasic->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderForwardBasic->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderForwardBasic->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderForwardBasic->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderForwardBasic->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	shaderForwardBasic->setInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderForwardBasic->setInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderForwardBasic->setInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderForwardBasic->setFloat("tilingFactor", 1.0f);
	shaderForwardBasic->setVec4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderForwardBasic->Validate();

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);

	shaderForwardBasic->Unbind();
}

void RendererDeferredOGL::Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererDeferredOGL::GeometryPass(Scene* scene, glm::mat4 projectionMatrix)
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

	glm::mat4 model = glm::mat4(1.0f);

	glm::mat4 gWVP = projectionMatrix * scene->GetCamera()->GetViewMatrix() * model;
	m_ShaderGeometryPass->SetMat4("gWVP", gWVP);

	glm::mat4 gWorld = model;
	m_ShaderGeometryPass->SetMat4("gWorld", gWorld);

	// m_ShaderGeometryPass->SetMat4("model", model);
	// m_ShaderGeometryPass->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	// m_ShaderGeometryPass->SetMat4("projection", projectionMatrix);

	int colorMapTextureSlot = 0;
	m_ShaderGeometryPass->setInt("gColorMap", colorMapTextureSlot);

	// m_Texture->Bind(colorMapTextureSlot);

	// Render a mesh
	// m_Mesh->Render();
}

void RendererDeferredOGL::LightPass(Window* mainWindow)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// then before rendering, configure the viewport to the original framebuffer's screen dimensions
	RendererBasic::SetDefaultFramebuffer((unsigned int)mainWindow->GetWidth(), (unsigned int)mainWindow->GetHeight());

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
