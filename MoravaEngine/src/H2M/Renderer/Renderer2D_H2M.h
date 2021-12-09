/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Editor/EditorCameraH2M.h"
#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/PipelineH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/VertexArrayH2M.h"
#include "H2M/Renderer/ShaderH2M.h"
#include "H2M/Scene/ComponentsH2M.h"


namespace H2M
{

	class Renderer2D_H2M : public RefCountedH2M
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(const CameraH2M& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCameraH2M& camera);
		// static void BeginScene(const OrthographicCameraH2M& camera);
		static void BeginScene(const glm::mat4& viewProj, bool depthTest);
		static void EndScene();
		static void Flush();

		// Primitives
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, Texture2D_H2M* texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, Texture2D_H2M* texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, RefH2M<Texture2D_H2M> texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);

		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, Texture2D_H2M* texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, Texture2D_H2M* texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);

		static void DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);

		static void DrawSprite(const glm::mat4& transform, SpriteRendererComponentH2M& src, int entityID);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t LineCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6; }
		};
		static void ResetStats();
		static Statistics GetStats();

	private:
		static void FlushAndReset();      // StartBatch
		static void FlushAndResetLines(); // NextBatch

	};

}
