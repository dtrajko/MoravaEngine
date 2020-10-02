#pragma once

#include "../Core/Base.h"
#include "../../Framebuffer.h"


namespace Hazel {

	struct RenderPassSpecification
	{
		Framebuffer* TargetFramebuffer;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() {}

		virtual const RenderPassSpecification& GetSpecification() const = 0;

		static RenderPass* Create(const RenderPassSpecification& spec);
	};

}
