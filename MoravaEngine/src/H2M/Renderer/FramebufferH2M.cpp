#include "FramebufferH2M.h"

#include "Hazel/Platform/OpenGL/OpenGLFramebuffer.h"
#include "Hazel/Platform/Vulkan/VulkanFramebuffer.h"
#include "Platform/DX11/DX11Framebuffer.h"


namespace Hazel {

	RefH2M<HazelFramebuffer> HazelFramebuffer::Create(const HazelFramebufferSpecification& spec)
	{
		RefH2M<HazelFramebuffer> result = RefH2M<HazelFramebuffer>();

		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:		return RefH2M<HazelFramebuffer>();
		case RendererAPIType::Vulkan:	result = RefH2M<VulkanFramebuffer>::Create(spec); break;
		case RendererAPIType::OpenGL:	result = RefH2M<OpenGLFramebuffer>::Create(spec); break;
		case RendererAPIType::DX11:		result = RefH2M<DX11Framebuffer>::Create(spec); break;
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

	void HazelFramebufferPool::Add(const RefH2M<HazelFramebuffer>& framebuffer)
	{
		m_Pool.push_back(framebuffer);
	}

}
