#include "UniformBufferH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Platform/Vulkan/VulkanUniformBufferH2M.h"
#include "H2M/Platform/OpenGL/OpenGLUniformBufferH2M.h"

#include "Platform/DX11/DX11UniformBuffer.h"
#include "Core/Log.h"


namespace Hazel
{

	RefH2M<UniformBufferH2M> UniformBufferH2M::Create(uint32_t size, uint32_t binding)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return Ref<UniformBufferH2M>();
			case RendererAPITypeH2M::OpenGL: return Ref<OpenGLUniformBuffer>::Create(size, binding);
			case RendererAPITypeH2M::Vulkan: return Ref<VulkanUniformBuffer>::Create(size, binding);
			case RendererAPITypeH2M::DX11:   return Ref<DX11UniformBuffer>::Create(size, binding);
		}

		Log::GetLogger()->error("Unknown RendererAPI!");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI!");

		return RefH2M<UniformBufferH2M>();
	}

}
