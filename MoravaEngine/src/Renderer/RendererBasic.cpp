#include "Renderer/RendererBasic.h"

#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"

#if defined(GRAPHICS_API_OPENGL)
#include "Platform/OpenGL/OpenGLRendererBasic.h"
#endif
#if defined(GRAPHICS_API_VULKAN)
#include "Platform/Vulkan/VulkanRendererBasic.h"
#endif
#if defined(GRAPHICS_API_DIRECTX11)
#include "Platform/DX11/DX11RendererBasic.h"
#endif


glm::mat4 RendererBasic::s_ProjectionMatrix;
std::map<std::string, H2M::RefH2M<MoravaShader>> RendererBasic::s_Shaders;
std::map<std::string, int> RendererBasic::s_Uniforms;
glm::vec4 RendererBasic::s_BgColor;
// bool RendererBasic::s_SpirV_Enabled;
H2M::RefH2M<H2M::RendererContextH2M> RendererBasic::s_RendererContext;


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

void RendererBasic::AppendRendererInfo(WindowSpecification& windowSpecification)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::RendererInfo(windowSpecification);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::RendererInfo(windowSpecification);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::RendererInfo(windowSpecification);
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::InitDebug()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::InitDebug();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::InitDebug();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::InitDebug();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableCulling()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableCulling();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableCulling();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableCulling();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableCulling()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableCulling();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableCulling();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableCulling();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableTransparency()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableTransparency();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableTransparency();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableTransparency();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableTransparency()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableTransparency();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableTransparency();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableTransparency();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableDepthBuffer()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableDepthBuffer();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableDepthBuffer();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableDepthBuffer();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableDepthBuffer()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableDepthBuffer();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableDepthBuffer();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableDepthBuffer();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::ClearDepthBuffer()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::ClearDepthBuffer();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::ClearDepthBuffer();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::ClearDepthBuffer();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableDepthTest()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableDepthTest();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableDepthTest();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableDepthTest();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableDepthTest()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableDepthTest();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableDepthTest();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableDepthTest();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableMSAA()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableMSAA();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableMSAA();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableMSAA();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableMSAA()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableMSAA();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableMSAA();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableMSAA();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableBlend()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableBlend();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableBlend();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableBlend();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableBlend()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableBlend();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableBlend();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableBlend();
#endif
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::EnableWireframe()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::EnableWireframe();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::EnableWireframe();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::EnableWireframe();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DisableWireframe()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DisableWireframe();
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DisableWireframe();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DisableWireframe();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetViewportSize(uint32_t width, uint32_t height)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetViewportSize(width, height);
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetViewportSize(width, height);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetViewportSize(width, height);
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetDefaultFramebuffer(width, height);
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetDefaultFramebuffer(width, height);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetDefaultFramebuffer(width, height);
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::RenderPassMain(scene, projectionMatrix, mainWindow);
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Cleanup()
{
	s_Shaders.clear();
	s_Uniforms.clear();
}

void RendererBasic::Clear()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::Clear();
#endif
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::Clear();
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::Clear();
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::Clear(float r, float g, float b, float a)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
		case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::Clear(r, g, b, a);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::Clear(r, g, b, a);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::Clear(r, g, b, a);
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, void* indicesPtr)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DrawIndexed(indexCount, startIndexLocation, baseVertexLocation, indicesPtr);
#endif
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetLineThickness(float thickness)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetLineThickness(thickness);
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetLineThickness(thickness);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetLineThickness(thickness);
#endif
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

// Obsolete method in vulkan branch 237c6703 (OpenGL-specific)
void RendererBasic::DrawIndexed(uint32_t count, H2M::PrimitiveTypeH2M type, bool depthTest)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DrawIndexed(count, type, depthTest);
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DrawIndexed(count, type, depthTest);
#endif
		// case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DrawIndexed(count, type, depthTest);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::DrawLines(RefH2M<H2M::VertexArrayH2M> vertexArray, uint32_t vertexCount)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::DrawLines(vertexArray, vertexCount);
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::DrawLines(vertexArray, vertexCount);
#endif
		// case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::DrawIndexed(count, type, depthTest);
	}
	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
}

void RendererBasic::SetLineWidth(float width)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetLineWidth(width);
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetLineWidth(width);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
	case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetLineWidth(width);
#endif
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

void RendererBasic::SetPolygonMode(PolygonMode polygonMode)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::None:   return;
#if defined(GRAPHICS_API_OPENGL)
	case H2M::RendererAPITypeH2M::OpenGL: return OpenGLRendererBasic::SetPolygonMode(polygonMode);
#endif
#if defined(GRAPHICS_API_VULKAN)
	case H2M::RendererAPITypeH2M::Vulkan: return VulkanRendererBasic::SetPolygonMode(polygonMode);
#endif
		// case H2M::RendererAPITypeH2M::DX11:   return DX11RendererBasic::SetPolygonMode(polygonMode);
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
