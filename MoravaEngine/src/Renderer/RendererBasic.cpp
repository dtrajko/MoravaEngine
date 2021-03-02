#include "Renderer/RendererBasic.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"


static void OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		Log::GetLogger()->error("[OpenGL Debug HIGH] {0}", message);
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		Log::GetLogger()->warn("[OpenGL Debug MEDIUM] {0}", message);
		break;
	case GL_DEBUG_SEVERITY_LOW:
		Log::GetLogger()->info("[OpenGL Debug LOW] {0}", message);
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		Log::GetLogger()->trace("[OpenGL Debug NOTIFICATION] {0}", message);
		break;
	}
}

glm::mat4 RendererBasic::s_ProjectionMatrix;
std::map<std::string, Shader*> RendererBasic::s_Shaders;
std::map<std::string, int> RendererBasic::s_Uniforms;
glm::vec4 RendererBasic::s_BgColor;
bool RendererBasic::s_SpirV_Enabled;

RendererBasic::RendererBasic()
{
}

void RendererBasic::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

	s_BgColor = glm::vec4(0.0, 0.0, 0.0, 1.0);

	InitDebug();
}

void RendererBasic::SetUniforms()
{
	// common
	s_Uniforms.insert(std::make_pair("model", 0));
	s_Uniforms.insert(std::make_pair("view", 0));
	s_Uniforms.insert(std::make_pair("projection", 0));
	s_Uniforms.insert(std::make_pair("nearPlane", 0));
	s_Uniforms.insert(std::make_pair("farPlane", 0));
	s_Uniforms.insert(std::make_pair("normalMap", 0));
	s_Uniforms.insert(std::make_pair("lightPosition", 0));

}

void RendererBasic::SetShaders()
{
}

void RendererBasic::RenderPass(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	Clear(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);

	// Rendering here
}

void RendererBasic::Clear(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RendererBasic::SetLineThickness(float thickness)
{
	glLineWidth(thickness);
}

void RendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void RendererBasic::InitDebug()
{
	glDebugMessageCallback(OpenGLLogMessage, nullptr);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

void RendererBasic::EnableCulling()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

void RendererBasic::DisableCulling()
{
	glDisable(GL_CULL_FACE);
}

void RendererBasic::EnableTransparency()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RendererBasic::DisableTransparency()
{
	glDisable(GL_BLEND);
}

void RendererBasic::EnableDepthBuffer()
{
	glEnable(GL_DEPTH);
}

void RendererBasic::DisableDepthBuffer()
{
	glDisable(GL_DEPTH);
}

void RendererBasic::ClearDepthBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void RendererBasic::EnableDepthTest()
{
	glEnable(GL_DEPTH_TEST);
}

void RendererBasic::DisableDepthTest()
{
	glDisable(GL_DEPTH_TEST);
}

void RendererBasic::EnableMSAA()
{
	glEnable(GL_MULTISAMPLE);
}

void RendererBasic::DisableMSAA()
{
	glDisable(GL_MULTISAMPLE);
}

void RendererBasic::Cleanup()
{
	for (auto& shader : s_Shaders)
		delete shader.second;

	s_Shaders.clear();
	s_Uniforms.clear();
}

void RendererBasic::UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene)
{
	float aspectRatio = scene->GetCameraController()->GetAspectRatio();
	*projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio, scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
	s_ProjectionMatrix = *projectionMatrix;
}

RendererBasic::~RendererBasic()
{
}
