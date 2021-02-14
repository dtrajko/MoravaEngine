#include "Renderer2D.h"

#include "../Renderer/RenderCommand.h"
#include "../Core/Assert.h"
#include "HazelRenderer.h"

#include "../../Log.h"
#include "../../Shader.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Hazel {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 2000; // 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		static const uint32_t MaxLines = 1000; // 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;
		static const uint32_t MaxLineIndices = MaxLines * 6;

		Ref <Pipeline> QuadPipeline;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;

		Shader* TextureShader; // Morava shader class
		Ref<HazelShader> TextureHazelShader; // Hazel shader class

		Ref<HazelTexture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<HazelTexture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 QuadVertexPositions[4];

		// Lines
		Ref<Pipeline> LinePipeline;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<IndexBuffer> LineIndexBuffer;

		Shader* LineShader; // Morava shader class
		Ref<HazelShader> LineHazelShader; // Hazel shader class

		uint32_t LineIndexCount = 0;
		Hazel::LineVertex* LineVertexBufferBase = nullptr;
		Hazel::LineVertex* LineVertexBufferPtr = nullptr;

		glm::mat4 CameraViewProj;
		bool DepthTest = true;

		Renderer2D::Statistics Stats;
	};

	static Renderer2DData s_Data2D;

	void Renderer2D::Init()
	{
		RenderCommand::Init();

		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Float,  "a_TexIndex" },
				{ ShaderDataType::Float,  "a_TilingFactor" }
			};
			s_Data2D.QuadPipeline = Pipeline::Create(pipelineSpecification);

			s_Data2D.QuadVertexBuffer = VertexBuffer::Create(s_Data2D.MaxVertices * sizeof(QuadVertex));
			s_Data2D.QuadVertexBufferBase = new QuadVertex[s_Data2D.MaxVertices];

			uint32_t* quadIndices = new uint32_t[s_Data2D.MaxIndices];

			uint32_t offset = 0;
			for (uint32_t i = 0; i < s_Data2D.MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;

				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;

				offset += 4;
			}

			s_Data2D.QuadIndexBuffer = IndexBuffer::Create(quadIndices, s_Data2D.MaxIndices);
			delete[] quadIndices;

			s_Data2D.WhiteTexture = HazelTexture2D::Create(HazelTextureFormat::RGBA, 1, 1);
			uint32_t whiteTextureData = 0xffffffff;
			s_Data2D.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
			// Working on Hazel LIVE! #004
			// s_Data2D.WhiteTexture->Lock();
			// s_Data2D.WhiteTexture->GetWriteableBuffer().Write(&whiteTextureData, sizeof(uint32_t));
			// s_Data2D.WhiteTexture->Unlock();

			int32_t samplers[s_Data2D.MaxTextureSlots];
			for (uint32_t i = 0; i < s_Data2D.MaxTextureSlots; i++)
				samplers[i] = i;

			// s_Data2D.TextureHazelShader = HazelShader::Create("assets/shaders/Renderer2D.glsl"); // not in use, only for constructor testing

			s_Data2D.TextureShader = new Shader("Shaders/Hazel/Renderer2D.vs", "Shaders/Hazel/Renderer2D.fs");
			s_Data2D.TextureShader->Bind();
			s_Data2D.TextureShader->setIntArray("u_Textures", samplers, s_Data2D.MaxTextureSlots);

			// Set all texture slots to 0
			s_Data2D.TextureSlots[0] = s_Data2D.WhiteTexture;

			s_Data2D.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
			s_Data2D.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
			s_Data2D.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
			s_Data2D.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
		}

		// Lines
		{
			// s_Data2D.LineHazelShader = HazelShader::Create("assets/shaders/Renderer2D_Line.glsl"); // not in use, only for constructor testing

			s_Data2D.LineShader = new Shader("Shaders/Hazel/Renderer2D_Line.vs", "Shaders/Hazel/Renderer2D_Line.fs");

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" }
			};
			s_Data2D.LinePipeline = Pipeline::Create(pipelineSpecification);

			s_Data2D.LineVertexBuffer = VertexBuffer::Create(s_Data2D.MaxLineVertices * sizeof(LineVertex));
			s_Data2D.LineVertexBufferBase = new LineVertex[s_Data2D.MaxLineVertices];

			uint32_t* lineIndices = new uint32_t[s_Data2D.MaxLineIndices];
			for (uint32_t i = 0; i < s_Data2D.MaxLineIndices; i++)
				lineIndices[i] = i;

			s_Data2D.LineIndexBuffer = IndexBuffer::Create(lineIndices, s_Data2D.MaxLineIndices);
			delete[] lineIndices;
		}
	}

	void Renderer2D::Shutdown()
	{
		delete[] s_Data2D.QuadVertexBufferBase;
	}

	void Renderer2D::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer2D::BeginScene(const glm::mat4& viewProj, bool depthTest)
	{
		s_Data2D.CameraViewProj = viewProj;
		s_Data2D.DepthTest = depthTest;

		s_Data2D.TextureShader->Bind();
		s_Data2D.TextureShader->setMat4("u_ViewProjection", viewProj);

		s_Data2D.QuadIndexCount = 0;
		s_Data2D.QuadVertexBufferPtr = s_Data2D.QuadVertexBufferBase;

		s_Data2D.LineIndexCount = 0;
		s_Data2D.LineVertexBufferPtr = s_Data2D.LineVertexBufferBase;

		s_Data2D.TextureSlotIndex = 1;
	}

	void Renderer2D::EndScene()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data2D.QuadVertexBufferPtr - (uint8_t*)s_Data2D.QuadVertexBufferBase);
		if (dataSize)
		{
			s_Data2D.QuadVertexBuffer->SetData(s_Data2D.QuadVertexBufferBase, dataSize);

			s_Data2D.TextureShader->Bind();
			s_Data2D.TextureShader->setMat4("u_ViewProjection", s_Data2D.CameraViewProj);

			for (uint32_t i = 0; i < s_Data2D.TextureSlotIndex; i++)
				s_Data2D.TextureSlots[i]->Bind(i);

			s_Data2D.QuadVertexBuffer->Bind();
			s_Data2D.QuadPipeline->Bind();
			s_Data2D.QuadIndexBuffer->Bind();
			HazelRenderer::DrawIndexed(s_Data2D.QuadIndexCount, PrimitiveType::Triangles, s_Data2D.DepthTest);
			s_Data2D.Stats.DrawCalls++;
		}

		dataSize = (uint32_t)((uint8_t*)s_Data2D.LineVertexBufferPtr - (uint8_t*)s_Data2D.LineVertexBufferBase);
		if (dataSize)
		{
			s_Data2D.LineVertexBuffer->SetData(s_Data2D.LineVertexBufferBase, dataSize);

			s_Data2D.LineShader->Bind();
			s_Data2D.LineShader->setMat4("u_ViewProjection", s_Data2D.CameraViewProj);

			s_Data2D.LineVertexBuffer->Bind();
			s_Data2D.LinePipeline->Bind();
			s_Data2D.LineIndexBuffer->Bind();
			HazelRenderer::SetLineThickness(2.0f);
			HazelRenderer::DrawIndexed(s_Data2D.LineIndexCount, PrimitiveType::Lines, s_Data2D.DepthTest);
			s_Data2D.Stats.DrawCalls++;
		}

#if OLD
		Flush();
#endif
	}

	void Renderer2D::Flush()
	{
#if OLD
		if (s_Data2D.QuadIndexCount == 0)
			return; // Nothing to draw

		// Bind textures
		for (uint32_t i = 0; i < s_Data2D.TextureSlotIndex; i++)
			s_Data2D.TextureSlots[i]->Bind(i);

		s_Data2D.QuadVertexArray->Bind();
		Renderer::DrawIndexed(s_Data2D.QuadIndexCount, false);
		s_Data2D.Stats.DrawCalls++;
#endif
	}

	void Renderer2D::FlushAndReset()
	{
		EndScene();

		s_Data2D.QuadIndexCount = 0;
		s_Data2D.QuadVertexBufferPtr = s_Data2D.QuadVertexBufferBase;

		s_Data2D.TextureSlotIndex = 1;
	}

	void Renderer2D::FlushAndResetLines()
	{
		EndScene();

		s_Data2D.LineIndexCount = 0;
		s_Data2D.LineVertexBufferPtr = s_Data2D.LineVertexBufferBase;

		s_Data2D.TextureSlotIndex = 1;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data2D.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data2D.QuadVertexBufferPtr->Position = transform * s_Data2D.QuadVertexPositions[i];
			s_Data2D.QuadVertexBufferPtr->Color = color;
			s_Data2D.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data2D.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data2D.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data2D.QuadVertexBufferPtr++;
		}

		s_Data2D.QuadIndexCount += 6;

		s_Data2D.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, HazelTexture2D* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, HazelTexture2D* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, HazelTexture2D* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data2D.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data2D.TextureSlotIndex; i++)
		{
			if (*s_Data2D.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data2D.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				FlushAndReset();

			textureIndex = (float)s_Data2D.TextureSlotIndex;
			s_Data2D.TextureSlots[s_Data2D.TextureSlotIndex] = texture;
			s_Data2D.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data2D.QuadVertexBufferPtr->Position = transform * s_Data2D.QuadVertexPositions[i];
			s_Data2D.QuadVertexBufferPtr->Color = tintColor;
			s_Data2D.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data2D.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data2D.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data2D.QuadVertexBufferPtr++;
		}

		s_Data2D.QuadIndexCount += 6;

		s_Data2D.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data2D.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data2D.QuadVertexBufferPtr->Position = transform * s_Data2D.QuadVertexPositions[i];
			s_Data2D.QuadVertexBufferPtr->Color = color;
			s_Data2D.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data2D.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data2D.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data2D.QuadVertexBufferPtr++;
		}

		s_Data2D.QuadIndexCount += 6;

		s_Data2D.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, HazelTexture2D* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, HazelTexture2D* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data2D.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data2D.TextureSlotIndex; i++)
		{
			if (*s_Data2D.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data2D.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				FlushAndReset();

			textureIndex = (float)s_Data2D.TextureSlotIndex;
			s_Data2D.TextureSlots[s_Data2D.TextureSlotIndex] = texture;
			s_Data2D.TextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data2D.QuadVertexBufferPtr->Position = transform * s_Data2D.QuadVertexPositions[i];
			s_Data2D.QuadVertexBufferPtr->Color = tintColor;
			s_Data2D.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data2D.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data2D.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data2D.QuadVertexBufferPtr++;
		}

		s_Data2D.QuadIndexCount += 6;

		s_Data2D.Stats.QuadCount++;
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_Data2D.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data2D.Stats;
	}

	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		if (s_Data2D.LineIndexCount >= s_Data2D.MaxLineIndices) {
			FlushAndResetLines();
		}

		s_Data2D.LineVertexBufferPtr->Position = p0;
		s_Data2D.LineVertexBufferPtr->Color = color;
		s_Data2D.LineVertexBufferPtr++;

		s_Data2D.LineVertexBufferPtr->Position = p1;
		s_Data2D.LineVertexBufferPtr->Color = color;
		s_Data2D.LineVertexBufferPtr++;

		s_Data2D.LineIndexCount += 2;

		s_Data2D.Stats.LineCount++;
	}

}
