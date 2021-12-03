#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Renderer/RenderCommandBuffer.h"

namespace Hazel {

	class PipelineCompute : public RefCounted
	{
	public:
		virtual void Begin(Ref<RenderCommandBuffer> renderCommandBuffer = Ref<RenderCommandBuffer>()) = 0;
		virtual void End() = 0;

		virtual Ref<HazelShader> GetShader() = 0;

		static Ref<PipelineCompute> Create(Ref<HazelShader> computeShader);
	};

}
