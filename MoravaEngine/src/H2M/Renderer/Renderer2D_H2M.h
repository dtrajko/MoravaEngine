/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#define _CRT_SECURE_NO_WARNINGS

#pragma once

// #include "Hazel/Renderer/OrthographicCamera.h"

#include "H2M/Editor/EditorCameraH2M.h"
#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/RenderCommandBufferH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/UniformBufferSetH2M.h"
#include "H2M/Scene/ComponentsH2M.h"


namespace H2M
{

	struct Renderer2DSpecificationH2M
	{
		bool SwapChainTarget = false;
	};

	class Renderer2D_H2M : public RefCountedH2M
	{
	public:
		Renderer2D_H2M(const Renderer2DSpecificationH2M& specification = Renderer2DSpecificationH2M());
		virtual ~Renderer2D_H2M();

		void Init_EnvMapVulkan();
		void Shutdown_EnvMapVulkan();

		static void InitObsolete();
		static void ShutdownObsolete();

		static void BeginScene(const CameraH2M& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCameraH2M& camera);
		// static void BeginScene(const OrthographicCamera& camera); // TODO: Remove
		static void BeginScene(const glm::mat4& viewProj, bool depthTest);
		static void EndScene();
		static void Flush();

		// Primitives
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const RefH2M<Texture2D_H2M>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const RefH2M<Texture2D_H2M>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const RefH2M<Texture2D_H2M>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);

		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const RefH2M<Texture2D_H2M>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const RefH2M<Texture2D_H2M>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);

		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);

		static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
		static void DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

		static void DrawLineWithTriangles(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);
		static void DrawRectWithTriangles(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

		static float GetLineWidth();
		static void SetLineWidth(float width);

		static void DrawSprite(const glm::mat4& transform, SpriteRendererComponentH2M& src, int entityID);


		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t LineCount = 0;

			uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
		};
		static void ResetStats();
		static Statistics GetStats();

	private:
		static void StartBatch();
		static void NextBatch();

	private:
		struct QuadVertexH2M
		{
			glm::vec3 Position;
			glm::vec4 Color;
			glm::vec2 TexCoord;
			float TexIndex;
			float TilingFactor;

			// Editor-only
			int EntityID;
		};

		struct TextVertexH2M
		{
			glm::vec3 Position;
			glm::vec4 Color;
			glm::vec2 TexCoord;
			float TexIndex;
		};

		struct LineVertexH2M
		{
			glm::vec3 Position;
			glm::vec4 Color;

			// Editor-only
			int EntityID;
		};

		struct CircleVertexH2M
		{
			glm::vec3 WorldPosition;
			glm::vec3 LocalPosition;
			glm::vec4 Color;
			float Thickness;
			float Fade;

			// Editor-only
			int EntityID;
		};

		static const uint32_t MaxQuads = 200000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;
		static const uint32_t MaxLineIndices = MaxLines * 6;

		Renderer2DSpecificationH2M m_Specification;
		RefH2M<RenderCommandBufferH2M> m_RenderCommandBuffer;

		RefH2M<Texture2D_H2M> m_WhiteTexture;

		// Quad
		RefH2M<PipelineH2M> m_QuadPipeline;
		RefH2M<VertexBufferH2M> m_QuadVertexBuffer;
		RefH2M<IndexBufferH2M> m_QuadIndexBuffer;
		RefH2M<MaterialH2M> m_QuadMaterial;

		uint32_t m_QuadIndexCount = 0;
		QuadVertexH2M* m_QuadVertexBufferBase = nullptr;
		QuadVertexH2M* m_QuadVertexBufferPtr = nullptr;

		// Circle
		RefH2M<PipelineH2M> m_CirclePipeline;
		RefH2M<MaterialH2M> m_CircleMaterial;
		RefH2M<VertexBufferH2M> m_CircleVertexBuffer;
		uint32_t m_CircleIndexCount = 0;
		CircleVertexH2M* m_CircleVertexBufferBase = nullptr;
		CircleVertexH2M* m_CircleVertexBufferPtr = nullptr;

		std::array<RefH2M<Texture2D_H2M>, MaxTextureSlots> m_TextureSlots;
		uint32_t m_TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 m_QuadVertexPositions[4];

		// Lines
		RefH2M<PipelineH2M> m_LinePipeline;
		RefH2M<PipelineH2M> m_LineOnTopPipeline;
		RefH2M<VertexBufferH2M> m_LineVertexBuffer;
		RefH2M<IndexBufferH2M> m_LineIndexBuffer;
		RefH2M<MaterialH2M> m_LineMaterial;

		uint32_t m_LineIndexCount = 0;
		LineVertexH2M* m_LineVertexBufferBase = nullptr;
		LineVertexH2M* m_LineVertexBufferPtr = nullptr;

		// Text
		RefH2M<PipelineH2M> m_TextPipeline;
		RefH2M<VertexBufferH2M> m_TextVertexBuffer;
		RefH2M<IndexBufferH2M> m_TextIndexBuffer;
		RefH2M<MaterialH2M> m_TextMaterial;
		std::array<RefH2M<Texture2D_H2M>, MaxTextureSlots> m_FontTextureSlots;
		uint32_t m_FontTextureSlotIndex = 0;

		uint32_t m_TextIndexCount = 0;
		TextVertexH2M* m_TextVertexBufferBase = nullptr;
		TextVertexH2M* m_TextVertexBufferPtr = nullptr;

		glm::mat4 m_CameraViewProj;
		glm::mat4 m_CameraView;
		bool m_DepthTest = true;

		float m_LineWidth = 1.0f;

		// Statistics m_Stats;

		RefH2M<UniformBufferSetH2M> m_UniformBufferSet;

		struct UBCamera
		{
			glm::mat4 ViewProjection;
		};

		friend struct Renderer2DData;

	};

}
