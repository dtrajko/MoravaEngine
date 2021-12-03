#include "FramebufferH2M.h"

#include "H2M/Platform/OpenGL/OpenGLFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "Platform/DX11/DX11Framebuffer.h"


namespace H2M
{

	RefH2M<FramebufferH2M> FramebufferH2M::Create(const FramebufferSpecificationH2M& spec)
	{
		RefH2M<FramebufferH2M> result = RefH2M<FramebufferH2M>();

		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:		return RefH2M<FramebufferH2M>();
			case RendererAPITypeH2M::OpenGL:	result = RefH2M<OpenGLFramebufferH2M>::Create(spec); break;
			case RendererAPITypeH2M::Vulkan:	result = RefH2M<VulkanFramebufferH2M>::Create(spec); break;
			case RendererAPITypeH2M::DX11:     result = RefH2M<DX11Framebuffer>::Create(spec); break;
		}
		FramebufferPoolH2M::GetGlobal()->Add(result);
		return result;
	}

	FramebufferPoolH2M* FramebufferPoolH2M::s_Instance = new FramebufferPoolH2M;

	FramebufferPoolH2M::FramebufferPoolH2M(uint32_t maxFBs /* = 32 */)
	{
	}

	FramebufferPoolH2M::~FramebufferPoolH2M()
	{
	}

	std::weak_ptr<FramebufferH2M> FramebufferPoolH2M::AllocateBuffer()
	{
		// m_Pool.push_back();
		return std::weak_ptr<FramebufferH2M>();
	}

	void FramebufferPoolH2M::Add(const RefH2M<FramebufferH2M>& framebuffer)
	{
		m_Pool.push_back(framebuffer);
	}

}
