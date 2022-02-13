/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "pch.h"

#include "H2M/Renderer/RendererContextH2M.h"
#include "H2M/Renderer/RenderCommandQueueH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererCapabilitiesH2M.h"


namespace H2M
{

	class ShaderLibraryH2M;

	struct RendererConfigH2M
	{
		uint32_t FramesInFlight = 3;

		// "Experimental" features
		bool ComputeEnvironmentMaps = true;

		// Tiering settings
		uint32_t EnvironmentMapResolution = 512; // 1024;
		uint32_t IrradianceMapComputeSamples = 512;
	};

	class RendererH2M
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		static RefH2M<RendererContextH2M> GetContext();

		static void Init();
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();

		// ~Actual~ Renderer here... TODO: remove confusion later
		static void BeginRenderPass(RefH2M<RenderPassH2M> renderPass, bool clear = true);
		static void EndRenderPass();
		static void SubmitFullscreenQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material);

		static void SetSceneEnvironment(RefH2M<EnvironmentH2M> environment, RefH2M<Image2D_H2M> shadow);

		static void RenderMesh(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform);
		static void RenderMeshWithoutMaterial(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform);
		static void RenderQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material, const glm::mat4& transform);

		static std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath);

		static RendererCapabilitiesH2M& GetCapabilities();

		static void Clear();                                                                   // TODO: to be removed from HazelRenderer
		static void Clear(float r, float g, float b, float a = 1.0f);                          // TODO: to be removed from HazelRenderer
		static void SetClearColor(float r, float g, float b, float a);                         // TODO: to be removed from HazelRenderer
		static void DrawIndexed(uint32_t count, PrimitiveTypeH2M type, bool depthTest = true); // TODO: to be removed from HazelRenderer
		static void SetLineThickness(float thickness); // For OpenGL                           // TODO: to be removed from HazelRenderer
		static void ClearMagenta();                                                            // TODO: to be removed from HazelRenderer

		static RefH2M<ShaderLibraryH2M>& GetShaderLibrary();

		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
#if 1
			// dtrajko: call lambda immediately instead of storing it to the render command buffer
			func();
#else
			// dtrajko: disable the following code because render command buffer is still not in use
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
			};
			auto storageBuffer = GetRenderCommandQueue().Allocate(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
#endif
		}

		/*static void* Submit(RenderCommandFn fn, unsigned int size)
		{
			return s_Instance->m_CommandQueue.Allocate(fn, size);
		}*/

		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
			};

			Submit([renderCmd, func]()
			{
				const uint32_t index = RendererH2M::GetCurrentFrameIndex();
				auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
				new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
			});
		}

		static void WaitAndRender();

		static void SubmitQuad(RefH2M<MaterialH2M> material, const glm::mat4& transform = glm::mat4(1.0f));
		static void SubmitMesh(RefH2M<MeshH2M> mesh, const glm::mat4& transform, RefH2M<MaterialInstanceH2M> overrideMaterial = RefH2M<MaterialInstanceH2M>());
		static void SubmitMeshWithShader(RefH2M<MeshH2M> mesh, const glm::mat4& transform, RefH2M<ShaderH2M> shader);

		static void DrawAABB(RefH2M<MeshH2M> mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawAABB(const AABB_H2M& aabb, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

		static RefH2M<Texture2D_H2M> GetWhiteTexture();
		static RefH2M<TextureCubeH2M> GetBlackCubeTexture();
		static RefH2M<EnvironmentH2M> GetEmptyEnvironment();

		static void RegisterShaderDependency(RefH2M<ShaderH2M> shader, RefH2M<PipelineH2M> pipeline);
		static void RegisterShaderDependency(RefH2M<ShaderH2M> shader, RefH2M<MaterialH2M> material);
		static void OnShaderReloaded(size_t hash);

		static uint32_t GetCurrentFrameIndex();

		static RendererConfigH2M& GetConfig();

		static RenderCommandQueueH2M& GetRenderResourceReleaseQueue(uint32_t index);

		static RendererAPI_H2M* GetRendererAPI();

	private:
		static RenderCommandQueueH2M& GetRenderCommandQueue();

	};

	namespace Utils {

		inline void DumpGPUInfo()
		{
			auto& caps = RendererH2M::GetCapabilities();
			Log::GetLogger()->trace("GPU Info:");
			Log::GetLogger()->trace("  Vendor: {0}", caps.Vendor);
			Log::GetLogger()->trace("  Device: {0}", caps.Device);
			Log::GetLogger()->trace("  Version: {0}", caps.Version);
		}
	}

}
