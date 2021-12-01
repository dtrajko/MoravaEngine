#pragma once

#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/RendererCapabilitiesH2M.h"
#include "H2M/Renderer/RendererTypesH2M.h"
#include "H2M/Renderer/SceneEnvironmentH2M.h"
#include "H2M/Renderer/RenderCommandBufferH2M.h"

#include <cstdint>
#include <string>


namespace H2M {

	enum class RendererAPITypeH2M
	{
		None,
		Vulkan,
		OpenGL,
		DX11,
	};

	enum class PrimitiveTypeH2M
	{
		None = 0, Triangles, Lines
	};

	class PipelineH2M;
	class MaterialH2M;
	class MeshH2M;
	class RenderPassH2M;
	class RenderCommandBufferH2M;
	class UniformBufferSetH2M;
	class SceneRendererH2M;
	class StorageBufferSetH2M;
	class MaterialTableH2M;
	class PipelineComputeH2M;
	class IndexBufferH2M;


	class RendererAPI_H2M
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void BeginRenderPass(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<RenderPass> renderPass, bool explicitClear = false) = 0;
		virtual void EndRenderPass(RefH2M<RenderCommandBuffer> renderCommandBuffer) = 0;
		virtual void SubmitFullscreenQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<HazelMaterial> material) = 0;
		virtual void SubmitFullscreenQuadWithOverrides(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<HazelMaterial> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides) = 0;

		virtual void SetSceneEnvironment(RefH2M<SceneRenderer> sceneRenderer, RefH2M<Environment> environment, RefH2M<HazelImage2D> shadow, RefH2M<HazelImage2D> linearDepth) = 0;
		virtual std::pair<RefH2M<HazelTextureCube>, RefH2M<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) = 0;
		virtual RefH2M<HazelTextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) = 0;

		virtual void RenderMesh(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMesh> mesh, RefH2M<MaterialTable> materialTable, const glm::mat4& transform) = 0;
		virtual void RenderMeshWithMaterial(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMesh> mesh, RefH2M<HazelMaterial> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) = 0;
		virtual void RenderQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material, const glm::mat4& transform) = 0;
		virtual void LightCulling(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<PipelineCompute> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups) = 0;
		virtual void SubmitFullscreenQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M< StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material) = 0;
		virtual void ClearImage(RefH2M<RenderCommandBuffer> commandBuffer, RefH2M<HazelImage2D> image) = 0;
		virtual void RenderGeometry(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBuffer, RefH2M<HazelMaterial> material, RefH2M<VertexBuffer> vertexBuffer, RefH2M<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0) = 0;
		virtual void DispatchComputeShader(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<PipelineCompute> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material, const glm::ivec3& workGroups) = 0;

		virtual RendererCapabilities& GetCapabilities() = 0;

		static RendererAPIType Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPIType api);

		// Obsolete methods
		virtual void RenderMesh(RefH2M<Pipeline> pipeline, RefH2M<MeshHazelLegacy> mesh, const glm::mat4& transform) = 0;
		virtual void RenderMeshWithoutMaterial(RefH2M<Pipeline> pipeline, RefH2M<MeshHazelLegacy> mesh, const glm::mat4& transform) = 0;
		virtual void RenderQuad(RefH2M<Pipeline> pipeline, RefH2M<HazelMaterial> material, const glm::mat4& transform) = 0;

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true);
		static void SetLineThickness(float thickness);

	private:
		static void LoadRequiredAssets();

	private:
		inline static RendererAPIType s_CurrentRendererAPI = RendererAPIType::Vulkan;

	};

}
