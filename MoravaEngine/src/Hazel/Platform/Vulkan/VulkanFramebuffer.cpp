#include "VulkanFramebuffer.h"


namespace Hazel {

	VulkanFramebuffer::VulkanFramebuffer(const HazelFramebufferSpecification& spec)
	{
		if (spec.SwapChainTarget) {
			return;
		}
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
	}

}
