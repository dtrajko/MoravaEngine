#pragma once

#include "../../../pch.h"

#include "Hazel/Renderer/RendererContext.h"

#include "Core/Application.h"


namespace Hazel {

	class HazelShaderLibrary;

	struct RendererConfig
	{
		// "Experimental" features
		bool ComputeEnvironmentMaps = false;
	};

	class HazelRenderer
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		static Ref<RendererContext> GetContext()
		{
			return Application::Get()->GetWindow()->GetRenderContext();
		}

		// Commands
		// static void Clear();
		// static void Clear(float r, float g, float b, float a = 1.0f);
		// static void SetClearColor(float r, float g, float b, float a);

		// static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true);

		// For OpenGL
		// static void SetLineThickness(float thickness);

		// static void ClearMagenta();

		static void Init();
		static void Shutdown();

		static RendererCapabilities& GetCapabilities();

		static Ref<HazelShaderLibrary>& GetShaderLibrary();

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

		static void WaitAndRender();

		// ~Actual~ Renderer here... TODO: remove confusion later
		static void BeginRenderPass(Ref<RenderPass> renderPass, bool clear = true);
		static void EndRenderPass();

		static void BeginFrame();
		static void EndFrame();

		static void SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow);
		static std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath);

		static void RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform);
		static void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform);
		static void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform);
		static void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material);

		static void SubmitQuad(Ref<HazelMaterial> material, const glm::mat4& transform = glm::mat4(1.0f));
		static void SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<HazelMaterial> materialoverrideMaterial = Ref<HazelMaterial>());


		static void DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawAABB(const Ref<Mesh>& mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

		static Ref<HazelTexture2D> GetWhiteTexture();
		static Ref<HazelTextureCube> GetBlackCubeTexture();
		static Ref<Environment> GetEmptyEnvironment();

		static void RegisterShaderDependency(Ref<HazelShader> shader, Ref<Pipeline> pipeline);
		static void RegisterShaderDependency(Ref<HazelShader> shader, Ref<HazelMaterial> material);
		static void OnShaderReloaded(size_t hash);

		static RendererConfig& GetConfig();

	private:
		static RenderCommandQueue& GetRenderCommandQueue();

	};

	namespace Utils {

		inline void DumpGPUInfo()
		{
			auto& caps = HazelRenderer::GetCapabilities();
			Log::GetLogger()->trace("GPU Info:");
			Log::GetLogger()->trace("  Vendor: {0}", caps.Vendor);
			Log::GetLogger()->trace("  Device: {0}", caps.Device);
			Log::GetLogger()->trace("  Version: {0}", caps.Version);
		}

	}

}
