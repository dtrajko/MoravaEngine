#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Ref.h"
#include "HazelShader.h"
#include "VertexBuffer.h"
#include "RenderPass.h"


namespace Hazel {

	struct PipelineSpecification
	{
		Ref<HazelShader> Shader;
		VertexBufferLayout Layout;
		Ref<RenderPass> RenderPass;

		std::string DebugName;
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
