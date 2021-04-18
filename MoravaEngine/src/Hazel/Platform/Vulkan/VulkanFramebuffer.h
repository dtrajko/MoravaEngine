#pragma once

#include "Vulkan.h"

#include "Hazel/Renderer/HazelFramebuffer.h"


namespace Hazel {

	class VulkanFramebuffer : public HazelFramebuffer
	{
	public:
		VulkanFramebuffer(const HazelFramebufferSpecification& spec);
		~VulkanFramebuffer();
	};

}
