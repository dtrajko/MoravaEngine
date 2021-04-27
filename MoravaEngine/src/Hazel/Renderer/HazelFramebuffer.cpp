#include "HazelFramebuffer.h"

#include "../Platform/OpenGL/OpenGLFramebuffer.h"


namespace Hazel {

	Ref<HazelFramebuffer> HazelFramebuffer::Create(const HazelFramebufferSpecification& spec)
	{
		Ref<HazelFramebuffer> result = Ref<HazelFramebuffer>();

		switch (RendererAPI::Current())
		{
			case RendererAPI::RendererAPIType::None:		return Ref<HazelFramebuffer>();
			case RendererAPI::RendererAPIType::OpenGL:	result = Ref<OpenGLFramebuffer>::Create(spec);
		}
		FramebufferPool::GetGlobal()->Add(result);
		return result;
	}

	FramebufferPool* FramebufferPool::s_Instance = new FramebufferPool;

	FramebufferPool::FramebufferPool(uint32_t maxFBs /* = 32 */)
	{

	}

	FramebufferPool::~FramebufferPool()
	{
		
	}

	std::weak_ptr<HazelFramebuffer> FramebufferPool::AllocateBuffer()
	{
		// m_Pool.push_back();
		return std::weak_ptr<HazelFramebuffer>();
	}

	void FramebufferPool::Add(const Ref<HazelFramebuffer>& framebuffer)
	{
		m_Pool.push_back(framebuffer);
	}

}
