#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/RefH2M.h"
#include "ShaderH2M.h"
#include "VertexBufferH2M.h"
#include "RenderPassH2M.h"


namespace H2M
{

	enum class PrimitiveTopologyH2M
	{
		None = 0,
		Points,
		Lines,
		Triangles,
		LineStrip,
		TriangleStrip,
		TriangleFan
	};

	struct PipelineSpecificationH2M
	{
		RefH2M<ShaderH2M> Shader;
		VertexBufferLayoutH2M Layout;
		RefH2M<RenderPassH2M> RenderPass;

		std::string DebugName;
	};

	struct PipelineStatisticsH2M
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

	class PipelineH2M : public RefCountedH2M
	{
	public:
		virtual ~PipelineH2M() = default;

		virtual PipelineSpecificationH2M& GetSpecification() = 0;
		virtual const PipelineSpecificationH2M& GetSpecification() const = 0;

		virtual void Invalidate() = 0;

		// TEMP: remove this when render command buffers are a thing
		virtual void Bind() = 0;

		static RefH2M<PipelineH2M> Create(const PipelineSpecificationH2M& spec);

	};

}
