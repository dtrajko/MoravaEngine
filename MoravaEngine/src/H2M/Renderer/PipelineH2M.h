#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel-dev/Core/Ref.h"
#include "HazelShader.h"
#include "VertexBuffer.h"
#include "UniformBuffer.h"


namespace H2M {

	class RenderPass;

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
		H2M::RefH2M<ShaderH2M> Shader;
		VertexBufferLayout Layout;
		Ref<RenderPass> RenderPass;
		PrimitiveTopology Topology = PrimitiveTopology::Triangles;
		bool BackfaceCulling = true;
		bool DepthTest = true;
		bool DepthWrite = true;
		bool Wireframe = false;
		float LineWidth = 1.0f;

		std::string DebugName;
	};

	class PipelineH2M : public H2M::RefCounted
	{
	public:
		virtual ~PipelineH2M() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		virtual void Invalidate() = 0;
		virtual void SetUniformBuffer(H2M::RefH2M<H2M::UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set = 0) = 0;

		// TEMP: remove this when render command buffers are a thing
		virtual void Bind() = 0;

		static H2M::RefH2M<PipelineH2M> Create(const PipelineSpecification& spec);

	};

}
