#pragma once

#include "../Core/Base.h"
#include "../Core/Ref.h"
#include "../Renderer/HazelFramebuffer.h"


namespace Hazel {

	struct RenderPassSpecification
	{
		Ref<HazelFramebuffer> TargetFramebuffer;
	};

	class RenderPass : public RefCounted
	{
	public:
		virtual ~RenderPass() = default;

		virtual RenderPassSpecification& GetSpecification() = 0;
		virtual const RenderPassSpecification& GetSpecification() const = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};

}
