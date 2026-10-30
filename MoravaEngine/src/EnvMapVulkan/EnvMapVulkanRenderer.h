/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/ModelH2M.h"
#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererCapabilitiesH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"

#include "Core/Window.h"

#include "EnvMapVulkanLights.h"
#include "EnvMapVulkanProbes.h"
#include "EnvMapVulkanRayTracing.h"
#include "EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkanSceneData.h"
#include "EnvMapVulkanViewOptions.h"

#include <functional>


struct DebugOverlayGame;

class EnvMapVulkanRenderer : public H2M::RendererAPI_H2M
{
public:
	virtual void Init() override;
	virtual void Shutdown() override;

	virtual void BeginFrame() override;
	virtual void EndFrame() override;

	virtual void BeginRenderPass(const H2M::RefH2M<H2M::RenderPassH2M>& renderPass) override;
	virtual void EndRenderPass() override;
	virtual void SubmitFullscreenQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material) override;

	virtual void SetSceneEnvironment(H2M::RefH2M<H2M::EnvironmentH2M> environment, H2M::RefH2M<H2M::Image2D_H2M> shadow) override;

	virtual void RenderMesh(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform) override;
	virtual void RenderMeshWithoutMaterial(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform) override;
	virtual void RenderQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material, const glm::mat4& transform) override;

	virtual void DrawIndexed(uint32_t indexCount, H2M::PrimitiveTypeH2M type, bool depthTest = true) override;
	virtual void DrawLines(H2M::RefH2M<H2M::VertexArrayH2M> vertexArray, uint32_t vertexCount) override;

	virtual void SetLineWidth(float width) override;

	virtual std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath) override;

	virtual H2M::RendererCapabilitiesH2M GetCapabilities() override;

	// materials: the Material Library material of each mesh (same order as the model's meshes)
	// previousMeshTransforms: where each mesh of the model was drawn in the previous frame, for the motion vectors (empty: where
	// it's drawn now)
	static void SubmitModelTemp(const H2M::RefH2M<H2M::ModelH2M>& model, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials,
		const std::vector<glm::mat4>& previousMeshTransforms = {}); // to be removed from VulkanRendererH2M
	static void OnResize(uint32_t width, uint32_t height);                                                 // to be removed from VulkanRendererH2M
	static uint32_t GetViewportWidth();                                                                    // to be removed from VulkanRendererH2M
	static uint32_t GetViewportHeight();                                                                   // to be removed from VulkanRendererH2M

	// Which pipelines draw a model's opaque meshes (all with the same layouts, so the same descriptor sets and push constants):
	// Scene: the PBR shaders, depth EQUAL against the G-buffer prepass; WaterReflection: the PBR shaders with their own depth
	// test (no prepass there); GBuffer: the prepass shaders
	enum class MeshPass { Scene, WaterReflection, GBuffer };
	static void RenderModelVulkan(H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials, VkCommandBuffer commandBuffer,
		MeshPass pass, uint32_t firstDraw = 0); // firstDraw: the model's first mesh among the frame's meshes (previous transforms, culling)

	static void RenderSkybox(VkCommandBuffer commandBuffer);

	static void Draw(H2M::CameraH2M* camera); // TODO: there should be no parameters

	// The scene Draw draws, how (the settings saved with it) and what the view shows besides it (debug views, the
	// selection): its owner's, read at the start of every Draw. The owner changes the scene only between frames.
	static void SetScene(EnvMapVulkanScene* scene, const EnvMapVulkanRenderSettings* renderSettings, const EnvMapVulkanViewOptions* viewOptions);
	// The user interface over the scene (the editor's ImGui windows): recorded into the given command buffer of the window's
	// render pass while the window's image is recorded. Without one the window shows only the scene.
	static void SetUserInterface(const std::function<void(VkCommandBuffer)>& recordUserInterface);
	// The debug overlay (7.4, UI/DebugOverlay) over the view, after the game UI, from the next frame on: shown or not, the
	// running game's numbers (nullptr: no game runs, only the frame's) and the display's content scale. Copied
	static void SetDebugOverlay(bool shown, const DebugOverlayGame* game = nullptr, float scale = 1.0f);

	// --- What the editor asks the renderer (src/Editor)

	// The images for ImGui::Image below are IDs of the user interface (ImTextureID; 0 without one, see H2M::VulkanUIImagesH2M)

	// The finished viewport image, for ImGui::Image, and the size it is drawn at (from the next frame on)
	static uint64_t GetViewportImage();
	static void SetViewportSize(uint32_t width, uint32_t height);
	// Waits until the GPU is idle: before resources a recorded frame may use are destroyed (models, materials, textures)
	static void WaitIdle();
	// The scene's content was replaced (New, Open): what was captured from the old one is thrown away (the probes)
	static void OnSceneReplaced();

	// The environment map loaded, a number that changes whenever another one is loaded, its blur levels (the last: one
	// average color), and the sun taken out of it (EnvMapVulkanExtractedSun::Found)
	static const std::string& GetEnvironmentMapFile();
	static bool HasEnvironmentMap(); // its pixels are loaded (FindEnvironmentSun)
	static uint32_t GetEnvironmentVersion();
	static float GetEnvironmentMaxLod();
	static const EnvMapVulkanExtractedSun& GetExtractedSun();
	// The map's sun: the extracted one, else its brightest region (direction and color only, Found false). False when the
	// map's pixels are not available.
	static bool FindEnvironmentSun(EnvMapVulkanExtractedSun& sun);

	static bool IsRayTracingSupported();
	// Rays can be traced this frame: the GPU can, and the settings' Ray Tracing is on
	static bool IsRayTracingActive();

	// The sun's shadow cascades of the last frame (false: none were drawn), and an image of one, for ImGui::Image
	static bool AreShadowsRendered();
	static const EnvMapVulkanShadowCascade& GetShadowCascade(uint32_t cascade);
	static uint64_t GetShadowCascadeImage(uint32_t cascade);
	// The box around everything that casts shadows (min > max: nothing)
	static void GetShadowCasterBounds(glm::vec3& boundsMin, glm::vec3& boundsMax);
	// The shadow slot of each point and spot light of the scene's lists (-1: no shadow this frame), and how many have one
	static void GetShadowSlotsPerLight(std::vector<int>& pointSlots, std::vector<int>& spotSlots);
	static void GetShadowedLightCounts(uint32_t& pointLights, uint32_t& spotLights);

	// Frustum culling of the main view (the G-buffer prepass, the scene pass and the glass) in the last frame: each mesh's
	// box against the camera's frustum. Skinned meshes are always drawn (their bones move them out of their boxes).
	struct CullingStats
	{
		uint32_t Meshes = 0;  // of the submitted models
		uint32_t Culled = 0;  // outside the frustum: not drawn
		uint32_t Skinned = 0; // not tested
	};
	static const CullingStats& GetCullingStats();
	// Point and spot lights can cast shadows on this GPU (point lights need cube map arrays)
	static bool CanPointLightsCastShadows();
	static bool CanSpotLightsCastShadows();
	// A point or spot light's shadow map, drawn with the next frame into an image for ImGui::Image (a spot light's map is
	// the left 3/4 of it, a point light's cube is unfolded); 0 when the light has no shadow map this frame
	static uint64_t ShowLocalShadowMap(EnvMapVulkanSceneData::LightKind kind, int index);

	// The bloom's lens dirt texture: loaded at the start of the next frame
	static void SetBloomDirtTexture(const std::string& filepath);
	static uint64_t GetBloomDirtImage();
	static std::string GetBloomDirtTextureFile();

	// The probe volume's state, and its commands (see EnvMapVulkanProbes)
	static EnvMapVulkanProbes::UpdateStatus GetProbeStatus();
	static uint32_t GetProbeCount();
	static void BakeProbes();
	static void ResetProbes();

	// The Path Traced view: start over, and the samples per pixel so far
	static void RestartPathTracer();
	static uint32_t GetPathTracerSamples();

	// Numbers for the panels: the ray tracing scene, the reflection and ambient occlusion images
	static const EnvMapVulkanRayTracing::Stats& GetRayTracingStats();
	static bool AreReflectionsTraced();
	static glm::uvec2 GetReflectionsSize();
	static bool IsAmbientOcclusionRayTraced();
	static glm::uvec2 GetAmbientOcclusionSize();

	// The GPU: its name, the graphics API and its version, the driver's version (empty when unknown)
	struct GPUInfo
	{
		std::string Name;
		std::string API;
		std::string Driver;
	};
	static GPUInfo GetGPUInfo();
	// The environment map (HDR) loaded at startup: set by the scene before Init (SceneEnvMapVulkan, from its user preferences)
	static void SetEnvironmentMapFile(const std::string& filepath);
	static void ViewportCompositePass(VkCommandBuffer commandBuffer);
	static void RenderGrid(VkCommandBuffer commandBuffer);
	static void GeometryPass();
	static void CompositePass();
	static void RecordUserInterface(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers);

	static int32_t& GetSelectedDrawCall();

	static void SetCamera(H2M::CameraH2M& camera);

	/**** BEGIN methods moved from VulkanTestLayer to VulkanRendererH2M ****/
	static H2M::SceneRendererOptionsH2M& GetOptions(); // moved from VulkanTestLayer to VulkanRendererH2M
	static void MapUniformBuffersVTL(H2M::RefH2M<H2M::ModelH2M> model, const H2M::EditorCameraH2M& camera);
	/**** END methods moved from VulkanTestLayer to VulkanRendererH2M ****/

public:
	static bool s_MipMapsEnabled;
	static bool s_ViewportFBNeedsResize;

};

namespace Utils {

	void InsertImageMemoryBarrier(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkImageSubresourceRange subresourceRange);

	void SetImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	void SetImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}
