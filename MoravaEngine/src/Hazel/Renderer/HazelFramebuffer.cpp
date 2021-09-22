#include "HazelFramebuffer.h"

#include "Hazel/Platform/OpenGL/OpenGLFramebuffer.h"
#include "Hazel/Platform/Vulkan/VulkanFramebuffer.h"
#include "Platform/DX11/DX11Framebuffer.h"


namespace Hazel {

	Ref<HazelFramebuffer> HazelFramebuffer::Create(const HazelFramebufferSpecification& spec)
	{
		Ref<HazelFramebuffer> result = Ref<HazelFramebuffer>();

		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:		return Ref<HazelFramebuffer>();
			case RendererAPIType::OpenGL:	result = Ref<OpenGLFramebuffer>::Create(spec); break;
			case RendererAPIType::Vulkan:	result = Ref<VulkanFramebuffer>::Create(spec); break;
			case RendererAPIType::DX11:     result = Ref<DX11Framebuffer>::Create(spec); break;
		}
		HazelFramebufferPool::GetGlobal()->Add(result);
		return result;
	}

	HazelFramebufferPool* HazelFramebufferPool::s_Instance = new HazelFramebufferPool;

	HazelFramebufferPool::HazelFramebufferPool(uint32_t maxFBs /* = 32 */)
	{
	}

	HazelFramebufferPool::~HazelFramebufferPool()
	{
	}

	std::weak_ptr<HazelFramebuffer> HazelFramebufferPool::AllocateBuffer()
	{
		// m_Pool.push_back();
		return std::weak_ptr<HazelFramebuffer>();
	}

	void HazelFramebufferPool::Add(const Ref<HazelFramebuffer>& framebuffer)
	{
		m_Pool.push_back(framebuffer);
	}

}
