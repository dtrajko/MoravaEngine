#pragma once

#include "RendererCapabilities.h"
#include "RendererTypes.h"
#include "SceneEnvironment.h"

#include <cstdint>
#include <string>


namespace Hazel {

	enum class RendererAPIType
	{
		None,
		Vulkan,
		OpenGL,
		DX11,
	};

	enum class PrimitiveType
	{
		None = 0, Triangles, Lines
	};

	class Pipeline;
	class HazelMaterial;
	class HazelMesh;
	class RenderPass;

	class RendererAPI
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void BeginRenderPass(const Ref<RenderPass>& renderPass) = 0;
		virtual void EndRenderPass() = 0;
		virtual void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material) = 0;
		// virtual void SubmitQuad(Ref<HazelMaterial> material, const glm::mat4& transform) = 0;

		virtual void SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow) = 0;

		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) = 0;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) = 0;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform) = 0;

		virtual std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) = 0;

		virtual RendererCapabilities& GetCapabilities() = 0;

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true);
		static void SetLineThickness(float thickness);

		static RendererAPIType Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPIType api);

	private:
		static void LoadRequiredAssets();

	private:
		inline static RendererAPIType s_CurrentRendererAPI = RendererAPIType::Vulkan;

	};

}
