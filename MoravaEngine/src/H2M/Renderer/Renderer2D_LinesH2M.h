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

	class Renderer2D_LinesH2M : public RefCountedH2M
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(const glm::mat4& viewProj, bool depthTest);
		static void EndScene();
		static void Flush();

		// Primitives
		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);

	private:
		static void FlushAndReset();      // StartBatch
		static void FlushAndResetLines(); // NextBatch

	};

}
