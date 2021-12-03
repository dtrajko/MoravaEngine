#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/RefH2M.h"

#include <string>


namespace H2M
{

	class FramebufferH2M;

	struct RenderPassSpecificationH2M
	{
		RefH2M<FramebufferH2M> TargetFramebuffer;
		std::string DebugName;
	};

	class RenderPassH2M : public RefCountedH2M
	{
	public:
		virtual ~RenderPassH2M() = default;

		virtual RenderPassSpecificationH2M& GetSpecification() = 0;
		virtual const RenderPassSpecificationH2M& GetSpecification() const = 0;

		static RefH2M<RenderPassH2M> Create(const RenderPassSpecificationH2M& spec);
	};

}
