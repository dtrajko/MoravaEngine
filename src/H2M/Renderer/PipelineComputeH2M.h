/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Renderer/ShaderH2M.h"
#include "H2M/Renderer/RenderCommandBufferH2M.h"


namespace H2M
{

	class PipelineComputeH2M : public RefCountedH2M
	{
	public:
		virtual void Begin(RefH2M<RenderCommandBufferH2M> renderCommandBuffer = RefH2M<RenderCommandBufferH2M>()) = 0;
		virtual void End() = 0;

		virtual RefH2M<ShaderH2M> GetShader() = 0;

		static RefH2M<PipelineComputeH2M> Create(RefH2M<ShaderH2M> computeShader);
	};

}
