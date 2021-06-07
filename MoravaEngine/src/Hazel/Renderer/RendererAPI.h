#pragma once

#include "RendererTypes.h"
#include "RendererCapabilities.h"
#include "SceneEnvironment.h"

#include <cstdint>
#include <string>


namespace Hazel {

	using RendererID = uint32_t;

	enum class RendererAPIType
	{
		None,
		OpenGL,
		Vulkan,
		DX11,
	};

	enum class PrimitiveType
	{
		None = 0, Triangles, Lines
	};

	struct RenderAPICapabilities
	{
		std::string Vendor;
		std::string Device;
		std::string Version;

		int MaxSamples = 0;
		float MaxAnisotropy = 0.0f;
		int MaxTextureUnits = 0;
	};

	class Pipeline;
	class HazelMaterial;
	class HazelMesh;
	class RenderPass;

	class RendererAPI
	{
	public:
		static void Init();
		static void Shutdown();

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true);
		static void SetLineThickness(float thickness);

		static RenderAPICapabilities& GetCapabilities()
		{
			static RenderAPICapabilities capabilities;
			return capabilities;
		}

		static RendererAPIType Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPIType api);

	private:
		static void LoadRequiredAssets();

	private:
		static RendererAPIType s_CurrentRendererAPI;

	public:
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void BeginRenderPass(const Ref<RenderPass>& renderPass) = 0;
		virtual void EndRenderPass() = 0;
		virtual void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material) = 0;

		virtual void SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow) = 0;
		virtual std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) = 0;

		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) = 0;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) = 0;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform) = 0;

	// private:
	// 	inline static RendererAPIType s_CurrentRendererAPI = RendererAPIType::Vulkan;

	};

}
