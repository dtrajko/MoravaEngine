#include "RenderPassHazelVulkan.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/RenderPass.h"

#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"
#include "Hazel/Platform/Vulkan/VulkanRenderPass.h"

#include "Core/Log.h"


RenderPassHazelVulkan::RenderPassHazelVulkan(const RenderPassSpecificationHazelVulkan& spec)
	: m_Specification(spec)
{
}

Hazel::Ref<RenderPassHazelVulkan> RenderPassHazelVulkan::Create(const RenderPassSpecificationHazelVulkan& spec)
{
	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return Hazel::Ref<RenderPassHazelVulkan>();
	case Hazel::RendererAPIType::Vulkan:  return Hazel::Ref<Hazel::VulkanRenderPass>::Create(spec);
	case Hazel::RendererAPIType::OpenGL:  return Hazel::Ref<Hazel::OpenGLRenderPass>::Create(spec);
	}

	HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
	return Hazel::Ref<RenderPassHazelVulkan>();
}
