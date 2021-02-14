#pragma once

#include "../Core/Ref.h"
#include "HazelShader.h"
#include "VertexBuffer.h"


namespace Hazel {

	struct PipelineSpecification
	{
		Ref<HazelShader> Shader;
		VertexBufferLayout Layout;
	};

	class Pipeline : public RefCounted
	{
	public:
		virtual ~Pipeline() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		virtual void Invalidate() = 0;

		// TEMP: remove this when render command buffers are a thing
		virtual void Bind() = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& spec);

	};

}
