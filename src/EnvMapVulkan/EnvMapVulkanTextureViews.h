#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include "EnvMapVulkanComponents.h"
#include "EnvMapVulkanGlass.h"
#include "EnvMapVulkanRenderTexture.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>


namespace H2M
{
	class FramebufferH2M;
	class PipelineH2M;
	class VertexBufferH2M;
	class IndexBufferH2M;
}

class EnvMapVulkanScene;
struct EnvMapVulkanWaterSettings;

/**
 * The views of the cameras that draw into a render texture (docs/engine/Game_Engine_Runtime_Guide.html, 7.5.2): a camera
 * with a Target (CameraComponent) gets a view of its own, drawn before the main view, and its picture is copied into the
 * render texture's image, which the materials and UI images that name the .mrtex show.
 *
 * A texture view is a smaller version of the main view, built like the water's planar reflection: its own set 0 (the PBR
 * shaders' per-frame set) with its own camera and scene data, while the lights, the shadow maps, the probe volume, the
 * environment maps and the material buffer are the main set's (copied each frame). It has no G-buffer prepass: its meshes
 * are drawn with their own depth test, then the skybox where nothing was drawn, then the glass (7.5.3: each view has its
 * own copy of its scene for the glass to look through, EnvMapVulkanGlass). So it has no ambient occlusion and no
 * ray-traced reflections (the probes' indirect light and the environment's reflection instead), no bloom and no water.
 * The sun's shadow cascades are fitted to the main camera: where they don't reach, the texture view's surfaces are lit as
 * if unshadowed. A camera draws every Draw Every frames (CameraComponent::DrawEvery); its texture keeps the last picture.
 *
 * A Color render texture gets the view exposed and tonemapped with the scene's exposure (RenderTextureComposite.glsl),
 * then copied bit for bit into the texture (the composite is gamma encoded as the sRGB texture expects); an HDR one gets
 * the linear light as it is (exposed by the view that shows it). Then the texture's smaller mip levels are made again.
 *
 * Each frame (one frame in flight: the views' buffers and sets are written while the GPU is idle): Update finds the
 * cameras and prepares their views, Record draws them. A view whose camera is gone is destroyed.
 */
class EnvMapVulkanTextureViews
{
public:
	// The pipelines and the full-screen quad the composite draws with (the renderer's: position, texture coordinate)
	void Create(const H2M::RefH2M<H2M::VertexBufferH2M>& quadVertexBuffer, const H2M::RefH2M<H2M::IndexBufferH2M>& quadIndexBuffer);
	void Destroy();

	// What a view's camera draws with this frame, besides the scene: from the renderer
	struct FrameInputs
	{
		VkDescriptorSet MainFrameSet = VK_NULL_HANDLE; // the main view's set 0, as written for this frame
		VkDescriptorImageInfo WhiteAmbientOcclusion = {}; // u_AOImage: no occlusion
		VkDescriptorImageInfo NoReflections = {};         // u_ReflectionImage: no ray-traced reflections
		VkDescriptorImageInfo SkyboxCube = {};            // the skybox's cube map (the unfiltered environment)
		uint32_t EnvironmentVersion = 0;                  // changes with the environment's cube maps
		float EnvMapRotation = 0.0f;
		const EnvMapVulkanWaterSettings* Water = nullptr; // the scene's (Enabled or not): the light under it (the water itself isn't drawn)
		glm::vec3 SunDirection = glm::vec3(0.0f, -1.0f, 0.0f);
		float Exposure = 1.0f;        // Color textures: the scene's exposure (with the environment's auto exposure)
		float HuePreservation = 0.5f; // and tonemapping
	};
	// This frame's views, from the scene's cameras with a target (their render textures loaded the first time). Called before
	// anything of the frame is recorded: a view that changed size may wait for the GPU.
	void Update(const EnvMapVulkanScene& scene, const FrameInputs& inputs);

	// What drawScene gets to draw a view: inside the view's render pass (viewport and scissor set), with these sets. The
	// glass ends that pass and goes on in the framebuffer's continue render pass (BeginInfo, Viewport, Scissor), which
	// drawScene leaves begun
	struct ViewDraw
	{
		VkDescriptorSet FrameSet = VK_NULL_HANDLE;  // set 0 of the mesh pipelines
		VkDescriptorSet SkyboxSet = VK_NULL_HANDLE; // the skybox shader's set (its camera, the cube map)
		glm::mat4 ViewProjection = glm::mat4(1.0f);
		glm::vec3 Position = glm::vec3(0.0f);
		const VkRenderPassBeginInfo* BeginInfo = nullptr; // the view's render pass, as begun
		VkViewport Viewport = {};
		VkRect2D Scissor = {};
		H2M::RefH2M<H2M::FramebufferH2M> SceneFramebuffer;
		EnvMapVulkanGlass* Glass = nullptr;               // the view's copy of its scene for the glass
	};
	// The views into their render textures: each view's pass (drawScene draws the meshes and the skybox), its composite and
	// the copy into the texture
	void Record(VkCommandBuffer commandBuffer, const std::function<void(VkCommandBuffer, const ViewDraw&)>& drawScene);

	// The number of views drawn this frame
	uint32_t GetViewCount() const { return m_ActiveViews; }
	// The cameras that draw into a render texture (drawn this frame or waiting for their next Draw Every frame; not the
	// disabled ones), by entity
	std::vector<EnvMapVulkanEntityID> GetCameras() const;
	// A short name of a texture camera, for the profiler and the debug overlay: "Camera #" and the last 6 digits of its
	// entity ID as the scene file writes it (unique enough in a scene, found by searching the file)
	static std::string GetCameraLabel(EnvMapVulkanEntityID camera);

private:
	struct View
	{
		std::string TargetFile;                                  // the camera's Target this view was made for
		std::shared_ptr<EnvMapVulkanRenderTexture> RenderTexture; // nullptr: it can't be loaded (not tried again for this file)
		bool Active = false;                                     // drawn this frame
		bool Enabled = true;                                     // its camera's (CameraComponent::Enabled): kept while off
		uint32_t Width = 0, Height = 0;                          // of its framebuffers
		// Draw Every: the frames it waits before it draws again, whether it has drawn yet, and its place among the views
		// made (which frame of the N it draws in)
		uint32_t FramesUntilDraw = 0;
		bool Drawn = false;
		uint32_t Stagger = 0;
		ViewDraw Draw;

		H2M::RefH2M<H2M::FramebufferH2M> SceneFramebuffer;     // linear HDR color and depth
		H2M::RefH2M<H2M::FramebufferH2M> CompositeFramebuffer; // Color textures: the tonemapped picture (8 bits, gamma encoded)
		std::unique_ptr<EnvMapVulkanGlass> Glass;              // the glass pipeline and the copy of the scene it looks through
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet FrameSet;     // set 0 of the PBR shaders
		H2M::VulkanShaderH2M::UniformBufferH2M CameraBuffer;            // binding 0: Camera
		H2M::VulkanShaderH2M::UniformBufferH2M SceneDataBuffer;         // binding 1: SceneData
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet SkyboxSet;    // the skybox shader's set
		H2M::VulkanShaderH2M::UniformBufferH2M SkyboxCameraBuffer;      // its binding 0: Camera
		uint32_t SkyboxEnvironmentVersion = 0;                          // the cube map its set points to (0: none yet; the renderer's start at 1)
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet CompositeSet; // the composite's: the scene framebuffer's color
	};

	void CreateView(View& view, const FrameInputs& inputs);
	void ResizeView(View& view, uint32_t width, uint32_t height);
	void DestroyView(View& view);
	void RecordCopy(VkCommandBuffer commandBuffer, View& view);

	H2M::RefH2M<H2M::VertexBufferH2M> m_QuadVertexBuffer;
	H2M::RefH2M<H2M::IndexBufferH2M> m_QuadIndexBuffer;
	H2M::RefH2M<H2M::PipelineH2M> m_CompositePipeline;
	// A view per camera entity with a target
	std::map<EnvMapVulkanEntityID, View> m_Views;
	uint32_t m_ActiveViews = 0;
	uint32_t m_ViewsCreated = 0; // ever: each view's Stagger
	float m_Exposure = 1.0f;        // this frame's (FrameInputs)
	float m_HuePreservation = 0.5f;
};
