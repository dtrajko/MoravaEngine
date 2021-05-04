#include "HazelFramebuffer.h"

#include "../Platform/OpenGL/OpenGLFramebuffer.h"


namespace Hazel {

	Ref<HazelFramebuffer> HazelFramebuffer::Create(const HazelFramebufferSpecification& spec)
	{
		Ref<HazelFramebuffer> result = Ref<HazelFramebuffer>();

		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:		return Ref<HazelFramebuffer>();
			case RendererAPIType::OpenGL:	result = Ref<OpenGLFramebuffer>::Create(spec);
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
