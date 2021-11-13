#pragma once

#include "Hazel/Renderer/RendererCapabilities.h"
#include "Hazel/Renderer/RendererTypes.h"
#include "Hazel/Renderer/SceneEnvironment.h"
#include "Hazel/Renderer/RenderCommandBuffer.h"

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"

#include <cstdint>
#include <string>


namespace Hazel {

	enum class RendererAPITypeHazelLegacy
	{
		None,
		Vulkan,
		OpenGL,
		DX11,
	};

	enum class PrimitiveTypeHazelLegacy
	{
		None = 0, Triangles, Lines
	};

	class Pipeline;
	class HazelMaterial;
	class MeshHazelLegacy;
	class RenderPass;
	class RenderCommandBuffer;
	class UniformBufferSet;
	class SceneRenderer;
	class StorageBufferSet;
	class MaterialTable;
	class PipelineCompute;
	class IndexBuffer;

	class RendererAPIHazelLegacy
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false) = 0;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;
		virtual void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<HazelMaterial> material) = 0;
		virtual void SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<HazelMaterial> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides) = 0;

		virtual void SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<HazelImage2D> shadow, Ref<HazelImage2D> linearDepth) = 0;
		virtual std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) = 0;
		virtual Ref<HazelTextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) = 0;

		virtual void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<MeshHazelLegacy> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform) = 0;
		virtual void RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<MeshHazelLegacy> mesh, Ref<HazelMaterial> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) = 0;
		virtual void RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material, const glm::mat4& transform) = 0;
		virtual void LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups) = 0;
		virtual void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref< StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material) = 0;
		virtual void ClearImage(Ref<RenderCommandBuffer> commandBuffer, Ref<HazelImage2D> image) = 0;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBuffer, Ref<HazelMaterial> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0) = 0;
		virtual void DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material, const glm::ivec3& workGroups) = 0;

		virtual RendererCapabilities& GetCapabilities() = 0;

		static RendererAPITypeHazelLegacy Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPITypeHazelLegacy api);

		// Obsolete methods
		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform) = 0;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform) = 0;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform) = 0;

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, PrimitiveTypeHazelLegacy type, bool depthTest = true);
		static void SetLineThickness(float thickness);

	private:
		static void LoadRequiredAssets();

	private:
		inline static RendererAPITypeHazelLegacy s_CurrentRendererAPI = RendererAPITypeHazelLegacy::Vulkan;

	};

}
