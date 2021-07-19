#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Ref.h"
#include "HazelShader.h"
#include "VertexBuffer.h"
#include "RenderPass.h"


namespace Hazel {

	enum class PrimitiveTopology
	{
		None = 0,
		Points,
		Lines,
		Triangles,
		LineStrip,
		TriangleStrip,
		TriangleFan
	};

	struct PipelineSpecification
	{
		Ref<HazelShader> Shader;
		VertexBufferLayout Layout;
		Ref<RenderPass> RenderPass;

		std::string DebugName;
	};

	struct PipelineStatistics
	{
		uint64_t InputAssemblyVertices = 0;
		uint64_t InputAssemblyPrimitives = 0;
		uint64_t VertexShaderInvocations = 0;
		uint64_t ClippingInvocations = 0;
		uint64_t ClippingPrimitives = 0;
		uint64_t FragmentShaderInvocations = 0;
		uint64_t ComputeShaderInvocations = 0;

		// TODO(Yan): tesselation shader stats when we have them
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
