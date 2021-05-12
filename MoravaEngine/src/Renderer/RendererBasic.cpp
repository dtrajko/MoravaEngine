#include "Renderer/RendererBasic.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"

#include "Platform/OpenGL/OpenGLRendererBasic.h"


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

	OpenGLRendererBasic::s_BgColor = glm::vec4(0.0, 0.0, 0.0, 1.0);

	InitDebug();
}

void RendererBasic::SetUniforms()
{
	// common
	OpenGLRendererBasic::s_Uniforms.insert(std::make_pair("model", 0));
	OpenGLRendererBasic::s_Uniforms.insert(std::make_pair("view", 0));
	OpenGLRendererBasic::s_Uniforms.insert(std::make_pair("projection", 0));
	OpenGLRendererBasic::s_Uniforms.insert(std::make_pair("nearPlane", 0));
	OpenGLRendererBasic::s_Uniforms.insert(std::make_pair("farPlane", 0));
	OpenGLRendererBasic::s_Uniforms.insert(std::make_pair("normalMap", 0));
	OpenGLRendererBasic::s_Uniforms.insert(std::make_pair("lightPosition", 0));
}

void RendererBasic::SetShaders()
{
}

void RendererBasic::RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow)
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
	// case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

glm::vec4 RendererBasic::GetBgColor()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return glm::vec4();
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetBgColor();
	// case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetBgColor();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return glm::vec4();
}

void RendererBasic::Clear()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::Clear();
		// case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::Clear();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Clear(float r, float g, float b, float a)
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::Clear(r, g, b, a);
	// case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::Clear(r, g, b, a);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
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

void RendererBasic::EnableWireframe()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void RendererBasic::DisableWireframe()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

glm::mat4 RendererBasic::GetProjectionMatrix()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return glm::mat4();
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetProjectionMatrix();
	// case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetProjectionMatrix();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return glm::mat4();
}

void RendererBasic::SetProjectionMatrix(glm::mat4 projectionMatrix)
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::SetProjectionMatrix(projectionMatrix);
	// case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::SetProjectionMatrix(projectionMatrix);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

std::map<std::string, Shader*>& RendererBasic::GetShaders()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return std::map<std::string, Shader*>();
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetShaders();
	// case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetShaders();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return std::map<std::string, Shader*>();
}

std::map<std::string, int>& RendererBasic::GetUniforms()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return std::map<std::string, int>();
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetUniforms();
	// case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetUniforms();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return std::map<std::string, int>();
}

void RendererBasic::DisableBlend()
{
	glDisable(GL_BLEND);
}

void RendererBasic::Cleanup()
{
	for (auto& shader : OpenGLRendererBasic::s_Shaders)
		delete shader.second;

	OpenGLRendererBasic::s_Shaders.clear();
	OpenGLRendererBasic::s_Uniforms.clear();
}

void RendererBasic::UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene)
{
	float aspectRatio = scene->GetCameraController()->GetAspectRatio();
	*projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio, scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
	OpenGLRendererBasic::s_ProjectionMatrix = *projectionMatrix;
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
