#include "Renderer/RendererBasic.h"

#include "H2M/Renderer/RendererAPI.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"

#include "Platform/OpenGL/OpenGLRendererBasic.h"
#include "Platform/Vulkan/VulkanRendererBasic.h"
#include "Platform/DX11/DX11RendererBasic.h"


glm::mat4 RendererBasic::s_ProjectionMatrix;
std::map<std::string, H2M::RefH2M<MoravaShader>> RendererBasic::s_Shaders;
std::map<std::string, int> RendererBasic::s_Uniforms;
glm::vec4 RendererBasic::s_BgColor;
// bool RendererBasic::s_SpirV_Enabled;
H2M::RefH2M<H2M::RendererContext> RendererBasic::s_RendererContext;


RendererBasic::RendererBasic()
{
}

RendererBasic::~RendererBasic()
{
}

void RendererBasic::AppendRendererInfo(WindowProps& windowProps)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::RendererInfo(windowProps);
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::RendererInfo(windowProps);
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::RendererInfo(windowProps);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
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
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Clear()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::Clear();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::Clear();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::Clear();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Clear(float r, float g, float b, float a)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::Clear(r, g, b, a);
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::Clear(r, g, b, a);
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::Clear(r, g, b, a);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetLineThickness(float thickness)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetLineThickness(thickness);
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetLineThickness(thickness);
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetLineThickness(thickness);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

bool RendererBasic::GetVulkanSupported()
{
	bool vulkanSupported = glfwVulkanSupported();
	Log::GetLogger()->info("Vulkan supported: {0}", vulkanSupported);
	return vulkanSupported;
}

void RendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetDefaultFramebuffer(width, height);
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetDefaultFramebuffer(width, height);
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetDefaultFramebuffer(width, height);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::InitDebug()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::InitDebug();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::InitDebug();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::InitDebug();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableCulling()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableCulling();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableCulling();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableCulling();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableCulling()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableCulling();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableCulling();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableCulling();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableTransparency()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableTransparency();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableTransparency();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableTransparency();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableTransparency()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableTransparency();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableTransparency();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableTransparency();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableDepthBuffer()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableDepthBuffer();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableDepthBuffer();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableDepthBuffer();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableDepthBuffer()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableDepthBuffer();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableDepthBuffer();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableDepthBuffer();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::ClearDepthBuffer()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::ClearDepthBuffer();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::ClearDepthBuffer();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::ClearDepthBuffer();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableDepthTest()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableDepthTest();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableDepthTest();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableDepthTest();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableDepthTest()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableDepthTest();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableDepthTest();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableDepthTest();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableMSAA()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableMSAA();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableMSAA();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableMSAA();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableMSAA()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableMSAA();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableMSAA();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableMSAA();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableBlend()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableBlend();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableBlend();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableBlend();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableWireframe()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableWireframe();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableWireframe();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableWireframe();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableWireframe()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableWireframe();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableWireframe();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableWireframe();
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetViewportSize(uint32_t width, uint32_t height)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetViewportSize(width, height);
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetViewportSize(width, height);
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetViewportSize(width, height);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableBlend()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableBlend();
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableBlend();
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableBlend();
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Cleanup()
{
	s_Shaders.clear();
	s_Uniforms.clear();
}

void RendererBasic::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, void* indicesPtr)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene)
{
	float aspectRatio = scene->GetCameraController()->GetAspectRatio();
	*projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio, scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
	s_ProjectionMatrix = *projectionMatrix;
}

// Obsolete method in vulkan branch 237c6703 (OpenGL-specific)
void RendererBasic::DrawIndexed(uint32_t count, H2M::PrimitiveType type, bool depthTest)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DrawIndexed(count, type, depthTest);
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DrawIndexed(count, type, depthTest);
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DrawIndexed(count, type, depthTest);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetPolygonMode(PolygonMode polygonMode)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetPolygonMode(polygonMode);
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetPolygonMode(polygonMode);
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetPolygonMode(polygonMode);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}
