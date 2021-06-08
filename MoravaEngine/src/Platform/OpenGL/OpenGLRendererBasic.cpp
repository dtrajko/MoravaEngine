#include "OpenGLRendererBasic.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"


OpenGLRendererBasic::OpenGLRendererBasic()
{
}

OpenGLRendererBasic::~OpenGLRendererBasic()
{
}

void OpenGLRendererBasic::RendererInfo(std::string& windowTitle)
{
	windowTitle += " [Renderer: OpenGL]";
}

void OpenGLRendererBasic::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

	s_BgColor = glm::vec4(0.0, 0.0, 0.0, 1.0);

	InitDebug();
}

void OpenGLRendererBasic::SetUniforms()
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

void OpenGLRendererBasic::SetShaders()
{
}

void OpenGLRendererBasic::RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	Clear(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);

	// Rendering here
}

void OpenGLRendererBasic::Clear()
{
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererBasic::Clear(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
}

void OpenGLRendererBasic::SetLineThickness(float thickness)
{
	glLineWidth(thickness);
}

void OpenGLRendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void OpenGLRendererBasic::InitDebug()
{
	glDebugMessageCallback(Util::OpenGLLogMessage, nullptr);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

void OpenGLRendererBasic::EnableCulling()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

void OpenGLRendererBasic::DisableCulling()
{
	glDisable(GL_CULL_FACE);
}

void OpenGLRendererBasic::EnableTransparency()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLRendererBasic::DisableTransparency()
{
	glDisable(GL_BLEND);
}

void OpenGLRendererBasic::EnableDepthBuffer()
{
	glEnable(GL_DEPTH);
}

void OpenGLRendererBasic::DisableDepthBuffer()
{
	glDisable(GL_DEPTH);
}

void OpenGLRendererBasic::ClearDepthBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererBasic::EnableDepthTest()
{
	glEnable(GL_DEPTH_TEST);
}

void OpenGLRendererBasic::DisableDepthTest()
{
	glDisable(GL_DEPTH_TEST);
}

void OpenGLRendererBasic::EnableMSAA()
{
	glEnable(GL_MULTISAMPLE);
}

void OpenGLRendererBasic::DisableMSAA()
{
	glDisable(GL_MULTISAMPLE);
}

void OpenGLRendererBasic::EnableBlend()
{
	glEnable(GL_BLEND);
}

void OpenGLRendererBasic::EnableWireframe()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void OpenGLRendererBasic::DisableWireframe()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void OpenGLRendererBasic::SetupViewportSize(uint32_t width, uint32_t height)
{
	// Setup Viewport size
	glViewport(0, 0, width, height);
}

void OpenGLRendererBasic::DisableBlend()
{
	glDisable(GL_BLEND);
}

void OpenGLRendererBasic::Cleanup()
{
	for (auto& shader : s_Shaders)
		delete shader.second;

	s_Shaders.clear();
	RendererBasic::GetUniforms().clear();
}

void OpenGLRendererBasic::UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene)
{
	float aspectRatio = scene->GetCameraController()->GetAspectRatio();
	*projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio, scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
	s_ProjectionMatrix = *projectionMatrix;
}

// Obsolete method in vulkan branch 237c6703 (OpenGL-specific)
void OpenGLRendererBasic::DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest)
{
	if (!depthTest) {
		glDisable(GL_DEPTH_TEST);
	}

	GLenum glPrimitiveType = 0;
	switch (type)
	{
	case Hazel::PrimitiveType::Triangles:
		glPrimitiveType = GL_TRIANGLES;
		break;
	case Hazel::PrimitiveType::Lines:
		glPrimitiveType = GL_LINES;
		break;
	}

	glDrawElements(glPrimitiveType, count, GL_UNSIGNED_INT, nullptr);

	if (!depthTest) {
		glEnable(GL_DEPTH_TEST);
	}
}
