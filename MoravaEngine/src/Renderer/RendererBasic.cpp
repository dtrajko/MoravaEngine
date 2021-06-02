#include "Renderer/RendererBasic.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"

#include "Platform/OpenGL/OpenGLRendererBasic.h"
#include "Platform/Vulkan/VulkanRendererBasic.h"


RendererBasic::RendererBasic()
{
}

RendererBasic::~RendererBasic()
{
}

void RendererBasic::AppendRendererInfo(std::string& windowTitle)
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::RendererInfo(windowTitle);
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::RendererInfo(windowTitle);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL:
		OpenGLRendererBasic::s_BgColor = glm::vec4(0.0, 0.0, 0.0, 1.0);
		return;
	case Hazel::RendererAPIType::Vulkan:
		VulkanRendererBasic::s_BgColor = glm::vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

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
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

glm::vec4 RendererBasic::GetBgColor()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return glm::vec4();
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetBgColor();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetBgColor();
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
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::Clear();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Clear(float r, float g, float b, float a)
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::Clear(r, g, b, a);
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::Clear(r, g, b, a);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetLineThickness(float thickness)
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::SetLineThickness(thickness);
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::SetLineThickness(thickness);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

bool RendererBasic::GetVulkanSupported()
{
	bool vulkanSupported = glfwVulkanSupported();
	Log::GetLogger()->info("Vulkan supported: {0}", vulkanSupported);
	return vulkanSupported;
}

void RendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::SetDefaultFramebuffer(width, height);
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::SetDefaultFramebuffer(width, height);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::InitDebug()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::InitDebug();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::InitDebug();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableCulling()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableCulling();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableCulling();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableCulling()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableCulling();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableCulling();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableTransparency()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableTransparency();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableTransparency();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableTransparency()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableTransparency();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableTransparency();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableDepthBuffer()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableDepthBuffer();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableDepthBuffer();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableDepthBuffer()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableDepthBuffer();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableDepthBuffer();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::ClearDepthBuffer()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::ClearDepthBuffer();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::ClearDepthBuffer();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableDepthTest()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableDepthTest();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableDepthTest();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableDepthTest()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableDepthTest();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableDepthTest();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableMSAA()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableMSAA();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableMSAA();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableMSAA()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableMSAA();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableMSAA();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableBlend()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableBlend();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableBlend();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableWireframe()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableWireframe();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableWireframe();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableWireframe()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableWireframe();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableWireframe();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

glm::mat4 RendererBasic::GetProjectionMatrix()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return glm::mat4();
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetProjectionMatrix();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetProjectionMatrix();
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
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::SetProjectionMatrix(projectionMatrix);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

std::map<std::string, MoravaShader*>& RendererBasic::GetShaders()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return std::map<std::string, MoravaShader*>();
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetShaders();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetShaders();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return std::map<std::string, MoravaShader*>();
}

std::map<std::string, int>& RendererBasic::GetUniforms()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return std::map<std::string, int>();
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetUniforms();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetUniforms();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return std::map<std::string, int>();
}

void RendererBasic::DisableBlend()
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableBlend();
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableBlend();
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
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
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return;
	case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DrawIndexed(count, type, depthTest);
	case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DrawIndexed(count, type, depthTest);
	}
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}
