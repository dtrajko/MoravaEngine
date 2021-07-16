#include "Renderer/RendererBasic.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"

#include "Platform/OpenGL/OpenGLRendererBasic.h"
#include "Platform/Vulkan/VulkanRendererBasic.h"
#include "Platform/DX11/DX11RendererBasic.h"


glm::mat4 RendererBasic::s_ProjectionMatrix;
std::map<std::string, MoravaShader*> RendererBasic::s_Shaders;
std::map<std::string, int> RendererBasic::s_Uniforms;
glm::vec4 RendererBasic::s_BgColor;
// bool RendererBasic::s_SpirV_Enabled;
Hazel::Ref<Hazel::RendererContext> RendererBasic::s_RendererContext;


RendererBasic::RendererBasic()
{
}

RendererBasic::~RendererBasic()
{
}

void RendererBasic::AppendRendererInfo(WindowProps& windowProps)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::RendererInfo(windowProps);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::RendererInfo(windowProps);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::RendererInfo(windowProps);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
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
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

glm::vec4 RendererBasic::GetBgColor()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return glm::vec4();
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetBgColor();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetBgColor();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::GetBgColor();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return glm::vec4();
}

void RendererBasic::Clear()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::Clear();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::Clear();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::Clear();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Clear(float r, float g, float b, float a)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::Clear(r, g, b, a);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::Clear(r, g, b, a);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::Clear(r, g, b, a);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetLineThickness(float thickness)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::SetLineThickness(thickness);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::SetLineThickness(thickness);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::SetLineThickness(thickness);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
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
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::SetDefaultFramebuffer(width, height);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::SetDefaultFramebuffer(width, height);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::SetDefaultFramebuffer(width, height);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::InitDebug()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::InitDebug();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::InitDebug();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::InitDebug();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableCulling()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableCulling();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableCulling();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::EnableCulling();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableCulling()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableCulling();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableCulling();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DisableCulling();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableTransparency()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableTransparency();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableTransparency();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::EnableTransparency();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableTransparency()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableTransparency();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableTransparency();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DisableTransparency();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableDepthBuffer()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableDepthBuffer();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableDepthBuffer();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::EnableDepthBuffer();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableDepthBuffer()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableDepthBuffer();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableDepthBuffer();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DisableDepthBuffer();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::ClearDepthBuffer()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::ClearDepthBuffer();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::ClearDepthBuffer();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::ClearDepthBuffer();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableDepthTest()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableDepthTest();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableDepthTest();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::EnableDepthTest();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableDepthTest()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableDepthTest();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableDepthTest();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DisableDepthTest();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableMSAA()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableMSAA();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableMSAA();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::EnableMSAA();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableMSAA()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableMSAA();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableMSAA();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DisableMSAA();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableBlend()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableBlend();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableBlend();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::EnableBlend();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableWireframe()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::EnableWireframe();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::EnableWireframe();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::EnableWireframe();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableWireframe()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableWireframe();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableWireframe();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DisableWireframe();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetViewportSize(uint32_t width, uint32_t height)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::SetViewportSize(width, height);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::SetViewportSize(width, height);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::SetViewportSize(width, height);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

glm::mat4 RendererBasic::GetProjectionMatrix()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return glm::mat4();
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetProjectionMatrix();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetProjectionMatrix();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::GetProjectionMatrix();
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return glm::mat4();
}

void RendererBasic::SetProjectionMatrix(glm::mat4 projectionMatrix)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::SetProjectionMatrix(projectionMatrix);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::SetProjectionMatrix(projectionMatrix);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::SetProjectionMatrix(projectionMatrix);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

std::map<std::string, MoravaShader*>& RendererBasic::GetShaders()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return std::map<std::string, MoravaShader*>();
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetShaders();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetShaders();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::GetShaders();
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return std::map<std::string, MoravaShader*>();
}

std::map<std::string, int>& RendererBasic::GetUniforms()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return std::map<std::string, int>();
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::GetUniforms();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::GetUniforms();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::GetUniforms();
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");

	return std::map<std::string, int>();
}

void RendererBasic::DisableBlend()
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DisableBlend();
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DisableBlend();
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DisableBlend();
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Cleanup()
{
	for (auto& shader : s_Shaders)
		delete shader.second;

	s_Shaders.clear();
	s_Uniforms.clear();
}

void RendererBasic::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, void* indicesPtr)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
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
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return;
		case Hazel::RendererAPIType::OpenGL: return OpenGLRendererBasic::DrawIndexed(count, type, depthTest);
		case Hazel::RendererAPIType::Vulkan: return VulkanRendererBasic::DrawIndexed(count, type, depthTest);
		case Hazel::RendererAPIType::DX11:   return DX11RendererBasic::DrawIndexed(count, type, depthTest);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
}
