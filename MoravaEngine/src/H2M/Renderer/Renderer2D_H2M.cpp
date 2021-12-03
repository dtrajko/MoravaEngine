#include "Renderer2D_H2M.h"

#include "H2M/Renderer/RenderCommandH2M.h"
#include "H2M/Core/AssertH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Log.h"
#include "Shader/MoravaShader.h"
#include "Platform/OpenGL/OpenGLMoravaShader.h"

#include <glm/gtc/matrix_transform.hpp>

namespace H2M
{

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

		RefH2M<PipelineH2M> QuadPipeline;
		RefH2M<VertexBufferH2M> QuadVertexBuffer;
		RefH2M<IndexBufferH2M> QuadIndexBuffer;

		RefH2M<OpenGLMoravaShader> TextureShader; // Morava shader class
		RefH2M<ShaderH2M> TextureHazelShader; // Hazel shader class

		RefH2M<Texture2D_H2M> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<RefH2M<Texture2D_H2M>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 QuadVertexPositions[4];

		// Lines
		RefH2M<PipelineH2M> LinePipeline;
		RefH2M<VertexBufferH2M> LineVertexBuffer;
		RefH2M<IndexBufferH2M> LineIndexBuffer;

		RefH2M<OpenGLMoravaShader> LineShader; // Morava shader class
		RefH2M<HazelShader> LineHazelShader; // Hazel shader class

		uint32_t LineIndexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		glm::mat4 CameraViewProj;
		bool DepthTest = true;

		Renderer2D_H2M::Statistics Stats;
	};

	static Renderer2DData s_Data;

	void Renderer2D_H2M::Init()
	{
		RenderCommandH2M::Init();

		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataTypeH2M::Float3, "a_Position" },
				{ ShaderDataTypeH2M::Float4, "a_Color" },
				{ ShaderDataTypeH2M::Float2, "a_TexCoord" },
				{ ShaderDataTypeH2M::Float,  "a_TexIndex" },
				{ ShaderDataTypeH2M::Float,  "a_TilingFactor" }
			};
			s_Data.QuadPipeline = PipelineH2M::Create(pipelineSpecification);

			s_Data.QuadVertexBuffer = VertexBufferH2M::Create(s_Data.MaxVertices * sizeof(QuadVertex));

			s_Data.QuadVertexBuffer->SetLayout({
				{ ShaderDataTypeH2M::Float3, "a_Position" },
				{ ShaderDataTypeH2M::Float4, "a_Color" },
				{ ShaderDataTypeH2M::Float2, "a_TexCoord" },
				{ ShaderDataTypeH2M::Float,  "a_TexIndex" },
				{ ShaderDataTypeH2M::Float,  "a_TilingFactor" }
			});

			s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

			uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];

			uint32_t offset = 0;
			for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;

				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;

				offset += 4;
			}

			s_Data.QuadIndexBuffer = IndexBufferH2M::Create(quadIndices, s_Data.MaxIndices);
			delete[] quadIndices;

			s_Data.WhiteTexture = Texture2D_H2M::Create(ImageFormatH2M::RGBA, 1, 1, nullptr);
			uint32_t whiteTextureData = 0xffffffff;
			s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

			//	Working on Hazel LIVE! #004
			//	s_Data.WhiteTexture->Lock();
			//	s_Data.WhiteTexture->GetWriteableBuffer().Write(&whiteTextureData, sizeof(uint32_t));
			//	s_Data.WhiteTexture->Unlock();

			int32_t samplers[s_Data.MaxTextureSlots];
			for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
				samplers[i] = i;

			//	if (RendererBasic::GetVulkanSupported()) {
			//		s_Data.TextureHazelShader = ShaderH2M::Create("assets/shaders/Renderer2D.glsl"); // not in use, only for constructor testing
			//	}

			s_Data.TextureShader = RefH2M<OpenGLMoravaShader>(MoravaShader::Create("Shaders/Hazel/Renderer2D.vs", "Shaders/Hazel/Renderer2D.fs"));
			s_Data.TextureShader->Bind();
			s_Data.TextureShader->SetIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);

			// Set all texture slots to 0
			s_Data.TextureSlots[0] = s_Data.WhiteTexture;

			s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
			s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
			s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
			s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
		}

		// Lines
		{
			if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
			{
				s_Data.LineHazelShader = ShaderH2M::Create("assets/shaders/Renderer2D_Line.glsl"); // not in use, only for constructor testing
			}

			s_Data.LineShader = RefH2M<OpenGLMoravaShader>(MoravaShader::Create("Shaders/Hazel/Renderer2D_Line.vs", "Shaders/Hazel/Renderer2D_Line.fs"));

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataTypeH2M::Float3, "a_Position" },
				{ ShaderDataTypeH2M::Float4, "a_Color" }
			};
			s_Data.LinePipeline = PipelineH2M::Create(pipelineSpecification);

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

	void Renderer2D_H2M::Shutdown()
	{
		delete[] s_Data.QuadVertexBufferBase;
	}

	void Renderer2D_H2M::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommandH2M::SetViewport(0, 0, width, height);
	}

	void Renderer2D_H2M::BeginScene(const glm::mat4& viewProj, bool depthTest)
	{
		s_Data.CameraViewProj = viewProj;
		s_Data.DepthTest = depthTest;

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetMat4("u_ViewProjection", viewProj);

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.LineIndexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D_H2M::EndScene()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
		if (dataSize)
		{
			s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

			s_Data.TextureShader->Bind();
			s_Data.TextureShader->SetMat4("u_ViewProjection", s_Data.CameraViewProj);

			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
				s_Data.TextureSlots[i]->Bind(i);

			s_Data.QuadVertexBuffer->Bind();
			s_Data.QuadPipeline->Bind();
			s_Data.QuadIndexBuffer->Bind();

			// RendererBasic::SetLineThickness(2.0f);
			RendererBasic::DrawIndexed(s_Data.QuadIndexCount, PrimitiveTypeH2M::Triangles, s_Data.DepthTest);
			s_Data.Stats.DrawCalls++;
		}

		dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
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
			s_Data.Stats.DrawCalls++;
		}

#if 1 // OLD
		Flush();
#endif
	}

	void Renderer2D_H2M::Flush()
	{
#if 1 // OLD
		if (s_Data.QuadIndexCount == 0)
			return; // Nothing to draw

		// Bind textures
		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->Bind(i);

		s_Data.QuadVertexBuffer->Bind();
		s_Data.QuadPipeline->Bind();
		s_Data.QuadIndexBuffer->Bind();

		RendererH2M::DrawIndexed(s_Data.QuadIndexCount, PrimitiveTypeH2M::Triangles, false);
		s_Data.Stats.DrawCalls++;
#endif
	}

	void Renderer2D_H2M::FlushAndReset()
	{
		EndScene();

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D_H2M::FlushAndResetLines()
	{
		EndScene();

		s_Data.LineIndexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D_H2M::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D_H2M::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D_H2M::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D_H2M::DrawQuad(const glm::vec2& position, const glm::vec2& size, Texture2D_H2M* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D_H2M::DrawQuad(const glm::vec3& position, const glm::vec2& size, Texture2D_H2M* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D_H2M::DrawQuad(const glm::mat4& transform, Texture2D_H2M* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				FlushAndReset();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D_H2M::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D_H2M::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D_H2M::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, Texture2D_H2M* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D_H2M::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, Texture2D_H2M* texture, float tilingFactor, const glm::vec4& tintColor)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				FlushAndReset();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D_H2M::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer2D_H2M::Statistics Renderer2D_H2M::GetStats()
	{
		return s_Data.Stats;
	}

	void Renderer2D_H2M::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
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

		s_Data.Stats.LineCount++;
	}

}
