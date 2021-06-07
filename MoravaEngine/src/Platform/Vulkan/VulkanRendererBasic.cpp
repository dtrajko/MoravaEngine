#include "VulkanRendererBasic.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"


glm::mat4 VulkanRendererBasic::s_ProjectionMatrix;
std::map<std::string, MoravaShader*> VulkanRendererBasic::s_Shaders;
std::map<std::string, int> VulkanRendererBasic::s_Uniforms;
glm::vec4 VulkanRendererBasic::s_BgColor;
// bool RendererBasic::s_SpirV_Enabled;


VulkanRendererBasic::VulkanRendererBasic()
{
}

VulkanRendererBasic::~VulkanRendererBasic()
{
}

void VulkanRendererBasic::RendererInfo(std::string& windowTitle)
{
	windowTitle += " [Renderer: Vulkan]";
}

void VulkanRendererBasic::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

	s_BgColor = glm::vec4(0.0, 0.0, 0.0, 1.0);

	InitDebug();
}

void VulkanRendererBasic::SetUniforms()
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

void VulkanRendererBasic::SetShaders()
{
}

void VulkanRendererBasic::RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow)
{
	// throw std::logic_error("The method or operation is not implemented.");
	Log::GetLogger()->warn("VulkanRendererBasic::RenderPassMain: Method not yet supported!");
}

void VulkanRendererBasic::Clear()
{
	Log::GetLogger()->warn("VulkanRendererBasic::Clear(): Method not yet supported!");
}

void VulkanRendererBasic::Clear(float r, float g, float b, float a)
{
	// Log::GetLogger()->warn("VulkanRendererBasic::Clear(r, g, b, a): Method not yet supported!");
}

void VulkanRendererBasic::SetLineThickness(float thickness)
{
	Log::GetLogger()->warn("VulkanRendererBasic::SetLineThickness: Method not yet supported!");
}

void VulkanRendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	Log::GetLogger()->warn("VulkanRendererBasic::SetDefaultFramebuffer: Method not yet supported!");
}

void VulkanRendererBasic::InitDebug()
{
	Log::GetLogger()->warn("VulkanRendererBasic::InitDebug: Method not yet supported!");
}

void VulkanRendererBasic::EnableCulling()
{
	Log::GetLogger()->warn("VulkanRendererBasic::EnableCulling: Method not yet supported!");
}

void VulkanRendererBasic::DisableCulling()
{
	// throw std::logic_error("The method or operation is not implemented.");
	Log::GetLogger()->warn("VulkanRendererBasic::DisableCulling: Method not yet supported!");
}

void VulkanRendererBasic::EnableTransparency()
{
	Log::GetLogger()->warn("VulkanRendererBasic::EnableTransparency: Method not yet supported!");
}

void VulkanRendererBasic::DisableTransparency()
{
	Log::GetLogger()->warn("VulkanRendererBasic::DisableTransparency: Method not yet supported!");
}

void VulkanRendererBasic::EnableDepthBuffer()
{
	Log::GetLogger()->warn("VulkanRendererBasic::EnableDepthBuffer: Method not yet supported!");
}

void VulkanRendererBasic::DisableDepthBuffer()
{
	Log::GetLogger()->warn("VulkanRendererBasic::DisableDepthBuffer: Method not yet supported!");
}

void VulkanRendererBasic::ClearDepthBuffer()
{
	Log::GetLogger()->warn("VulkanRendererBasic::ClearDepthBuffer: Method not yet supported!");
}

void VulkanRendererBasic::EnableDepthTest()
{
	Log::GetLogger()->warn("VulkanRendererBasic::EnableDepthTest: Method not yet supported!");
}

void VulkanRendererBasic::DisableDepthTest()
{
	Log::GetLogger()->warn("VulkanRendererBasic::DisableDepthTest: Method not yet supported!");
}

void VulkanRendererBasic::EnableMSAA()
{
	Log::GetLogger()->warn("VulkanRendererBasic::EnableMSAA: Method not yet supported!");
}

void VulkanRendererBasic::DisableMSAA()
{
	Log::GetLogger()->warn("VulkanRendererBasic::DisableMSAA: Method not yet supported!");
}

void VulkanRendererBasic::EnableBlend()
{
	Log::GetLogger()->warn("VulkanRendererBasic::EnableBlend: Method not yet supported!");
}

void VulkanRendererBasic::EnableWireframe()
{
	// throw std::logic_error("The method or operation is not implemented.");
	// Log::GetLogger()->warn("VulkanRendererBasic::EnableWireframe: Method not yet supported!");
}

void VulkanRendererBasic::DisableWireframe()
{
	// throw std::logic_error("The method or operation is not implemented.");
	// Log::GetLogger()->warn("VulkanRendererBasic::DisableWireframe: Method not yet supported!");
}

void VulkanRendererBasic::SetupViewportSize(uint32_t width, uint32_t height)
{
	Log::GetLogger()->warn("VulkanRendererBasic::SetupViewportSize({0}, {1}): Method not yet supported!", width, height);
}

void VulkanRendererBasic::DisableBlend()
{
	Log::GetLogger()->warn("VulkanRendererBasic::DisableBlend: Method not yet supported!");
}

void VulkanRendererBasic::Cleanup()
{
	for (auto& shader : s_Shaders)
		delete shader.second;

	s_Shaders.clear();
	RendererBasic::GetUniforms().clear();
}

void VulkanRendererBasic::UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene)
{
	float aspectRatio = scene->GetCameraController()->GetAspectRatio();
	*projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio, scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
	s_ProjectionMatrix = *projectionMatrix;
}

// Obsolete method in vulkan branch 237c6703 (OpenGL-specific)
void VulkanRendererBasic::DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest)
{
	// throw std::logic_error("The method or operation is not implemented.");
	Log::GetLogger()->warn("VulkanRendererBasic::DrawIndexed: Method not yet supported!");
}
