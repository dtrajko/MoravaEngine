#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "../../../pch.h"

#include "H2M/Renderer/PipelineCompute.h"
#include "H2M/Renderer/RendererContext.h"
#include "H2M/Renderer/RenderCommandQueue.h"
#include "H2M/Renderer/RendererAPI.h"
#include "H2M/Renderer/RendererCapabilities.h"
#include "H2M/Renderer/StorageBufferSet.h"
#include "H2M/Renderer/MaterialAsset.h"

#include "Core/Application.h"


namespace H2M {

	class HazelShaderLibrary;

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

		static RefH2M<RendererContext> GetContext();

		static void Init();
		static void Shutdown();

		static RendererCapabilities& GetCapabilities();

		// Commands
		// static void Clear();                                                             // TODO: to be removed from RendererH2M
		// static void Clear(float r, float g, float b, float a = 1.0f);                    // TODO: to be removed from RendererH2M
		// static void SetClearColor(float r, float g, float b, float a);                   // TODO: to be removed from RendererH2M
		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true); // TODO: to be removed from RendererH2M
		// static void SetLineThickness(float thickness); // For OpenGL                     // TODO: to be removed from RendererH2M
		// static void ClearMagenta();                                                      // TODO: to be removed from RendererH2M

		static RefH2M<HazelShaderLibrary>& GetShaderLibrary();

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
#if 1
			// dtrajko: call lambda immediately instead of storing it to the render command buffer
			func();
#else
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
					const uint32_t index = Renderer::GetCurrentFrameIndex();
					auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
					new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
				});
#endif
		}

		static void WaitAndRender();

		// ~Actual~ Renderer here... TODO: remove confusion later
		// ~Actual~ Renderer here... TODO: remove confusion later
		static void BeginRenderPass(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<RenderPass> renderPass, bool explicitClear = false);
		static void EndRenderPass(RefH2M<RenderCommandBuffer> renderCommandBuffer);

		static void BeginFrame();
		static void EndFrame();

		static void SetSceneEnvironment(RefH2M<SceneRenderer> sceneRenderer, RefH2M<Environment> environment, RefH2M<HazelImage2D> shadow, RefH2M<HazelImage2D> linearDepth);
		static std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath);
		static RefH2M<TextureCubeH2M> CreatePreethamSky(float turbidity, float azimuth, float inclination);

		static void RenderMesh(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<Mesh> mesh, RefH2M<MaterialTable> materialTable, const glm::mat4& transform);
		static void RenderMeshWithMaterial(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<Mesh> mesh, const glm::mat4& transform, RefH2M<Material> material, Buffer additionalUniforms = Buffer());
		static void RenderQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<Material> material, const glm::mat4& transform);
		static void SubmitFullscreenQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<Material> material);
		static void SubmitFullscreenQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<Material> material);
		static void SubmitFullscreenQuadWithOverrides(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides);
		static void LightCulling(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<PipelineCompute> computePipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups);
		static void DispatchComputeShader(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<PipelineCompute> computePipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<Material> material, const glm::ivec3& workGroups);
		static void RenderGeometry(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<Material> material, RefH2M<VertexBuffer> vertexBuffer, RefH2M<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0);
		static void SubmitQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Material> material, const glm::mat4& transform = glm::mat4(1.0f));
		static void ClearImage(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<HazelImage2D> image);

		static RefH2M<Texture2D_H2M> GetWhiteTexture();
		static RefH2M<Texture2D_H2M> GetBlackTexture();
		static RefH2M<Texture2D_H2M> GetBRDFLutTexture();
		static RefH2M<TextureCubeH2M> GetBlackCubeTexture();
		static RefH2M<Environment> GetEmptyEnvironment();

		static void RegisterShaderDependency(RefH2M<HazelShader> shader, RefH2M<PipelineCompute> computePipeline);
		static void RegisterShaderDependency(RefH2M<HazelShader> shader, RefH2M<Pipeline> pipeline);
		static void RegisterShaderDependency(RefH2M<HazelShader> shader, RefH2M<HazelMaterial> material);
		static void OnShaderReloaded(size_t hash);

		static uint32_t GetCurrentFrameIndex();

		static RendererConfigH2M& GetConfig();

		static RendererAPI* GetRendererAPI();
		static RenderCommandQueue& GetRenderResourceReleaseQueue(uint32_t index);

		// Obsolete methods
		static void SubmitQuad(RefH2M<HazelMaterial> material, const glm::mat4& transform = glm::mat4(1.0f));
		static void SubmitMesh(RefH2M<MeshH2M> mesh, const glm::mat4& transform, RefH2M<HazelMaterialInstance> overrideMaterial = RefH2M<HazelMaterialInstance>());
		static void SubmitMeshWithShader(RefH2M<MeshH2M> mesh, const glm::mat4& transform, RefH2M<HazelShader> shader);

		static void RenderMesh(RefH2M<Pipeline> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform);
		static void RenderMeshWithoutMaterial(RefH2M<Pipeline> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform);
		static void RenderQuad(RefH2M<Pipeline> pipeline, RefH2M<HazelMaterial> material, const glm::mat4& transform);

		static void DrawAABB(RefH2M<MeshH2M> mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

	private:
		static RenderCommandQueue& GetRenderCommandQueue();

	};

	namespace Utils {

		inline void DumpGPUInfoH2M()
		{
			auto& caps = RendererH2M::GetCapabilities();
			Log::GetLogger()->trace("GPU Info:");
			Log::GetLogger()->trace("  Vendor: {0}", caps.Vendor);
			Log::GetLogger()->trace("  Device: {0}", caps.Device);
			Log::GetLogger()->trace("  Version: {0}", caps.Version);
		}
	}

}
