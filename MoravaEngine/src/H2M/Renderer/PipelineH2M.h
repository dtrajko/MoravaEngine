#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/ShaderH2M.h"
#include "H2M/Renderer/VertexBufferH2M.h"
#include "H2M/Renderer/UniformBufferH2M.h"


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
		RefH2M<ShaderH2M> Shader;
		VertexBufferLayoutH2M Layout;
		RefH2M<RenderPass> RenderPass;
		PrimitiveTopology Topology = PrimitiveTopology::Triangles;
		bool BackfaceCulling = true;
		bool DepthTest = true;
		bool DepthWrite = true;
		bool Wireframe = false;
		float LineWidth = 1.0f;

		std::string DebugName;
	};

	class PipelineH2M : public RefCountedH2M
	{
	public:
		virtual ~PipelineH2M() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		virtual void Invalidate() = 0;
		virtual void SetUniformBuffer(RefH2M<UniformBufferH2M> uniformBuffer, uint32_t binding, uint32_t set = 0) = 0;

		// TEMP: remove this when render command buffers are a thing
		virtual void Bind() = 0;

		static RefH2M<PipelineH2M> Create(const PipelineSpecification& spec);

	};

}
