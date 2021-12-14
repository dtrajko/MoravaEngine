/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "Renderer2D_LinesH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Renderer/RenderCommandH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/UniformBufferH2M.h"

#include "Core/Log.h"
#include "Shader/MoravaShader.h"

#include <glm/gtc/matrix_transform.hpp>

namespace H2M
{

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct Renderer2DDataLines
	{
		static const uint32_t MaxLines = 1000; // 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;
		static const uint32_t MaxLineIndices = MaxLines * 6;

		// Lines
		RefH2M<PipelineH2M> LinePipeline;
		RefH2M<VertexBufferH2M> LineVertexBuffer;
		RefH2M<IndexBufferH2M> LineIndexBuffer;

		RefH2M<ShaderH2M> LineShader;

		uint32_t LineIndexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		glm::mat4 CameraViewProj;
		bool DepthTest = true;
	};

	static Renderer2DDataLines s_Data;

	void Renderer2D_LinesH2M::Init()
	{
		RenderCommandH2M::Init();

		// Lines
		{
			if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
			{
				s_Data.LineShader = ShaderH2M::Create("Resources/Shaders/Renderer2D_Line.glsl"); // not in use, only for constructor testing
			}
			else
			{
				s_Data.LineShader = MoravaShader::Create("Shaders/Hazel/Renderer2D_Line.vs", "Shaders/Hazel/Renderer2D_Line.fs");
			}

			PipelineSpecificationH2M pipelineSpecLines;
			pipelineSpecLines.Layout = {
				{ ShaderDataTypeH2M::Float3, "a_Position" },
				{ ShaderDataTypeH2M::Float4, "a_Color" }
			};
			s_Data.LinePipeline = PipelineH2M::Create(pipelineSpecLines);

			s_Data.LineVertexBuffer = VertexBufferH2M::Create(s_Data.MaxLineVertices * sizeof(LineVertex));
			s_Data.LineVertexBuffer->SetLayout({
				{ ShaderDataTypeH2M::Float3, "a_Position" },
				{ ShaderDataTypeH2M::Float4, "a_Color" }
			});

			s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxLineVertices];

			uint32_t* lineIndices = new uint32_t[s_Data.MaxLineIndices];
			for (uint32_t i = 0; i < s_Data.MaxLineIndices; i++)
				lineIndices[i] = i;

			s_Data.LineIndexBuffer = IndexBufferH2M::Create(lineIndices, s_Data.MaxLineIndices);

			delete[] lineIndices;
		}
	}

	void Renderer2D_LinesH2M::Shutdown()
	{
		delete[] s_Data.LineVertexBufferBase;
	}

	void Renderer2D_LinesH2M::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommandH2M::SetViewport(0, 0, width, height);
	}

	// void Renderer2D_LinesH2M::BeginScene(const OrthographicCameraH2M& camera) {}

	void Renderer2D_LinesH2M::BeginScene(const glm::mat4& viewProj, bool depthTest)
	{
		s_Data.CameraViewProj = viewProj;
		s_Data.DepthTest = depthTest;

		s_Data.LineIndexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;
	}

	void Renderer2D_LinesH2M::EndScene()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
		if (dataSize)
		{
			s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

			s_Data.LineShader->Bind();
			s_Data.LineShader->SetMat4("u_ViewProjection", s_Data.CameraViewProj);

			s_Data.LineVertexBuffer->Bind();
			s_Data.LinePipeline->Bind();
			s_Data.LineIndexBuffer->Bind();

			// HazelRenderer::SetLineThickness(2.0f);

			RendererBasic::DrawIndexed(s_Data.LineIndexCount, PrimitiveTypeH2M::Lines, s_Data.DepthTest);
		}

		Flush();
	}

	void Renderer2D_LinesH2M::Flush()
	{
	}

	void Renderer2D_LinesH2M::FlushAndReset()
	{
		EndScene();
	}

	void Renderer2D_LinesH2M::FlushAndResetLines()
	{
		EndScene();

		s_Data.LineIndexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;
	}

	void Renderer2D_LinesH2M::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		if (s_Data.LineIndexCount >= s_Data.MaxLineIndices) {
			FlushAndResetLines();
		}

		s_Data.LineVertexBufferPtr->Position = p0;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexBufferPtr->Position = p1;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineIndexCount += 2;
	}

}
