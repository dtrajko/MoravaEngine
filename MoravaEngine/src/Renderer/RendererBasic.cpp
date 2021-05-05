#include "Renderer/RendererBasic.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"


glm::mat4 RendererBasic::s_ProjectionMatrix;
std::map<std::string, Shader*> RendererBasic::s_Shaders;
std::map<std::string, int> RendererBasic::s_Uniforms;
glm::vec4 RendererBasic::s_BgColor;
// bool RendererBasic::s_SpirV_Enabled;


RendererBasic::RendererBasic()
{
}

RendererBasic::~RendererBasic()
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

void RendererBasic::RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow)
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

bool RendererBasic::GetVulkanSupported()
{
	bool vulkanSupported = glfwVulkanSupported();
	Log::GetLogger()->info("Vulkan supported: {0}", vulkanSupported);
	return vulkanSupported;
}

void RendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void RendererBasic::InitDebug()
{
	glDebugMessageCallback(Util::OpenGLLogMessage, nullptr);
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

void RendererBasic::EnableBlend()
{
	glEnable(GL_BLEND);
}

void RendererBasic::DisableBlend()
{
	glDisable(GL_BLEND);
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

// Obsolete method in vulkan branch 237c6703 (OpenGL-specific)
void RendererBasic::DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest)
{
	if (Hazel::RendererAPI::Current() == Hazel::RendererAPIType::OpenGL)
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
}
