/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EnvMapVulkanRenderer.h"
#include "EnvMapVulkanLights.h"
#include "EnvMapVulkanShadows.h"
#include "EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkanWater.h"

#include "Core/ResourceManager.h"

#include "H2M/Platform/Vulkan/VulkanH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanIndexBufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanMaterialH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanTestLayer.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Platform/Vulkan/VulkanVertexBufferH2M.h"
#include "H2M/Editor/ContentBrowserPanelH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/Renderer2D_H2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"

#include "Platform/Vulkan/VulkanSkyboxCube.h"

#include "imgui.h"
#include "imgui_internal.h" // BeginDragDropTargetCustom (the Models and Meshes panel as one drop area)

#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "ImGuizmo.h"

#include "stb_image.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <set>


namespace Utils
{
	static const char* VulkanVendorIDToString(uint32_t vendorID)
	{
		switch (vendorID)
		{
		case 0x10DE: return "NVIDIA";
		case 0x1002: return "AMD";
		case 0x8086: return "INTEL";
		case 0x13B5: return "ARM";
		}
		return "Unknown";
	}
}

bool EnvMapVulkanRenderer::s_MipMapsEnabled = true;
bool EnvMapVulkanRenderer::s_ViewportFBNeedsResize = false;

static VkCommandBuffer s_ImGuiCommandBuffer;         // to be removed from VulkanRenderer
static VkCommandBuffer s_CompositeCommandBuffer;     // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::FramebufferH2M> s_Framebuffer;          // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::FramebufferH2M> s_CompositeFramebuffer; // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::PipelineH2M> s_CompositePipeline;            // to be removed from VulkanRenderer
// The Viewport panel shows this image: s_Framebuffer (linear HDR scene) after exposure, tonemapping and gamma
static H2M::RefH2M<H2M::FramebufferH2M> s_ViewportCompositeFramebuffer;
static H2M::RefH2M<H2M::PipelineH2M> s_ViewportCompositePipeline;
static H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet s_ViewportCompositeDescriptorSet; // scene, bloom, lens dirt (ViewportComposite.glsl)

// Bloom (Resources/Shaders/BloomPass.glsl), with the settings of SceneHazelEnvMap's "Bloom Settings" panel. The bright parts of
// the scene are downsampled into a chain of half-resolution levels and upsampled back, which spreads them into a wide glow
// that the viewport composite adds to the scene.
struct BloomSettingsVulkan
{
	bool Enabled = true;
	float Threshold = 1.0f;     // brightness (after exposure) where bloom starts
	float Knee = 0.1f;          // soft transition below the threshold
	float UpsampleScale = 1.0f; // radius of the upsample filter: larger values spread the glow further
	float Intensity = 1.0f;
	bool DirtEnabled = false;   // lens dirt on/off (the texture and intensity are kept)
	float DirtIntensity = 1.0f; // lens dirt texture, lit by the bloom
};
static BloomSettingsVulkan s_BloomSettings;
static const uint32_t s_BloomLevelCount = 6; // level i is 1/2^(i+1) of the viewport size
static H2M::RefH2M<H2M::FramebufferH2M> s_BloomDownFramebuffers[s_BloomLevelCount];
static H2M::RefH2M<H2M::FramebufferH2M> s_BloomUpFramebuffers[s_BloomLevelCount - 1]; // the result is s_BloomUpFramebuffers[0]
static H2M::RefH2M<H2M::PipelineH2M> s_BloomPipeline;
static H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet s_BloomDescriptorSets; // one per pass: prefilter, downsamples, upsamples
static bool s_BloomChainRendered = false; // the composite samples the chain, so it is rendered at least once even when disabled
static H2M::RefH2M<H2M::Texture2D_H2M> s_BloomDirtTexture;
static std::string s_PendingBloomDirtFilename; // requested from the UI, loaded at the start of the next Draw

static void CreateBloomResources();
static void ResizeBloomResources();
static void WriteBloomDescriptorSets();
static void RecordBloomPasses(VkCommandBuffer commandBuffer);

// Editor grid on the ground plane (Resources/Shaders/Grid.glsl), as in SceneHazelEnvMap
static H2M::RefH2M<H2M::PipelineH2M> s_GridPipeline;
static H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet s_GridDescriptorSet; // allocated and written once (camera uniform buffer)
static bool s_DisplayGrid = true;
static float s_GridScale = 16.025f; // cells across the grid (it spans 32 x 32 units)
static float s_GridSize = 0.025f;   // line width, as a fraction of a cell

// Editor overlays: selection outline, wireframe and bounding boxes (as "Display Outline / Wireframe / Bounding Boxes" in
// SceneHazelEnvMap). They are drawn into two LDR framebuffers, which the viewport composite adds after tonemapping, so they
// keep their colors (no exposure, tonemapping or bloom):
// - overlay: wireframe (depth tested against all meshes, drawn there first) and bounding boxes (always on top)
// - selection mask: silhouette of the selected model or mesh; the composite draws the outline around it
enum OverlayScope { OverlayScopeOff = 0, OverlayScopeSelected = 1, OverlayScopeAll = 2 };
static const char* s_OverlayScopeNames[] = { "Off", "Selected", "All" };

struct EditorOverlaySettings
{
	bool Outline = true;
	float OutlineWidth = 2.0f; // pixels (1..10)
	glm::vec4 OutlineColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
	int Wireframe = OverlayScopeOff;
	glm::vec4 WireframeColor = glm::vec4(0.1f, 0.9f, 0.3f, 1.0f);
	int BoundingBoxes = OverlayScopeOff;
	glm::vec4 BoundingBoxColor = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f);
	glm::vec4 SelectedBoundingBoxColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); // the selected mesh's box (all boxes of the selected model when no mesh is selected)
	float LineWidth = 1.0f; // wireframe, bounding boxes, and the normal/tangent/bitangent lines (pixels, 1..10)

	// Vertex vectors: a line per vertex along its normal, tangent and/or bitangent
	int Vectors = OverlayScopeOff;
	bool ShowNormals = true;
	bool ShowTangents = false;
	bool ShowBitangents = false;
	float VectorLength = 2.0f; // percent of the model's size (its bounding box diagonal), so it suits models of any scale
	int VectorColorMode = 0;   // 0: by vector (tangent red, bitangent green, normal blue), 1: by direction (xyz as rgb)
};
static EditorOverlaySettings s_OverlaySettings;
static H2M::RefH2M<H2M::FramebufferH2M> s_OverlayFramebuffer;
static H2M::RefH2M<H2M::FramebufferH2M> s_SelectionMaskFramebuffer;
static H2M::RefH2M<H2M::PipelineH2M> s_OverlayDepthPipeline;        // meshes with color alpha 0: depth only, hides the wireframe behind them
static H2M::RefH2M<H2M::PipelineH2M> s_OverlayDepthPipelineAnim;
static H2M::RefH2M<H2M::PipelineH2M> s_WireframePipeline;
static H2M::RefH2M<H2M::PipelineH2M> s_WireframePipelineAnim;
static H2M::RefH2M<H2M::PipelineH2M> s_BoundingBoxPipeline;         // line list: unit cube edges (s_BoundingBoxVertexBuffer)
static H2M::RefH2M<H2M::PipelineH2M> s_SelectionMaskPipeline;
static H2M::RefH2M<H2M::PipelineH2M> s_SelectionMaskPipelineAnim;
static H2M::RefH2M<H2M::PipelineH2M> s_VectorsPipeline;             // line per vertex, instanced: the model's vertex buffer is per-instance data
static H2M::RefH2M<H2M::PipelineH2M> s_VectorsPipelineAnim;
static H2M::RefH2M<H2M::VertexBufferH2M> s_BoundingBoxVertexBuffer;
static const uint32_t s_BoundingBoxVertexCount = 24; // 12 edges

static void CreateEditorOverlayResources();
static void RecordEditorOverlayPasses(VkCommandBuffer commandBuffer);
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipeline;                 // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipelineAnim; // skinned models (HazelPBR_Anim.glsl): vertex layout with bone IDs and weights
static ImTextureID s_TextureID;                      // to be removed from VulkanRenderer
static bool s_ViewportTextureNeedsUpdate = false;     // the viewport framebuffer was recreated (resize)

// ImGui::Image() needs the viewport framebuffer's color image registered with the ImGui Vulkan backend
// (as a descriptor set). Called from OnImGuiRender, when the backend is initialized. After a resize the
// framebuffer has a new image, so the old registration is replaced.
static void RegisterViewportTextureWithImGui()
{
	if (s_TextureID)
	{
		// The old descriptor set may still be used by frames in flight (only happens on resize)
		vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
		ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)(uintptr_t)s_TextureID);
	}

	// The tonemapped image (not the linear HDR s_Framebuffer)
	auto vulkanFB = s_ViewportCompositeFramebuffer.As<H2M::VulkanFramebufferH2M>();
	const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
	s_TextureID = (ImTextureID)(uintptr_t)ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
	s_ViewportTextureNeedsUpdate = false;
}
static uint32_t s_ViewportWidth = 1280;              // to be removed from VulkanRenderer
static uint32_t s_ViewportHeight = 720;              // to be removed from VulkanRenderer
// Models submitted for this frame, with their transforms and the material of each mesh
struct SubmittedModel
{
	H2M::RefH2M<H2M::ModelH2M> Model;
	glm::mat4 Transform;
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> Materials;
};
static std::vector<SubmittedModel> s_SubmittedModels;

static H2M::RefH2M<H2M::MeshH2M> s_SelectedMesh;
static glm::mat4* s_Transform_ImGuizmo = nullptr;

struct VulkanRendererData
{
	VkCommandBuffer ActiveCommandBuffer = nullptr;
	H2M::RefH2M<H2M::Texture2D_H2M> BRDFLut;
	// Per-frame descriptor set of the mesh shaders (set 0, H2M::VulkanShaderH2M::FrameDescriptorSet): camera, scene data,
	// environment maps and BRDF LUT. Allocated once, bound once per frame for all meshes (see GeometryPass).
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet FrameDescriptorSet;
	// std::unordered_map<SceneRenderer*, std::vector<H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet>> RendererDescriptorSet;

	H2M::RefH2M<H2M::VertexBufferH2M> QuadVertexBuffer;
	H2M::RefH2M<H2M::IndexBufferH2M> QuadIndexBuffer;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet QuadDescriptorSet;

	// float Exposure = 0.8f; // to be removed from VulkanRenderer

	struct SceneInfo
	{
		H2M::SceneRendererCameraH2M SceneCamera;
		H2M::EnvironmentH2M SceneEnvironment;
		float SkyboxLod;
	} SceneData;

	std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> EnvironmentMap;

	/**** BEGIN dtrajko Keep smart references alive ****/
	H2M::RefH2M<H2M::TextureCubeH2M> envUnfiltered;
	H2M::RefH2M<H2M::Texture2D_H2M> envEquirect;
	H2M::RefH2M<H2M::TextureCubeH2M> envFiltered;
	H2M::RefH2M<H2M::TextureCubeH2M> irradianceMap;
	/**** END dtrajko Keep smart references alive ****/

	H2M::RendererCapabilitiesH2M RenderCaps;

	VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
	std::vector<VkDescriptorPool> DescriptorPools;
	std::vector<uint32_t> DescriptorPoolAllocationCount;

	// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
	// std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
	// std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;

	// Default samplers
	VkSampler SamplerClamp = nullptr;

	int32_t SelectedDrawCall = -1;
	int32_t DrawCallCount = 0;

	// H2M::RefH2M<HazelShaderLibrary> m_ShaderLibrary;

	// dtrajko vulkan skybox
	H2M::RefH2M<VulkanSkyboxCube> VulkanSkyboxCube;
	H2M::RefH2M<H2M::PipelineH2M> SkyboxPipeline;
	H2M::RefH2M<H2M::ShaderH2M> SkyboxShader;

	/**** BEGIN temporary properties from VulkanTestLayer, while moving logic from VulkanTestLayer to VulkanRenderer ****/
	H2M::RefH2M<H2M::RenderPassH2M> GeoPass;
	H2M::RefH2M<H2M::PipelineH2M> GeometryPipeline;

	struct DrawCommand
	{
		H2M::RefH2M<H2M::ModelH2M> Model;
		H2M::RefH2M<H2M::MaterialH2M> Material;
		glm::mat4 Transform;
	};

	std::vector<DrawCommand> DrawList;
	std::vector<DrawCommand> SelectedMeshDrawList;

	H2M::SceneRendererOptionsH2M Options;
	/**** END temporary properties from VulkanTestLayer, while moving logic from VulkanTestLayer to VulkanRenderer ****/
};

static VulkanRendererData s_Data;

// Compute pipelines, descriptor sets and single-mip views used by CreateEnvironmentMap.
// Created on the first call and reused by every later load (the output cubemaps are reused too).
struct EnvMapComputeResources
{
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> EquirectPipeline;
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> MipFilterPipeline;
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> IrradiancePipeline;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet EquirectDescriptorSet;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet MipFilterDescriptorSets; // one set per envFiltered mip level
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet IrradianceDescriptorSet;
	std::vector<VkDescriptorImageInfo> MipImageInfos;                        // single-mip storage views of envFiltered
};
static EnvMapComputeResources s_EnvMapCompute;

// The skybox descriptor set is allocated once; it is (re)written only when the environment map changes,
// never while frames that use it may still be in flight
static H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet s_SkyboxDescriptorSet;
static bool s_SkyboxDescriptorSetNeedsUpdate = true;

// Environment map shown in the Environment panel, and a map requested from the UI (loaded at the start of the next Draw)
static std::string s_EnvMapFilename;
static std::string s_PendingEnvMapFilename;

// Exposure applied in the composite pass: user exposure * auto exposure (from the loaded environment map)
static float s_Exposure = 1.0f;
static float s_TonemapHuePreservation = 0.5f; // 0: ACES per channel (bright colors turn white), 1: hue-preserving (ViewportComposite.glsl)
static bool s_AutoExposureEnabled = true;
static float s_EnvMapAutoExposure = 1.0f;

/****
 * HDR environment maps differ in absolute brightness by several stops, so one fixed exposure can't suit them all.
 * Returns the exposure that maps the log-average luminance of the equirectangular map to middle grey (0.18),
 * weighted by the solid angle of each row (rows shrink towards the poles). Samples at most ~256x128 pixels.
 ****/
static float ComputeAutoExposure(H2M::RefH2M<H2M::Texture2D_H2M> equirect)
{
	H2M::BufferH2M pixels = equirect->GetWriteableBuffer();
	uint32_t width = equirect->GetWidth(), height = equirect->GetHeight();
	if (!pixels.Data || width == 0 || height == 0 || equirect->GetFormat() != H2M::ImageFormatH2M::RGBA32F ||
		pixels.Size < (uint64_t)width * height * 4 * sizeof(float))
	{
		return 1.0f;
	}

	const float* rgba = (const float*)pixels.Data;
	uint32_t stepX = glm::max(1u, width / 256), stepY = glm::max(1u, height / 128);
	double sumLogLuminance = 0.0, sumWeight = 0.0;
	for (uint32_t y = 0; y < height; y += stepY)
	{
		const double weight = glm::sin(glm::pi<double>() * (y + 0.5) / height);
		for (uint32_t x = 0; x < width; x += stepX)
		{
			const float* p = &rgba[((size_t)y * width + x) * 4];
			const double luminance = 0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2];
			sumLogLuminance += weight * glm::log(glm::max(luminance, 0.0) + 1e-4);
			sumWeight += weight;
		}
	}

	const double averageLuminance = glm::exp(sumLogLuminance / sumWeight);
	const float exposure = glm::clamp((float)(0.18 / averageLuminance), 0.05f, 20.0f);
	Log::GetLogger()->info("Environment map auto exposure: average luminance {0}, exposure {1}", averageLuminance, exposure);
	return exposure;
}

// Loads an .hdr file as the scene environment (called at the start of a frame, see Draw)
static void LoadEnvironmentMap(const std::string& filepath)
{
	if (!std::filesystem::exists(filepath) || !stbi_is_hdr(filepath.c_str()))
	{
		Log::GetLogger()->error("Environment map '{0}' was not loaded: the file does not exist or is not an HDR image. Keeping the current one.", filepath);
		return;
	}

	// The environment cubemaps are rewritten in place: nothing in flight may still be reading them
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());

	s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap(filepath);
	H2M::RendererH2M::SetSceneEnvironment(H2M::RefH2M<H2M::EnvironmentH2M>::Create(s_Data.EnvironmentMap.first, s_Data.EnvironmentMap.second), H2M::RefH2M<H2M::Image2D_H2M>());
	s_SkyboxDescriptorSetNeedsUpdate = true;
	s_EnvMapFilename = filepath;
}

// The scene's lights (the Lights uniform buffer of the PBR shaders) and the environment map rotation
static EnvMapVulkanLightEnvironment s_Lights;
// A real sun is taken out of a loaded environment map and moved to the directional sun (see ExtractSun)
static bool s_ExtractSunFromEnvironment = true;
static EnvMapVulkanExtractedSun s_ExtractedSun;   // of the current environment map
static bool s_PendingSunAlign = true;             // align the sun at the start of the next Draw (startup, a sun extracted)
// Cascaded shadow maps of the sun (see EnvMapVulkanShadows.h)
static EnvMapVulkanShadowMap s_ShadowMap;
static EnvMapVulkanShadowSettings s_ShadowSettings;
static EnvMapVulkanShadowPipeline s_ShadowPipeline;     // ShadowDepth.glsl (static models)
static EnvMapVulkanShadowPipeline s_ShadowPipelineAnim; // ShadowDepth_Anim.glsl (skinned models)
// Shadow maps of the spot and point lights (see EnvMapVulkanShadows.h): a layer per shadowed spot light, a cube per
// shadowed point light (only when the GPU supports cube map arrays)
static EnvMapVulkanShadowMap s_SpotShadowMaps;
static EnvMapVulkanShadowMap s_PointShadowMaps;
static EnvMapVulkanLocalShadowSettings s_LocalShadowSettings;

// World bounds of each submitted model (same order as s_SubmittedModels), for the shadow passes: the cascades are fitted around all
// of them, and a spot or point light draws only the models within its range. Computed once per frame.
static std::vector<std::pair<glm::vec3, glm::vec3>> s_ShadowCasterBounds;
// Their union, this frame (min > max: no models)
static glm::vec3 s_ShadowCasterBoundsMin = glm::vec3(1.0f);
static glm::vec3 s_ShadowCasterBoundsMax = glm::vec3(-1.0f);
static glm::uvec2 s_PendingLocalShadowResolutions = glm::uvec2(0); // spot, point: chosen in the Lights panel, applied in Draw (0: none)
// The spot / point shadow map viewer of the Lights panel: requested while the panel shows it, recorded with the next frame
static EnvMapVulkanShadowMapViewer s_ShadowMapViewer;
struct ShadowMapViewerRequest
{
	bool Active = false;
	bool Cube = false;      // a point light's cube (or a spot light's map)
	uint32_t BaseLayer = 0;
	float Near = 0.0f;
	float Far = 1.0f;
};
static ShadowMapViewerRequest s_ShadowMapViewerRequest;

// This frame's shadow slots of the spot and point lights: which light (index into s_Lights.SpotLights / PointLights) has
// which layer of s_SpotShadowMaps / which cube of s_PointShadowMaps, and their matrices. The first lights in the list that
// shine (enabled, intensity above 0) and have Cast Shadows on get the slots.
struct LocalShadowSlots
{
	uint32_t SpotCount = 0;
	uint32_t PointCount = 0;
	std::array<int, MaxShadowedSpotLights> SpotLight = {};
	std::array<int, MaxShadowedPointLights> PointLight = {};
	std::array<glm::mat4, MaxShadowedSpotLights> SpotViewProjection = {};
	std::array<std::array<glm::mat4, 6>, MaxShadowedPointLights> PointFaceViewProjection = {};
	uint32_t ModelsDrawn = 0; // over all local light passes, this frame
};
static LocalShadowSlots s_LocalShadowSlots;

static void AssignLocalShadowSlots()
{
	LocalShadowSlots& slots = s_LocalShadowSlots;
	slots.SpotCount = 0;
	slots.PointCount = 0;
	for (int i = 0; i < (int)s_Lights.SpotLights.size() && slots.SpotCount < MaxShadowedSpotLights && s_SpotShadowMaps.IsValid(); i++)
	{
		const EnvMapVulkanSpotLight& light = s_Lights.SpotLights[i];
		if (light.Enabled && light.CastShadows && light.Intensity > 0.0f)
		{
			slots.SpotViewProjection[slots.SpotCount] = ComputeSpotShadowViewProjection(light.Position, light.GetDirection(), light.OuterAngle, light.Range);
			slots.SpotLight[slots.SpotCount++] = i;
		}
	}
	for (int i = 0; i < (int)s_Lights.PointLights.size() && slots.PointCount < MaxShadowedPointLights && s_PointShadowMaps.IsValid(); i++)
	{
		const EnvMapVulkanPointLight& light = s_Lights.PointLights[i];
		if (light.Enabled && light.CastShadows && light.Intensity > 0.0f)
		{
			slots.PointFaceViewProjection[slots.PointCount] = ComputePointShadowFaceViewProjections(light.Position, light.Range);
			slots.PointLight[slots.PointCount++] = i;
		}
	}
}

// Each light's shadow slot (same order as s_Lights.PointLights / SpotLights), -1 without one: for packing the Lights buffer
static void GetShadowSlotsPerLight(std::vector<int>& pointSlots, std::vector<int>& spotSlots)
{
	pointSlots.assign(s_Lights.PointLights.size(), -1);
	spotSlots.assign(s_Lights.SpotLights.size(), -1);
	for (uint32_t slot = 0; slot < s_LocalShadowSlots.PointCount; slot++)
	{
		pointSlots[s_LocalShadowSlots.PointLight[slot]] = (int)slot;
	}
	for (uint32_t slot = 0; slot < s_LocalShadowSlots.SpotCount; slot++)
	{
		spotSlots[s_LocalShadowSlots.SpotLight[slot]] = (int)slot;
	}
}

static std::array<EnvMapVulkanShadowCascade, ShadowCascadeCount> s_ShadowCascades; // this frame's, see RecordShadowPasses
static bool s_ShadowsRendered = false; // the shadow map holds this frame's cascades
static uint32_t s_PendingShadowResolution = 0; // chosen in the Lights panel, applied at the start of the next Draw
static void WriteShadowMapDescriptor();
static float s_EnvMapRotation = 0.0f; // degrees, applied to the PBR environment lookups (as in SceneHazelEnvMap)

// Models loaded from the Models and Meshes panel: the Vulkan counterpart of the mesh entities in SceneHazelEnvMap.
// Terms: a model is a loaded model file (an H2M::ModelH2M); a mesh is one part of it (an H2M::MeshH2M).
struct LoadedModelVulkan
{
	H2M::RefH2M<H2M::ModelH2M> Model;
	std::string FilePath;
	glm::vec3 Translation = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f); // degrees
	glm::vec3 Scale = glm::vec3(1.0f);
	// Material slots: the Material Library material each mesh is drawn with (same order as the model's meshes)
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> MeshMaterials;
	// Each mesh's transform (its place in the model) as loaded from the file, for "Reset Mesh". The current ones are the
	// meshes' own (MeshH2M::Transform): every loaded model has its own ModelH2M, so editing them changes only this model.
	std::vector<glm::mat4> OriginalMeshTransforms;

	// Composed with ImGuizmo's own convention (Euler angles in degrees), so the gizmo (Manipulate +
	// DecomposeMatrixToComponents) and the values in the Models and Meshes panel round-trip exactly
	glm::mat4 GetTransform() const
	{
		glm::mat4 transform;
		ImGuizmo::RecomposeMatrixFromComponents(&Translation.x, &Rotation.x, &Scale.x, glm::value_ptr(transform));
		return transform;
	}
};
static std::vector<LoadedModelVulkan> s_LoadedModels;
static int s_SelectedModelIndex = -1;
static int s_SelectedMeshIndex = -1; // mesh of the selected model; -1 = none
static std::string s_PendingModelFilename;  // requested from the UI, loaded at the start of the next Draw
static std::optional<glm::vec3> s_PendingModelGroundPosition; // dropped on the viewport: where the model is placed (see LoadModel)
static int s_PendingRemoveModelIndex = -1;  // requested from the UI, removed at the start of the next Draw
static int s_PendingRemoveMeshIndex = -1; // a part of the selected model, requested from the UI, removed at the start of the next Draw

// Screen rectangle of the scene image in the Viewport window (for the gizmo and mouse picking)
static ImVec2 s_ViewportImageMin = ImVec2(0.0f, 0.0f);
static ImVec2 s_ViewportImageSize = ImVec2(0.0f, 0.0f);

// Mouse position in normalized device coordinates of the scene image in the Viewport window (-1..1, +y up).
// The image is shown vertically flipped (see ImGui::Image in OnImGuiRender), so its top edge is NDC y = +1.
static glm::vec2 GetViewportMouseNdc()
{
	ImVec2 mouse = ImGui::GetMousePos();
	return glm::vec2((mouse.x - s_ViewportImageMin.x) / s_ViewportImageSize.x * 2.0f - 1.0f,
		1.0f - (mouse.y - s_ViewportImageMin.y) / s_ViewportImageSize.y * 2.0f);
}

// World space ray from the camera through a point of the viewport (NDC)
static void GetCameraRay(float ndcX, float ndcY, glm::vec3& origin, glm::vec3& direction)
{
	H2M::CameraH2M& camera = s_Data.SceneData.SceneCamera.Camera;
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 inverseViewProjection = glm::inverse(camera.GetProjectionMatrix() * view);

	origin = glm::vec3(glm::inverse(view)[3]); // camera position
	glm::vec4 farPoint = inverseViewProjection * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
	direction = glm::normalize(glm::vec3(farPoint) / farPoint.w - origin);
}

// Where a model dropped on the viewport goes: the point under the cursor on the ground plane (y = 0, the editor grid), or a few
// units in front of the camera when the cursor ray doesn't meet the ground nearby (looking at the sky or along the horizon)
static glm::vec3 GetDropGroundPosition(float ndcX, float ndcY)
{
	glm::vec3 origin, direction;
	GetCameraRay(ndcX, ndcY, origin, direction);

	const float maxDistance = 100.0f;
	if (direction.y < -1e-4f)
	{
		float t = -origin.y / direction.y;
		if (t > 0.0f && t < maxDistance)
		{
			return origin + direction * t;
		}
	}
	return origin + direction * 5.0f;
}

// Finds the model and mesh under the mouse: a ray through the cursor is tested against every mesh's bounding box,
// then its triangles (as in SceneHazelEnvMap); the nearest hit wins. hitModel/hitMesh are -1 when nothing is hit.
// ndcX, ndcY: cursor position in normalized device coordinates of the viewport (-1..1, +y up)
// hitPosition (optional): the world position of the hit, when something is hit
static void RaycastMesh(float ndcX, float ndcY, int& hitModel, int& hitMesh, glm::vec3* hitPosition = nullptr)
{
	glm::vec3 origin, direction;
	GetCameraRay(ndcX, ndcY, origin, direction);

	float nearestT = std::numeric_limits<float>::max();
	hitModel = -1;
	hitMesh = -1;

	for (int m = 0; m < (int)s_LoadedModels.size(); m++)
	{
		LoadedModelVulkan& entry = s_LoadedModels[m];
		glm::mat4 modelTransform = entry.GetTransform();
		auto& meshes = entry.Model->GetMeshes();

		for (int s = 0; s < (int)meshes.size(); s++)
		{
			// Ray in the mesh's local space; the direction is not renormalized, so t is the same distance
			// along the world ray for every mesh and the hits can be compared
			glm::mat4 toLocal = glm::inverse(modelTransform * meshes[s]->Transform);
			H2M::RayH2M ray = { glm::vec3(toLocal * glm::vec4(origin, 1.0f)), glm::mat3(toLocal) * direction };

			float t;
			if (!ray.IntersectsAABB(meshes[s]->BoundingBox, t) || t < 0.0f || t >= nearestT)
			{
				continue;
			}

			const auto triangles = entry.Model->GetTriangleCache((uint32_t)s);
			if (triangles.empty())
			{
				nearestT = t; hitModel = m; hitMesh = s; // no triangle data: the bounding box has to do
				continue;
			}
			for (const auto& triangle : triangles)
			{
				if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t) && t >= 0.0f && t < nearestT)
				{
					nearestT = t; hitModel = m; hitMesh = s;
				}
			}
		}
	}

	if (hitPosition && hitModel >= 0)
	{
		*hitPosition = origin + direction * nearestT;
	}
}

// Material Library (the materials themselves are in EnvMapVulkanMaterialLibrary)
static H2M::RefH2M<EnvMapVulkanMaterial> s_SelectedMaterial;      // edited in the Material Editor
static H2M::RefH2M<EnvMapVulkanMaterial> s_PendingDeleteMaterial; // requested from the UI, deleted at the start of the next Draw
static const char* s_MaterialPayload = "VULKAN_LIBRARY_MATERIAL";    // drag & drop payload: an index into EnvMapVulkanMaterialLibrary::GetMaterials()

// A map assigned or removed in the Material Editor (button or drag & drop), applied at the start of the next Draw
struct PendingMaterialTexture
{
	H2M::RefH2M<EnvMapVulkanMaterial> Material;
	uint32_t Slot;        // EnvMapVulkanMaterial::Map
	std::string FilePath; // empty: remove the map
};
static std::vector<PendingMaterialTexture> s_PendingMaterialTextures;

static bool IsImageFile(const std::string& filepath)
{
	static const std::set<std::string> s_Extensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", ".gif" };
	std::string extension = std::filesystem::path(filepath).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return s_Extensions.find(extension) != s_Extensions.end();
}

static bool IsModelFile(const std::string& filepath)
{
	static const std::set<std::string> s_Extensions = { ".fbx", ".obj", ".gltf", ".glb", ".dae", ".3ds", ".blend", ".ply", ".stl", ".x", ".md5mesh" };
	std::string extension = std::filesystem::path(filepath).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return s_Extensions.find(extension) != s_Extensions.end();
}

// Number of meshes, over all loaded models, drawn with the material
static int CountMaterialUsers(const H2M::RefH2M<EnvMapVulkanMaterial>& material)
{
	int count = 0;
	for (const LoadedModelVulkan& entry : s_LoadedModels)
	{
		count += (int)std::count(entry.MeshMaterials.begin(), entry.MeshMaterials.end(), material);
	}
	return count;
}

// The Material Editor follows the selection: selecting a mesh (Models and Meshes panel or viewport) selects the material it is drawn with
static void SyncSelectedMaterial()
{
	static int s_LastModelIndex = -1;
	static int s_LastMeshIndex = -1;
	if (s_SelectedModelIndex == s_LastModelIndex && s_SelectedMeshIndex == s_LastMeshIndex)
	{
		return;
	}
	s_LastModelIndex = s_SelectedModelIndex;
	s_LastMeshIndex = s_SelectedMeshIndex;

	if (s_SelectedModelIndex >= 0 && s_SelectedModelIndex < (int)s_LoadedModels.size())
	{
		const auto& slots = s_LoadedModels[s_SelectedModelIndex].MeshMaterials;
		if (s_SelectedMeshIndex >= 0 && s_SelectedMeshIndex < (int)slots.size())
		{
			s_SelectedMaterial = slots[s_SelectedMeshIndex];
		}
	}
}

static glm::mat4 GetMeshTransform(H2M::RefH2M<H2M::ModelH2M> model, const H2M::RefH2M<H2M::MeshH2M>& mesh, const glm::mat4& transform);

// Loads a model file and adds it to the scene (called at the start of a frame, see Draw). With a ground position (a model dropped
// on the viewport), the model stands on that point: the bottom center of its bounding box is placed there, rather than its origin.
// A model far too large or too small for the view (e.g. modeled in millimeters) is also scaled to fit (see below).
static void LoadModel(const std::string& filepath, std::optional<glm::vec3> groundPosition = std::nullopt)
{
	if (!std::filesystem::exists(filepath) || !IsModelFile(filepath))
	{
		Log::GetLogger()->error("Model '{0}' was not loaded: the file does not exist or is not a supported model file.", filepath);
		return;
	}

	H2M::RefH2M<H2M::ModelH2M> model = H2M::RefH2M<H2M::ModelH2M>::Create(filepath);
	if (!model || model->GetMeshes().empty())
	{
		Log::GetLogger()->error("Model '{0}' was not loaded: no geometry found.", filepath);
		return;
	}

	if (model->HasAnimations())
	{
		Log::GetLogger()->info("Model '{0}': {1} animation(s), {2} bones", filepath, model->GetAnimationCount(), model->GetBoneCount());
	}

	LoadedModelVulkan entry;
	entry.Model = model;
	entry.FilePath = filepath;

	// The model's materials go into the Material Library (reused if this model was loaded before); each mesh starts
	// with the material the model assigns to it, or the library's Default material if the model has none for it
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> modelMaterials = EnvMapVulkanMaterialLibrary::ImportModelMaterials(model);
	for (auto& mesh : model->GetMeshes())
	{
		entry.MeshMaterials.push_back(mesh->MaterialIndex < modelMaterials.size() ?
			modelMaterials[mesh->MaterialIndex] : EnvMapVulkanMaterialLibrary::GetDefaultMaterial());
		entry.OriginalMeshTransforms.push_back(mesh->Transform);
	}

	if (groundPosition)
	{
		// Bounding box of the model at rest: the corners of every mesh's bounding box, placed as they are drawn
		glm::vec3 boundsMin(std::numeric_limits<float>::max());
		glm::vec3 boundsMax(-std::numeric_limits<float>::max());
		for (auto& mesh : model->GetMeshes())
		{
			glm::mat4 meshTransform = GetMeshTransform(model, mesh, glm::mat4(1.0f));
			const H2M::AABB_H2M& box = mesh->BoundingBox;
			for (int corner = 0; corner < 8; corner++)
			{
				glm::vec3 point((corner & 1) ? box.Max.x : box.Min.x, (corner & 2) ? box.Max.y : box.Min.y, (corner & 4) ? box.Max.z : box.Min.z);
				glm::vec3 placed = glm::vec3(meshTransform * glm::vec4(point, 1.0f));
				boundsMin = glm::min(boundsMin, placed);
				boundsMax = glm::max(boundsMax, placed);
			}
		}

		if (boundsMin.x <= boundsMax.x)
		{
			// Fit to the view, only for extreme cases: a model that can't fit in the view at all (larger than 10x the distance from the
			// camera to the drop point, e.g. modeled in millimeters) or would be a few pixels (smaller than 1/200 of that distance)
			// gets a uniform scale that makes it a quarter of the distance. Every other model keeps its scale of 1.
			float size = glm::compMax(boundsMax - boundsMin);
			glm::vec3 cameraPosition = glm::vec3(glm::inverse(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix())[3]);
			float distance = glm::max(glm::length(*groundPosition - cameraPosition), 1.0f);
			if (size > 0.0f && (size > distance * 10.0f || size < distance * 0.005f))
			{
				entry.Scale = glm::vec3(distance * 0.25f / size);
				Log::GetLogger()->info("Model '{0}' is {1} units across: scaled by {2} to fit the view (set Scale to 1 for its original size)",
					filepath, size, entry.Scale.x);
			}

			// The bottom center of the (scaled) bounding box goes to the drop point
			glm::vec3 bottomCenter((boundsMin.x + boundsMax.x) * 0.5f, boundsMin.y, (boundsMin.z + boundsMax.z) * 0.5f);
			entry.Translation = *groundPosition - bottomCenter * entry.Scale;
		}
		else
		{
			entry.Translation = *groundPosition;
		}
	}

	s_LoadedModels.push_back(entry);
	s_SelectedModelIndex = (int)s_LoadedModels.size() - 1;
	s_SelectedMeshIndex = -1;
	Log::GetLogger()->info("Model '{0}' loaded: {1} meshes, {2} materials", filepath, model->GetMeshes().size(), modelMaterials.size());
}

// Loads an image and binds it as one of a material's maps, or removes the map when the request has no file path
// (called at the start of a frame, see Draw)
static void ApplyMaterialTexture(const PendingMaterialTexture& request)
{
	H2M::RefH2M<EnvMapVulkanMaterial> material = request.Material;
	if (!material || request.Slot >= EnvMapVulkanMaterial::MapCount)
	{
		return;
	}

	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	if (request.FilePath.empty())
	{
		vkDeviceWaitIdle(device); // the descriptor set may be used by frames in flight
		material->RemoveMap(request.Slot);
		Log::GetLogger()->info("{0} removed from material '{1}'", EnvMapVulkanMaterial::GetMapTextureName(request.Slot), material->GetName());
		return;
	}

	if (!std::filesystem::exists(request.FilePath) || !IsImageFile(request.FilePath))
	{
		Log::GetLogger()->error("Map '{0}' was not loaded: the file does not exist or is not a supported image.", request.FilePath);
		return;
	}

	// Through the texture cache: an image already loaded (by a model or another material) in this slot's color space is shared
	H2M::RefH2M<H2M::Texture2D_H2M> texture = ResourceManager::LoadTexture2D_H2M(request.FilePath, EnvMapVulkanMaterial::IsColorMap(request.Slot));
	if (!texture || !texture->Loaded())
	{
		Log::GetLogger()->error("Map '{0}' could not be loaded.", request.FilePath);
		return;
	}

	vkDeviceWaitIdle(device); // the descriptor set may be used by frames in flight
	material->SetMap(request.Slot, texture);

	Log::GetLogger()->info("Map '{0}' assigned to {1} of material '{2}'", request.FilePath, EnvMapVulkanMaterial::GetMapTextureName(request.Slot), material->GetName());
}

// Deletes a material from the library (called at the start of a frame, see Draw). Meshes that used it get the Default material.
static void DeleteMaterial(H2M::RefH2M<EnvMapVulkanMaterial> material)
{
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice()); // its descriptor set may be used by frames in flight

	int users = CountMaterialUsers(material);
	EnvMapVulkanMaterialLibrary::Remove(material);
	if (users > 0)
	{
		H2M::RefH2M<EnvMapVulkanMaterial> replacement = EnvMapVulkanMaterialLibrary::GetDefaultMaterial(); // a new one if the Default was deleted
		for (LoadedModelVulkan& entry : s_LoadedModels)
		{
			std::replace(entry.MeshMaterials.begin(), entry.MeshMaterials.end(), material, replacement);
		}
	}
	if (s_SelectedMaterial == material)
	{
		s_SelectedMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
	}
	Log::GetLogger()->info("Material '{0}' deleted ({1} meshes now use the Default material)", material->GetName(), users);
}

// The sun turns with the environment map (EnvMapVulkanDirectionalLight::FollowEnvironmentRotation). Checked once per frame,
// so every way of changing the rotation (Environment panel, code) is covered. The shaders look the environment up at
// RotateVectorAboutY(rotation, worldDirection), so a direction of the map has the azimuth (map azimuth + rotation) in
// the world: a rotation change of N degrees moves the sun by N degrees of azimuth.
static void SyncSunWithEnvironmentRotation()
{
	static float s_LastEnvMapRotation = s_EnvMapRotation;
	float delta = s_EnvMapRotation - s_LastEnvMapRotation;
	s_LastEnvMapRotation = s_EnvMapRotation;
	if (delta == 0.0f || !s_Lights.Sun.FollowEnvironmentRotation)
	{
		return;
	}
	// The rotation wraps around at 360 degrees: 359 -> 1 is a change of +2, not -358
	delta = std::remainder(delta, 360.0f);
	s_Lights.Sun.Azimuth = std::remainder(s_Lights.Sun.Azimuth + delta, 360.0f); // stays in -180..180
}

// The water plane (Water panel, see EnvMapVulkanWater.h)
static EnvMapVulkanWaterSettings s_WaterSettings;
static EnvMapVulkanWater s_Water;

// Toward the sun for the water (the light that gets into it, the caustics): pointing down while the sun is off
static glm::vec3 GetWaterSunDirection()
{
	return s_Lights.Sun.Enabled && s_Lights.Sun.Intensity > 0.0f ? s_Lights.Sun.GetDirection() : glm::vec3(0.0f, -1.0f, 0.0f);
}

// Per-frame uniform buffers of the mesh shaders (set 0): written once per frame, read by every mesh. They belong to the
// HazelPBR_Static shader in the shader library, whose buffers the per-frame descriptor set points to (see Init).
static void UpdateFrameUniforms()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	H2M::CameraH2M& camera = s_Data.SceneData.SceneCamera.Camera;
	const uint32_t frameSet = H2M::VulkanShaderH2M::FrameDescriptorSet;

	// binding 0: Camera (vertex stage)
	glm::mat4 viewProjection = camera.GetViewProjection();
	void* ubPtr = shader->MapUniformBuffer(0, frameSet);
	memcpy(ubPtr, &viewProjection, sizeof(glm::mat4));
	shader->UnmapUniformBuffer(0, frameSet);

	// binding 1: SceneData (fragment stage), see EnvMapVulkanSceneDataGPU: with the water, the light under it (the
	// water volume stays off: in the scene pass the water's full-screen pass adds it, see EnvMapVulkanWater)
	EnvMapVulkanSceneDataGPU ub;
	ub.CameraPosition = camera.GetPosition();
	ub.EnvMapRotation = s_EnvMapRotation;
	FillWaterSceneData(s_WaterSettings, camera.GetPosition(), GetWaterSunDirection(), ub);

	ubPtr = shader->MapUniformBuffer(1, frameSet);
	memcpy(ubPtr, &ub, sizeof(ub));
	shader->UnmapUniformBuffer(1, frameSet);

	// binding 5: Lights (fragment stage), see EnvMapVulkanLightsGPU
	SyncSunWithEnvironmentRotation();
	// The shadow slots of the spot and point lights first: the packed lights carry them (the shadow passes, recorded later
	// this frame, use the same ones)
	AssignLocalShadowSlots();
	std::vector<int> pointShadowSlots, spotShadowSlots;
	GetShadowSlotsPerLight(pointShadowSlots, spotShadowSlots);
	EnvMapVulkanLightsGPU::LightsUB lights;
	s_Lights.Pack(lights, pointShadowSlots, spotShadowSlots);

	ubPtr = shader->MapUniformBuffer(5, frameSet);
	memcpy(ubPtr, &lights, sizeof(EnvMapVulkanLightsGPU::LightsUB));
	shader->UnmapUniformBuffer(5, frameSet);
}

// Per-object uniform buffers (set 2): the bone matrices of the current animation frame of a skinned model (up to 128).
// Every model has its own shader instance (see ModelH2M::Create), so every skinned model has its own bone buffer.
static void UpdateObjectUniforms(const H2M::RefH2M<H2M::ModelH2M>& model)
{
	H2M::RefH2M<H2M::ModelH2M> modelRef = model;
	if (!modelRef->IsSkinned() || modelRef->GetObjectDescriptorSet() == VK_NULL_HANDLE)
	{
		return;
	}

	const std::vector<glm::mat4>& boneTransforms = modelRef->GetBoneTransforms();
	size_t boneCount = std::min<size_t>(boneTransforms.size(), 128);
	if (boneCount > 0)
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = modelRef->GetMeshShader().As<H2M::VulkanShaderH2M>();
		void* ubPtr = shader->MapUniformBuffer(0, H2M::VulkanShaderH2M::ObjectDescriptorSet);
		memcpy(ubPtr, boneTransforms.data(), boneCount * sizeof(glm::mat4));
		shader->UnmapUniformBuffer(0, H2M::VulkanShaderH2M::ObjectDescriptorSet);
	}
}

static bool IsEnvironmentMapFile(const std::string& filepath)
{
	std::string extension = std::filesystem::path(filepath).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return extension == ".hdr";
}

// Accepts a file dragged from the Content Browser over the current drop target, if accepts(path) is true. While dragging, an
// accepted file outlines the drop area (highlightMin..highlightMax) and any other file shows what can be dropped there
// (expected, e.g. "a model file"). Returns true once an accepted file is dropped, with its path in filepath.
static bool AcceptFileDrop(const ImVec2& highlightMin, const ImVec2& highlightMax, bool (*accepts)(const std::string&), const char* expected,
	std::string& filepath)
{
	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM",
		ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
	if (!payload)
	{
		return false;
	}

	std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
	if (!accepts(itemPath))
	{
		ImGui::SetTooltip("'%s' can't be dropped here: drop %s", std::filesystem::path(itemPath).filename().string().c_str(), expected);
		return false;
	}

	ImGui::GetForegroundDrawList()->AddRect(highlightMin, highlightMax, ImGui::GetColorU32(ImGuiCol_DragDropTarget), 0.0f, 0, 2.0f);
	if (!payload->IsDelivery())
	{
		return false;
	}
	filepath = itemPath;
	return true;
}

// The selected light: the sun, or an index into the point or spot light list. Lights and models share one selection
// (one gizmo): selecting a light clears the model selection and selecting a model or mesh clears the light selection.
enum class LightKind { None, Sun, Point, Spot };
static LightKind s_SelectedLightKind = LightKind::None;
static int s_SelectedLightIndex = 0;
static bool s_ShowLightGizmos = true; // light icons and shapes in the viewport
static bool s_ShowShadowsOnly = false; // Shadows Only view: the selected light's shadow (see ShadowDebugValue in the PBR shaders)

// The water plane (Water panel, see EnvMapVulkanWater.h, declared with the per-frame uniforms). It joins the same
// selection: selecting it clears the light and model selection, and selecting a light or a model clears it.
static bool s_WaterSelected = false;

static void SelectLight(LightKind kind, int index = 0)
{
	s_SelectedLightKind = kind;
	s_SelectedLightIndex = index;
	if (kind != LightKind::None)
	{
		s_SelectedModelIndex = -1;
		s_SelectedMeshIndex = -1;
		s_WaterSelected = false;
	}
}

static void SelectWater()
{
	s_WaterSelected = s_WaterSettings.Enabled;
	if (s_WaterSelected)
	{
		s_SelectedLightKind = LightKind::None;
		s_SelectedModelIndex = -1;
		s_SelectedMeshIndex = -1;
	}
}

// Called once per frame: a model or mesh selected anywhere (Models and Meshes panel, viewport, a dropped model) clears the light selection,
// and a selection that no longer exists (a deleted light) is cleared
static void SyncLightSelection()
{
	static int s_LastModelIndex = -1;
	static int s_LastMeshIndex = -1;
	if (s_SelectedModelIndex >= 0 && (s_SelectedModelIndex != s_LastModelIndex || s_SelectedMeshIndex != s_LastMeshIndex))
	{
		s_SelectedLightKind = LightKind::None;
		s_WaterSelected = false;
	}
	s_LastModelIndex = s_SelectedModelIndex;
	s_LastMeshIndex = s_SelectedMeshIndex;

	if ((s_SelectedLightKind == LightKind::Point && s_SelectedLightIndex >= (int)s_Lights.PointLights.size()) ||
		(s_SelectedLightKind == LightKind::Spot && s_SelectedLightIndex >= (int)s_Lights.SpotLights.size()))
	{
		s_SelectedLightKind = LightKind::None;
	}
	if (!s_WaterSettings.Enabled)
	{
		s_WaterSelected = false; // the water was removed
	}
}

// Points the sun at the brightest light source of the environment map (the sun of an outdoor HDR, a bright window
// indoors) and gives it that light's color. The intensity stays as it is.
static void AlignSunToEnvironment()
{
	H2M::RefH2M<H2M::Texture2D_H2M> equirect = s_Data.envEquirect;
	if (!equirect)
	{
		return;
	}
	glm::vec3 mapDirection, color;
	if (s_ExtractedSun.Found)
	{
		// The sun taken out of the map: its direction, color and intensity (the directional sun now carries its light)
		mapDirection = s_ExtractedSun.Direction;
		color = s_ExtractedSun.Color;
		s_Lights.Sun.Intensity = s_ExtractedSun.Intensity;
	}
	else
	{
		// No sun in the map: the brightest region (a window, bright sky), direction and color only
		H2M::BufferH2M pixels = equirect->GetWriteableBuffer();
		uint32_t width = equirect->GetWidth(), height = equirect->GetHeight();
		if (!pixels.Data || equirect->GetFormat() != H2M::ImageFormatH2M::RGBA32F || pixels.Size < (uint64_t)width * height * 4 * sizeof(float) ||
			!FindBrightestDirection((const float*)pixels.Data, width, height, mapDirection, color))
		{
			Log::GetLogger()->warn("Align to Environment: the environment map's pixels are not available");
			return;
		}
	}

	// The shaders look the environment up at RotateVectorAboutY(rotation, worldDirection): undo that rotation
	float angle = glm::radians(s_EnvMapRotation);
	float c = std::cos(angle), s = std::sin(angle);
	glm::vec3 worldDirection(c * mapDirection.x + s * mapDirection.z, mapDirection.y, -s * mapDirection.x + c * mapDirection.z);

	s_Lights.Sun.SetDirection(worldDirection);
	s_Lights.Sun.Color = color;
	Log::GetLogger()->info("Sun aligned to the environment: azimuth {0}, elevation {1}, color ({2}, {3}, {4}), intensity {5}",
		s_Lights.Sun.Azimuth, s_Lights.Sun.Elevation, color.r, color.g, color.b, s_Lights.Sun.Intensity);
}

// Where the sun's icon is drawn: in the sky, far away in the sun's direction from the camera, so it moves like the skybox
// (after "Align to Environment" it sits on the sun of the HDR map); once moved with the gizmo, at its icon position
// (editor only: a directional light has no position)
static glm::vec3 GetSunIconPosition()
{
	if (s_Lights.Sun.IconMoved)
	{
		return s_Lights.Sun.IconPosition;
	}
	glm::vec3 cameraPosition = glm::vec3(glm::inverse(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix())[3]);
	return cameraPosition + s_Lights.Sun.GetDirection() * 1000.0f;
}

// Where the sun's gizmo, arrow and aim line are: at the icon once it was moved; before that on the same line from the
// camera as the sky icon (so on top of it on screen), 10 units away, where dragging moves it at a usable scale
static glm::vec3 GetSunGizmoPosition()
{
	if (s_Lights.Sun.IconMoved)
	{
		return s_Lights.Sun.IconPosition;
	}
	glm::vec3 cameraPosition = glm::vec3(glm::inverse(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix())[3]);
	return cameraPosition + s_Lights.Sun.GetDirection() * 10.0f;
}

// World position -> viewport pixel (same NDC convention as GetViewportMouseNdc). False behind the camera.
static bool ProjectToViewport(const glm::mat4& viewProjection, const glm::vec3& position, ImVec2& pixel)
{
	glm::vec4 clip = viewProjection * glm::vec4(position, 1.0f);
	if (clip.w <= 1e-4f)
	{
		return false;
	}
	glm::vec2 ndc = glm::vec2(clip) / clip.w;
	pixel = ImVec2(s_ViewportImageMin.x + (ndc.x + 1.0f) * 0.5f * s_ViewportImageSize.x,
		s_ViewportImageMin.y + (1.0f - ndc.y) * 0.5f * s_ViewportImageSize.y);
	return true;
}

// A world space line; a line with an end behind the camera is skipped (the shapes are made of short segments)
static void DrawWorldLine(ImDrawList* drawList, const glm::mat4& viewProjection, const glm::vec3& a, const glm::vec3& b, ImU32 color, float thickness = 1.5f)
{
	ImVec2 pa, pb;
	if (ProjectToViewport(viewProjection, a, pa) && ProjectToViewport(viewProjection, b, pb))
	{
		drawList->AddLine(pa, pb, color, thickness);
	}
}

// A world space circle in the plane spanned by the unit vectors u and v
static void DrawWorldCircle(ImDrawList* drawList, const glm::mat4& viewProjection, const glm::vec3& center, const glm::vec3& u, const glm::vec3& v,
	float radius, ImU32 color, int segments = 48)
{
	const float step = glm::two_pi<float>() / segments;
	for (int i = 0; i < segments; i++)
	{
		glm::vec3 a = center + radius * (std::cos(i * step) * u + std::sin(i * step) * v);
		glm::vec3 b = center + radius * (std::cos((i + 1) * step) * u + std::sin((i + 1) * step) * v);
		DrawWorldLine(drawList, viewProjection, a, b, color);
	}
}

// Two unit vectors perpendicular to the unit vector w and to each other
static void GetPerpendicularAxes(const glm::vec3& w, glm::vec3& u, glm::vec3& v)
{
	glm::vec3 up = std::abs(w.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	u = glm::normalize(glm::cross(up, w));
	v = glm::cross(w, u);
}

// The light's color at full brightness (a dark red light still gets a clearly red icon); gray when disabled
static ImU32 GetLightGizmoColor(const glm::vec3& color, bool enabled, float alpha = 1.0f)
{
	if (!enabled)
	{
		return ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, alpha));
	}
	float maxChannel = std::max(color.r, std::max(color.g, color.b));
	glm::vec3 c = maxChannel > 1e-3f ? color / maxChannel : glm::vec3(1.0f);
	return ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, alpha));
}

// An icon: a disc in the light's color (a ring when disabled) on a dark backdrop, with rays; a white ring when selected.
// The rays and the ring have dark outlines, so a white icon stays visible on a bright sky.
// castsShadow: a small half-dark disc at the lower right (the light casts shadows this frame)
static void DrawLightIcon(ImDrawList* drawList, const ImVec2& center, ImU32 color, bool enabled, bool selected, int rays, bool castsShadow)
{
	const float radius = 6.0f;
	drawList->AddCircleFilled(center, radius + 3.0f, IM_COL32(0, 0, 0, 150));
	if (enabled)
	{
		drawList->AddCircleFilled(center, radius, color);
	}
	else
	{
		drawList->AddCircle(center, radius, color, 0, 2.0f);
	}
	for (int i = 0; i < rays; i++)
	{
		float a = glm::two_pi<float>() * i / rays;
		ImVec2 d(std::cos(a), std::sin(a));
		ImVec2 from(center.x + d.x * (radius + 4.0f), center.y + d.y * (radius + 4.0f));
		ImVec2 to(center.x + d.x * (radius + 8.0f), center.y + d.y * (radius + 8.0f));
		drawList->AddLine(from, to, IM_COL32(0, 0, 0, 150), 4.0f);
		drawList->AddLine(from, to, color, 2.0f);
	}
	if (selected)
	{
		drawList->AddCircle(center, radius + 11.0f, IM_COL32(0, 0, 0, 150), 0, 4.0f);
		drawList->AddCircle(center, radius + 11.0f, IM_COL32(255, 255, 255, 230), 0, 2.0f);
	}
	if (castsShadow)
	{
		const ImVec2 mark(center.x + radius + 7.0f, center.y + radius + 7.0f);
		drawList->AddCircleFilled(mark, 5.5f, IM_COL32(0, 0, 0, 200));
		drawList->PathArcTo(mark, 4.0f, -glm::half_pi<float>(), glm::half_pi<float>(), 10); // lit half (right)
		drawList->PathFillConvex(IM_COL32(255, 255, 255, 230));
		drawList->AddCircle(mark, 4.0f, IM_COL32(255, 255, 255, 230), 0, 1.0f);
	}
}

// Light gizmos over the viewport image. Not depth tested (as editor light gizmos in Unity and Unreal), so a light is
// never lost behind a model:
// - every light: an icon in its color (8 rays: point light, 4 rays and an aim line: spot light, 12 rays: the sun)
// - the selected point light: its range as three circles
// - the selected spot light: its range and outer cone (solid), inner cone (faint)
// - the selected sun: an arrow ending at the world origin, in the direction its light travels
static void DrawLightGizmos()
{
	if (!s_ShowLightGizmos || s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	const glm::mat4 viewProjection = s_Data.SceneData.SceneCamera.Camera.GetViewProjection();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(s_ViewportImageMin, ImVec2(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y), true);

	// Shapes of the selected light first, icons on top
	if (s_SelectedLightKind == LightKind::Sun)
	{
		// An arrow from the icon in the direction the light travels
		ImU32 color = GetLightGizmoColor(s_Lights.Sun.Color, s_Lights.Sun.Enabled, 0.8f);
		glm::vec3 travel = -s_Lights.Sun.GetDirection();
		glm::vec3 from = GetSunGizmoPosition();
		glm::vec3 to = from + travel * 2.0f;
		DrawWorldLine(drawList, viewProjection, from, to, color, 2.0f);
		glm::vec3 u, v;
		GetPerpendicularAxes(travel, u, v);
		for (int i = 0; i < 4; i++) // arrowhead
		{
			float a = glm::half_pi<float>() * i;
			DrawWorldLine(drawList, viewProjection, to, to - travel * 0.4f + (std::cos(a) * u + std::sin(a) * v) * 0.15f, color, 2.0f);
		}
	}
	else if (s_SelectedLightKind == LightKind::Point)
	{
		const EnvMapVulkanPointLight& light = s_Lights.PointLights[s_SelectedLightIndex];
		ImU32 color = GetLightGizmoColor(light.Color, light.Enabled, 0.7f);
		const glm::vec3 x(1.0f, 0.0f, 0.0f), y(0.0f, 1.0f, 0.0f), z(0.0f, 0.0f, 1.0f);
		DrawWorldCircle(drawList, viewProjection, light.Position, x, y, light.Range, color);
		DrawWorldCircle(drawList, viewProjection, light.Position, x, z, light.Range, color);
		DrawWorldCircle(drawList, viewProjection, light.Position, y, z, light.Range, color);
	}
	else if (s_SelectedLightKind == LightKind::Spot)
	{
		const EnvMapVulkanSpotLight& light = s_Lights.SpotLights[s_SelectedLightIndex];
		glm::vec3 direction = light.GetDirection();
		glm::vec3 u, v;
		GetPerpendicularAxes(direction, u, v);
		auto drawCone = [&](float angle, ImU32 color, int sides) {
			// The cone ends where it meets the range sphere
			float halfAngle = glm::radians(glm::clamp(angle, 0.1f, 89.0f));
			glm::vec3 center = light.Position + direction * (light.Range * std::cos(halfAngle));
			float radius = light.Range * std::sin(halfAngle);
			DrawWorldCircle(drawList, viewProjection, center, u, v, radius, color);
			for (int i = 0; i < sides; i++)
			{
				float a = glm::two_pi<float>() * i / sides;
				DrawWorldLine(drawList, viewProjection, light.Position, center + radius * (std::cos(a) * u + std::sin(a) * v), color);
			}
		};
		drawCone(light.OuterAngle, GetLightGizmoColor(light.Color, light.Enabled, 0.8f), 8);
		drawCone(light.InnerAngle, GetLightGizmoColor(light.Color, light.Enabled, 0.3f), 4);
	}

	std::vector<int> pointShadowSlots, spotShadowSlots;
	GetShadowSlotsPerLight(pointShadowSlots, spotShadowSlots);
	ImVec2 pixel;
	DrawWorldLine(drawList, viewProjection, GetSunGizmoPosition(), GetSunGizmoPosition() - s_Lights.Sun.GetDirection() * 0.75f,
		GetLightGizmoColor(s_Lights.Sun.Color, s_Lights.Sun.Enabled), 2.0f); // aim line, as a spot light's
	if (ProjectToViewport(viewProjection, GetSunIconPosition(), pixel))
	{
		DrawLightIcon(drawList, pixel, GetLightGizmoColor(s_Lights.Sun.Color, s_Lights.Sun.Enabled), s_Lights.Sun.Enabled,
			s_SelectedLightKind == LightKind::Sun, 12, s_ShadowsRendered);
	}
	for (int i = 0; i < (int)s_Lights.PointLights.size(); i++)
	{
		const EnvMapVulkanPointLight& light = s_Lights.PointLights[i];
		if (ProjectToViewport(viewProjection, light.Position, pixel))
		{
			DrawLightIcon(drawList, pixel, GetLightGizmoColor(light.Color, light.Enabled), light.Enabled,
				s_SelectedLightKind == LightKind::Point && s_SelectedLightIndex == i, 8, pointShadowSlots[i] >= 0);
		}
	}
	for (int i = 0; i < (int)s_Lights.SpotLights.size(); i++)
	{
		const EnvMapVulkanSpotLight& light = s_Lights.SpotLights[i];
		ImU32 color = GetLightGizmoColor(light.Color, light.Enabled);
		DrawWorldLine(drawList, viewProjection, light.Position, light.Position + light.GetDirection() * 0.75f, color, 2.0f);
		if (ProjectToViewport(viewProjection, light.Position, pixel))
		{
			DrawLightIcon(drawList, pixel, color, light.Enabled, s_SelectedLightKind == LightKind::Spot && s_SelectedLightIndex == i, 4,
				spotShadowSlots[i] >= 0);
		}
	}

	drawList->PopClipRect();
}

// The light whose icon is under the mouse (the nearest one within 12 pixels)
// The selected water's border: an orange rectangle (made of short segments, so the parts in front of the camera show when
// the rest is behind it)
static void DrawWaterOutline()
{
	if (!s_WaterSelected || s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	const glm::mat4 viewProjection = s_Data.SceneData.SceneCamera.Camera.GetViewProjection();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(s_ViewportImageMin, ImVec2(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y), true);
	const glm::mat4 transform = s_WaterSettings.GetTransform();
	const glm::vec3 corners[4] = {
		glm::vec3(transform * glm::vec4(-0.5f, 0.0f, -0.5f, 1.0f)), glm::vec3(transform * glm::vec4(0.5f, 0.0f, -0.5f, 1.0f)),
		glm::vec3(transform * glm::vec4(0.5f, 0.0f, 0.5f, 1.0f)), glm::vec3(transform * glm::vec4(-0.5f, 0.0f, 0.5f, 1.0f)) };
	const int segments = 32;
	for (int side = 0; side < 4; side++)
	{
		const glm::vec3& a = corners[side];
		const glm::vec3& b = corners[(side + 1) % 4];
		for (int i = 0; i < segments; i++)
		{
			DrawWorldLine(drawList, viewProjection, glm::mix(a, b, (float)i / segments), glm::mix(a, b, (float)(i + 1) / segments), IM_COL32(255, 140, 0, 255), 2.0f);
		}
	}
	drawList->PopClipRect();
}

static bool PickLightIcon(LightKind& kind, int& index)
{
	if (!s_ShowLightGizmos)
	{
		return false;
	}
	const glm::mat4 viewProjection = s_Data.SceneData.SceneCamera.Camera.GetViewProjection();
	const ImVec2 mouse = ImGui::GetMousePos();
	float nearest = 12.0f * 12.0f;
	bool found = false;
	auto test = [&](const glm::vec3& position, LightKind k, int i) {
		ImVec2 pixel;
		if (ProjectToViewport(viewProjection, position, pixel))
		{
			float dx = pixel.x - mouse.x, dy = pixel.y - mouse.y;
			if (dx * dx + dy * dy < nearest)
			{
				nearest = dx * dx + dy * dy;
				kind = k;
				index = i;
				found = true;
			}
		}
	};
	test(GetSunIconPosition(), LightKind::Sun, 0);
	for (int i = 0; i < (int)s_Lights.PointLights.size(); i++)
	{
		test(s_Lights.PointLights[i].Position, LightKind::Point, i);
	}
	for (int i = 0; i < (int)s_Lights.SpotLights.size(); i++)
	{
		test(s_Lights.SpotLights[i].Position, LightKind::Spot, i);
	}
	return found;
}

// The gizmo for the selected sun, at its icon: 1 moves the icon (editor only, the sun has no position), 2 turns the sun.
// Its local Z axis points towards the sun. The first change detaches the icon from the sky: from then on it stays where
// it is (rotating the sky icon would otherwise move it, and the gizmo with it, while turning).
static void ManipulateSun(int gizmoType, bool snap)
{
	const ImGuizmo::OPERATION operation = gizmoType == ImGuizmo::OPERATION::TRANSLATE ? ImGuizmo::OPERATION::TRANSLATE : ImGuizmo::OPERATION::ROTATE;
	glm::vec3 z = s_Lights.Sun.GetDirection(), x, y;
	GetPerpendicularAxes(z, x, y);
	glm::mat4 transform(1.0f);
	transform[0] = glm::vec4(x, 0.0f);
	transform[1] = glm::vec4(y, 0.0f);
	transform[2] = glm::vec4(z, 0.0f);
	transform[3] = glm::vec4(GetSunGizmoPosition(), 1.0f);

	float snapValue = operation == ImGuizmo::OPERATION::ROTATE ? 15.0f : 0.5f;
	float snapValues[3] = { snapValue, snapValue, snapValue };
	if (ImGuizmo::Manipulate(
		glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix()),
		glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix()),
		operation,
		ImGuizmo::WORLD,
		glm::value_ptr(transform),
		nullptr,
		snap ? snapValues : nullptr))
	{
		s_Lights.Sun.IconPosition = glm::vec3(transform[3]);
		s_Lights.Sun.IconMoved = true;
		if (operation == ImGuizmo::OPERATION::ROTATE)
		{
			s_Lights.Sun.SetDirection(glm::vec3(transform[2])); // azimuth and elevation follow (azimuth in -180..180)
		}
	}
}

// The gizmo for the selected point or spot light: 1 moves it; 2 aims a spot light (any mode moves a point light)
static void ManipulateSelectedLight(int gizmoType, bool snap)
{
	bool isSpot = s_SelectedLightKind == LightKind::Spot;
	glm::vec3& position = isSpot ? s_Lights.SpotLights[s_SelectedLightIndex].Position : s_Lights.PointLights[s_SelectedLightIndex].Position;
	ImGuizmo::OPERATION operation = isSpot && gizmoType == ImGuizmo::OPERATION::ROTATE ? ImGuizmo::OPERATION::ROTATE : ImGuizmo::OPERATION::TRANSLATE;

	// Local Z is the spot's direction
	glm::mat4 transform(1.0f);
	if (isSpot)
	{
		glm::vec3 z = s_Lights.SpotLights[s_SelectedLightIndex].GetDirection(), x, y;
		GetPerpendicularAxes(z, x, y);
		transform[0] = glm::vec4(x, 0.0f);
		transform[1] = glm::vec4(y, 0.0f);
		transform[2] = glm::vec4(z, 0.0f);
	}
	transform[3] = glm::vec4(position, 1.0f);

	float snapValue = operation == ImGuizmo::OPERATION::ROTATE ? 15.0f : 0.5f;
	float snapValues[3] = { snapValue, snapValue, snapValue };
	if (ImGuizmo::Manipulate(
		glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix()),
		glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix()),
		operation,
		ImGuizmo::WORLD,
		glm::value_ptr(transform),
		nullptr,
		snap ? snapValues : nullptr))
	{
		position = glm::vec3(transform[3]);
		if (isSpot && operation == ImGuizmo::OPERATION::ROTATE)
		{
			s_Lights.SpotLights[s_SelectedLightIndex].SetDirection(glm::vec3(transform[2]));
		}
	}
}

// Where a new light goes: where the camera looks, so the light is within reach of its default range:
// - the model surface at the center of the viewport
// - else the ground (y = 0) there, when it's not too far
// - else (the camera looks over the scene, e.g. horizontally) the point of the view ray nearest the models' center,
//   at the models' height
// The range of a new point or spot light at this position: far enough to reach every loaded model (the distance to the
// farthest corner of their bounds), at least 10, at most 1000 (the Range drag's limit). Without models: 10.
static float GetDefaultLightRange(const glm::vec3& position)
{
	float range = 10.0f;
	if (glm::all(glm::lessThanEqual(s_ShadowCasterBoundsMin, s_ShadowCasterBoundsMax)))
	{
		for (int c = 0; c < 8; c++)
		{
			glm::vec3 corner((c & 1) ? s_ShadowCasterBoundsMax.x : s_ShadowCasterBoundsMin.x, (c & 2) ? s_ShadowCasterBoundsMax.y : s_ShadowCasterBoundsMin.y,
				(c & 4) ? s_ShadowCasterBoundsMax.z : s_ShadowCasterBoundsMin.z);
			range = std::max(range, glm::length(corner - position));
		}
	}
	return std::min(range, 1000.0f);
}

static glm::vec3 GetLightSpawnTarget()
{
	int hitModel, hitMesh;
	glm::vec3 target;
	RaycastMesh(0.0f, 0.0f, hitModel, hitMesh, &target);
	if (hitModel < 0)
	{
		glm::vec3 origin, direction;
		GetCameraRay(0.0f, 0.0f, origin, direction);
		direction = glm::normalize(direction);
		float groundT = direction.y < -1e-4f ? -origin.y / direction.y : -1.0f;
		if (groundT > 0.0f && groundT < 50.0f)
		{
			target = origin + direction * groundT;
		}
		else
		{
			glm::vec3 sceneCenter(0.0f);
			for (const LoadedModelVulkan& entry : s_LoadedModels)
			{
				sceneCenter += entry.Translation / (float)s_LoadedModels.size();
			}
			float t = glm::max(glm::dot(sceneCenter - origin, direction), 1.0f);
			target = origin + direction * t;
			target.y = sceneCenter.y;
		}
	}
	return glm::any(glm::isnan(target)) ? glm::vec3(0.0f) : target; // NaN: the camera has no valid view matrix yet
}

// The shadow map's cascades in the Lights panel ("Show Shadow Map"): the scene seen from the sun, darker = closer to the sun
static bool s_ShowShadowMap = false;
static int s_ShadowMapShownCascade = 0;
static std::array<ImTextureID, ShadowCascadeCount> s_ShadowMapTextureIDs = {};
static uint32_t s_ShadowMapTexturesGeneration = 0; // the shadow map the IDs were registered for (EnvMapVulkanShadowMap::GetGeneration)

static void DrawShadowMapCascades()
{
	if (!s_ShadowMap.IsValid())
	{
		return;
	}
	// Registered with the ImGui Vulkan backend once per shadow map (again after it is recreated)
	if (s_ShadowMapTexturesGeneration != s_ShadowMap.GetGeneration())
	{
		for (uint32_t i = 0; i < ShadowCascadeCount; i++)
		{
			if (s_ShadowMapTextureIDs[i])
			{
				ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)(uintptr_t)s_ShadowMapTextureIDs[i]);
			}
			s_ShadowMapTextureIDs[i] = (ImTextureID)(uintptr_t)ImGui_ImplVulkan_AddTexture(s_ShadowMap.GetDisplaySampler(), s_ShadowMap.GetDisplayView(i),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		s_ShadowMapTexturesGeneration = s_ShadowMap.GetGeneration();
	}

	if (!s_ShadowsRendered)
	{
		ImGui::TextDisabled("No shadows this frame (sun off, Cast Shadows off or no models)");
		return;
	}
	// One cascade at a time, as large as the panel allows
	for (int i = 0; i < (int)ShadowCascadeCount; i++)
	{
		if (i > 0)
		{
			ImGui::SameLine();
		}
		char label[16];
		snprintf(label, sizeof(label), "%d##ShadowCascade", i);
		ImGui::RadioButton(label, &s_ShadowMapShownCascade, i);
	}
	const EnvMapVulkanShadowCascade& cascade = s_ShadowCascades[s_ShadowMapShownCascade];
	ImGui::TextDisabled("Up to %.1f units from the camera, %.1f mm per texel", cascade.SplitDistance, cascade.TexelWorldSize * 1000.0f);
	const float size = glm::max(ImGui::GetContentRegionAvail().x, 16.0f);
	ImGui::Image(s_ShadowMapTextureIDs[s_ShadowMapShownCascade], ImVec2(size, size));
}

// An azimuth drag without limits (as Env Map Rotation): it can be dragged endlessly in both directions and wraps around
// to stay in -180..180
static bool AzimuthProperty(float& azimuth)
{
	if (ImGuiWrapper::Property("Azimuth", azimuth, 0.5f, 0.0f, 0.0f, PropertyFlag::DragProperty)) // min = max = 0: no limits
	{
		azimuth = std::remainder(azimuth, 360.0f);
		return true;
	}
	return false;
}

// The sun's shadow settings, in the Lights panel when the sun is selected
static void OnImGuiRenderShadowSettings()
{
	if (!ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	EnvMapVulkanShadowSettings& settings = s_ShadowSettings;

	ImGui::Columns(2);
	ImGuiWrapper::Property("Distance", settings.Distance, 0.5f, 2.0f, 1000.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Shadows reach this far from the camera (they fade out before it).\nShorter: sharper shadows, as the cascades cover less");
	}

	// Resolution: the shadow map is recreated at the start of the next frame
	static const uint32_t s_Resolutions[] = { 1024, 2048, 4096 };
	static const char* s_ResolutionLabels[] = { "1024 (16 MB)", "2048 (64 MB)", "4096 (256 MB)" };
	uint32_t resolution = s_PendingShadowResolution ? s_PendingShadowResolution : s_ShadowMap.GetResolution();
	int selected = 1;
	for (int i = 0; i < 3; i++)
	{
		if (s_Resolutions[i] == resolution)
		{
			selected = i;
		}
	}
	ImGui::Text("Resolution");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	if (ImGui::BeginCombo("##ShadowResolution", s_ResolutionLabels[selected]))
	{
		for (int i = 0; i < 3; i++)
		{
			if (ImGui::Selectable(s_ResolutionLabels[i], i == selected))
			{
				s_PendingShadowResolution = s_Resolutions[i] != s_ShadowMap.GetResolution() ? s_Resolutions[i] : 0;
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of each of the 4 cascades (GPU memory for all of them)");
	}
	ImGui::PopItemWidth();
	ImGui::NextColumn();

	ImGuiWrapper::Property("Split", settings.SplitLambda, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How the distance is split into cascades:\n0 = equal parts, 1 = short near ones (sharp close to the camera), long far ones");
	}
	ImGuiWrapper::Property("Softness", settings.Softness, 0.0f, 4.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Width of the soft shadow edge (spacing of the 5 x 5 PCF samples, in shadow map texels)");
	}
	ImGuiWrapper::Property("Depth Bias", settings.DepthBias, 0.0f, 10.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Pushes the shadow map's depth away from the sun: removes \"shadow acne\" (stripes on lit surfaces).\nToo much detaches shadows from the objects (\"peter panning\")");
	}
	ImGuiWrapper::Property("Slope Bias", settings.SlopeBias, 0.0f, 10.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Depth bias that grows on surfaces at a grazing angle to the sun, where acne appears first");
	}
	ImGuiWrapper::Property("Normal Bias", settings.NormalBias, 0.0f, 5.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Looks the shadow map up this many texels off the surface, along its normal: removes acne\nwithout detaching the shadows as much as the depth bias");
	}
	ImGuiWrapper::Property("Show Cascades", settings.ShowCascades);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Tints the scene by cascade: red, green, blue, yellow (nearest to farthest)");
	}
	ImGui::Columns(1);

	if (ImGui::Button("Reset Shadow Settings"))
	{
		EnvMapVulkanShadowSettings defaults;
		defaults.ShowCascades = settings.ShowCascades; // a view option, not a setting
		s_PendingShadowResolution = defaults.Resolution != s_ShadowMap.GetResolution() ? defaults.Resolution : 0;
		defaults.Resolution = s_ShadowMap.GetResolution(); // applied with the next frame, see above
		settings = defaults;
	}

	ImGui::Checkbox("Show Shadow Map", &s_ShowShadowMap);
	if (s_ShowShadowMap)
	{
		DrawShadowMapCascades();
	}
}

// "Cast Shadows" of a point or spot light: off when all shadow slots of its kind are taken by other lights
static void LocalCastShadowsProperty(bool& castShadows, uint32_t shadowedCount, uint32_t maxShadowed, bool available, const char* kind)
{
	const bool full = !castShadows && shadowedCount >= maxShadowed;
	ImGui::BeginDisabled(full || !available);
	ImGuiWrapper::Property("Cast Shadows", castShadows);
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		if (!available)
		{
			ImGui::SetTooltip("Not available: the GPU doesn't support cube map arrays (imageCubeArray)");
		}
		else
		{
			ImGui::SetTooltip("%u of %u %s lights cast shadows%s", shadowedCount, maxShadowed, kind, full ? ": all shadow slots are taken" : "");
		}
	}
}

// A resolution combo (the map is recreated at the start of the next frame): labels "<resolution> (<memory>)"
static void ShadowResolutionProperty(const char* label, uint32_t current, const uint32_t (&resolutions)[3], const char* const (&labels)[3], uint32_t& pending)
{
	uint32_t shown = pending ? pending : current;
	int selected = 1;
	for (int i = 0; i < 3; i++)
	{
		if (resolutions[i] == shown)
		{
			selected = i;
		}
	}
	ImGui::Text("%s", label);
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	ImGui::PushID(label);
	if (ImGui::BeginCombo("##Resolution", labels[selected]))
	{
		for (int i = 0; i < 3; i++)
		{
			if (ImGui::Selectable(labels[i], i == selected))
			{
				pending = resolutions[i] != current ? resolutions[i] : 0;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();
	ImGui::PopItemWidth();
	ImGui::NextColumn();
}

// The shadow settings of the spot and point lights (shared by all of them), and the selected light's shadow map
static void OnImGuiRenderLocalShadowSettings()
{
	if (!ImGui::CollapsingHeader("Shadows (Spot and Point Lights)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	EnvMapVulkanLocalShadowSettings& settings = s_LocalShadowSettings;

	ImGui::Columns(2);
	static const uint32_t s_SpotResolutions[3] = { 512, 1024, 2048 };
	static const char* const s_SpotLabels[3] = { "512 (4 MB)", "1024 (16 MB)", "2048 (64 MB)" };
	static const uint32_t s_PointResolutions[3] = { 256, 512, 1024 };
	static const char* const s_PointLabels[3] = { "256 (6 MB)", "512 (24 MB)", "1024 (96 MB)" };
	ShadowResolutionProperty("Spot Resolution", s_SpotShadowMaps.GetResolution(), s_SpotResolutions, s_SpotLabels, s_PendingLocalShadowResolutions.x);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of each spot light's shadow map (GPU memory for all %u)", MaxShadowedSpotLights);
	}
	ShadowResolutionProperty("Point Resolution", s_PointShadowMaps.IsValid() ? s_PointShadowMaps.GetResolution() : settings.PointResolution,
		s_PointResolutions, s_PointLabels, s_PendingLocalShadowResolutions.y);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of each cube face of a point light's shadow map (GPU memory for all %u cubes)", MaxShadowedPointLights);
	}
	ImGuiWrapper::Property("Softness", settings.Softness, 0.0f, 4.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Width of the soft shadow edge (spacing of the 5 x 5 PCF samples, in shadow map texels)");
	}
	ImGuiWrapper::Property("Depth Bias", settings.DepthBias, 0.0f, 10.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Removes \"shadow acne\" (stripes on lit surfaces); too much detaches shadows from the objects");
	}
	ImGuiWrapper::Property("Slope Bias", settings.SlopeBias, 0.0f, 10.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Depth bias that grows on surfaces at a grazing angle to the light");
	}
	ImGuiWrapper::Property("Normal Bias", settings.NormalBias, 0.0f, 5.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Looks the shadow map up this many texels off the surface, along its normal");
	}
	ImGui::Columns(1);

	if (ImGui::Button("Reset Shadow Settings##Local"))
	{
		EnvMapVulkanLocalShadowSettings defaults;
		s_PendingLocalShadowResolutions.x = defaults.SpotResolution != s_SpotShadowMaps.GetResolution() ? defaults.SpotResolution : 0;
		s_PendingLocalShadowResolutions.y = s_PointShadowMaps.IsValid() && defaults.PointResolution != s_PointShadowMaps.GetResolution() ? defaults.PointResolution : 0;
		defaults.SpotResolution = settings.SpotResolution; // applied with the next frame, see above
		defaults.PointResolution = settings.PointResolution;
		settings = defaults;
	}

	// The selected light's shadow map, drawn by the viewer with the next frame (see EnvMapVulkanShadowMapViewer)
	ImGui::Checkbox("Show Shadow Map", &s_ShowShadowMap);
	if (!s_ShowShadowMap)
	{
		return;
	}
	const bool spot = s_SelectedLightKind == LightKind::Spot;
	int slot = -1;
	float range = 1.0f;
	if (spot)
	{
		for (uint32_t i = 0; i < s_LocalShadowSlots.SpotCount; i++)
		{
			slot = s_LocalShadowSlots.SpotLight[i] == s_SelectedLightIndex ? (int)i : slot;
		}
		range = s_Lights.SpotLights[s_SelectedLightIndex].Range;
	}
	else
	{
		for (uint32_t i = 0; i < s_LocalShadowSlots.PointCount; i++)
		{
			slot = s_LocalShadowSlots.PointLight[i] == s_SelectedLightIndex ? (int)i : slot;
		}
		range = s_Lights.PointLights[s_SelectedLightIndex].Range;
	}
	if (slot < 0 || !s_ShadowMapViewer.IsValid())
	{
		ImGui::TextDisabled("No shadow map this frame (Cast Shadows off, light off,\nor all shadow slots taken)");
		return;
	}

	s_ShadowMapViewerRequest.Active = true;
	s_ShadowMapViewerRequest.Cube = !spot;
	s_ShadowMapViewerRequest.BaseLayer = spot ? (uint32_t)slot : (uint32_t)slot * 6;
	s_ShadowMapViewerRequest.Near = GetLocalShadowNearPlane(range);
	s_ShadowMapViewerRequest.Far = std::max(range, 0.02f);

	static ImTextureID s_ViewerTextureID = 0;
	if (!s_ViewerTextureID)
	{
		s_ViewerTextureID = (ImTextureID)(uintptr_t)ImGui_ImplVulkan_AddTexture(s_ShadowMapViewer.GetSampler(), s_ShadowMapViewer.GetView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	const float width = glm::max(ImGui::GetContentRegionAvail().x, 16.0f);
	ImGui::TextDisabled(spot ? "Seen from the light, darker = closer to it" : "The 6 cube faces: -X +Z +X -Z, +Y above, -Y below; darker = closer");
	if (spot)
	{
		// The spot map is the left square of the viewer image
		ImGui::Image(s_ViewerTextureID, ImVec2(width, width), ImVec2(0.0f, 0.0f), ImVec2(0.75f, 1.0f));
	}
	else
	{
		ImGui::Image(s_ViewerTextureID, ImVec2(width, width * 0.75f));
	}
}

static void LightNameProperty(std::string& name)
{
	char buffer[128] = {};
	strncpy_s(buffer, name.c_str(), sizeof(buffer) - 1);
	ImGui::Text("Name");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
	{
		name = buffer;
	}
	ImGui::PopItemWidth();
	ImGui::NextColumn();
}

static void LightRangeProperty(float& range)
{
	ImGuiWrapper::Property("Range", range, 0.05f, 0.01f, 1000.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The distance where the light fades out completely");
	}
}

// The sun, point lights and spot lights: add, delete, enable and edit
static void OnImGuiRenderLights()
{
	ImGui::Begin("Lights");

	// New lights go above GetLightSpawnTarget(): a point light 1.5 units above it, a spot light 3 units above it, pointing down

	ImGui::BeginDisabled(!s_Lights.CanAddPointLight());
	if (ImGui::Button("Add Point Light"))
	{
		if (EnvMapVulkanPointLight* light = s_Lights.AddPointLight(GetLightSpawnTarget() + glm::vec3(0.0f, 1.5f, 0.0f), MaxShadowedPointLights))
		{
			light->Range = GetDefaultLightRange(light->Position);
			SelectLight(LightKind::Point, (int)s_Lights.PointLights.size() - 1);
		}
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Up to %u point lights", EnvMapVulkanLightsGPU::MaxPointLights);
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!s_Lights.CanAddSpotLight());
	if (ImGui::Button("Add Spot Light"))
	{
		if (EnvMapVulkanSpotLight* light = s_Lights.AddSpotLight(GetLightSpawnTarget() + glm::vec3(0.0f, 3.0f, 0.0f), MaxShadowedSpotLights))
		{
			light->Range = GetDefaultLightRange(light->Position);
			SelectLight(LightKind::Spot, (int)s_Lights.SpotLights.size() - 1);
		}
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Up to %u spot lights (pointing down when added)", EnvMapVulkanLightsGPU::MaxSpotLights);
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(s_SelectedLightKind != LightKind::Point && s_SelectedLightKind != LightKind::Spot);
	if (ImGui::Button("Delete"))
	{
		if (s_SelectedLightKind == LightKind::Point)
		{
			s_Lights.PointLights.erase(s_Lights.PointLights.begin() + s_SelectedLightIndex);
		}
		else
		{
			s_Lights.SpotLights.erase(s_Lights.SpotLights.begin() + s_SelectedLightIndex);
		}
		SelectLight(LightKind::None);
	}
	ImGui::EndDisabled();

	ImGui::Checkbox("Show in Viewport", &s_ShowLightGizmos);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Light icons (click one to select the light) and the selected light's range and cone");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Shadows Only", &s_ShowShadowsOnly);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The viewport shows only the selected light's shadow, whatever the other lights and the exposure:\n"
			"white = lit, black = in shadow, dark gray = the light doesn't reach (out of range or cone, or facing away)");
	}

	// The light list: a checkbox (enabled) and a selectable name per light
	auto lightRow = [](const char* name, bool& enabled, LightKind kind, int index) {
		ImGui::PushID((int)kind * 1000 + index);
		ImGui::Checkbox("##Enabled", &enabled);
		ImGui::SameLine();
		if (ImGui::Selectable(name, s_SelectedLightKind == kind && s_SelectedLightIndex == index))
		{
			SelectLight(kind, index);
		}
		ImGui::PopID();
	};

	ImGui::Separator();
	if (ImGui::BeginChild("LightList", ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 8.0f), ImGuiChildFlags_Borders))
	{
		lightRow("Sun", s_Lights.Sun.Enabled, LightKind::Sun, 0);
		for (int i = 0; i < (int)s_Lights.PointLights.size(); i++)
		{
			lightRow(s_Lights.PointLights[i].Name.c_str(), s_Lights.PointLights[i].Enabled, LightKind::Point, i);
		}
		for (int i = 0; i < (int)s_Lights.SpotLights.size(); i++)
		{
			lightRow(s_Lights.SpotLights[i].Name.c_str(), s_Lights.SpotLights[i].Enabled, LightKind::Spot, i);
		}
	}
	ImGui::EndChild();

	// Properties of the selected light
	ImGui::Separator();
	if (s_SelectedLightKind == LightKind::None)
	{
		ImGui::TextDisabled("Select a light in the list,\nor click its icon in the viewport");
		ImGui::End();
		return;
	}

	if (s_SelectedLightKind == LightKind::Sun)
	{
		ImGui::BeginDisabled(!s_Data.envEquirect);
		if (ImGui::Button("Align to Environment"))
		{
			AlignSunToEnvironment();
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Points the sun at the brightest light of the environment map\n(the sun outdoors, a bright window indoors) and takes its color");
		}
	}

	ImGui::Columns(2);
	if (s_SelectedLightKind == LightKind::Sun)
	{
		EnvMapVulkanDirectionalLight& sun = s_Lights.Sun;
		ImGuiWrapper::Property("Enabled", sun.Enabled);
		ImGuiWrapper::Property("Color", sun.Color, PropertyFlag::ColorProperty);
		ImGuiWrapper::Property("Intensity", sun.Intensity, 0.01f, 0.0f, 20.0f, PropertyFlag::DragProperty);
		AzimuthProperty(sun.Azimuth);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Compass direction of the sun (degrees around the vertical axis, 0 = +Z, 90 = +X)");
		}
		ImGuiWrapper::Property("Elevation", sun.Elevation, 0.5f, -90.0f, 90.0f, PropertyFlag::DragProperty);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Height of the sun above the horizon (degrees, 90 = straight overhead)");
		}
		ImGuiWrapper::Property("Cast Shadows", sun.CastShadows);
		ImGuiWrapper::Property("Follow Env Rotation", sun.FollowEnvironmentRotation);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Env Map Rotation (Environment panel) turns the sun too,\nso a sun aligned to the environment stays on the sun of the map");
		}
	}
	else if (s_SelectedLightKind == LightKind::Point)
	{
		EnvMapVulkanPointLight& light = s_Lights.PointLights[s_SelectedLightIndex];
		LightNameProperty(light.Name);
		ImGuiWrapper::Property("Enabled", light.Enabled);
		ImGuiWrapper::Property("Color", light.Color, PropertyFlag::ColorProperty);
		ImGuiWrapper::Property("Intensity", light.Intensity, 0.1f, 0.0f, 10000.0f, PropertyFlag::DragProperty);
		ImGuiWrapper::Property("Position", light.Position, 0.05f, 0.0f, 0.0f, PropertyFlag::DragProperty);
		LightRangeProperty(light.Range);
		LocalCastShadowsProperty(light.CastShadows, s_Lights.CountShadowedPointLights(), MaxShadowedPointLights, s_PointShadowMaps.IsValid(), "point");
	}
	else
	{
		EnvMapVulkanSpotLight& light = s_Lights.SpotLights[s_SelectedLightIndex];
		LightNameProperty(light.Name);
		ImGuiWrapper::Property("Enabled", light.Enabled);
		ImGuiWrapper::Property("Color", light.Color, PropertyFlag::ColorProperty);
		ImGuiWrapper::Property("Intensity", light.Intensity, 0.1f, 0.0f, 10000.0f, PropertyFlag::DragProperty);
		ImGuiWrapper::Property("Position", light.Position, 0.05f, 0.0f, 0.0f, PropertyFlag::DragProperty);
		LightRangeProperty(light.Range);
		AzimuthProperty(light.Azimuth);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Compass direction the spot points to (degrees around the vertical axis, 0 = +Z, 90 = +X)");
		}
		ImGuiWrapper::Property("Elevation", light.Elevation, 0.5f, -90.0f, 90.0f, PropertyFlag::DragProperty);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Tilt of the spot (degrees, -90 = straight down, 0 = horizontal)");
		}
		// The inner cone always stays inside the outer one: dragging either one past the other moves both
		if (ImGuiWrapper::Property("Inner Angle", light.InnerAngle, 0.2f, 0.0f, 89.0f, PropertyFlag::DragProperty))
		{
			light.OuterAngle = std::max(light.OuterAngle, light.InnerAngle);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Half-angle of the fully lit cone (degrees)");
		}
		if (ImGuiWrapper::Property("Outer Angle", light.OuterAngle, 0.2f, 0.1f, 89.0f, PropertyFlag::DragProperty))
		{
			light.InnerAngle = std::min(light.InnerAngle, light.OuterAngle);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Half-angle where the light fades out completely (degrees)");
		}
		LocalCastShadowsProperty(light.CastShadows, s_Lights.CountShadowedSpotLights(), MaxShadowedSpotLights, s_SpotShadowMaps.IsValid(), "spot");
		ImGui::Columns(1);
		ImGui::TextDisabled("Viewport gizmo: 1 moves the light, 2 aims it");
	}
	ImGui::Columns(1);

	if (s_SelectedLightKind == LightKind::Sun)
	{
		ImGui::TextDisabled("Viewport gizmo: 1 moves the sun's icon (only the icon:\nthe sun has no position), 2 turns the sun");
		if (ImGui::Button("Icon to View"))
		{
			glm::mat4 cameraTransform = glm::inverse(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix());
			s_Lights.Sun.IconPosition = glm::vec3(cameraTransform[3]) - glm::normalize(glm::vec3(cameraTransform[2])) * 10.0f;
			s_Lights.Sun.IconMoved = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Puts the sun's icon 10 units in front of the camera (when the sun is behind you)");
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(!s_Lights.Sun.IconMoved);
		if (ImGui::Button("Reset Icon"))
		{
			s_Lights.Sun.IconMoved = false;
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Back in the sky, in the sun's direction");
		}
		OnImGuiRenderShadowSettings();
	}
	else
	{
		OnImGuiRenderLocalShadowSettings();
	}

	ImGui::End();
}

// The water plane: add / remove it, its place and size, the waves and the look (EnvMapVulkanWaterSettings)
static void OnImGuiRenderWater()
{
	ImGui::SetNextWindowSize(ImVec2(320.0f, 420.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Water");
	EnvMapVulkanWaterSettings& water = s_WaterSettings;

	if (!water.Enabled)
	{
		if (ImGui::Button("Add Water"))
		{
			// Under the center of the view, at the height of the ground plane
			glm::vec3 target = GetLightSpawnTarget();
			water.Center = glm::vec2(target.x, target.z);
			water.Enabled = true;
			SelectWater();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("One water plane per scene: a rectangle with waves and reflections,\nmoved and resized with the gizmo when selected");
		}
		ImGui::TextDisabled("No water on the scene");
		ImGui::End();
		return;
	}

	if (ImGui::Button("Remove Water"))
	{
		water.Enabled = false;
		s_WaterSelected = false;
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(s_WaterSelected);
	if (ImGui::Button("Select"))
	{
		SelectWater();
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Selects the water for the gizmo (or click it in the viewport):\n1 move (height and position), 3 resize");
	}

	ImGui::Columns(2);
	ImGuiWrapper::Property("Height", water.Height, 0.05f, 0.0f, 0.0f, PropertyFlag::DragProperty); // min = max = 0: no limits
	ImGuiWrapper::Property("Center X", water.Center.x, 0.1f, 0.0f, 0.0f, PropertyFlag::DragProperty);
	ImGuiWrapper::Property("Center Z", water.Center.y, 0.1f, 0.0f, 0.0f, PropertyFlag::DragProperty);
	ImGuiWrapper::Property("Size X", water.Size.x, 0.1f, 0.1f, 10000.0f, PropertyFlag::DragProperty);
	ImGuiWrapper::Property("Size Z", water.Size.y, 0.1f, 0.1f, 10000.0f, PropertyFlag::DragProperty);
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Waves");
	ImGui::Columns(2);
	if (ImGuiWrapper::Property("Direction", water.WaveDirection, 0.5f, 0.0f, 0.0f, PropertyFlag::DragProperty)) // no limits, wraps
	{
		water.WaveDirection = std::remainder(water.WaveDirection, 360.0f);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Degrees around the vertical axis: where the waves move");
	}
	ImGuiWrapper::Property("Speed", water.WaveSpeed, 0.01f, 0.0f, 10.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("World units per second (the ripples move a bit slower, in another direction)");
	}
	ImGuiWrapper::Property("Strength", water.WaveStrength, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Steepness of the waves: 0 is a mirror");
	}
	ImGuiWrapper::Property("Wave Size", water.WaveScale1, 0.05f, 0.1f, 100.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("World size of the larger wave pattern");
	}
	ImGuiWrapper::Property("Ripple Size", water.WaveScale2, 0.05f, 0.1f, 100.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("World size of the smaller wave pattern");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Swell");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Swell Height", water.SwellHeight, 0.01f, 0.0f, 5.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters: the most the surface rises above its level. Gerstner waves move the surface's vertices\n"
			"(the waves above only shade it); 0 keeps it flat");
	}
	ImGuiWrapper::Property("Swell Length", water.SwellLength, 0.1f, 1.0f, 200.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters: the wavelength of the longest of the four waves (the others are shorter and cross it).\n"
			"Longer waves travel faster, as on real water");
	}
	ImGuiWrapper::Property("Steepness", water.SwellSteepness, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("0: round, rolling waves; 1: sharp crests and wide troughs (the surface moves toward the crests)");
	}
	ImGuiWrapper::Property("Wireframe", water.Wireframe);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Draws the surface's grid over it, where the waves move it (brighter on the crests)");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Look");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Scatter Color", water.ScatterColor, PropertyFlag::ColorProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The light the water body sends back up: dark blue-green for deep, clear water,\nbrighter and greener or browner for shallow or murky water");
	}
	ImGuiWrapper::Property("Roughness", water.Roughness, 0.01f, 0.5f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Small: a sharp sun highlight and a clear reflection; larger: a wider highlight and a blurred reflection");
	}
	ImGuiWrapper::Property("Reflection", water.ReflectionStrength, 0.0f, 2.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Strength of the reflected environment (1 = physically based)");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Into the Water");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Tint", water.Transmittance, PropertyFlag::ColorProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The color light keeps after Clarity meters of water: what is under the water\nturns this color with depth (water absorbs red first)");
	}
	ImGuiWrapper::Property("Clarity", water.Clarity, 0.01f, 0.05f, 100.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters: larger values let you see deeper into the water");
	}
	ImGuiWrapper::Property("Refraction", water.RefractionStrength, 0.0f, 3.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How much the waves bend the view of what is under the water");
	}
	ImGuiWrapper::Property("Edge Softness", water.EdgeSoftness, 0.0f, 2.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters of water over which the surface fades in at the shore (no hard line)");
	}
	ImGuiWrapper::Property("Foam", water.FoamAmount, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Foam along the shore and around objects standing in the water");
	}
	ImGuiWrapper::Property("Foam Width", water.FoamWidth, 0.01f, 3.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters of water depth that get foam");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Under the Water");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Transparency", water.TransparencyFromBelow, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The surface seen from under the water. 0 is physical: the sky shows only within Snell's window\n"
			"(about 49 degrees around the vertical), outside it the surface mirrors the water below.\n"
			"0.5: the window covers the whole sky (only grazing views are mirrored); 1: the mirror fades too.\n"
			"How far you see through the water itself is its Clarity (Into the Water).");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Caustics");
	ImGui::PushID("Caustics"); // the labels are the widget ids, and the waves have a Strength too
	ImGui::Columns(2);
	ImGuiWrapper::Property("Caustics", water.Caustics);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The waves bend the sun's light as it enters the water and focus it into moving bright lines\n"
			"on what is under the water (computed from the waves themselves, around the camera)");
	}
	ImGui::BeginDisabled(!water.Caustics);
	ImGuiWrapper::Property("Strength", water.CausticsStrength, 0.0f, 3.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("1: as the waves focus the light; more exaggerates the pattern, less fades it");
	}
	ImGuiWrapper::Property("Focus Depth", water.CausticsFocus, 0.01f, 0.1f, 20.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Meters under the surface where the pattern is computed: the deeper, the more the rays\n"
			"have gathered (sharper, brighter lines). Shallower than this, the caustics fade in from the surface.");
	}
	ImGuiWrapper::Property("Area", water.CausticsArea, 0.5f, 2.0f, 500.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Meters: the square around the camera that gets caustics (they fade out at its edges).\n"
			"Smaller: finer detail (the same map covers less water)");
	}
	ImGui::EndDisabled();
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::Separator();
	ImGui::Text("Reflection");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Planar Reflection", water.PlanarReflection);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The scene reflected in the water (drawn a second time, from the camera mirrored in the water);\n"
			"from under the water, the mirror the surface is outside Snell's window.\n"
			"Off: only the environment map is reflected");
	}
	ImGui::Text("Resolution");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	const char* resolutions[] = { "Full", "Half", "Quarter" };
	int resolution = water.ReflectionDivisor == 1 ? 0 : (water.ReflectionDivisor == 4 ? 2 : 1);
	if (ImGui::Combo("##ReflectionResolution", &resolution, resolutions, IM_ARRAYSIZE(resolutions)))
	{
		water.ReflectionDivisor = resolution == 0 ? 1 : (resolution == 2 ? 4 : 2);
	}
	ImGui::PopItemWidth();
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of the reflection image, relative to the viewport (the waves hide most of the difference)");
	}
	ImGui::NextColumn();
	ImGuiWrapper::Property("Distortion", water.ReflectionDistortion, 0.0f, 3.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How much the waves bend the reflection");
	}
	ImGui::Columns(1);

	ImGui::End();
}

// A mesh (a part of a model) that the gizmo and the Mesh Transform fields move on their own: a model with more than one
// part, and not a rigged part of a skinned model (the skeleton places those, see GetMeshTransform)
static bool CanManipulateMesh(const LoadedModelVulkan& entry, int meshIndex)
{
	const auto& meshes = entry.Model->GetMeshes();
	if (meshIndex < 0 || meshIndex >= (int)meshes.size() || meshes.size() < 2)
	{
		return false;
	}
	return !(entry.Model->IsSkinned() && meshes[meshIndex]->IsRigged);
}

static void OnImGuiRenderModelsAndMeshes()
{
	ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Models and Meshes");

	if (ImGui::Button("Load Model"))
	{
		std::string filepath = Util::ToUtf8(Application::Get()->OpenFile());
		if (!filepath.empty())
		{
			s_PendingModelFilename = filepath; // loaded at the start of the next frame (see Draw)
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("or drop a model here (?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Drag a model file from the Content Browser:\n"
			"- anywhere on this panel: placed at the scene origin\n"
			"- onto the viewport: placed standing on the ground under the cursor");
	}

	ImGui::Separator();

	// Empty scene: a large box that says where models can be dropped
	if (s_LoadedModels.empty())
	{
		ImVec2 boxMin = ImGui::GetCursorScreenPos();
		ImVec2 boxSize(ImGui::GetContentRegionAvail().x, glm::max(ImGui::GetContentRegionAvail().y, 80.0f));
		ImVec2 boxMax(boxMin.x + boxSize.x, boxMin.y + boxSize.y);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(boxMin, boxMax, ImGui::GetColorU32(ImGuiCol_TextDisabled), 6.0f, 0, 1.5f);

		const char* hint = "Drop a model here\n(.obj, .fbx, .gltf, .glb, .dae...)";
		ImVec2 textSize = ImGui::CalcTextSize(hint);
		ImGui::SetCursorScreenPos(ImVec2(boxMin.x + (boxSize.x - textSize.x) * 0.5f, boxMin.y + (boxSize.y - textSize.y) * 0.5f));
		ImGui::TextDisabled("%s", hint);
		ImGui::SetCursorScreenPos(boxMin);
		ImGui::Dummy(boxSize);
	}

	for (int i = 0; i < (int)s_LoadedModels.size(); i++)
	{
		ImGui::PushID(i);
		std::string name = std::filesystem::path(s_LoadedModels[i].FilePath).filename().string();
		if (ImGui::Selectable(name.c_str(), s_SelectedModelIndex == i))
		{
			if (s_SelectedModelIndex != i)
			{
				s_SelectedMeshIndex = -1;
			}
			s_SelectedModelIndex = i;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", s_LoadedModels[i].FilePath.c_str());
		}
		ImGui::PopID();
	}

	if (s_SelectedModelIndex >= 0 && s_SelectedModelIndex < (int)s_LoadedModels.size())
	{
		LoadedModelVulkan& entry = s_LoadedModels[s_SelectedModelIndex];

		ImGui::Separator();
		ImGui::Text("Transform");
		ImGui::DragFloat3("Translation", &entry.Translation.x, 0.1f);
		ImGui::DragFloat3("Rotation", &entry.Rotation.x, 1.0f);
		ImGui::DragFloat3("Scale", &entry.Scale.x, 0.01f, 0.001f, 1000.0f);

		// The selected part's place in the model (the viewport gizmo moves it too)
		if (s_SelectedMeshIndex >= 0 && s_SelectedMeshIndex < (int)entry.Model->GetMeshes().size())
		{
			H2M::RefH2M<H2M::MeshH2M> part = entry.Model->GetMeshes()[s_SelectedMeshIndex];
			ImGui::Separator();
			const std::string partName = !part->MeshName.empty() ? part->MeshName : (!part->NodeName.empty() ? part->NodeName : "Mesh " + std::to_string(s_SelectedMeshIndex));
			ImGui::Text("Mesh Transform: %s", partName.c_str());
			if (CanManipulateMesh(entry, s_SelectedMeshIndex))
			{
				glm::vec3 translation, rotation, scale;
				ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(part->Transform), &translation.x, &rotation.x, &scale.x);
				bool changed = ImGui::DragFloat3("Translation##Mesh", &translation.x, 0.1f);
				changed |= ImGui::DragFloat3("Rotation##Mesh", &rotation.x, 1.0f);
				changed |= ImGui::DragFloat3("Scale##Mesh", &scale.x, 0.01f, 0.001f, 1000.0f);
				if (changed)
				{
					ImGuizmo::RecomposeMatrixFromComponents(&translation.x, &rotation.x, &scale.x, glm::value_ptr(part->Transform));
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Relative to the model. Shift + click in the viewport selects the whole model");
				}
				if (ImGui::Button("Reset Mesh") && s_SelectedMeshIndex < (int)entry.OriginalMeshTransforms.size())
				{
					part->Transform = entry.OriginalMeshTransforms[s_SelectedMeshIndex];
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Back to its place in the model file");
				}
			}
			else
			{
				ImGui::TextDisabled(entry.Model->GetMeshes().size() < 2 ? "The model has a single mesh: the Transform above moves it"
					: "A rigged mesh: the skeleton places it (the gizmo moves the whole model)");
			}
		}

		if (ImGui::Button("Remove Model"))
		{
			s_PendingRemoveModelIndex = s_SelectedModelIndex; // removed at the start of the next frame (see Draw)
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Removes the whole model, with all its meshes");
		}
		ImGui::SameLine();
		const bool canRemoveMesh = s_SelectedMeshIndex >= 0 && s_SelectedMeshIndex < (int)entry.Model->GetMeshes().size() &&
			entry.Model->GetMeshes().size() > 1;
		ImGui::BeginDisabled(!canRemoveMesh);
		if (ImGui::Button("Remove Mesh"))
		{
			s_PendingRemoveMeshIndex = s_SelectedMeshIndex; // removed at the start of the next frame (see Draw)
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip(canRemoveMesh ? "Removes only the selected mesh, a part of the model" :
				entry.Model->GetMeshes().size() > 1 ? "Select a mesh (a part of the model) below or in the viewport" :
				"The model has a single mesh: use Remove Model");
		}

		// Animation playback (skinned models), like the Animation section of the Mesh Debug panel in SceneHazelEnvMap
		H2M::RefH2M<H2M::ModelH2M> model = entry.Model;
		if (model->HasAnimations() && model->IsSkinned())
		{
			ImGui::Separator();
			ImGui::Text("Animation");

			ImGui::Checkbox("Animated", &model->IsAnimated());
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Off: the model is shown in its bind pose");
			}

			ImGui::BeginDisabled(!model->IsAnimated());
			{
				uint32_t animationCount = model->GetAnimationCount();
				if (animationCount > 1)
				{
					if (ImGui::BeginCombo("Clip", model->GetAnimationName(model->GetAnimationIndex()).c_str()))
					{
						for (uint32_t a = 0; a < animationCount; a++)
						{
							ImGui::PushID((int)a);
							if (ImGui::Selectable(model->GetAnimationName(a).c_str(), model->GetAnimationIndex() == a))
							{
								model->SetAnimationIndex(a);
							}
							ImGui::PopID();
						}
						ImGui::EndCombo();
					}
				}
				else
				{
					ImGui::TextDisabled("Clip: %s", model->GetAnimationName(0).c_str());
				}

				if (ImGui::Button(model->AnimationPlaying() ? "Pause" : "Play", ImVec2(60.0f, 0.0f)))
				{
					model->AnimationPlaying() = !model->AnimationPlaying();
				}
				ImGui::SameLine();
				if (ImGui::Button("Restart"))
				{
					model->AnimationTime() = 0.0f;
				}

				// Scrub through the animation (in seconds; ModelH2M keeps the time in animation ticks)
				float ticksPerSecond = model->GetAnimationTicksPerSecond();
				float durationSeconds = model->GetAnimationDuration() / ticksPerSecond;
				float timeSeconds = model->AnimationTime() / ticksPerSecond;
				if (ImGui::SliderFloat("Time", &timeSeconds, 0.0f, durationSeconds, "%.2f s"))
				{
					model->AnimationTime() = timeSeconds * ticksPerSecond;
				}
				ImGui::DragFloat("Time Scale", &model->TimeMultiplier(), 0.01f, 0.0f, 10.0f, "%.2fx");
				ImGui::TextDisabled("Duration %.2f s (%.0f ticks at %.0f/s), %u bones", durationSeconds, model->GetAnimationDuration(), ticksPerSecond, model->GetBoneCount());
			}
			ImGui::EndDisabled();
		}

		// Material slots, one per mesh: select a mesh to edit its material in the Material Editor. The dropdown, or a
		// material dropped from the Material Library, chooses which library material the mesh is drawn with.
		auto& meshes = entry.Model->GetMeshes();
		const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();

		// Assigns a library material to a mesh; the Material Editor follows if that mesh is selected
		auto assignMaterial = [&](int s, H2M::RefH2M<EnvMapVulkanMaterial> material) {
			entry.MeshMaterials[s] = material; // used by RenderModelVulkan from the next frame
			if (s == s_SelectedMeshIndex)
			{
				s_SelectedMaterial = material;
			}
		};
		auto acceptMaterialDrop = [&](int s) {
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_MaterialPayload))
				{
					uint32_t index = *(const uint32_t*)payload->Data;
					if (index < materials.size())
					{
						assignMaterial(s, materials[index]);
					}
				}
				ImGui::EndDragDropTarget();
			}
		};

		ImGui::Separator();
		ImGui::Text("Meshes (%d)", (int)meshes.size());

		for (int s = 0; s < (int)meshes.size() && s < (int)entry.MeshMaterials.size(); s++)
		{
			H2M::RefH2M<H2M::MeshH2M> mesh = meshes[s];
			ImGui::PushID(1000 + s);

			std::string meshName = !mesh->MeshName.empty() ? mesh->MeshName :
				(!mesh->NodeName.empty() ? mesh->NodeName : "Mesh " + std::to_string(s));
			if (ImGui::Selectable(meshName.c_str(), s_SelectedMeshIndex == s, 0, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f)))
			{
				s_SelectedMeshIndex = (s_SelectedMeshIndex == s) ? -1 : s; // click again to deselect
			}
			acceptMaterialDrop(s);

			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::BeginCombo("##material", entry.MeshMaterials[s]->GetName().c_str()))
			{
				for (uint32_t m = 0; m < (uint32_t)materials.size(); m++)
				{
					ImGui::PushID((int)m);
					if (ImGui::Selectable(materials[m]->GetName().c_str(), entry.MeshMaterials[s] == materials[m]))
					{
						assignMaterial(s, materials[m]);
					}
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
			acceptMaterialDrop(s);

			ImGui::PopID();
		}

		if (s_SelectedMaterial)
		{
			std::string label = "Apply '" + s_SelectedMaterial->GetName() + "' to all meshes";
			if (ImGui::Button(label.c_str()))
			{
				std::fill(entry.MeshMaterials.begin(), entry.MeshMaterials.end(), s_SelectedMaterial);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The material shown in the Material Editor");
			}
		}
	}

	// The whole panel is a drop area for model files (loaded at the scene origin)
	ImVec2 panelMin = ImGui::GetWindowPos();
	ImVec2 panelMax(panelMin.x + ImGui::GetWindowSize().x, panelMin.y + ImGui::GetWindowSize().y);
	if (ImGui::BeginDragDropTargetCustom(ImRect(panelMin, panelMax), ImGui::GetID("##ModelsAndMeshesPanelDropArea")))
	{
		std::string filepath;
		if (AcceptFileDrop(panelMin, panelMax, IsModelFile, "a model file", filepath))
		{
			s_PendingModelFilename = filepath; // loaded at the start of the next frame (see Draw)
			s_PendingModelGroundPosition.reset();
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::End();
}

// All materials of the scene: create, duplicate, delete, and drag onto a mesh (Models and Meshes panel or viewport) to assign
static void OnImGuiRenderMaterialLibrary()
{
	ImGui::Begin("Material Library");

	SyncSelectedMaterial();

	// Renaming in place (double-click, F2, the Rename button or the context menu): the row turns into a text field.
	// Enter or clicking elsewhere applies the name, Escape cancels.
	static EnvMapVulkanMaterial* s_RenamingMaterial = nullptr;
	static bool s_RenameNeedsFocus = false;
	static char s_RenameBuffer[128] = "";
	auto startRename = [](const H2M::RefH2M<EnvMapVulkanMaterial>& material) {
		s_RenamingMaterial = const_cast<EnvMapVulkanMaterial*>(material.Raw());
		s_RenameNeedsFocus = true;
		strncpy_s(s_RenameBuffer, material->GetName().c_str(), sizeof(s_RenameBuffer) - 1);
	};

	// Duplicating adds to the material list, so a request from a row is carried out after the list is drawn
	H2M::RefH2M<EnvMapVulkanMaterial> duplicateRequest = H2M::RefH2M<EnvMapVulkanMaterial>();

	if (ImGui::Button("New Material"))
	{
		s_SelectedMaterial = EnvMapVulkanMaterialLibrary::CreateMaterial();
		startRename(s_SelectedMaterial); // a new material usually gets a name right away
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!s_SelectedMaterial);
	if (ImGui::Button("Rename"))
	{
		startRename(s_SelectedMaterial);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Rename the selected material (or double-click it, or press F2)");
	}
	ImGui::SameLine();
	if (ImGui::Button("Duplicate"))
	{
		duplicateRequest = s_SelectedMaterial;
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("An independent copy of the selected material, with the same values and maps");
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
	{
		s_PendingDeleteMaterial = s_SelectedMaterial; // deleted at the start of the next frame (see Draw)
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Meshes that use the selected material get the Default material");
	}
	ImGui::EndDisabled();

	static char s_Filter[64] = "";
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##filter", "Filter by name", s_Filter, sizeof(s_Filter));

	ImGui::Separator();

	std::string filter = s_Filter;
	std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) { return (char)std::tolower(c); });

	const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();
	if (materials.empty())
	{
		ImGui::TextDisabled("No materials. Load a model or click New Material.");
	}

	const float countColumnWidth = ImGui::CalcTextSize("000 uses").x;
	for (uint32_t m = 0; m < (uint32_t)materials.size(); m++)
	{
		const H2M::RefH2M<EnvMapVulkanMaterial>& material = materials[m];

		std::string lowerName = material->GetName();
		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		if (!filter.empty() && lowerName.find(filter) == std::string::npos)
		{
			continue;
		}

		ImGui::PushID((int)m);
		ImVec2 rowSize(ImGui::GetContentRegionAvail().x - countColumnWidth, 0.0f);

		if (s_RenamingMaterial == material.Raw())
		{
			ImGui::SetNextItemWidth(rowSize.x);
			if (s_RenameNeedsFocus)
			{
				ImGui::SetKeyboardFocusHere();
				s_RenameNeedsFocus = false;
			}
			bool entered = ImGui::InputText("##rename", s_RenameBuffer, sizeof(s_RenameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
			if (entered || (ImGui::IsItemDeactivated() && !ImGui::IsKeyPressed(ImGuiKey_Escape)))
			{
				EnvMapVulkanMaterialLibrary::Rename(material, s_RenameBuffer);
				s_RenamingMaterial = nullptr;
			}
			else if (ImGui::IsItemDeactivated())
			{
				s_RenamingMaterial = nullptr; // Escape: keep the old name
			}
		}
		else if (ImGui::Selectable(material->GetName().c_str(), s_SelectedMaterial == material, ImGuiSelectableFlags_AllowDoubleClick, rowSize))
		{
			s_SelectedMaterial = material;
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				startRename(material);
			}
		}

		if (ImGui::BeginPopupContextItem("##materialContextMenu"))
		{
			s_SelectedMaterial = material;
			if (ImGui::MenuItem("Rename", "F2"))
			{
				startRename(material);
			}
			if (ImGui::MenuItem("Duplicate"))
			{
				duplicateRequest = material;
			}
			if (ImGui::MenuItem("Delete"))
			{
				s_PendingDeleteMaterial = material; // deleted at the start of the next frame (see Draw)
			}
			ImGui::EndPopup();
		}

		if (s_RenamingMaterial != material.Raw() && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(s_MaterialPayload, &m, sizeof(uint32_t));
			ImGui::Text("%s", material->GetName().c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::IsItemHovered())
		{
			std::string origin = material->GetSourceFile().empty() ? "Created in the editor" : "Imported from " + material->GetSourceFile();
			ImGui::SetTooltip("%s\nDrag onto a mesh in the Models and Meshes panel or the viewport to assign it", origin.c_str());
		}

		int users = CountMaterialUsers(material);
		ImGui::SameLine();
		if (users > 0)
		{
			ImGui::TextDisabled("%d %s", users, users == 1 ? "use" : "uses");
		}
		else
		{
			ImGui::TextDisabled("unused");
		}
		ImGui::PopID();
	}

	// F2 renames the selected material while this panel has focus (and no text field is being edited)
	if (s_SelectedMaterial && !s_RenamingMaterial && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
		!ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_F2, false))
	{
		startRename(s_SelectedMaterial);
	}

	if (duplicateRequest)
	{
		s_SelectedMaterial = EnvMapVulkanMaterialLibrary::Duplicate(duplicateRequest);
	}

	ImGui::End();
}

// Edits the selected library material in place: every mesh drawn with it changes (values are push constants of every
// draw; maps are written to the material's descriptor set at the start of the next frame)
static void OnImGuiRenderMaterialEditor()
{
	ImGui::Begin("Material Editor");

	SyncSelectedMaterial();

	H2M::RefH2M<EnvMapVulkanMaterial> material = s_SelectedMaterial;
	if (!material)
	{
		ImGui::TextDisabled("Select a material in the Material Library,\nor a mesh in the Models and Meshes panel or the viewport");
		ImGui::End();
		return;
	}

	// Name (renamed when editing ends; names stay unique in the library)
	static char s_NameBuffer[128] = "";
	static EnvMapVulkanMaterial* s_NameBufferMaterial = nullptr;
	if (s_NameBufferMaterial != material.Raw() || !ImGui::IsAnyItemActive())
	{
		strncpy_s(s_NameBuffer, material->GetName().c_str(), sizeof(s_NameBuffer) - 1);
		s_NameBufferMaterial = material.Raw();
	}
	ImGui::InputText("Name", s_NameBuffer, sizeof(s_NameBuffer));
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		EnvMapVulkanMaterialLibrary::Rename(material, s_NameBuffer);
	}

	int users = CountMaterialUsers(material);
	if (users > 1)
	{
		ImGui::TextDisabled("Used by %d meshes: changes apply to all of them.", users);
	}
	else if (users == 0)
	{
		ImGui::TextDisabled("Not used by any mesh.");
	}

	ImGui::Separator();

	glm::vec3& albedoColor = material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
	float& metalness = material->Get<float>("u_MaterialUniforms.Metalness");
	float& roughness = material->Get<float>("u_MaterialUniforms.Roughness");

	ImGui::ColorEdit3("Albedo Color", &albedoColor.x);
	ImGui::SliderFloat("Metalness", &metalness, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f);

	float& tilingFactor = material->Get<float>("u_MaterialUniforms.TilingFactor");
	float& emissiveIntensity = material->Get<float>("u_MaterialUniforms.EmissiveIntensity");
	float& metalRoughPacked = material->Get<float>("u_MaterialUniforms.MetalRoughPacked");

	ImGui::DragFloat("Tiling Factor", &tilingFactor, 0.01f, 0.01f, 100.0f);
	ImGui::DragFloat("Emissive Intensity", &emissiveIntensity, 0.05f, 0.0f, 100.0f);

	bool packed = metalRoughPacked > 0.5f;
	if (ImGui::Checkbox("Packed Metalness/Roughness Map", &packed))
	{
		metalRoughPacked = packed ? 1.0f : 0.0f;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("glTF metallicRoughness map: roughness is read from G and metalness from B (otherwise both from R).\n"
			"Assign the same map to both the metalness and the roughness slot.");
	}

	static const char* s_MapLabels[EnvMapVulkanMaterial::MapCount] = {
		"Use Albedo Map", "Use Normal Map", "Use Metalness Map", "Use Roughness Map", "Use Emissive Map", "Use AO Map" };

	for (uint32_t i = 0; i < EnvMapVulkanMaterial::MapCount; i++)
	{
		ImGui::PushID((int)i);
		ImGui::Separator();

		H2M::RefH2M<H2M::Texture2D_H2M> map = material->GetMap(i);

		// Thumbnail of the map; also a drop target for images from the Content Browser
		const ImVec2 thumbnailSize(64.0f, 64.0f);
		ImTextureID thumbnail = map ? map->GetImTextureID() : ImTextureID{};
		if (thumbnail)
		{
			ImGui::Image(thumbnail, thumbnailSize, ImVec2(0, 1), ImVec2(1, 0)); // the Vulkan loader flips LDR images
		}
		else
		{
			ImGui::Button(map ? "map" : "no map", thumbnailSize);
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
				if (IsImageFile(itemPath))
				{
					s_PendingMaterialTextures.push_back({ material, i, itemPath }); // applied at the start of the next frame
				}
				else
				{
					Log::GetLogger()->warn("'{0}' is not a supported image file", itemPath);
				}
			}
			ImGui::EndDragDropTarget();
		}
		if (ImGui::IsItemHovered())
		{
			const char* colorSpace = EnvMapVulkanMaterial::IsColorMap(i) ? "sRGB (color data)" : "linear (raw data)";
			if (map)
			{
				ImGui::SetTooltip("%s\n%s", map->GetPath().c_str(), colorSpace);
			}
			else
			{
				ImGui::SetTooltip("Drop an image from the Content Browser here\nLoaded as %s", colorSpace);
			}
		}

		ImGui::SameLine();
		ImGui::BeginGroup();
		{
			float& toggle = material->Get<float>(EnvMapVulkanMaterial::GetMapToggleName(i));
			bool enabled = toggle > 0.5f;

			ImGui::BeginDisabled(!map);
			if (ImGui::Checkbox(s_MapLabels[i], &enabled))
			{
				toggle = enabled ? 1.0f : 0.0f;
			}
			ImGui::EndDisabled();

			if (ImGui::Button("Load..."))
			{
				std::string filepath = Util::ToUtf8(Application::Get()->OpenFile(L"Images\0*.png;*.jpg;*.jpeg;*.tga;*.bmp\0All\0*.*\0"));
				if (!filepath.empty())
				{
					s_PendingMaterialTextures.push_back({ material, i, filepath }); // applied at the start of the next frame
				}
			}

			// Remove the map; the material then uses its value
			if (map)
			{
				ImGui::SameLine();
				if (ImGui::Button("Remove"))
				{
					s_PendingMaterialTextures.push_back({ material, i, std::string() }); // applied at the start of the next frame
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Remove this map: the material uses its value instead\n(the model file is not changed)");
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%s", std::filesystem::path(map->GetPath()).filename().string().c_str());
			}
		}
		ImGui::EndGroup();

		ImGui::PopID();
	}

	ImGui::End();
}

// Where a mesh is drawn: model transform (Models and Meshes panel) * node transform from the model file. Rigged meshes: the bone matrices
// already place the skinned vertices relative to the root node, so the root node transform replaces the node transform.
static glm::mat4 GetMeshTransform(H2M::RefH2M<H2M::ModelH2M> model, const H2M::RefH2M<H2M::MeshH2M>& mesh, const glm::mat4& transform)
{
	return (model->IsSkinned() && mesh->IsRigged) ? transform * model->GetRootTransform() : transform * mesh->Transform;
}

// Framebuffers, pipelines and the bounding box vertex buffer of the editor overlays (see EditorOverlaySettings)
static void CreateEditorOverlayResources()
{
	H2M::FramebufferSpecificationH2M framebufferSpec;
	framebufferSpec.Attachments = { H2M::ImageFormatH2M::RGBA, H2M::ImageFormatH2M::Depth };
	framebufferSpec.Samples = 1;
	framebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	framebufferSpec.Width = s_ViewportWidth;
	framebufferSpec.Height = s_ViewportHeight;
	framebufferSpec.DebugName = "EditorOverlay";
	s_OverlayFramebuffer = H2M::FramebufferH2M::Create(framebufferSpec);
	framebufferSpec.DebugName = "SelectionMask";
	s_SelectionMaskFramebuffer = H2M::FramebufferH2M::Create(framebufferSpec);

	// Vertex layouts of ModelH2M's Vertex and AnimatedVertex (the overlay shaders read the position, and the bones)
	H2M::VertexBufferLayoutH2M staticLayout = {
		{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Normal" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Tangent" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Binormal" },
		{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
	};
	H2M::VertexBufferLayoutH2M animLayout = {
		{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Normal" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Tangent" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Binormal" },
		{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		{ H2M::ShaderDataTypeH2M::Int4,   "a_BoneIndices" },
		{ H2M::ShaderDataTypeH2M::Float4, "a_BoneWeights" },
	};

	auto createPipeline = [](const char* debugName, bool anim, const H2M::VertexBufferLayoutH2M& layout, const H2M::RefH2M<H2M::FramebufferH2M>& target,
		H2M::PrimitiveTopologyH2M topology, bool wireframe, bool depthTest, bool depthWrite)
	{
		H2M::PipelineSpecificationH2M pipelineSpecification;
		pipelineSpecification.Layout = layout;
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get(anim ? "EditorOverlay_Anim" : "EditorOverlay");
		pipelineSpecification.Topology = topology;
		pipelineSpecification.Wireframe = wireframe;
		pipelineSpecification.BackfaceCulling = false; // the whole silhouette / all edges, also of open or single-sided meshes
		pipelineSpecification.DepthTest = depthTest;
		pipelineSpecification.DepthWrite = depthWrite;
		pipelineSpecification.DebugName = debugName;
		H2M::RenderPassSpecificationH2M renderPassSpec;
		renderPassSpec.TargetFramebuffer = target;
		pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
		return H2M::PipelineH2M::Create(pipelineSpecification);
	};

	const auto triangles = H2M::PrimitiveTopologyH2M::Triangles;
	s_OverlayDepthPipeline      = createPipeline("OverlayDepth",      false, staticLayout, s_OverlayFramebuffer, triangles, false, true, true);
	s_OverlayDepthPipelineAnim  = createPipeline("OverlayDepth-Anim", true,  animLayout,   s_OverlayFramebuffer, triangles, false, true, true);
	s_WireframePipeline         = createPipeline("Wireframe",         false, staticLayout, s_OverlayFramebuffer, triangles, true,  true, false);
	s_WireframePipelineAnim     = createPipeline("Wireframe-Anim",    true,  animLayout,   s_OverlayFramebuffer, triangles, true,  true, false);
	s_BoundingBoxPipeline       = createPipeline("BoundingBox",       false, staticLayout, s_OverlayFramebuffer, H2M::PrimitiveTopologyH2M::Lines, false, false, false);
	s_SelectionMaskPipeline     = createPipeline("SelectionMask",      false, staticLayout, s_SelectionMaskFramebuffer, triangles, false, false, false);
	s_SelectionMaskPipelineAnim = createPipeline("SelectionMask-Anim", true,  animLayout,   s_SelectionMaskFramebuffer, triangles, false, false, false);

	// Normal, tangent and bitangent lines: no per-vertex input; the mesh vertex layout is the per-instance input (binding 1),
	// so one instance is one mesh vertex and its two line vertices come from gl_VertexIndex (see EditorVectors.glsl)
	auto createVectorsPipeline = [](const char* debugName, const char* shaderName, const H2M::VertexBufferLayoutH2M& meshLayout)
	{
		H2M::PipelineSpecificationH2M pipelineSpecification;
		pipelineSpecification.InstanceLayout = meshLayout;
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get(shaderName);
		pipelineSpecification.Topology = H2M::PrimitiveTopologyH2M::Lines;
		pipelineSpecification.BackfaceCulling = false;
		pipelineSpecification.DepthTest = true; // hidden behind meshes (the depth pass of RecordEditorOverlayPasses)
		pipelineSpecification.DepthWrite = false;
		pipelineSpecification.DebugName = debugName;
		H2M::RenderPassSpecificationH2M renderPassSpec;
		renderPassSpec.TargetFramebuffer = s_OverlayFramebuffer;
		pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
		return H2M::PipelineH2M::Create(pipelineSpecification);
	};
	s_VectorsPipeline     = createVectorsPipeline("Vectors",      "EditorVectors",      staticLayout);
	s_VectorsPipelineAnim = createVectorsPipeline("Vectors-Anim", "EditorVectors_Anim", animLayout);

	// Edges of the unit cube (0..1), scaled to each bounding box. The vertices have the static mesh layout (only the position
	// is set), so the bounding box pipeline uses the same shader and vertex inputs as the others.
	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec3 Unused[3]; // normal, tangent, binormal
		glm::vec2 TexCoord;
	};
	glm::vec3 corners[8];
	for (uint32_t i = 0; i < 8; i++)
	{
		corners[i] = glm::vec3((float)(i & 1), (float)((i >> 1) & 1), (float)((i >> 2) & 1));
	}
	const uint32_t edges[s_BoundingBoxVertexCount] = { 0,1, 2,3, 4,5, 6,7,  0,2, 1,3, 4,6, 5,7,  0,4, 1,5, 2,6, 3,7 };
	LineVertex lines[s_BoundingBoxVertexCount] = {};
	for (uint32_t i = 0; i < s_BoundingBoxVertexCount; i++)
	{
		lines[i].Position = corners[edges[i]];
	}
	s_BoundingBoxVertexBuffer = H2M::VertexBufferH2M::Create(lines, (uint32_t)sizeof(lines));
}

// Draws a loaded model (all meshes, or only mesh onlyMesh) in one color with an overlay pipeline.
// lineWidth: for the wireframe pipelines (dynamic state), 0 for the others.
static void DrawModelOverlay(VkCommandBuffer commandBuffer, LoadedModelVulkan& entry, int onlyMesh, const H2M::RefH2M<H2M::PipelineH2M>& staticPipeline,
	const H2M::RefH2M<H2M::PipelineH2M>& animPipeline, const glm::vec4& color, const glm::mat4& viewProjection, float lineWidth = 0.0f)
{
	H2M::RefH2M<H2M::ModelH2M> model = entry.Model;
	bool skinned = model->IsSkinned();
	// EditorOverlay_Anim.glsl declares the bone matrices exactly like set 2 of HazelPBR_Anim.glsl (one uniform buffer at
	// binding 0, vertex stage), only as its set 0: the set layouts are identical, so the model's per-object set is bound directly
	VkDescriptorSet boneDescriptorSet = skinned ? model->GetObjectDescriptorSet() : VK_NULL_HANDLE;
	if (skinned && !boneDescriptorSet)
	{
		return; // no bone buffer (the model isn't drawn by RenderModelVulkan either)
	}
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = (skinned ? animPipeline : staticPipeline).As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

	VkBuffer vertexBuffer = model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, model->GetIndexBuffer().As<H2M::VulkanIndexBufferH2M>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());
	if (lineWidth > 0.0f)
	{
		vkCmdSetLineWidth(commandBuffer, lineWidth);
	}
	if (skinned)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &boneDescriptorSet, 0, nullptr);
	}
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);

	glm::mat4 transform = entry.GetTransform();
	auto& meshes = model->GetMeshes();
	for (int s = 0; s < (int)meshes.size(); s++)
	{
		if (onlyMesh >= 0 && s != onlyMesh)
		{
			continue;
		}
		glm::mat4 mvp = viewProjection * GetMeshTransform(model, meshes[s], transform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
		vkCmdDrawIndexed(commandBuffer, meshes[s]->IndexCount, 1, meshes[s]->BaseIndex, meshes[s]->BaseVertex, 0);
	}
}

// Draws a loaded model's normal, tangent or bitangent lines (vectorIndex 0, 1, 2): one line per vertex of every mesh (or
// only of mesh onlyMesh), lineLength long in world units
static void DrawModelVectors(VkCommandBuffer commandBuffer, LoadedModelVulkan& entry, int onlyMesh, uint32_t vectorIndex, float lineLength,
	int colorMode, const glm::mat4& viewProjection, float lineWidth)
{
	H2M::RefH2M<H2M::ModelH2M> model = entry.Model;
	bool skinned = model->IsSkinned();
	VkDescriptorSet boneDescriptorSet = skinned ? model->GetObjectDescriptorSet() : VK_NULL_HANDLE;
	if (skinned && !boneDescriptorSet)
	{
		return;
	}
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = (skinned ? s_VectorsPipelineAnim : s_VectorsPipeline).As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

	// The model's vertex buffer as the per-instance input (binding 1); binding 0 has no attributes, the same buffer is bound there
	VkBuffer vertexBuffer = model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkBuffer buffers[2] = { vertexBuffer, vertexBuffer };
	VkDeviceSize offsets[2] = { 0, 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());
	vkCmdSetLineWidth(commandBuffer, lineWidth);
	if (skinned)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &boneDescriptorSet, 0, nullptr);
	}

	struct VectorsPushConstants
	{
		glm::mat4 ViewProjection;
		glm::vec4 Model[4]; // model matrix columns; w: length, vector, color mode, unused (see EditorVectors.glsl)
	} pushConstants;
	pushConstants.ViewProjection = viewProjection;

	glm::mat4 transform = entry.GetTransform();
	auto& meshes = model->GetMeshes();
	for (int s = 0; s < (int)meshes.size(); s++)
	{
		if ((onlyMesh >= 0 && s != onlyMesh) || meshes[s]->VertexCount == 0)
		{
			continue;
		}
		glm::mat4 meshMatrix = GetMeshTransform(model, meshes[s], transform);
		for (int c = 0; c < 4; c++)
		{
			pushConstants.Model[c] = glm::vec4(glm::vec3(meshMatrix[c]), 0.0f);
		}
		pushConstants.Model[0].w = lineLength;
		pushConstants.Model[1].w = (float)vectorIndex;
		pushConstants.Model[2].w = (float)colorMode;
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

		// Two vertices per line, one instance per mesh vertex: firstInstance selects the mesh's vertices in the buffer
		vkCmdDraw(commandBuffer, 2, meshes[s]->VertexCount, 0, meshes[s]->BaseVertex);
	}
}

// Size of a loaded model as placed in the scene: the diagonal of the bounding box of all its mesh boxes (world space)
static float GetModelWorldSize(LoadedModelVulkan& entry)
{
	glm::vec3 boundsMin(std::numeric_limits<float>::max());
	glm::vec3 boundsMax(-std::numeric_limits<float>::max());
	glm::mat4 transform = entry.GetTransform();
	for (auto& mesh : entry.Model->GetMeshes())
	{
		glm::mat4 meshTransform = GetMeshTransform(entry.Model, mesh, transform);
		const H2M::AABB_H2M& box = mesh->BoundingBox;
		for (int corner = 0; corner < 8; corner++)
		{
			glm::vec3 point((corner & 1) ? box.Max.x : box.Min.x, (corner & 2) ? box.Max.y : box.Min.y, (corner & 4) ? box.Max.z : box.Min.z);
			glm::vec3 placed = glm::vec3(meshTransform * glm::vec4(point, 1.0f));
			boundsMin = glm::min(boundsMin, placed);
			boundsMax = glm::max(boundsMax, placed);
		}
	}
	return boundsMin.x <= boundsMax.x ? glm::length(boundsMax - boundsMin) : 1.0f;
}

// Draws the editor overlays into s_OverlayFramebuffer and s_SelectionMaskFramebuffer. Both are rendered every frame (cleared
// when there is nothing to show): the viewport composite samples them, so they always need valid content.
static void RecordEditorOverlayPasses(VkCommandBuffer commandBuffer)
{
	const glm::mat4 viewProjection = s_Data.SceneData.SceneCamera.Camera.GetViewProjection();
	const bool hasSelection = s_SelectedModelIndex >= 0 && s_SelectedModelIndex < (int)s_LoadedModels.size();
	const EditorOverlaySettings& settings = s_OverlaySettings;

	// Wide lines are limited by the GPU (at least 8 px is guaranteed)
	const float maxLineWidth = H2M::VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetProperties().limits.lineWidthRange[1];
	const float lineWidth = glm::clamp(settings.LineWidth, 1.0f, maxLineWidth);

	auto beginPass = [commandBuffer](const H2M::RefH2M<H2M::FramebufferH2M>& target)
	{
		H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = target.As<H2M::VulkanFramebufferH2M>();
		uint32_t width = framebuffer->GetWidth();
		uint32_t height = framebuffer->GetHeight();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
		renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2; // Color + depth
		renderPassBeginInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		VkRect2D scissor = { { 0, 0 }, { width, height } };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	};
	auto inScope = [hasSelection](int scope, int modelIndex)
	{
		return scope == OverlayScopeAll || (scope == OverlayScopeSelected && hasSelection && modelIndex == s_SelectedModelIndex);
	};

	// Wireframe and bounding boxes
	beginPass(s_OverlayFramebuffer);
	{
		const bool showVectors = settings.Vectors != OverlayScopeOff && (settings.ShowNormals || settings.ShowTangents || settings.ShowBitangents);

		// The edges and vector lines are pulled slightly towards the camera (clip z - bias * w), so they win the depth test
		// against their own faces
		glm::mat4 depthBias(1.0f);
		depthBias[3][2] = -2.0e-5f;

		if (settings.Wireframe != OverlayScopeOff || showVectors)
		{
			// Depth of every mesh first (color alpha 0 leaves the image unchanged), so lines hidden behind a mesh are hidden
			for (LoadedModelVulkan& entry : s_LoadedModels)
			{
				DrawModelOverlay(commandBuffer, entry, -1, s_OverlayDepthPipeline, s_OverlayDepthPipelineAnim, glm::vec4(0.0f), viewProjection);
			}
		}

		if (settings.Wireframe != OverlayScopeOff)
		{
			for (int m = 0; m < (int)s_LoadedModels.size(); m++)
			{
				if (inScope(settings.Wireframe, m))
				{
					// "Selected": the selected mesh, or the whole model when no mesh is selected (like the selection outline)
					int onlyMesh = -1;
					if (settings.Wireframe == OverlayScopeSelected && s_SelectedMeshIndex < (int)s_LoadedModels[m].Model->GetMeshes().size())
					{
						onlyMesh = s_SelectedMeshIndex;
					}
					DrawModelOverlay(commandBuffer, s_LoadedModels[m], onlyMesh, s_WireframePipeline, s_WireframePipelineAnim, settings.WireframeColor,
						depthBias * viewProjection, lineWidth);
				}
			}
		}

		if (showVectors)
		{
			const bool show[3] = { settings.ShowNormals, settings.ShowTangents, settings.ShowBitangents };
			for (int m = 0; m < (int)s_LoadedModels.size(); m++)
			{
				if (!inScope(settings.Vectors, m))
				{
					continue;
				}
				// "Selected": the selected mesh, or the whole model when no mesh is selected (like the wireframe)
				int onlyMesh = -1;
				if (settings.Vectors == OverlayScopeSelected && s_SelectedMeshIndex < (int)s_LoadedModels[m].Model->GetMeshes().size())
				{
					onlyMesh = s_SelectedMeshIndex;
				}
				float length = GetModelWorldSize(s_LoadedModels[m]) * settings.VectorLength * 0.01f;
				for (uint32_t v = 0; v < 3; v++)
				{
					if (show[v])
					{
						DrawModelVectors(commandBuffer, s_LoadedModels[m], onlyMesh, v, length, settings.VectorColorMode, depthBias * viewProjection, lineWidth);
					}
				}
			}
		}

		if (settings.BoundingBoxes != OverlayScopeOff)
		{
			H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_BoundingBoxPipeline.As<H2M::VulkanPipelineH2M>();
			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
			VkBuffer vertexBuffer = s_BoundingBoxVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());
			vkCmdSetLineWidth(commandBuffer, lineWidth);

			for (int m = 0; m < (int)s_LoadedModels.size(); m++)
			{
				if (!inScope(settings.BoundingBoxes, m))
				{
					continue;
				}
				LoadedModelVulkan& entry = s_LoadedModels[m];
				glm::mat4 transform = entry.GetTransform();
				auto& meshes = entry.Model->GetMeshes();
				for (int s = 0; s < (int)meshes.size(); s++)
				{
					// "Selected": the selected mesh's box, or all boxes of the model when no mesh is selected (like the wireframe)
					if (settings.BoundingBoxes == OverlayScopeSelected && s_SelectedMeshIndex >= 0 &&
						s_SelectedMeshIndex < (int)meshes.size() && s != s_SelectedMeshIndex)
					{
						continue;
					}

					// Each mesh's box in its own space, so it turns with the model (as in SceneHazelEnvMap)
					const H2M::AABB_H2M& box = meshes[s]->BoundingBox;
					glm::mat4 mvp = viewProjection * GetMeshTransform(entry.Model, meshes[s], transform) *
						glm::translate(glm::mat4(1.0f), box.Min) * glm::scale(glm::mat4(1.0f), box.Max - box.Min);

					// The selection (the selected mesh, or the whole selected model when no mesh is selected) in its own color
					bool selected = m == s_SelectedModelIndex && (s_SelectedMeshIndex < 0 || s == s_SelectedMeshIndex);
					glm::vec4 color = selected ? settings.SelectedBoundingBoxColor : settings.BoundingBoxColor;

					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
					vkCmdDraw(commandBuffer, s_BoundingBoxVertexCount, 1, 0, 0);
				}
			}
		}
	}
	vkCmdEndRenderPass(commandBuffer);

	// Silhouette of the selection: the selected mesh, or the whole model when no mesh is selected
	beginPass(s_SelectionMaskFramebuffer);
	if (settings.Outline && hasSelection)
	{
		LoadedModelVulkan& entry = s_LoadedModels[s_SelectedModelIndex];
		int mesh = s_SelectedMeshIndex < (int)entry.Model->GetMeshes().size() ? s_SelectedMeshIndex : -1;
		DrawModelOverlay(commandBuffer, entry, mesh, s_SelectionMaskPipeline, s_SelectionMaskPipelineAnim, glm::vec4(1.0f), viewProjection);
	}
	vkCmdEndRenderPass(commandBuffer);
}

/**** BEGIN to be removed from VulkanRenderer ****/
void EnvMapVulkanRenderer::SubmitModelTemp(const H2M::RefH2M<H2M::ModelH2M>& model, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials)
{
	// Temporary code - populate selected mesh
	// std::vector<Submesh> submeshes = mesh->GetMeshes();
	// s_SelectedSubmesh = &submeshes.at(0);

	s_SubmittedModels.push_back({ model, transform, materials });

	// VulkanRendererData::DrawCommand drawCommand = {};
	// drawCommand.Mesh = mesh;
	// drawCommand.Transform = transform;
	s_Data.DrawList.push_back({ model, H2M::RefH2M<H2M::MaterialH2M>(), transform });
}
/**** END to be removed from VulkanRenderer ****/

/**** BEGIN to be removed from VulkanRenderer ****/
void EnvMapVulkanRenderer::OnResize(uint32_t width, uint32_t height)
{
	// H2M::RendererH2M::Submit([=]() {});
	{
		auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer.As<H2M::VulkanFramebufferH2M>();

		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = *s_Data.QuadDescriptorSet.DescriptorSets.data();
		writeDescriptorSet.descriptorCount = (uint32_t)s_Data.QuadDescriptorSet.DescriptorSets.size();
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.pImageInfo = &framebuffer->GetVulkanDescriptorInfo();
		writeDescriptorSet.dstBinding = 0;

		auto vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
		vkUpdateDescriptorSets(vulkanDevice, 1, &writeDescriptorSet, 0, nullptr);
	}
}
/**** END to be removed from VulkanRenderer ****/

// The sun's shadow map and its pipelines (the pipelines are built for the shadow map's render pass, so they are recreated
// with it). writeDescriptor: point set 0 of the PBR shaders at the new map (at startup set 0 doesn't exist yet). Nothing in
// flight may still use the old shadow map.
static void CreateShadowMap(uint32_t resolution, bool writeDescriptor)
{
	s_ShadowPipelineAnim.Destroy();
	s_ShadowPipeline.Destroy();
	s_ShadowMap.Create(resolution, ShadowCascadeCount);
	s_ShadowSettings.Resolution = resolution;
	{
		// The mesh vertex layouts (ModelH2M's Vertex and AnimatedVertex), as in the PBR pipelines
		H2M::VertexBufferLayoutH2M staticLayout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Normal" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Tangent" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Binormal" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		};
		H2M::VertexBufferLayoutH2M animLayout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Normal" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Tangent" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Binormal" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
			{ H2M::ShaderDataTypeH2M::Int4,   "a_BoneIndices" },
			{ H2M::ShaderDataTypeH2M::Float4, "a_BoneWeights" },
		};
		s_ShadowPipeline.Create(H2M::RendererH2M::GetShaderLibrary()->Get("ShadowDepth").As<H2M::VulkanShaderH2M>(), staticLayout, { 0 }, s_ShadowMap.GetRenderPass());
		s_ShadowPipelineAnim.Create(H2M::RendererH2M::GetShaderLibrary()->Get("ShadowDepth_Anim").As<H2M::VulkanShaderH2M>(), animLayout, { 0, 5, 6 }, s_ShadowMap.GetRenderPass());
	}
	if (writeDescriptor)
	{
		WriteShadowMapDescriptor();
	}
	Log::GetLogger()->info("Shadow map: {0} cascades of {1} x {1}", ShadowCascadeCount, resolution);
}

// The spot and point light shadow maps. Point lights need cube map arrays (the imageCubeArray device feature): without
// them only spot lights cast shadows. Nothing in flight may still use the old maps.
static void CreateLocalShadowMaps()
{
	s_SpotShadowMaps.Create(s_LocalShadowSettings.SpotResolution, MaxShadowedSpotLights);
	if (H2M::VulkanContextH2M::GetCurrentDevice()->GetEnabledFeatures().imageCubeArray)
	{
		s_PointShadowMaps.Create(s_LocalShadowSettings.PointResolution, 6 * MaxShadowedPointLights, true);
	}
	else
	{
		s_PointShadowMaps.Destroy();
		Log::GetLogger()->warn("Point light shadows are not available: the GPU doesn't support cube map arrays (imageCubeArray)");
	}
	Log::GetLogger()->info("Local light shadow maps: {0} spot lights at {1} x {1}, {2} point lights at {3} x {3} per cube face",
		MaxShadowedSpotLights, s_LocalShadowSettings.SpotResolution, s_PointShadowMaps.IsValid() ? MaxShadowedPointLights : 0,
		s_LocalShadowSettings.PointResolution);
}

void EnvMapVulkanRenderer::Init()
{
	/**** BEGIN: to be removed from VulkanRenderer ****/
	// H2M::RendererH2M::Submit([=]() {});
	{
		s_ImGuiCommandBuffer = H2M::VulkanContextH2M::GetCurrentDevice()->CreateSecondaryCommandBuffer();
		s_CompositeCommandBuffer = H2M::VulkanContextH2M::GetCurrentDevice()->CreateSecondaryCommandBuffer();
	}
	/**** END: to be removed from VulkanRenderer ****/

	// s_Data = VulkanRendererData{};
	auto& caps = s_Data.RenderCaps;
	auto& properties = H2M::VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
	caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
	caps.Device = properties.deviceName;
	caps.Version = std::to_string(properties.driverVersion);

	H2M::Utils::DumpGPUInfo();

	// TODO: Create descriptor pools

	/**** BEGIN code from H2M::RendererH2M::Init() ****/

	// s_Data.m_ShaderLibrary = H2M::RefH2M<HazelShaderLibrary>::Create();

	// s_Data.m_ShaderLibrary->Load("Resources/Shaders/Grid.glsl");
	// s_Data.m_ShaderLibrary->Load("Resources/Shaders/SceneComposite.glsl");
	// s_Data.m_ShaderLibrary->Load("Resources/Shaders/HazelSimple.glsl");
	// s_Data.m_ShaderLibrary->Load("Resources/Shaders/Outline.glsl");
	// The shaders used here (EquirectangularToCubeMap, EnvironmentMipFilter, EnvironmentIrradiance, HazelPBR_Static,
	// Skybox, Texture, SceneComposite, Grid, Outline) are already loaded by H2M::RendererH2M::Init()

	H2M::SceneRendererH2M::Init();

	// Renderer2D::Init();

	/**** END code from H2M::RendererH2M::Init() ****/

	/**** BEGIN: to be removed from VulkanRenderer ****/
	{
		H2M::FramebufferSpecificationH2M framebufferSpec;
		framebufferSpec.Attachments = { H2M::ImageFormatH2M::RGBA16F, H2M::ImageFormatH2M::Depth }; // linear HDR: half floats are plenty
		framebufferSpec.Samples = 1;
		framebufferSpec.ClearOnLoad = false;
		framebufferSpec.ClearColor = { 0.1f, 0.5f, 0.5f, 1.0f };
		framebufferSpec.CopySource = true; // the water copies its color and depth (see GeometryPass)
		framebufferSpec.DebugName = "Viewport";
		framebufferSpec.Width = s_ViewportWidth;
		framebufferSpec.Height = s_ViewportHeight;
		s_Framebuffer = H2M::FramebufferH2M::Create(framebufferSpec);
		s_Framebuffer->AddResizeCallback([](H2M::RefH2M<H2M::FramebufferH2M> framebuffer)
		{
			// H2M::RendererH2M::Submit([framebuffer]() mutable {});
			{
				auto vulkanFB = framebuffer.As<H2M::VulkanFramebufferH2M>();
				const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
				Log::GetLogger()->warn("Resizing framebuffer; image layout is {0}", static_cast<uint32_t>(imageInfo.imageLayout));
				s_ViewportTextureNeedsUpdate = true; // re-register the new image with ImGui (see RegisterViewportTextureWithImGui)

				auto shader = s_CompositePipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();

				VkWriteDescriptorSet writeDescriptorSet = {};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = *s_Data.QuadDescriptorSet.DescriptorSets.data();
				writeDescriptorSet.descriptorCount = (uint32_t)s_Data.QuadDescriptorSet.DescriptorSets.size();
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSet.pImageInfo = &vulkanFB->GetVulkanDescriptorInfo();
				writeDescriptorSet.dstBinding = 0;

				auto vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
				vkUpdateDescriptorSets(vulkanDevice, 1, &writeDescriptorSet, 0, nullptr);
			}
		});

		H2M::PipelineSpecificationH2M pipelineSpecification;
		pipelineSpecification.Layout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Normal" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Tangent" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Binormal" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		};
		// pipelineSpecification.Shader = s_Data.m_ShaderLibrary->Get("HazelPBR_Static");
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");

		H2M::RenderPassSpecificationH2M renderPassSpec;
		renderPassSpec.TargetFramebuffer = s_Framebuffer;
		pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
		pipelineSpecification.DebugName = "PBR-Static";
		s_MeshPipeline = H2M::PipelineH2M::Create(pipelineSpecification);

		// Skinned models: same render pass, vertex layout of ModelH2M's AnimatedVertex
		pipelineSpecification.Layout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Normal" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Tangent" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Binormal" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
			{ H2M::ShaderDataTypeH2M::Int4,   "a_BoneIndices" },
			{ H2M::ShaderDataTypeH2M::Float4, "a_BoneWeights" },
		};
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Anim");
		pipelineSpecification.DebugName = "PBR-Anim";
		s_MeshPipelineAnim = H2M::PipelineH2M::Create(pipelineSpecification);
	}
	/**** END: to be removed from VulkanRenderer ****/

	/**** BEGIN: to be removed from VulkanRenderer ****/
	{
		H2M::FramebufferSpecificationH2M framebufferSpec;
		framebufferSpec.Attachments = { H2M::ImageFormatH2M::RGBA, H2M::ImageFormatH2M::Depth };
		framebufferSpec.DebugName = "CompositeFramebuffer";
		framebufferSpec.SwapChainTarget = true;
		framebufferSpec.Width = s_ViewportWidth;
		framebufferSpec.Height = s_ViewportHeight;
		s_CompositeFramebuffer = H2M::FramebufferH2M::Create(framebufferSpec);
		s_CompositeFramebuffer->AddResizeCallback([](H2M::RefH2M<H2M::FramebufferH2M> framebuffer)
		{
			// H2M::RendererH2M::Submit([framebuffer]() mutable {});
			{
				auto vulkanFB = framebuffer.As<H2M::VulkanFramebufferH2M>();
				const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
				H2M_CORE_WARN("Resizing framebuffer; image layout is {0}", static_cast<uint32_t>(imageInfo.imageLayout));
				// s_TextureID = ImGui_ImplVulkan_UpdateTextureInfo((VkDescriptorSet)s_TextureID, imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			}
		});

		H2M::PipelineSpecificationH2M pipelineSpecification;
		pipelineSpecification.Layout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		};
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("SceneComposite");

		H2M::RenderPassSpecificationH2M renderPassSpec;
		renderPassSpec.TargetFramebuffer = s_CompositeFramebuffer;
		pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
		pipelineSpecification.DebugName = "SceneComposite";
		s_CompositePipeline = H2M::PipelineH2M::Create(pipelineSpecification);
	}
	/**** END: to be removed from VulkanRenderer ****/

	// Offscreen composite for the Viewport panel (exposure, tonemapping, gamma), see ViewportCompositePass
	{
		H2M::FramebufferSpecificationH2M framebufferSpec;
		framebufferSpec.Attachments = { H2M::ImageFormatH2M::RGBA, H2M::ImageFormatH2M::Depth };
		framebufferSpec.Samples = 1;
		framebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		framebufferSpec.DebugName = "ViewportComposite";
		framebufferSpec.Width = s_ViewportWidth;
		framebufferSpec.Height = s_ViewportHeight;
		s_ViewportCompositeFramebuffer = H2M::FramebufferH2M::Create(framebufferSpec);
		s_ViewportCompositeFramebuffer->AddResizeCallback([](H2M::RefH2M<H2M::FramebufferH2M> framebuffer)
		{
			s_ViewportTextureNeedsUpdate = true; // re-register the new image with ImGui (see RegisterViewportTextureWithImGui)
		});

		H2M::PipelineSpecificationH2M pipelineSpecification;
		pipelineSpecification.Layout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		};
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("ViewportComposite");

		H2M::RenderPassSpecificationH2M renderPassSpec;
		renderPassSpec.TargetFramebuffer = s_ViewportCompositeFramebuffer;
		pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
		pipelineSpecification.DebugName = "ViewportComposite";
		s_ViewportCompositePipeline = H2M::PipelineH2M::Create(pipelineSpecification);

		CreateEditorOverlayResources(); // before CreateBloomResources: the composite samples the overlay and selection mask images
		CreateBloomResources(); // also writes s_ViewportCompositeDescriptorSet
	}

	// Editor grid: drawn into the scene framebuffer after the meshes (see RenderGrid)
	{
		H2M::PipelineSpecificationH2M pipelineSpecification;
		pipelineSpecification.Layout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		};
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("Grid");
		pipelineSpecification.BackfaceCulling = false; // visible from above and below
		pipelineSpecification.DepthWrite = false;      // transparent: tested against the meshes, but hides nothing behind it

		H2M::RenderPassSpecificationH2M renderPassSpec;
		renderPassSpec.TargetFramebuffer = s_Framebuffer;
		pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
		pipelineSpecification.DebugName = "Grid";
		s_GridPipeline = H2M::PipelineH2M::Create(pipelineSpecification);
	}

	// Water: drawn into the scene framebuffer after the opaque meshes (see GeometryPass)
	s_Water.Create(s_Framebuffer);

	/**** BEGIN code moved from VulkanTestLayer to VulkanRenderer ****/
	H2M::RenderPassSpecificationH2M renderPassSpec;
	H2M::FramebufferSpecificationH2M framebufferSpec;
	framebufferSpec.DebugName = "GeoPassFramebufferSpec";
	framebufferSpec.Width = 1280;
	framebufferSpec.Height = 720;
	renderPassSpec.TargetFramebuffer = H2M::FramebufferH2M::Create(framebufferSpec);
	s_Data.GeoPass = H2M::RenderPassH2M::Create(renderPassSpec);

	// Geometry pipeline
	{
		H2M::FramebufferSpecificationH2M spec;
		H2M::RefH2M<H2M::FramebufferH2M> framebuffer = H2M::FramebufferH2M::Create(spec);

		H2M::PipelineSpecificationH2M pipelineSpecification;
		pipelineSpecification.Layout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Normal" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Tangent" },
			{ H2M::ShaderDataTypeH2M::Float3, "a_Binormal" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		};
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");
		pipelineSpecification.RenderPass = s_Data.GeoPass;
		pipelineSpecification.DebugName = "PBR-Static";
		s_Data.GeometryPipeline = H2M::PipelineH2M::Create(pipelineSpecification);
	}
	/**** BEGIN code moved from VulkanTestLayer to VulkanRenderer ****/

	// Create fullscreen quad
	float x = -1;
	float y = -1;
	float width = 2;
	float height = 2;

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
	};

	QuadVertex* data = new QuadVertex[4];

	data[0].Position = glm::vec3(x, y, 0.1f);
	data[0].TexCoord = glm::vec2(0, 0);

	data[1].Position = glm::vec3(x + width, y, 0.1f);
	data[1].TexCoord = glm::vec2(1, 0);

	data[2].Position = glm::vec3(x + width, y + height, 0.1f);
	data[2].TexCoord = glm::vec2(1, 1);

	data[3].Position = glm::vec3(x, y + height, 0.1f);
	data[3].TexCoord = glm::vec2(0, 1);

	s_Data.QuadVertexBuffer = H2M::VertexBufferH2M::Create(data, 4 * sizeof(QuadVertex));
	uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
	s_Data.QuadIndexBuffer = H2M::IndexBufferH2M::Create(indices, 6 * sizeof(uint32_t));

	// H2M::RendererH2M::Submit([=]() {});
	{
		auto shader = s_CompositePipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
		auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer.As<H2M::VulkanFramebufferH2M>();
		s_Data.QuadDescriptorSet = shader->CreateDescriptorSets();

		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = *s_Data.QuadDescriptorSet.DescriptorSets.data();
		writeDescriptorSet.descriptorCount = (uint32_t)s_Data.QuadDescriptorSet.DescriptorSets.size();
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.pImageInfo = &framebuffer->GetVulkanDescriptorInfo();
		writeDescriptorSet.dstBinding = 0;

		auto vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
		vkUpdateDescriptorSets(vulkanDevice, 1, &writeDescriptorSet, 0, nullptr);

		auto vulkanFB = s_Framebuffer.As<H2M::VulkanFramebufferH2M>();
		const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
		// s_TextureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
	}

	// s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap("Textures/HDR/pink_sunrise_4k.hdr");
	// s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap("Textures/HDR/umhlanga_sunrise_4k.hdr");
	// s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap("Textures/HDR/venice_dawn_1_4k.hdr");
	// s_EnvMapFilename = "Textures/HDR/newport_loft.hdr";
	// The scene's choice (SetEnvironmentMapFile, from SceneEnvMapVulkan's user preferences); without one, an outdoor map
	// with a real sun (a compact disc, about 7,000x brighter than anything else). A real sun is extracted and the sun light
	// aligned to it at startup (see Draw).
	if (s_EnvMapFilename.empty())
	{
		s_EnvMapFilename = "Textures/HDR/rooitou_park_4k.hdr";
	}
	s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap(s_EnvMapFilename);

	s_Data.BRDFLut = H2M::Texture2D_H2M::Create("assets/textures/BRDF_LUT.tga", false);

	CreateShadowMap(s_ShadowSettings.Resolution, false); // the descriptor is written with the rest of set 0, below
	CreateLocalShadowMaps();
	s_ShadowMapViewer.Create(H2M::RendererH2M::GetShaderLibrary()->Get("ShadowMapView").As<H2M::VulkanShaderH2M>());

	// H2M::RendererH2M::Submit([environment]() mutable {});
	{
		// The per-frame set (set 0) is shared by the static and the skinned mesh pipelines: set 0 is declared identically in
		// HazelPBR_Static.glsl and HazelPBR_Anim.glsl, so a set allocated with one shader's layout is valid for both
		auto shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");
		H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = shader.As<H2M::VulkanShaderH2M>();
		const uint32_t frameSet = H2M::VulkanShaderH2M::FrameDescriptorSet;
		s_Data.FrameDescriptorSet = pbrShader->CreateDescriptorSets(frameSet);

		// Camera, SceneData and Lights uniform buffers (written every frame, see UpdateFrameUniforms); the environment maps
		// are written in SetSceneEnvironment
		const char* bufferNames[] = { "Camera", "SceneData", "Lights", "Shadows" };
		const uint32_t bufferBindings[] = { 0, 1, 5, 7 };
		std::array<VkWriteDescriptorSet, 4> writes;
		for (size_t i = 0; i < writes.size(); i++)
		{
			writes[i] = *pbrShader->GetDescriptorSet(bufferNames[i], frameSet);
			writes[i].dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
			writes[i].descriptorCount = 1;
			writes[i].pBufferInfo = &pbrShader->GetUniformBuffer(bufferBindings[i], frameSet).Descriptor;
		}
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);

		WriteShadowMapDescriptor();

		// binding 10: the water's caustics map (it exists from s_Water.Create on, with or without water on the scene)
		VkWriteDescriptorSet causticsWrite = *pbrShader->GetDescriptorSet("u_CausticsMap", frameSet);
		causticsWrite.dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
		causticsWrite.descriptorCount = 1;
		causticsWrite.pImageInfo = &s_Water.GetCausticsDescriptorInfo();
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &causticsWrite, 0, nullptr);
	}

	H2M::RendererH2M::SetSceneEnvironment(H2M::RefH2M<H2M::EnvironmentH2M>::Create(s_Data.EnvironmentMap.first, s_Data.EnvironmentMap.second), H2M::RefH2M<H2M::Image2D_H2M>());

	/*** BEGIN Setup the Skybox ****/
	s_Data.VulkanSkyboxCube = H2M::RefH2M<VulkanSkyboxCube>::Create();

	H2M::PipelineSpecificationH2M skyboxPipelineSpecification;
	skyboxPipelineSpecification.DebugName = "Skybox Pipeline Specification";
	skyboxPipelineSpecification.Layout = {
		{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
		{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
	};
	s_Data.SkyboxShader = H2M::RendererH2M::GetShaderLibrary()->Get("Skybox");
	skyboxPipelineSpecification.Shader = s_Data.SkyboxShader;

	H2M::RenderPassSpecificationH2M renderPassSpecSkybox;
	renderPassSpecSkybox.DebugName = "Skybox RenderPass Specification";
	renderPassSpecSkybox.TargetFramebuffer = s_Framebuffer;
	skyboxPipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpecSkybox);

	s_Data.SkyboxPipeline = H2M::PipelineH2M::Create(skyboxPipelineSpecification);
	/*** END Setup the Skybox ****/

	Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

	s_Data.SceneData.SkyboxLod = 0.0f;
	Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE; // as in SceneHazelEnvMap (keys 1/2/3/4 switch the mode)

	OnResize(s_ViewportWidth, s_ViewportHeight); // to be removed from VulkanRenderer
}

void EnvMapVulkanRenderer::SetEnvironmentMapFile(const std::string& filepath)
{
	s_EnvMapFilename = filepath;
}

void EnvMapVulkanRenderer::Shutdown()
{
	s_ShadowPipelineAnim.Destroy();
	s_ShadowPipeline.Destroy();
	s_ShadowMap.Destroy();
	s_SpotShadowMaps.Destroy();
	s_PointShadowMaps.Destroy();
	s_ShadowMapViewer.Destroy();
	s_Water.Destroy();
	H2M::VulkanShaderH2M::ClearUniformBuffers();
	// delete s_Data;
}

void EnvMapVulkanRenderer::RenderModelVulkan(H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials, VkCommandBuffer commandBuffer)
{
	/**** BEGIN keep smart references alive ****/
	H2M::RefH2M<H2M::TextureCubeH2M> envUnfiltered = s_Data.envUnfiltered;
	H2M::RefH2M<H2M::TextureCubeH2M> envFiltered = s_Data.envFiltered;
	H2M::RefH2M<H2M::TextureCubeH2M> irradianceMap = s_Data.irradianceMap;
	std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> environmentMap = s_Data.EnvironmentMap;
	H2M::RefH2M<H2M::Texture2D_H2M> BRDFLut = s_Data.BRDFLut;
	H2M::RefH2M<H2M::Texture2D_H2M> envEquirect = s_Data.envEquirect;

	// auto vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	// auto shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");
	// H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = shader.As<H2M::VulkanShaderH2M>();

	// std::array<VkWriteDescriptorSet, 1> writeDescriptors;

	// writeDescriptors[0] = *pbrShader->GetDescriptorSet("u_AlbedoTexture", 0);
	// writeDescriptors[0].dstSet = pbrShader->CreateDescriptorSets(0).DescriptorSet;
	// writeDescriptors[0].pBufferInfo = &pbrShader->GetUniformBuffer(2, 0).Descriptor;

	// vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
	/**** END keep smart references alive ****/

	/**** BEGIN Non-composite ****
	H2M::RefH2M<VulkanPipeline> vulkanPipeline = mesh->GetPipeline().As<VulkanPipeline>();
	/**** END Non-composite ****/
	/**** BEGIN Composite ****/
	// Skinned models have a different vertex layout (bone IDs and weights) and shader
	bool skinned = model->IsSkinned();
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = (skinned ? s_MeshPipelineAnim : s_MeshPipeline).As<H2M::VulkanPipelineH2M>(); // to be removed from VulkanRenderer
	/**** END Composite ****/

	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

	auto vulkanMeshVB = model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>();
	VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

	auto vulkanMeshIB = H2M::RefH2M<H2M::VulkanIndexBufferH2M>(model->GetIndexBuffer());
	VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
	vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

	VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// Set 0 (per frame) was bound once for all meshes in GeometryPass. It stays bound across the static and skinned
	// pipelines: their layouts declare set 0 and the push constants identically ("compatible for set 0").

	// Set 2 (per object): the bone matrices of a skinned model
	if (skinned)
	{
		VkDescriptorSet objectDescriptorSet = model->GetObjectDescriptorSet();
		if (objectDescriptorSet == VK_NULL_HANDLE)
		{
			return; // no bone buffer: the skinned vertices can't be placed
		}
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, H2M::VulkanShaderH2M::ObjectDescriptorSet, 1, &objectDescriptorSet, 0, nullptr);
	}

	auto& meshes = model->GetMeshes();
	for (size_t s = 0; s < meshes.size() && s < materials.size(); s++)
	{
		H2M::RefH2M<H2M::MeshH2M> mesh = meshes[s];
		H2M::RefH2M<EnvMapVulkanMaterial> material = materials[s];
		H2M::BufferH2M uniformStorageBuffer = material->GetUniformStorageBuffer();

		// Set 1 (per material): the texture maps of the mesh's library material
		VkDescriptorSet materialDescriptorSet = material->GetDescriptorSet();
		if (materialDescriptorSet == VK_NULL_HANDLE)
		{
			continue; // drawing with a missing descriptor set is undefined behavior in Vulkan
		}
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, H2M::VulkanShaderH2M::MaterialDescriptorSet, 1,
			&materialDescriptorSet, 0, nullptr);

		// Push Constants
		// glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		// vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
		glm::mat4 meshTransform = GetMeshTransform(model, mesh, transform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &meshTransform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
		vkCmdDrawIndexed(commandBuffer, mesh->IndexCount, 1, mesh->BaseIndex, mesh->BaseVertex, 0);
	}
}

// The Shadows uniform buffer of the PBR shaders (set 0, binding 7, std140). Written right after the cascades are computed
// (see RecordShadowPasses), still before the frame is submitted, so the shaders use this frame's cascades.
static void WriteShadowUniforms()
{
	struct ShadowsUB
	{
		glm::mat4 CascadeViewProjection[ShadowCascadeCount];
		glm::vec4 CascadeSplits;
		glm::vec4 CascadeTexelSizes;
		glm::vec3 CameraForward;
		float ShadowsEnabled;
		float NormalBias;
		float Softness;
		float ShowCascades;
		float ShadowMapTexelSize;
		glm::mat4 SpotShadowViewProjection[MaxShadowedSpotLights];
		glm::vec4 PointShadowDepthParams[MaxShadowedPointLights]; // xy used
		glm::vec4 SpotShadowTanHalfFov;
		glm::vec4 LocalShadowParams; // normal bias, softness, 1 / spot resolution, 1 / point resolution
		glm::vec4 ShadowDebug;       // Shadows Only: x = 1 sun, 2 point, 3 spot, 4 light off (0: normal view); y = index in the packed lights
	};
	static_assert(sizeof(ShadowsUB) == 688, "std140 layout mismatch with the Shadows block of the PBR shaders");
	static_assert(MaxShadowedSpotLights == 4 && MaxShadowedPointLights == 4, "the Shadows block of the PBR shaders has 4 slots of each");

	ShadowsUB ub = {};
	glm::mat4 cameraTransform = glm::inverse(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix());
	for (uint32_t i = 0; i < ShadowCascadeCount; i++)
	{
		ub.CascadeViewProjection[i] = s_ShadowCascades[i].ViewProjection;
		ub.CascadeSplits[i] = s_ShadowCascades[i].SplitDistance;
		ub.CascadeTexelSizes[i] = s_ShadowCascades[i].TexelWorldSize;
	}
	ub.CameraForward = -glm::normalize(glm::vec3(cameraTransform[2]));
	ub.ShadowsEnabled = s_ShadowsRendered ? 1.0f : 0.0f;
	ub.NormalBias = s_ShadowSettings.NormalBias;
	ub.Softness = s_ShadowSettings.Softness;
	ub.ShowCascades = s_ShadowSettings.ShowCascades ? 1.0f : 0.0f;
	ub.ShadowMapTexelSize = s_ShadowMap.IsValid() ? 1.0f / (float)s_ShadowMap.GetResolution() : 0.0f;

	for (uint32_t slot = 0; slot < s_LocalShadowSlots.SpotCount; slot++)
	{
		const EnvMapVulkanSpotLight& light = s_Lights.SpotLights[s_LocalShadowSlots.SpotLight[slot]];
		ub.SpotShadowViewProjection[slot] = s_LocalShadowSlots.SpotViewProjection[slot];
		ub.SpotShadowTanHalfFov[slot] = std::tan(glm::radians(GetSpotShadowFieldOfView(light.OuterAngle) * 0.5f));
	}
	for (uint32_t slot = 0; slot < s_LocalShadowSlots.PointCount; slot++)
	{
		const EnvMapVulkanPointLight& light = s_Lights.PointLights[s_LocalShadowSlots.PointLight[slot]];
		ub.PointShadowDepthParams[slot] = glm::vec4(GetPointShadowDepthParams(light.Range), 0.0f, 0.0f);
	}
	// Shadows Only: the selected light, by its index among the packed lights (Pack leaves out the lights that are off)
	if (s_ShowShadowsOnly && s_SelectedLightKind != LightKind::None)
	{
		if (s_SelectedLightKind == LightKind::Sun)
		{
			ub.ShadowDebug = glm::vec4(s_Lights.Sun.Enabled ? 1.0f : 4.0f, 0.0f, 0.0f, 0.0f);
		}
		else
		{
			const bool point = s_SelectedLightKind == LightKind::Point;
			int packedIndex = 0;
			bool enabled = false;
			for (int i = 0; i <= s_SelectedLightIndex; i++)
			{
				bool on = point ? s_Lights.PointLights[i].Enabled : s_Lights.SpotLights[i].Enabled;
				if (i == s_SelectedLightIndex)
				{
					enabled = on;
				}
				else if (on)
				{
					packedIndex++;
				}
			}
			ub.ShadowDebug = glm::vec4(enabled ? (point ? 2.0f : 3.0f) : 4.0f, (float)packedIndex, 0.0f, 0.0f);
		}
	}
	ub.LocalShadowParams = glm::vec4(s_LocalShadowSettings.NormalBias, s_LocalShadowSettings.Softness,
		s_SpotShadowMaps.IsValid() ? 1.0f / (float)s_SpotShadowMaps.GetResolution() : 0.0f,
		s_PointShadowMaps.IsValid() ? 1.0f / (float)s_PointShadowMaps.GetResolution() : 0.0f);

	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	void* ubPtr = shader->MapUniformBuffer(7, H2M::VulkanShaderH2M::FrameDescriptorSet);
	memcpy(ubPtr, &ub, sizeof(ShadowsUB));
	shader->UnmapUniformBuffer(7, H2M::VulkanShaderH2M::FrameDescriptorSet);
}

// The sun's shadow map in the per-frame set of the PBR shaders (set 0, binding 6): all cascades, with the comparison
// sampler. Written at startup and whenever the shadow map is recreated (resolution change).
static void WriteShadowMapDescriptor()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	// The sun's cascades, the spot light maps and the point light cubes (bindings 6, 8, 9)
	const EnvMapVulkanShadowMap* maps[3] = { &s_ShadowMap, &s_SpotShadowMaps, &s_PointShadowMaps };
	const char* names[3] = { "u_ShadowMap", "u_SpotShadowMaps", "u_PointShadowMaps" };
	std::array<VkDescriptorImageInfo, 3> imageInfos;
	std::vector<VkWriteDescriptorSet> writes;
	for (int i = 0; i < 3; i++)
	{
		if (!maps[i]->IsValid())
		{
			continue;
		}
		imageInfos[i] = { maps[i]->GetCompareSampler(), maps[i]->GetArrayView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkWriteDescriptorSet write = *shader->GetDescriptorSet(names[i], H2M::VulkanShaderH2M::FrameDescriptorSet);
		write.dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = &imageInfos[i];
		writes.push_back(write);
	}
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}


static void ComputeShadowCasterBounds(glm::vec3& totalMin, glm::vec3& totalMax)
{
	totalMin = glm::vec3(std::numeric_limits<float>::max());
	totalMax = glm::vec3(-std::numeric_limits<float>::max());
	s_ShadowCasterBounds.clear();
	for (const SubmittedModel& submitted : s_SubmittedModels)
	{
		glm::vec3 modelMin(std::numeric_limits<float>::max());
		glm::vec3 modelMax(-std::numeric_limits<float>::max());
		for (const H2M::RefH2M<H2M::MeshH2M>& mesh : submitted.Model->GetMeshes())
		{
			glm::mat4 transform = GetMeshTransform(submitted.Model, mesh, submitted.Transform);
			for (int c = 0; c < 8; c++)
			{
				glm::vec3 corner((c & 1) ? mesh->BoundingBox.Max.x : mesh->BoundingBox.Min.x, (c & 2) ? mesh->BoundingBox.Max.y : mesh->BoundingBox.Min.y,
					(c & 4) ? mesh->BoundingBox.Max.z : mesh->BoundingBox.Min.z);
				glm::vec3 world = glm::vec3(transform * glm::vec4(corner, 1.0f));
				modelMin = glm::min(modelMin, world);
				modelMax = glm::max(modelMax, world);
			}
		}
		s_ShadowCasterBounds.push_back({ modelMin, modelMax });
		totalMin = glm::min(totalMin, modelMin);
		totalMax = glm::max(totalMax, modelMax);
	}
	s_ShadowCasterBoundsMin = totalMin;
	s_ShadowCasterBoundsMax = totalMax;
}

// Starts a depth-only pass into one layer of a shadow map (cleared to the far plane)
static void BeginShadowPass(VkCommandBuffer commandBuffer, const EnvMapVulkanShadowMap& shadowMap, uint32_t layer, float depthBias, float slopeBias)
{
	const uint32_t resolution = shadowMap.GetResolution();
	VkClearValue clearValue = {};
	clearValue.depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = shadowMap.GetRenderPass();
	beginInfo.framebuffer = shadowMap.GetFramebuffer(layer);
	beginInfo.renderArea.extent = { resolution, resolution };
	beginInfo.clearValueCount = 1;
	beginInfo.pClearValues = &clearValue;
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = { 0.0f, 0.0f, (float)resolution, (float)resolution, 0.0f, 1.0f };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	VkRect2D scissor = { { 0, 0 }, { resolution, resolution } };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	vkCmdSetDepthBias(commandBuffer, depthBias, 0.0f, slopeBias);
}

// Draws the submitted models into the current shadow pass. With a light sphere (center, radius), only the models whose
// bounds reach into it: a spot or point light can't shadow anything beyond its range. The shadow pipelines were built for
// the sun's render pass; the spot and point light passes are compatible with it (one depth attachment of the same format).
// Returns the number of models drawn.
static uint32_t DrawShadowCasters(VkCommandBuffer commandBuffer, const glm::mat4& viewProjection, const glm::vec3* lightCenter = nullptr, float lightRadius = 0.0f)
{
	uint32_t drawn = 0;
	for (size_t m = 0; m < s_SubmittedModels.size(); m++)
	{
		if (lightCenter)
		{
			// Distance from the light to the nearest point of the model's bounds
			const glm::vec3 nearest = glm::clamp(*lightCenter, s_ShadowCasterBounds[m].first, s_ShadowCasterBounds[m].second);
			if (glm::length(nearest - *lightCenter) > lightRadius)
			{
				continue;
			}
		}

		H2M::RefH2M<H2M::ModelH2M> model = s_SubmittedModels[m].Model;
		const bool skinned = model->IsSkinned();
		const EnvMapVulkanShadowPipeline& pipeline = skinned ? s_ShadowPipelineAnim : s_ShadowPipeline;
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Pipeline);
		if (skinned)
		{
			VkDescriptorSet boneDescriptorSet = model->GetObjectDescriptorSet();
			if (boneDescriptorSet == VK_NULL_HANDLE)
			{
				continue; // no bone buffer: the skinned vertices can't be placed
			}
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Layout, 0, 1, &boneDescriptorSet, 0, nullptr);
		}

		VkBuffer vertexBuffer = model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, H2M::RefH2M<H2M::VulkanIndexBufferH2M>(model->GetIndexBuffer())->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);

		for (const H2M::RefH2M<H2M::MeshH2M>& mesh : model->GetMeshes())
		{
			glm::mat4 matrices[2] = { viewProjection, GetMeshTransform(model, mesh, s_SubmittedModels[m].Transform) };
			vkCmdPushConstants(commandBuffer, pipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), matrices);
			vkCmdDrawIndexed(commandBuffer, mesh->IndexCount, 1, mesh->BaseIndex, mesh->BaseVertex, 0);
		}
		drawn++;
	}
	return drawn;
}

// The shadow map cascades of the sun (see EnvMapVulkanShadows.h), recorded at the start of the frame, before the geometry
// pass that samples them. Every submitted model casts; the grid and the skybox don't.
static void RecordShadowPasses(VkCommandBuffer commandBuffer, const glm::vec3& boundsMin, const glm::vec3& boundsMax)
{
	s_ShadowsRendered = false;
	if (!s_ShadowMap.IsValid() || !s_Lights.Sun.Enabled || !s_Lights.Sun.CastShadows || s_Lights.Sun.Intensity <= 0.0f ||
		!s_ShadowPipeline.Pipeline || !s_ShadowPipelineAnim.Pipeline)
	{
		WriteShadowUniforms(); // shadows off
		return;
	}

	// The camera's view: position, direction and the rays through the 4 corners of the viewport
	H2M::CameraH2M& camera = s_Data.SceneData.SceneCamera.Camera;
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 cameraTransform = glm::inverse(view);
	glm::mat4 inverseViewProjection = glm::inverse(camera.GetProjectionMatrix() * view);
	glm::vec3 cameraPosition = glm::vec3(cameraTransform[3]);
	glm::vec3 cameraForward = -glm::normalize(glm::vec3(cameraTransform[2]));
	std::array<glm::vec3, 4> cornerRays;
	for (int c = 0; c < 4; c++)
	{
		glm::vec4 p = inverseViewProjection * glm::vec4((c & 1) ? 1.0f : -1.0f, (c & 2) ? 1.0f : -1.0f, 0.5f, 1.0f);
		cornerRays[c] = glm::normalize(glm::vec3(p) / p.w - cameraPosition);
	}
	if (glm::any(glm::isnan(cameraPosition)) || glm::any(glm::isnan(cornerRays[0])))
	{
		WriteShadowUniforms(); // the camera has no valid matrices yet (first frame): no shadows
		return;
	}

	s_ShadowSettings.Resolution = s_ShadowMap.GetResolution();
	ComputeShadowCascades(cameraPosition, cameraForward, cornerRays, camera.GetPerspectiveNearClip(), s_ShadowSettings, s_Lights.Sun.GetDirection(),
		boundsMin, boundsMax, s_ShadowCascades);

	for (uint32_t cascade = 0; cascade < ShadowCascadeCount; cascade++)
	{
		BeginShadowPass(commandBuffer, s_ShadowMap, cascade, s_ShadowSettings.DepthBias, s_ShadowSettings.SlopeBias);
		DrawShadowCasters(commandBuffer, s_ShadowCascades[cascade].ViewProjection);
		vkCmdEndRenderPass(commandBuffer);
	}
	s_ShadowsRendered = true;
	WriteShadowUniforms();
}

// The shadow maps of the spot lights (one pass each) and point lights (6 passes each, one per cube face), after the sun's
static void RecordLocalShadowPasses(VkCommandBuffer commandBuffer)
{
	// The slots were assigned when the Lights buffer was packed (UpdateFrameUniforms)
	LocalShadowSlots& slots = s_LocalShadowSlots;
	slots.ModelsDrawn = 0;
	if (!s_ShadowPipeline.Pipeline || !s_ShadowPipelineAnim.Pipeline)
	{
		return;
	}
	const EnvMapVulkanLocalShadowSettings& settings = s_LocalShadowSettings;

	for (uint32_t slot = 0; slot < slots.SpotCount; slot++)
	{
		const EnvMapVulkanSpotLight& light = s_Lights.SpotLights[slots.SpotLight[slot]];
		BeginShadowPass(commandBuffer, s_SpotShadowMaps, slot, settings.DepthBias, settings.SlopeBias);
		slots.ModelsDrawn += DrawShadowCasters(commandBuffer, slots.SpotViewProjection[slot], &light.Position, light.Range);
		vkCmdEndRenderPass(commandBuffer);
	}

	for (uint32_t slot = 0; slot < slots.PointCount; slot++)
	{
		const EnvMapVulkanPointLight& light = s_Lights.PointLights[slots.PointLight[slot]];
		for (uint32_t face = 0; face < 6; face++)
		{
			BeginShadowPass(commandBuffer, s_PointShadowMaps, slot * 6 + face, settings.DepthBias, settings.SlopeBias);
			slots.ModelsDrawn += DrawShadowCasters(commandBuffer, slots.PointFaceViewProjection[slot][face], &light.Position, light.Range);
			vkCmdEndRenderPass(commandBuffer);
		}
	}
}

void EnvMapVulkanRenderer::RenderSkybox(VkCommandBuffer commandBuffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanSkyboxPipeline = s_Data.SkyboxPipeline.As<H2M::VulkanPipelineH2M>();

	VkPipelineLayout skyboxPipelineLayout = vulkanSkyboxPipeline->GetVulkanPipelineLayout();

	H2M::RefH2M<H2M::VulkanShaderH2M> vulkanSkyboxShader = s_Data.SkyboxShader.As<H2M::VulkanShaderH2M>();

	void* ubPtr = vulkanSkyboxShader->MapUniformBuffer(0, 0);
	struct SkyboxUniformCamera
	{
		glm::mat4 ViewProjectionMatrix;  // u_ViewProjectionMatrix
		glm::mat4 InverseViewProjection; // u_InverseViewProjection
	} skyboxUniformCamera;
		
	skyboxUniformCamera.ViewProjectionMatrix = s_Data.SceneData.SceneCamera.Camera.GetViewProjection();
	skyboxUniformCamera.InverseViewProjection = glm::inverse(s_Data.SceneData.SceneCamera.Camera.GetViewProjection());
	memcpy(ubPtr, &skyboxUniformCamera, sizeof(SkyboxUniformCamera));
	vulkanSkyboxShader->UnmapUniformBuffer(0, 0);

	// Allocated once (previously a new descriptor pool was created every frame and never freed)
	if (!s_SkyboxDescriptorSet.Pool)
	{
		s_SkyboxDescriptorSet = vulkanSkyboxShader->CreateDescriptorSets();
		s_SkyboxDescriptorSetNeedsUpdate = true;
	}
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& descriptorSet = s_SkyboxDescriptorSet;

	// Written only when the environment map changed (the device is idle then, see LoadEnvironmentMap):
	// updating a descriptor set used by frames in flight is not allowed
	if (s_SkyboxDescriptorSetNeedsUpdate)
	{
		std::array<VkWriteDescriptorSet, 2> writeDescriptors;

		writeDescriptors[0] = *vulkanSkyboxShader->GetDescriptorSet("Camera");
		writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
		writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[0].pBufferInfo = &vulkanSkyboxShader->GetUniformBuffer(0, 0).Descriptor;

		// The skybox shows the unfiltered environment map: sharp at Skybox LOD 0, blurred at higher LODs
		// (sampled from its mip chain). The prefiltered map (envFiltered) is for PBR reflections and is always slightly blurred.
		H2M::RefH2M<H2M::VulkanTextureCubeH2M> envUnfilteredCubemap = s_Data.envUnfiltered.As<H2M::VulkanTextureCubeH2M>();
		writeDescriptors[1] = *vulkanSkyboxShader->GetDescriptorSet("u_Texture");
		writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
		writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		s_SkyboxDescriptorSetNeedsUpdate = false;
	}

	H2M::RefH2M<H2M::VulkanVertexBufferH2M> vulkanSkyboxCubeVB = s_Data.VulkanSkyboxCube->m_VertexBuffer.As<H2M::VulkanVertexBufferH2M>();
	VkBuffer skyboxCubeVertexVkBuffer = vulkanSkyboxCubeVB->GetVulkanBuffer();
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &skyboxCubeVertexVkBuffer, offsets);

	H2M::RefH2M<H2M::VulkanIndexBufferH2M> vulkanSkyboxCubeIB = s_Data.VulkanSkyboxCube->m_IndexBuffer.As<H2M::VulkanIndexBufferH2M>();
	VkBuffer skyboxCubeIndexVkBuffer = vulkanSkyboxCubeIB->GetVulkanBuffer();
	vkCmdBindIndexBuffer(commandBuffer, skyboxCubeIndexVkBuffer, 0, VK_INDEX_TYPE_UINT32);

	VkPipeline skyboxPipeline = vulkanSkyboxPipeline->GetVulkanPipeline();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout, 0, (uint32_t)descriptorSet.DescriptorSets.size(), descriptorSet.DescriptorSets.data(), 0, nullptr);

	// push constants
	struct SkyboxUniforms
	{
		float TextureLod;
		float Rotation; // degrees around Y: the same rotation as the PBR environment lookups
	} skyboxUniforms = { s_Data.SceneData.SkyboxLod, s_EnvMapRotation };
	vkCmdPushConstants(commandBuffer, skyboxPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxUniforms), &skyboxUniforms);

	vkCmdDrawIndexed(commandBuffer, s_Data.VulkanSkyboxCube->m_IndexCount, 1, 0, 0, 0);
}

void EnvMapVulkanRenderer::BeginFrame()
{
	// H2M::RendererH2M::Submit([]() {});
	{
		H2M::RefH2M<H2M::VulkanContextH2M> context = H2M::VulkanContextH2M::Get();
		H2M::VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.pNext = nullptr;

		VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
		s_Data.ActiveCommandBuffer = drawCommandBuffer;
		H2M_CORE_ASSERT(s_Data.ActiveCommandBuffer);
		// VK_CHECK_RESULT_H2M(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo)); // commandBuffer must not be in the recording or pending state
	}
}

void EnvMapVulkanRenderer::EndFrame()
{
	// H2M::RendererH2M::Submit([]() {});
	{
		// VK_CHECK_RESULT_H2M(vkEndCommandBuffer(s_Data.ActiveCommandBuffer));
		s_Data.ActiveCommandBuffer = nullptr;
	}
}

// TODO: virtual or static?
void EnvMapVulkanRenderer::BeginRenderPass(const H2M::RefH2M<H2M::RenderPassH2M>& renderPass)
{
	// H2M::RendererH2M::Submit([renderPass]() {});
	{
		// H2M_CORE_ASSERT(s_Data.ActiveCommandBuffer);

		auto fb = renderPass->GetSpecification().TargetFramebuffer;
		H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = fb.As<H2M::VulkanFramebufferH2M>();
		const auto& fbSpec = framebuffer->GetSpecification();

		uint32_t width = framebuffer->GetWidth();
		uint32_t height = framebuffer->GetHeight();

		BeginFrame();

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;

		// TODO: Does out framebuffer has a depth attachment?
		VkClearValue clearValues[2];
		clearValues[0].color = { { fbSpec.ClearColor.r, fbSpec.ClearColor.g, fbSpec.ClearColor.b, fbSpec.ClearColor.a } };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassBeginInfo.clearValueCount = 2; // Color + depth
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

		// vkCmdBeginRenderPass(s_Data.ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE); // You must call vkBeginCommandBuffer() before this call to vkCmdBeginRenderPass()

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.height = (float)height;
		viewport.width = (float)width;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		// vkCmdSetViewport(s_Data.ActiveCommandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		// vkCmdSetScissor(s_Data.ActiveCommandBuffer, 0, 1, &scissor);

		EndFrame();
	}
}

// TODO: virtual or static?
void EnvMapVulkanRenderer::EndRenderPass()
{
	// H2M::RendererH2M::Submit([]() {});
	{
		// vkCmdEndRenderPass(s_Data.ActiveCommandBuffer);
		s_Data.ActiveCommandBuffer = nullptr;
	}
}

void EnvMapVulkanRenderer::SetSceneEnvironment(H2M::RefH2M<H2M::EnvironmentH2M> environment, H2M::RefH2M<H2M::Image2D_H2M> shadow)
{
	// H2M::RendererH2M::Submit([environment]() mutable {});
	{
		auto shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");
		H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = shader.As<H2M::VulkanShaderH2M>();

		std::array<VkWriteDescriptorSet, 3> writeDescriptors;

		H2M::RefH2M<H2M::VulkanTextureCubeH2M> radianceMap = environment->RadianceMap.As<H2M::VulkanTextureCubeH2M>();
		H2M::RefH2M<H2M::VulkanTextureCubeH2M> irradianceMap = environment->IrradianceMap.As<H2M::VulkanTextureCubeH2M>();

		writeDescriptors[0] = *pbrShader->GetDescriptorSet("u_EnvRadianceTex", H2M::VulkanShaderH2M::FrameDescriptorSet);
		writeDescriptors[0].dstSet = *s_Data.FrameDescriptorSet.DescriptorSets.data();
		writeDescriptors[0].descriptorCount = (uint32_t)s_Data.FrameDescriptorSet.DescriptorSets.size();
		auto& radianceMapImageInfo = radianceMap->GetVulkanDescriptorInfo();
		writeDescriptors[0].pImageInfo = &radianceMapImageInfo;

		writeDescriptors[1] = *pbrShader->GetDescriptorSet("u_EnvIrradianceTex", H2M::VulkanShaderH2M::FrameDescriptorSet);
		writeDescriptors[1].dstSet = *s_Data.FrameDescriptorSet.DescriptorSets.data();
		writeDescriptors[1].descriptorCount = (uint32_t)s_Data.FrameDescriptorSet.DescriptorSets.size();
		auto& irradianceMapImageInfo = irradianceMap->GetVulkanDescriptorInfo();
		writeDescriptors[1].pImageInfo = &irradianceMapImageInfo;

		writeDescriptors[2] = *pbrShader->GetDescriptorSet("u_BRDFLUTTexture", H2M::VulkanShaderH2M::FrameDescriptorSet);
		writeDescriptors[2].dstSet = *s_Data.FrameDescriptorSet.DescriptorSets.data();
		writeDescriptors[2].descriptorCount = (uint32_t)s_Data.FrameDescriptorSet.DescriptorSets.size();
		auto& brdfLutImageInfo = s_Data.BRDFLut.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
		writeDescriptors[2].pImageInfo = &brdfLutImageInfo;

		auto vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
		vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
	}
}

void EnvMapVulkanRenderer::GeometryPass()
{
	// H2M::RendererH2M::Submit([=]() {});
	{
		// H2M::RefH2M<H2M::VulkanContextH2M> context = H2M::VulkanContextH2M::Get();
		H2M::VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.pNext = nullptr;

		VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
		VK_CHECK_RESULT_H2M(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));

		// The shadow maps first (the sun's, then the spot and point lights'): the geometry pass below samples them
		glm::vec3 casterBoundsMin, casterBoundsMax;
		ComputeShadowCasterBounds(casterBoundsMin, casterBoundsMax);
		RecordShadowPasses(drawCommandBuffer, casterBoundsMin, casterBoundsMax);
		RecordLocalShadowPasses(drawCommandBuffer);
		if (s_ShadowMapViewerRequest.Active)
		{
			const EnvMapVulkanShadowMap& map = s_ShadowMapViewerRequest.Cube ? s_PointShadowMaps : s_SpotShadowMaps;
			s_ShadowMapViewer.Record(drawCommandBuffer, map, s_ShadowMapViewerRequest.Cube, s_ShadowMapViewerRequest.BaseLayer,
				s_ShadowMapViewerRequest.Near, s_ShadowMapViewerRequest.Far);
			s_ShadowMapViewerRequest.Active = false; // requested again by the panel while it shows the viewer
		}

		// The water's caustics: the meshes under the water (in the reflection and the scene pass) are lit through them
		if (s_WaterSettings.Enabled)
		{
			s_Water.RecordCaustics(drawCommandBuffer);
		}

		// The water's planar reflection: the meshes seen by the camera mirrored in the water plane, into the water's
		// reflection image (with the usual pipelines; its own per-frame set holds the mirrored camera)
		if (s_WaterSettings.Enabled && s_Water.IsReflectionActive())
		{
			VkDescriptorSet reflectionFrameSet = s_Water.BeginReflectionPass(drawCommandBuffer);
			VkPipelineLayout reflectionLayout = s_MeshPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout();
			vkCmdBindDescriptorSets(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, reflectionLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
				&reflectionFrameSet, 0, nullptr);
			for (const SubmittedModel& submitted : s_SubmittedModels)
			{
				RenderModelVulkan(submitted.Model, submitted.Transform, submitted.Materials, drawCommandBuffer);
			}
			s_Water.EndReflectionPass(drawCommandBuffer);
		}

		H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = s_Framebuffer.As<H2M::VulkanFramebufferH2M>();

		uint32_t width = framebuffer->GetWidth();
		uint32_t height = framebuffer->GetHeight();

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;

		VkClearValue clearValues[2];
		clearValues[0].color = { {0.1f, 0.1f,0.1f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassBeginInfo.clearValueCount = 2; // Color + depth
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

		vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.height = (float)height;
		viewport.width = (float)width;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(drawCommandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(drawCommandBuffer, 0, 1, &scissor);

		EnvMapVulkanRenderer::RenderSkybox(drawCommandBuffer); // in progress

		// Set 0 (per frame: camera, scene data, environment maps) is bound once, for all meshes
		VkPipelineLayout meshPipelineLayout = s_MeshPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout();
		vkCmdBindDescriptorSets(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
			s_Data.FrameDescriptorSet.DescriptorSets.data(), 0, nullptr);

		for (const SubmittedModel& submitted : s_SubmittedModels)
		{
			RenderModelVulkan(submitted.Model, submitted.Transform, submitted.Materials, drawCommandBuffer);
		}

		s_SubmittedModels.clear();

		// The water: it looks into the opaque scene drawn so far. The render pass ends, its color and depth are copied
		// for the water to read, and the framebuffer's continue render pass (which loads the attachments instead of
		// clearing them) takes over for the water and the rest.
		if (s_WaterSettings.Enabled)
		{
			vkCmdEndRenderPass(drawCommandBuffer);
			s_Water.CopyScene(drawCommandBuffer, s_Framebuffer);

			VkRenderPassBeginInfo continueBeginInfo = renderPassBeginInfo;
			continueBeginInfo.renderPass = framebuffer->GetContinueRenderPass();
			continueBeginInfo.framebuffer = framebuffer->GetContinueVulkanFramebuffer();
			continueBeginInfo.clearValueCount = 0;
			continueBeginInfo.pClearValues = nullptr;
			vkCmdBeginRenderPass(drawCommandBuffer, &continueBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(drawCommandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(drawCommandBuffer, 0, 1, &scissor);

			s_Water.RecordVolume(drawCommandBuffer, s_Data.FrameDescriptorSet.DescriptorSets[0]); // underwater fog, the water seen from the side
			// The water's wireframe: its own toggle (Water panel), or the editor's (Environment panel, Wireframe: All, or
			// Selected while the water is selected), with the editor's line width
			EnvMapVulkanWaterSettings waterDrawSettings = s_WaterSettings;
			waterDrawSettings.Wireframe = s_WaterSettings.Wireframe || s_OverlaySettings.Wireframe == OverlayScopeAll ||
				(s_OverlaySettings.Wireframe == OverlayScopeSelected && s_WaterSelected);
			const float maxLineWidth = H2M::VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetProperties().limits.lineWidthRange[1];
			s_Water.Record(drawCommandBuffer, s_Data.FrameDescriptorSet.DescriptorSets[0], waterDrawSettings,
				glm::clamp(s_OverlaySettings.LineWidth, 1.0f, maxLineWidth));
		}

		// Transparent, so after the opaque meshes and the water
		if (s_DisplayGrid)
		{
			RenderGrid(drawCommandBuffer);
		}

		vkCmdEndRenderPass(drawCommandBuffer);

		RecordBloomPasses(drawCommandBuffer);
		RecordEditorOverlayPasses(drawCommandBuffer);
		ViewportCompositePass(drawCommandBuffer);
	}
}

// Editor grid on the ground plane (y = 0), 32 x 32 units, like RenderHazelGrid in SceneHazelEnvMap
void EnvMapVulkanRenderer::RenderGrid(VkCommandBuffer commandBuffer)
{
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_GridPipeline.As<H2M::VulkanPipelineH2M>();
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = s_GridPipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

	// Camera uniform buffer (set 0, binding 0)
	struct GridCamera
	{
		glm::mat4 ViewProjection;
		glm::mat4 InverseViewProjection;
	} camera;
	camera.ViewProjection = s_Data.SceneData.SceneCamera.Camera.GetViewProjection();
	camera.InverseViewProjection = glm::inverse(camera.ViewProjection);
	void* ubPtr = shader->MapUniformBuffer(0, 0);
	memcpy(ubPtr, &camera, sizeof(GridCamera));
	shader->UnmapUniformBuffer(0, 0);

	// The descriptor set only references the uniform buffer, so it is written once
	if (!s_GridDescriptorSet.Pool)
	{
		s_GridDescriptorSet = shader->CreateDescriptorSets();

		VkWriteDescriptorSet write = *shader->GetDescriptorSet("Camera");
		write.dstSet = s_GridDescriptorSet.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pBufferInfo = &shader->GetUniformBuffer(0, 0).Descriptor;
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
	}

	VkBuffer vertexBuffer = s_Data.QuadVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, s_Data.QuadIndexBuffer.As<H2M::VulkanIndexBufferH2M>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, s_GridDescriptorSet.DescriptorSets.data(), 0, nullptr);

	// The quad spans -1..1 in XY at z = 0.1: lay it on the ground plane (rotated into XZ, lifted back to y = 0) and scale it to 32 x 32
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.1f, 0.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(16.0f, 1.0f, 16.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);

	struct GridSettings
	{
		float Scale;
		float Size;
	} settings = { s_GridScale, s_GridSize };
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(GridSettings), &settings);

	vkCmdDrawIndexed(commandBuffer, s_Data.QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
}

static uint32_t BloomLevelSize(uint32_t viewportSize, uint32_t level)
{
	return std::max<uint32_t>(viewportSize >> (level + 1), 1);
}

static void CreateBloomResources()
{
	for (uint32_t i = 0; i < s_BloomLevelCount; i++)
	{
		H2M::FramebufferSpecificationH2M framebufferSpec;
		framebufferSpec.Attachments = { H2M::ImageFormatH2M::RGBA16F, H2M::ImageFormatH2M::Depth }; // VulkanFramebufferH2M always has a depth attachment
		framebufferSpec.Samples = 1;
		framebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		framebufferSpec.Width = BloomLevelSize(s_ViewportWidth, i);
		framebufferSpec.Height = BloomLevelSize(s_ViewportHeight, i);

		framebufferSpec.DebugName = "BloomDown" + std::to_string(i);
		s_BloomDownFramebuffers[i] = H2M::FramebufferH2M::Create(framebufferSpec);

		if (i < s_BloomLevelCount - 1)
		{
			framebufferSpec.DebugName = "BloomUp" + std::to_string(i);
			s_BloomUpFramebuffers[i] = H2M::FramebufferH2M::Create(framebufferSpec);
		}
	}

	// All levels have the same formats, so their render passes are compatible and one pipeline draws into any of them
	H2M::PipelineSpecificationH2M pipelineSpecification;
	pipelineSpecification.Layout = {
		{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
		{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
	};
	pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("BloomPass");
	H2M::RenderPassSpecificationH2M renderPassSpec;
	renderPassSpec.TargetFramebuffer = s_BloomDownFramebuffers[0];
	pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
	pipelineSpecification.DebugName = "Bloom";
	s_BloomPipeline = H2M::PipelineH2M::Create(pipelineSpecification);

	uint32_t passCount = 2 * s_BloomLevelCount - 1;
	s_BloomDescriptorSets = pipelineSpecification.Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets(0, passCount);
	s_ViewportCompositeDescriptorSet = s_ViewportCompositePipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets();

	try
	{
		s_BloomDirtTexture = H2M::Texture2D_H2M::Create("Textures/dirt.png", true);
	}
	catch (...)
	{
		Log::GetLogger()->warn("Lens dirt texture 'Textures/dirt.png' could not be loaded.");
	}
	if (!s_BloomDirtTexture)
	{
		s_BloomDirtTexture = H2M::RendererH2M::GetWhiteTexture();
	}

	WriteBloomDescriptorSets();
}

// Called after the viewport framebuffers were resized
static void ResizeBloomResources()
{
	for (uint32_t i = 0; i < s_BloomLevelCount; i++)
	{
		uint32_t width = BloomLevelSize(s_ViewportWidth, i);
		uint32_t height = BloomLevelSize(s_ViewportHeight, i);
		s_BloomDownFramebuffers[i]->Resize(width, height);
		if (i < s_BloomLevelCount - 1)
		{
			s_BloomUpFramebuffers[i]->Resize(width, height);
		}
	}

	// The descriptor sets may be used by frames in flight
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
	WriteBloomDescriptorSets();
	s_BloomChainRendered = false; // new images: no content (and no valid layout) until the chain is rendered
}

// Which image every bloom pass reads, and the composite's scene/bloom/dirt/overlay/selection mask images
static void WriteBloomDescriptorSets()
{
	auto imageInfo = [](const H2M::RefH2M<H2M::FramebufferH2M>& framebuffer) -> const VkDescriptorImageInfo*
	{
		return &framebuffer.As<H2M::VulkanFramebufferH2M>()->GetVulkanDescriptorInfo();
	};

	std::vector<VkWriteDescriptorSet> writes;

	H2M::RefH2M<H2M::VulkanShaderH2M> bloomShader = s_BloomPipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	const VkWriteDescriptorSet* textureBinding = bloomShader->GetDescriptorSet("u_Texture");
	const VkWriteDescriptorSet* bloomTextureBinding = bloomShader->GetDescriptorSet("u_BloomTexture");

	auto addPass = [&](uint32_t pass, const VkDescriptorImageInfo* texture, const VkDescriptorImageInfo* bloomTexture)
	{
		VkWriteDescriptorSet write = *textureBinding;
		write.dstSet = s_BloomDescriptorSets.DescriptorSets[pass];
		write.descriptorCount = 1;
		write.pImageInfo = texture;
		writes.push_back(write);

		write = *bloomTextureBinding;
		write.dstSet = s_BloomDescriptorSets.DescriptorSets[pass];
		write.descriptorCount = 1;
		write.pImageInfo = bloomTexture; // only read by the upsample passes, but every binding needs a valid image
		writes.push_back(write);
	};

	uint32_t pass = 0;
	addPass(pass++, imageInfo(s_Framebuffer), imageInfo(s_Framebuffer)); // prefilter: scene -> down 0
	for (uint32_t i = 1; i < s_BloomLevelCount; i++)
	{
		addPass(pass++, imageInfo(s_BloomDownFramebuffers[i - 1]), imageInfo(s_BloomDownFramebuffers[i - 1])); // down i-1 -> down i
	}
	for (int i = (int)s_BloomLevelCount - 2; i >= 0; i--)
	{
		// up i = down i + (the smallest level, or up i+1) upsampled
		const VkDescriptorImageInfo* smaller = i == (int)s_BloomLevelCount - 2 ? imageInfo(s_BloomDownFramebuffers[i + 1]) : imageInfo(s_BloomUpFramebuffers[i + 1]);
		addPass(pass++, imageInfo(s_BloomDownFramebuffers[i]), smaller);
	}

	H2M::RefH2M<H2M::VulkanShaderH2M> compositeShader = s_ViewportCompositePipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	const VkDescriptorImageInfo* compositeImages[5] = {
		imageInfo(s_Framebuffer),
		imageInfo(s_BloomUpFramebuffers[0]),
		&s_BloomDirtTexture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo(),
		imageInfo(s_OverlayFramebuffer),
		imageInfo(s_SelectionMaskFramebuffer),
	};
	const char* compositeBindings[5] = { "u_Texture", "u_BloomTexture", "u_BloomDirtTexture", "u_OverlayTexture", "u_SelectionMask" };
	for (uint32_t i = 0; i < 5; i++)
	{
		VkWriteDescriptorSet write = *compositeShader->GetDescriptorSet(compositeBindings[i]);
		write.dstSet = s_ViewportCompositeDescriptorSet.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = compositeImages[i];
		writes.push_back(write);
	}

	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

// Prefilter, downsample and upsample passes (BloomPass.glsl), each a fullscreen quad drawn into one level of the chain.
// Every render pass ends with its image in SHADER_READ_ONLY layout, ready for the next pass to sample.
static void RecordBloomPasses(VkCommandBuffer commandBuffer)
{
	if (!s_BloomSettings.Enabled && s_BloomChainRendered)
	{
		return; // the composite multiplies the (old) bloom by 0
	}

	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_BloomPipeline.As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
	VkBuffer vertexBuffer = s_Data.QuadVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkBuffer indexBuffer = s_Data.QuadIndexBuffer.As<H2M::VulkanIndexBufferH2M>()->GetVulkanBuffer();

	struct BloomUniforms
	{
		glm::vec4 Params;
		float UpsampleScale;
		float Exposure;
		int Mode;
	} uniforms;
	float knee = std::max(s_BloomSettings.Knee, 1.0e-4f);
	float threshold = s_BloomSettings.Threshold;
	uniforms.Params = { threshold, threshold - knee, knee * 2.0f, 0.25f / knee };
	uniforms.UpsampleScale = s_BloomSettings.UpsampleScale;
	uniforms.Exposure = s_Exposure * (s_AutoExposureEnabled ? s_EnvMapAutoExposure : 1.0f);

	enum { ModePrefilter = 0, ModeDownsample = 1, ModeUpsample = 2 };

	auto drawPass = [&](const H2M::RefH2M<H2M::FramebufferH2M>& target, uint32_t pass, int mode)
	{
		H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = target.As<H2M::VulkanFramebufferH2M>();
		uint32_t width = framebuffer->GetWidth();
		uint32_t height = framebuffer->GetHeight();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
		renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2; // Color + depth
		renderPassBeginInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		VkRect2D scissor = { { 0, 0 }, { width, height } };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());

		uniforms.Mode = mode;
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomUniforms), &uniforms);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &s_BloomDescriptorSets.DescriptorSets[pass], 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, s_Data.QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
	};

	// Same pass order as in WriteBloomDescriptorSets
	uint32_t pass = 0;
	drawPass(s_BloomDownFramebuffers[0], pass++, ModePrefilter);
	for (uint32_t i = 1; i < s_BloomLevelCount; i++)
	{
		drawPass(s_BloomDownFramebuffers[i], pass++, ModeDownsample);
	}
	for (int i = (int)s_BloomLevelCount - 2; i >= 0; i--)
	{
		drawPass(s_BloomUpFramebuffers[i], pass++, ModeUpsample);
	}

	s_BloomChainRendered = true;
}

// Draws s_Framebuffer (linear HDR) into s_ViewportCompositeFramebuffer with exposure, ACES tonemapping and gamma
// (Resources/Shaders/SceneComposite.glsl). The Viewport panel shows the result.
void EnvMapVulkanRenderer::ViewportCompositePass(VkCommandBuffer commandBuffer)
{
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = s_ViewportCompositeFramebuffer.As<H2M::VulkanFramebufferH2M>();
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_ViewportCompositePipeline.As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

	uint32_t width = framebuffer->GetWidth();
	uint32_t height = framebuffer->GetHeight();

	VkClearValue clearValues[2];
	clearValues[0].color = { {0.1f, 0.1f, 0.1f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
	renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2; // Color + depth
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	VkRect2D scissor = { { 0, 0 }, { width, height } };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffer = s_Data.QuadVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, s_Data.QuadIndexBuffer.As<H2M::VulkanIndexBufferH2M>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());

	struct CompositeUniforms
	{
		float Exposure;
		float BloomIntensity;
		float BloomDirtIntensity;
		float OutlineWidth;
		glm::vec4 OutlineColor;
		float HuePreservation;
		float RawScene;
	} uniforms;
	uniforms.Exposure = s_Exposure * (s_AutoExposureEnabled ? s_EnvMapAutoExposure : 1.0f);
	uniforms.BloomIntensity = s_BloomSettings.Enabled ? s_BloomSettings.Intensity : 0.0f;
	uniforms.BloomDirtIntensity = (s_BloomSettings.Enabled && s_BloomSettings.DirtEnabled) ? s_BloomSettings.DirtIntensity : 0.0f;
	uniforms.OutlineWidth = s_OverlaySettings.Outline ? s_OverlaySettings.OutlineWidth : 0.0f;
	uniforms.OutlineColor = s_OverlaySettings.OutlineColor;
	uniforms.HuePreservation = s_TonemapHuePreservation;
	uniforms.RawScene = s_ShowShadowsOnly && s_SelectedLightKind != LightKind::None ? 1.0f : 0.0f;
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositeUniforms), &uniforms);

	// Scene, bloom, lens dirt, overlay and selection mask images (rewritten when the framebuffers are resized, see WriteBloomDescriptorSets)
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, s_ViewportCompositeDescriptorSet.DescriptorSets.data(), 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, s_Data.QuadIndexBuffer->GetCount(), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void EnvMapVulkanRenderer::CompositePass()
{
	// H2M::RendererH2M::Submit([=]() {});
	{
		// H2M::RefH2M<H2M::VulkanContextH2M> context = H2M::VulkanContextH2M::Get();
		H2M::VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();
		VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.pNext = nullptr;

		H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = s_CompositeFramebuffer.As<H2M::VulkanFramebufferH2M>();

		// uint32_t width = framebuffer->GetWidth();   // framebuffer resize still not enabled
		// uint32_t height = framebuffer->GetHeight(); // framebuffer resize still not enabled

		uint32_t width = swapChain.GetWidth();
		uint32_t height = swapChain.GetHeight();

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		// renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
		renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassBeginInfo.clearValueCount = 2; // Color + depth
		renderPassBeginInfo.pClearValues = clearValues;
		// renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();
		renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

		vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		// vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkCommandBuffer commandBuffer = s_CompositeCommandBuffer;

		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = swapChain.GetRenderPass();
		inheritanceInfo.framebuffer = swapChain.GetCurrentFramebuffer();

		std::vector<VkCommandBuffer> commandBuffers;

		// VkCommandBufferBeginInfo cmdBufInfo = {};
		// cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

		VK_CHECK_RESULT_H2M(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = (float)height;
		viewport.height = -(float)height;
		viewport.width = (float)width;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// Copy 3D scene here!
		H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_CompositePipeline.As<H2M::VulkanPipelineH2M>();

		VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				
		/**** BEGIN CompositeRenderPass(inheritanceInfo) ****/

		auto vulkanMeshVB = s_Data.QuadVertexBuffer.As<H2M::VulkanVertexBufferH2M>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = s_Data.QuadIndexBuffer.As<H2M::VulkanIndexBufferH2M>();
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		float exposure = s_Exposure * (s_AutoExposureEnabled ? s_EnvMapAutoExposure : 1.0f);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &exposure);

		// Bind descriptor sets describing shader binding points
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)s_Data.QuadDescriptorSet.DescriptorSets.size(), s_Data.QuadDescriptorSet.DescriptorSets.data(), 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, s_Data.QuadIndexBuffer->GetCount(), 1, 0, 0, 0);

		VK_CHECK_RESULT_H2M(vkEndCommandBuffer(commandBuffer));

		/**** END CompositeRenderPass(inheritanceInfo) ****/

		commandBuffers.push_back(s_CompositeCommandBuffer);

		OnImGuiRender(inheritanceInfo, commandBuffers);

		vkCmdExecuteCommands(drawCommandBuffer, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkCmdEndRenderPass(drawCommandBuffer);

		VK_CHECK_RESULT_H2M(vkEndCommandBuffer(drawCommandBuffer));
	}
}

void EnvMapVulkanRenderer::OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers)
{
	// H2M::RefH2M<H2M::VulkanContextH2M> context = H2M::VulkanContextH2M::Get();
	H2M::VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();

	uint32_t width = swapChain.GetWidth();
	uint32_t height = swapChain.GetHeight();

	// ImGui Pass
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

		VK_CHECK_RESULT_H2M(vkBeginCommandBuffer(s_ImGuiCommandBuffer, &cmdBufInfo));

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = (float)height;
		viewport.height = -(float)height;
		viewport.width = (float)width;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(s_ImGuiCommandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(s_ImGuiCommandBuffer, 0, 1, &scissor);

		// ImGui Dockspace
		bool open = true;
		bool* p_open = &open;

		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", p_open, window_flags);
		{
			if (opt_fullscreen)
			{
				ImGui::PopStyleVar(2);
			}

			// DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}
			else
			{
				// ShowDockingDisabledMessage();
			}

			/**** BEGIN Viewport ****/

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("Viewport");

			// Tell ImGuiWrapper the scene is shown in this ImGui window, so camera input (CameraController)
			// works while the viewport is hovered/focused, instead of being blocked as "mouse over an ImGui window"
			ImGuiWrapper::SetViewportEnabled(true);
			ImGuiWrapper::SetViewportHovered(ImGui::IsWindowHovered());
			ImGuiWrapper::SetViewportFocused(ImGui::IsWindowFocused());

			auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
			auto viewportSize = ImGui::GetContentRegionAvail();
			if (!s_TextureID || s_ViewportTextureNeedsUpdate)
			{
				RegisterViewportTextureWithImGui();
			}
			ImGui::Image(s_TextureID, viewportSize, { 0, 1 }, { 1, 0 });
			s_ViewportImageMin = ImGui::GetItemRectMin();
			s_ViewportImageSize = ImGui::GetItemRectSize();
			bool viewportImageHovered = ImGui::IsItemHovered();

			// Drop onto the scene:
			// - a material from the Material Library: the mesh under the cursor gets it and becomes the selection
			//   (so the Material Editor shows the material)
			// - a model file from the Content Browser: placed standing on the ground under the cursor (see LoadModel)
			// - an .hdr file from the Content Browser: loaded as the environment map
			if (ImGui::BeginDragDropTarget())
			{
				glm::vec2 ndc = GetViewportMouseNdc();

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_MaterialPayload))
				{
					uint32_t index = *(const uint32_t*)payload->Data;
					const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();

					int hitModel, hitMesh;
					RaycastMesh(ndc.x, ndc.y, hitModel, hitMesh);

					if (index < materials.size() && hitModel >= 0 && hitMesh >= 0 && hitMesh < (int)s_LoadedModels[hitModel].MeshMaterials.size())
					{
						s_LoadedModels[hitModel].MeshMaterials[hitMesh] = materials[index];
						s_SelectedModelIndex = hitModel;
						s_SelectedMeshIndex = hitMesh;
					}
				}

				ImVec2 imageMax(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y);
				auto isSceneFile = [](const std::string& path) { return IsModelFile(path) || IsEnvironmentMapFile(path); };
				std::string droppedPath;
				if (AcceptFileDrop(s_ViewportImageMin, imageMax, isSceneFile, "a model file or an .hdr environment map", droppedPath))
				{
					// loaded at the start of the next frame (see Draw)
					if (IsEnvironmentMapFile(droppedPath))
					{
						s_PendingEnvMapFilename = droppedPath;
					}
					else
					{
						s_PendingModelFilename = droppedPath;
						s_PendingModelGroundPosition = GetDropGroundPosition(ndc.x, ndc.y);
					}
				}
				ImGui::EndDragDropTarget();
			}

			// Compare whole pixels: the panel size can be fractional (DPI scaling, docking), and comparing the float size with
			// the stored integer size requested a framebuffer resize every frame. A collapsed/hidden panel has no area: keep the size.
			uint32_t viewportWidth = (uint32_t)glm::max(viewportSize.x, 0.0f);
			uint32_t viewportHeight = (uint32_t)glm::max(viewportSize.y, 0.0f);
			if (viewportWidth > 0 && viewportHeight > 0 && (s_ViewportWidth != viewportWidth || s_ViewportHeight != viewportHeight))
			{
				s_ViewportWidth = viewportWidth;
				s_ViewportHeight = viewportHeight;
				s_ViewportFBNeedsResize = true;
			}

			SyncLightSelection();
			DrawLightGizmos();
			DrawWaterOutline();

			Window* mainWindow = Application::Get()->GetWindow();
			UpdateImGuizmo(mainWindow);

			// Mouse picking: left click on the scene (not on the gizmo, not with Alt) selects the light icon or else the
			// model/mesh under the cursor
			if (viewportImageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing() &&
				!Input::IsKeyPressed(KeyH2M::LeftAlt) && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
			{
				LightKind lightKind;
				int lightIndex;
				if (PickLightIcon(lightKind, lightIndex))
				{
					SelectLight(lightKind, lightIndex);
				}
				else
				{
					glm::vec2 ndc = GetViewportMouseNdc();
					glm::vec3 meshHit;
					RaycastMesh(ndc.x, ndc.y, s_SelectedModelIndex, s_SelectedMeshIndex, &meshHit);
					SelectLight(LightKind::None);
					s_WaterSelected = false;
					// The water, when it is in front of the model under the cursor (or there is none)
					glm::vec3 rayOrigin, rayDirection;
					GetCameraRay(ndc.x, ndc.y, rayOrigin, rayDirection);
					float waterT;
					if (RaycastWater(s_WaterSettings, rayOrigin, rayDirection, waterT) &&
						(s_SelectedModelIndex < 0 || waterT < glm::length(meshHit - rayOrigin)))
					{
						SelectWater();
					}
					// Shift + click: the whole model (the gizmo moves the model, not the part under the mouse)
					if (Input::IsKeyPressed(KeyH2M::LeftShift) || Input::IsKeyPressed(KeyH2M::RightShift))
					{
						s_SelectedMeshIndex = -1;
					}
				}
			}

			ImGui::End();
			ImGui::PopStyleVar();

			/**** END Viewport ****/

			/**** BEGIN ImGui panels ****/

			// ImGui::Begin("Scene Hierarchy");
			// ImGui::End();

			bool showSceneHierarchyPanel = true;
			// H2M::VulkanTestLayer::s_SceneHierarchyPanel->OnImGuiRender(&showSceneHierarchyPanel);

			// Content Browser (shared with SceneHazelEnvMap): drag .hdr files onto the Environment panel and
			// model files onto the Models and Meshes panel's "Load Model" button
			static H2M::ContentBrowserPanelH2M* s_ContentBrowserPanel = nullptr;
			static bool s_ShowContentBrowserPanel = true;
			if (!s_ContentBrowserPanel)
			{
				s_ContentBrowserPanel = new H2M::ContentBrowserPanelH2M();
			}
			s_ContentBrowserPanel->OnImGuiRender(&s_ShowContentBrowserPanel);

			bool showMaterialEditorPanel = true;
			// H2M::VulkanTestLayer::s_MaterialEditorPanel->OnImGuiRender(&showMaterialEditorPanel);

			/**** END ImGui panels ****/

			/////////////////////////////////////////////////////////////////////////////////////
			//// ENVIRONMENT
			/////////////////////////////////////////////////////////////////////////////////////

			/**** BEGIN Environment ****/
			ImGui::Begin("Environment");
			{
				if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					{
						ImGui::Columns(2);

						// Currently loaded environment map (file name only; the full path is in the tooltip)
						std::string envMapName = std::filesystem::path(s_EnvMapFilename).filename().string();
						char envMapNameBuffer[256] = {};
						strncpy(envMapNameBuffer, envMapName.c_str(), sizeof(envMapNameBuffer) - 1);
						ImGui::InputText("##envmapfilepath", envMapNameBuffer, sizeof(envMapNameBuffer), ImGuiInputTextFlags_ReadOnly);
						if (ImGui::IsItemHovered() && !s_EnvMapFilename.empty())
						{
							ImGui::SetTooltip("%s", s_EnvMapFilename.c_str());
						}

						// Drop an .hdr file from the Content Browser here to load it
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
							{
								std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
								Log::GetLogger()->debug("END DRAG & DROP FILE '{0}'", itemPath);
								if (std::filesystem::path(itemPath).extension() == ".hdr")
								{
									s_PendingEnvMapFilename = itemPath;
								}
								else
								{
									Log::GetLogger()->warn("Only .hdr files can be used as environment maps ('{0}')", itemPath);
								}
							}
							ImGui::EndDragDropTarget();
						}

						ImGui::NextColumn();

						if (ImGui::Button("Load Environment Map"))
						{
							std::string filepath = Util::ToUtf8(Application::Get()->OpenFile(L"*.hdr"));
							if (!filepath.empty())
							{
								s_PendingEnvMapFilename = filepath; // loaded at the start of the next frame (see Draw)
							}
						}

						ImGui::NextColumn();

						ImGui::AlignTextToFramePadding();

						// Skybox blur: 0 = sharp, up to the environment map's last mip level (a single average color)
						float maxSkyboxLod = s_Data.envUnfiltered ? (float)(s_Data.envUnfiltered->GetMipLevelCount() - 1) : 10.0f;
						if (ImGuiWrapper::Property("Skybox LOD", s_Data.SceneData.SkyboxLod, 0.01f, 0.0f, maxSkyboxLod, PropertyFlag::DragProperty))
						{
							// SetSkyboxLOD(skyboxLOD);
						}

						ImGuiWrapper::Property("Exposure", s_Exposure, 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty);
						ImGuiWrapper::Property("Auto Exposure", s_AutoExposureEnabled);
						if (ImGuiWrapper::Property("Extract Sun", s_ExtractSunFromEnvironment))
						{
							s_PendingEnvMapFilename = s_EnvMapFilename; // reloaded with or without its sun
						}
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip(s_ExtractedSun.Found
								? "This map's sun is moved to the directional sun (intensity %.2f), so it casts shadows with all its light\nand isn't counted twice. Off: the map keeps its sun (the directional sun adds to it)."
								: "A real sun in a map (far brighter than the rest) is moved to the directional sun,\nso it casts shadows with all its light. This map has no sun.", s_ExtractedSun.Intensity);
						}
						ImGuiWrapper::Property("Preserve Hue", s_TonemapHuePreservation, 0.01f, 0.0f, 1.0f, PropertyFlag::DragProperty);
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("How bright colors are tonemapped:\n0 = per channel (a bright colored light turns white at its center, as on film)\n1 = hue-preserving (keeps the light's color all the way to the center)");
						}
						// No drag limits (min = max = 0): the value wraps around, so it can be dragged endlessly in both directions
						if (ImGuiWrapper::Property("Env Map Rotation", s_EnvMapRotation, 1.0f, 0.0f, 0.0f, PropertyFlag::DragProperty))
						{
							s_EnvMapRotation = std::fmod(s_EnvMapRotation, 360.0f);
							if (s_EnvMapRotation < 0.0f)
							{
								s_EnvMapRotation += 360.0f;
							}
						}
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("Turns the environment around the vertical axis (yaw, degrees):\nskybox, reflections and environment lighting together");
						}
						ImGuiWrapper::Property("Display Grid", s_DisplayGrid);
						ImGuiWrapper::Property("Grid Scale", s_GridScale, 0.1f, 1.0f, 256.0f, PropertyFlag::DragProperty);
						ImGuiWrapper::Property("Grid Line Width", s_GridSize, 0.001f, 0.001f, 0.5f, PropertyFlag::DragProperty);

						ImGui::Columns(1);
					}
				}

				// Like "Display Outline / Wireframe / Bounding Boxes" in SceneHazelEnvMap
				if (ImGui::CollapsingHeader("Selection and Overlays", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					EditorOverlaySettings& overlay = s_OverlaySettings;
					ImGui::PushID("Overlays");

					ImGui::Checkbox("Selection Outline", &overlay.Outline);
					ImGui::BeginDisabled(!overlay.Outline);
					ImGui::DragFloat("Outline Width", &overlay.OutlineWidth, 0.05f, 1.0f, 10.0f, "%.1f px", ImGuiSliderFlags_AlwaysClamp);
					ImGui::ColorEdit4("Outline Color", &overlay.OutlineColor.x, ImGuiColorEditFlags_NoInputs);
					ImGui::EndDisabled();

					ImGui::Separator();
					ImGui::Combo("Wireframe", &overlay.Wireframe, s_OverlayScopeNames, IM_ARRAYSIZE(s_OverlayScopeNames));
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Selected: the selected mesh, or the whole model when no mesh is selected (the water's grid\n"
							"when the water is selected)\nAll: every loaded model and the water");
					}
					ImGui::ColorEdit4("Wireframe Color", &overlay.WireframeColor.x, ImGuiColorEditFlags_NoInputs);

					ImGui::Separator();
					ImGui::Combo("Bounding Boxes", &overlay.BoundingBoxes, s_OverlayScopeNames, IM_ARRAYSIZE(s_OverlayScopeNames));
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Selected: the selected mesh's box, or all boxes of the model when no mesh is selected\nAll: the boxes of every loaded model");
					}
					ImGui::ColorEdit4("Selected Box", &overlay.SelectedBoundingBoxColor.x, ImGuiColorEditFlags_NoInputs);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Box of the selected mesh\n(all boxes of the selected model when no mesh is selected)");
					}
					ImGui::SameLine();
					ImGui::ColorEdit4("Unselected Boxes", &overlay.BoundingBoxColor.x, ImGuiColorEditFlags_NoInputs);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("All other boxes: other models, and the other meshes of the selected model.\n"
							"Only drawn with Bounding Boxes = All.");
					}

					ImGui::Separator();
					ImGui::Combo("Vertex Vectors", &overlay.Vectors, s_OverlayScopeNames, IM_ARRAYSIZE(s_OverlayScopeNames));
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("A line per vertex along its normal, tangent and/or bitangent\n"
							"Selected: the selected mesh, or the whole model when no mesh is selected\nAll: every loaded model");
					}
					ImGui::BeginDisabled(overlay.Vectors == OverlayScopeOff);
					ImGui::Checkbox("Normals", &overlay.ShowNormals);
					ImGui::SameLine();
					ImGui::Checkbox("Tangents", &overlay.ShowTangents);
					ImGui::SameLine();
					ImGui::Checkbox("Bitangents", &overlay.ShowBitangents);
					ImGui::DragFloat("Vector Length", &overlay.VectorLength, 0.05f, 0.1f, 50.0f, "%.1f %% of model size", ImGuiSliderFlags_AlwaysClamp);
					static const char* s_VectorColorModes[] = { "By Vector", "By Direction" };
					ImGui::Combo("Vector Colors", &overlay.VectorColorMode, s_VectorColorModes, IM_ARRAYSIZE(s_VectorColorModes));
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("By Vector: tangent red, bitangent green, normal blue (tangent space x, y, z)\n"
							"By Direction: the world direction as a color (x red, y green, z blue)\n"
							"Lines are darker at the vertex and brighter at the tip");
					}
					ImGui::EndDisabled();

					ImGui::Separator();
					ImGui::DragFloat("Line Width", &overlay.LineWidth, 0.05f, 1.0f, 10.0f, "%.1f px", ImGuiSliderFlags_AlwaysClamp);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Wireframe, bounding box and vertex vector lines");
					}

					ImGui::PopID();
				}

				// Same settings as "Bloom Settings" in SceneHazelEnvMap's Scene Renderer panel
				if (ImGui::CollapsingHeader("Bloom Settings", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Columns(2);
					ImGui::AlignTextToFramePadding();
					ImGuiWrapper::Property("Bloom Enabled", s_BloomSettings.Enabled);
					ImGuiWrapper::Property("Threshold", s_BloomSettings.Threshold, 0.01f, 0.0f, 20.0f, PropertyFlag::DragProperty);
					ImGuiWrapper::Property("Knee", s_BloomSettings.Knee, 0.01f, 0.0f, 10.0f, PropertyFlag::DragProperty);
					ImGuiWrapper::Property("Upsample Scale", s_BloomSettings.UpsampleScale, 0.01f, 0.0f, 10.0f, PropertyFlag::DragProperty);
					ImGuiWrapper::Property("Intensity", s_BloomSettings.Intensity, 0.05f, 0.0f, 20.0f, PropertyFlag::DragProperty);
					ImGuiWrapper::Property("Lens Dirt", s_BloomSettings.DirtEnabled);
					ImGui::BeginDisabled(!s_BloomSettings.DirtEnabled);
					ImGuiWrapper::Property("Dirt Intensity", s_BloomSettings.DirtIntensity, 0.05f, 0.0f, 20.0f, PropertyFlag::DragProperty);
					ImGui::EndDisabled();
					ImGui::Columns(1);

					// Lens dirt texture: click the thumbnail (or drop an image on it) to choose another one
					ImTextureID dirtThumbnail = s_BloomDirtTexture ? s_BloomDirtTexture->GetImTextureID() : ImTextureID{};
					bool clicked = dirtThumbnail ? ImGui::ImageButton("##bloomdirt", dirtThumbnail, ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0))
						: ImGui::Button("Dirt", ImVec2(64.0f, 64.0f));
					if (clicked)
					{
						std::string filepath = Util::ToUtf8(Application::Get()->OpenFile(L"Images\0*.png;*.jpg;*.jpeg;*.tga;*.bmp\0All\0*.*\0"));
						if (!filepath.empty())
						{
							s_PendingBloomDirtFilename = filepath;
						}
					}
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
							if (IsImageFile(itemPath))
							{
								s_PendingBloomDirtFilename = itemPath;
							}
						}
						ImGui::EndDragDropTarget();
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Lens dirt: %s\nClick to load another image, or drop one from the Content Browser", s_BloomDirtTexture ? s_BloomDirtTexture->GetPath().c_str() : "");
					}
					ImGui::SameLine();
					ImGui::TextDisabled("Lens dirt texture");
				}
			}
			ImGui::End();
			/**** END Environment ****/

			OnImGuiRenderModelsAndMeshes();
			OnImGuiRenderMaterialLibrary();
			OnImGuiRenderMaterialEditor();
			OnImGuiRenderLights();
			OnImGuiRenderWater();

			/**** BEGIN DockSpace menu bar ****/

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Docking"))
				{
					// Disabling fullscreen would allow the window to be moved to the front of other windows,
					// which we can't undo at the moment without finer window depth/z control.
					//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

					// if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
					// if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
					// if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^=  ImGuiDockNodeFlags_NoDockingInCentralNode;
					// if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
					// if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
					ImGui::Separator();
					if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
						*p_open = false;
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
		}
		ImGui::End(); // END DockSpace Demo
		ImGui::PopStyleVar();

		/**** END DockSpace menu bar ****/

		// TODO: Move to VulkanImGuiLayer
		// Rendering
		ImGui::Render();

		ImDrawData* main_draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(main_draw_data, s_ImGuiCommandBuffer);

		VK_CHECK_RESULT_H2M(vkEndCommandBuffer(s_ImGuiCommandBuffer));

		commandBuffers.push_back(s_ImGuiCommandBuffer);
	}
}

// TODO: Temporary method until composite rendering is enabled
void EnvMapVulkanRenderer::Draw(H2M::CameraH2M* camera)
{
	// The sun points at the sun of the environment map: at startup, and after loading a map whose sun was taken out of
	// it (its light now has to come from the directional sun), once the map's pixels are available
	if (s_PendingSunAlign && s_Data.envEquirect)
	{
		AlignSunToEnvironment();
		s_PendingSunAlign = false;
	}

	// A shadow map resolution chosen in the Lights panel: the map is recreated before any command buffer of this frame is
	// recorded (its descriptor in set 0 can't change while a recorded frame uses it)
	if (s_PendingShadowResolution != 0)
	{
		vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
		CreateShadowMap(s_PendingShadowResolution, true);
		s_PendingShadowResolution = 0;
	}

	// Spot / point shadow map resolutions chosen in the Lights panel (recreated as the sun's, see above)
	if (s_PendingLocalShadowResolutions != glm::uvec2(0))
	{
		vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
		if (s_PendingLocalShadowResolutions.x)
		{
			s_LocalShadowSettings.SpotResolution = s_PendingLocalShadowResolutions.x;
		}
		if (s_PendingLocalShadowResolutions.y)
		{
			s_LocalShadowSettings.PointResolution = s_PendingLocalShadowResolutions.y;
		}
		CreateLocalShadowMaps();
		WriteShadowMapDescriptor();
		s_PendingLocalShadowResolutions = glm::uvec2(0);
	}

	// An environment map requested from the UI (button or drag & drop) is loaded here, before any command buffer
	// of this frame is recorded, not in the middle of the ImGui pass that requested it
	if (!s_PendingEnvMapFilename.empty())
	{
		std::string filepath = s_PendingEnvMapFilename;
		s_PendingEnvMapFilename.clear();
		LoadEnvironmentMap(filepath);
	}

	// Set when a change below may leave cached textures unused (a model or mesh removed, a map replaced or removed, a material deleted)
	bool texturesMayBeUnused = false;

	// Model / mesh removal and model loading requested from the Models and Meshes panel
	if (s_PendingRemoveMeshIndex >= 0)
	{
		if (s_SelectedModelIndex >= 0 && s_SelectedModelIndex < (int)s_LoadedModels.size())
		{
			// Only a part of the selected model: its geometry stays in the model's buffers, so nothing on the GPU is freed
			LoadedModelVulkan& entry = s_LoadedModels[s_SelectedModelIndex];
			const int index = s_PendingRemoveMeshIndex;
			if (index < (int)entry.Model->GetMeshes().size() && entry.Model->GetMeshes().size() > 1)
			{
				Log::GetLogger()->info("Mesh '{0}' removed from model '{1}'", entry.Model->GetMeshes()[index]->MeshName, entry.FilePath);
				entry.Model->RemoveMesh((uint32_t)index);
				if (index < (int)entry.MeshMaterials.size())
				{
					entry.MeshMaterials.erase(entry.MeshMaterials.begin() + index);
				}
				if (index < (int)entry.OriginalMeshTransforms.size())
				{
					entry.OriginalMeshTransforms.erase(entry.OriginalMeshTransforms.begin() + index);
				}
				s_SelectedMeshIndex = -1;
				texturesMayBeUnused = true; // its material may have no other users
			}
		}
		s_PendingRemoveMeshIndex = -1;
	}
	if (s_PendingRemoveModelIndex >= 0)
	{
		if (s_PendingRemoveModelIndex < (int)s_LoadedModels.size())
		{
			// The model's buffers and descriptor sets may still be used by frames in flight
			vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
			s_LoadedModels.erase(s_LoadedModels.begin() + s_PendingRemoveModelIndex);
			s_SelectedModelIndex = glm::min(s_SelectedModelIndex, (int)s_LoadedModels.size() - 1);
			s_SelectedMeshIndex = -1;
			texturesMayBeUnused = true;
		}
		s_PendingRemoveModelIndex = -1;
	}
	if (!s_PendingModelFilename.empty())
	{
		std::string filepath = s_PendingModelFilename;
		std::optional<glm::vec3> groundPosition = s_PendingModelGroundPosition;
		s_PendingModelFilename.clear();
		s_PendingModelGroundPosition.reset();
		LoadModel(filepath, groundPosition);
	}
	// Lens dirt texture chosen in the Bloom settings
	if (!s_PendingBloomDirtFilename.empty())
	{
		std::string filepath = s_PendingBloomDirtFilename;
		s_PendingBloomDirtFilename.clear();
		try
		{
			H2M::RefH2M<H2M::Texture2D_H2M> texture = H2M::Texture2D_H2M::Create(filepath, true);
			vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice()); // the composite descriptor set may be in use
			s_BloomDirtTexture = texture;
			WriteBloomDescriptorSets();
		}
		catch (...)
		{
			Log::GetLogger()->error("Lens dirt texture '{0}' could not be loaded.", filepath);
		}
	}
	// Maps assigned or removed in the Material Editor, then a material deleted in the Material Library
	for (const PendingMaterialTexture& request : s_PendingMaterialTextures)
	{
		ApplyMaterialTexture(request);
		texturesMayBeUnused = true;
	}
	s_PendingMaterialTextures.clear();
	if (s_PendingDeleteMaterial)
	{
		DeleteMaterial(s_PendingDeleteMaterial);
		s_PendingDeleteMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
		texturesMayBeUnused = true;
	}

	// Free the GPU memory of textures nothing uses any more. Each change above waited for the device to be idle, and no
	// command buffer of this frame has been recorded yet, so no frame in flight can still sample them.
	if (texturesMayBeUnused)
	{
		uint32_t released = ResourceManager::PurgeUnusedTextures2D();
		if (released > 0)
		{
			Log::GetLogger()->info("{0} unused texture(s) released", released);
		}
	}

	// The aspect ratio must follow the viewport panel, not the window (Scene::OnWindowResize sets the window size),
	// otherwise resizing the panel in one dimension stretches the scene. The FOV is vertical, as in SceneHazelEnvMap.
	camera->SetViewportSize((float)s_ViewportWidth, (float)s_ViewportHeight);
	s_Data.SceneData.SceneCamera.Camera = *camera;

	// Frame time for the animations (clamped: a long stall, e.g. loading a model, would otherwise skip ahead)
	static auto s_LastAnimationUpdate = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	float deltaTime = std::min(std::chrono::duration<float>(now - s_LastAnimationUpdate).count(), 0.1f);
	s_LastAnimationUpdate = now;

	for (LoadedModelVulkan& entry : s_LoadedModels)
	{
		if (entry.Model->IsSkinned())
		{
			entry.Model->OnUpdate(H2M::TimestepH2M(deltaTime), false); // bone matrices of the current frame (bind pose when not animated)
		}
		UpdateObjectUniforms(entry.Model);
		SubmitModelTemp(entry.Model, entry.GetTransform(), entry.MeshMaterials);
	}
	UpdateFrameUniforms();
	if (s_WaterSettings.Enabled)
	{
		H2M::CameraH2M& sceneCamera = s_Data.SceneData.SceneCamera.Camera;
		s_Water.Update(s_WaterSettings, deltaTime, sceneCamera.GetViewMatrix(), sceneCamera.GetProjectionMatrix(), sceneCamera.GetPosition(),
			s_EnvMapRotation, GetWaterSunDirection(), s_Data.FrameDescriptorSet.DescriptorSets[0]);
	}

	if (s_ViewportFBNeedsResize)
	{
		s_Framebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		s_Water.Resize(s_Framebuffer->GetWidth(), s_Framebuffer->GetHeight());
		s_ViewportCompositeFramebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		s_OverlayFramebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		s_SelectionMaskFramebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		ResizeBloomResources(); // also rewrites the composite descriptor set
		s_ViewportFBNeedsResize = false;
	}

	GeometryPass();
	CompositePass();
}

std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> EnvMapVulkanRenderer::CreateEnvironmentMap(const std::string& filepath)
{
	const uint32_t cubemapSize = 1024;
	const uint32_t irradianceMapSize = 32;

	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	VkQueue computeQueue = H2M::VulkanContextH2M::GetCurrentDevice()->GetComputeQueue();

	// The output cubemaps are created once and rewritten by every load, so descriptor sets that reference them
	// (skybox, PBR set 1) stay valid. The caller must make sure nothing in flight still reads them (see LoadEnvironmentMap).
	if (!s_Data.envUnfiltered)
	{
		s_Data.envUnfiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize);
	}
	if (!s_Data.envFiltered)
	{
		s_Data.envFiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize);
	}
	if (!s_Data.irradianceMap)
	{
		s_Data.irradianceMap = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, irradianceMapSize, irradianceMapSize);
	}

	H2M::RefH2M<H2M::VulkanTextureCubeH2M> envUnfilteredCubemap = s_Data.envUnfiltered.As<H2M::VulkanTextureCubeH2M>();
	H2M::RefH2M<H2M::VulkanTextureCubeH2M> envFilteredCubemap = s_Data.envFiltered.As<H2M::VulkanTextureCubeH2M>();
	H2M::RefH2M<H2M::VulkanTextureCubeH2M> irradianceCubemap = s_Data.irradianceMap.As<H2M::VulkanTextureCubeH2M>();

	// Loaded as 32-bit float (full dynamic range)
	s_Data.envEquirect = H2M::Texture2D_H2M::Create(filepath, false);

	// A real sun is taken out of the map before the environment lighting is built from it, and moved to the directional
	// sun (see ExtractSun): the texture is recreated from the edited pixels
	s_ExtractedSun = EnvMapVulkanExtractedSun();
	{
		H2M::BufferH2M pixels = s_Data.envEquirect->GetWriteableBuffer();
		uint32_t width = s_Data.envEquirect->GetWidth(), height = s_Data.envEquirect->GetHeight();
		if (s_ExtractSunFromEnvironment && pixels.Data && s_Data.envEquirect->GetFormat() == H2M::ImageFormatH2M::RGBA32F &&
			pixels.Size >= (uint64_t)width * height * 4 * sizeof(float) && ExtractSun((float*)pixels.Data, width, height, s_ExtractedSun))
		{
			s_Data.envEquirect = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA32F, width, height, pixels.Data);
			s_PendingSunAlign = true;
			Log::GetLogger()->info("Sun extracted from '{0}': {1} pixels, {2}x brighter than the map's average, intensity {3}",
				filepath, s_ExtractedSun.PixelCount, s_ExtractedSun.PeakToAverage, s_ExtractedSun.Intensity);
		}
	}
	s_EnvMapAutoExposure = ComputeAutoExposure(s_Data.envEquirect);

	uint32_t mipFilterLevels = s_MipMapsEnabled ? glm::min(11u, envFilteredCubemap->GetMipLevelCount()) : 1;

	// First call: create the compute pipelines, their descriptor sets and the single-mip views (reused by later loads)
	if (!s_EnvMapCompute.EquirectPipeline)
	{
		s_EnvMapCompute.EquirectPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("EquirectangularToCubeMap"));
		s_EnvMapCompute.MipFilterPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("EnvironmentMipFilter"));
		s_EnvMapCompute.IrradiancePipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("EnvironmentIrradiance"));

		s_EnvMapCompute.EquirectDescriptorSet = s_EnvMapCompute.EquirectPipeline->GetShader()->CreateDescriptorSets();
		s_EnvMapCompute.MipFilterDescriptorSets = s_EnvMapCompute.MipFilterPipeline->GetShader()->CreateDescriptorSets(0, mipFilterLevels);
		s_EnvMapCompute.IrradianceDescriptorSet = s_EnvMapCompute.IrradiancePipeline->GetShader()->CreateDescriptorSets();

		s_EnvMapCompute.MipImageInfos.resize(mipFilterLevels);
		for (uint32_t i = 0; i < mipFilterLevels; i++)
		{
			VkDescriptorImageInfo& mipImageInfo = s_EnvMapCompute.MipImageInfos[i];
			mipImageInfo = envFilteredCubemap->GetVulkanDescriptorInfo();
			mipImageInfo.imageView = envFilteredCubemap->CreateImageViewSingleMip(i);
			mipImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		}
	}
	mipFilterLevels = (uint32_t)s_EnvMapCompute.MipImageInfos.size();

	// After a previous load these cubemaps are in SHADER_READ_ONLY (GenerateMips(true)); compute shaders write them in GENERAL
	envUnfilteredCubemap->TransitionToGeneralLayout();
	irradianceCubemap->TransitionToGeneralLayout();

	// Convert equirectangular to cubemap
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = s_EnvMapCompute.EquirectPipeline->GetShader();
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& descriptorSet = s_EnvMapCompute.EquirectDescriptorSet;

		std::array<VkWriteDescriptorSet, 2> writeDescriptors;

		writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
		writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data();
		writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

		H2M::RefH2M<H2M::VulkanTexture2D_H2M> envEquirectVK = s_Data.envEquirect.As<H2M::VulkanTexture2D_H2M>();
		writeDescriptors[1] = *shader->GetDescriptorSet("u_EquirectangularTex");
		writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data();
		writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[1].pImageInfo = &envEquirectVK->GetVulkanDescriptorInfo();

		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		s_EnvMapCompute.EquirectPipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), cubemapSize / 32, cubemapSize / 32, 6);
		vkQueueWaitIdle(computeQueue);

		envUnfilteredCubemap->GenerateMips(true);
	}

	// Mip filtering (prefiltered radiance for PBR reflections: one roughness per mip level)
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = s_EnvMapCompute.MipFilterPipeline->GetShader();
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& descriptorSet = s_EnvMapCompute.MipFilterDescriptorSets;

		std::vector<VkWriteDescriptorSet> writeDescriptors(mipFilterLevels * 2);
		for (uint32_t i = 0; i < mipFilterLevels; i++)
		{
			writeDescriptors[i * 2 + 0] = *shader->GetDescriptorSet("outputTexture");
			writeDescriptors[i * 2 + 0].dstSet = descriptorSet.DescriptorSets[i];
			writeDescriptors[i * 2 + 0].pImageInfo = &s_EnvMapCompute.MipImageInfos[i];

			writeDescriptors[i * 2 + 1] = *shader->GetDescriptorSet("inputTexture");
			writeDescriptors[i * 2 + 1].dstSet = descriptorSet.DescriptorSets[i];
			writeDescriptors[i * 2 + 1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
		}
		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

		s_EnvMapCompute.MipFilterPipeline->Begin(); // begin compute pass
		const float deltaRoughness = 1.0f / glm::max((float)s_Data.envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
		for (uint32_t i = 0, size = cubemapSize; i < mipFilterLevels; i++, size /= 2)
		{
			uint32_t numGroups = glm::max(1u, size / 32);
			float roughness = i * deltaRoughness;
			roughness = glm::max(roughness, 0.05f);
			s_EnvMapCompute.MipFilterPipeline->SetPushConstants(&roughness, sizeof(float));
			s_EnvMapCompute.MipFilterPipeline->Dispatch(descriptorSet.DescriptorSets[i], numGroups, numGroups, 6);
		}
		s_EnvMapCompute.MipFilterPipeline->End();
		vkQueueWaitIdle(computeQueue);
	}

	// Irradiance map (diffuse lighting)
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = s_EnvMapCompute.IrradiancePipeline->GetShader();
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& descriptorSet = s_EnvMapCompute.IrradianceDescriptorSet;

		std::array<VkWriteDescriptorSet, 2> writeDescriptors;

		writeDescriptors[0] = *shader->GetDescriptorSet("o_IrradianceMap");
		writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data();
		writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[0].pImageInfo = &irradianceCubemap->GetVulkanDescriptorInfo();

		writeDescriptors[1] = *shader->GetDescriptorSet("u_RadianceMap");
		writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data();
		writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		s_EnvMapCompute.IrradiancePipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), irradianceCubemap->GetWidth() / 32, irradianceCubemap->GetHeight() / 32, 6);
		vkQueueWaitIdle(computeQueue);

		irradianceCubemap->GenerateMips(true);
	}

	return { s_Data.envFiltered, s_Data.irradianceMap };
}


void EnvMapVulkanRenderer::RenderMeshWithoutMaterial(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform)
{
}

void EnvMapVulkanRenderer::RenderMesh(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform)
{
	// H2M::RendererH2M::Submit([mesh, transform]() mutable {});
	{
		H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_MeshPipeline.As<H2M::VulkanPipelineH2M>();

		VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

		auto vulkanMeshVB = model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(s_Data.ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = H2M::RefH2M<H2M::VulkanIndexBufferH2M>(model->GetIndexBuffer());
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(s_Data.ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		std::vector<H2M::RefH2M<H2M::MeshH2M>>& meshes = model->GetMeshes();
		for (H2M::RefH2M<H2M::MeshH2M> mesh : meshes)
		{
			auto& material = model->GetMaterials()[mesh->MaterialIndex].As<H2M::VulkanMaterialH2M>();
			material->UpdateForRendering();

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points: set 0 per frame, set 1 the material's own set
			std::array<VkDescriptorSet, 2> descriptorSets = {
				s_Data.FrameDescriptorSet.DescriptorSets[0],
				material->GetDescriptorSet().DescriptorSets[0],
			};
			vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

			glm::mat4 worldTransform = transform * mesh->Transform;
			H2M::BufferH2M uniformStorageBuffer = material->GetUniformStorageBuffer();
			vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
			vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
			vkCmdDrawIndexed(s_Data.ActiveCommandBuffer, mesh->IndexCount, 1, mesh->BaseIndex, mesh->BaseVertex, 0);
		}
	}
}

void EnvMapVulkanRenderer::RenderQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material, const glm::mat4& transform)
{
	H2M::RefH2M<H2M::VulkanMaterialH2M> vulkanMaterial = material.As<H2M::VulkanMaterialH2M>();
	vulkanMaterial->UpdateForRendering(); // Broken at the moment

	// H2M::RendererH2M::Submit([pipeline, vulkanMaterial, transform]() {});
	{
		H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = pipeline.As<H2M::VulkanPipelineH2M>();

		// VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

		auto vulkanMeshVB = s_Data.QuadVertexBuffer.As<H2M::VulkanVertexBufferH2M>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		// vkCmdBindVertexBuffers(s_Data.ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = s_Data.QuadIndexBuffer.As<H2M::VulkanIndexBufferH2M>();
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		// vkCmdBindIndexBuffer(s_Data.ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		// VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
		// vkCmdBindPipeline(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Bind descriptor sets describing shader binding points
		// vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &vulkanMaterial->GetDescriptorSet().DescriptorSets[0], 0, nullptr);

		// Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

		// vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
		// vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
		// vkCmdDrawIndexed(s_Data.ActiveCommandBuffer, s_Data.QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
	}
}

void EnvMapVulkanRenderer::DrawIndexed(uint32_t indexCount, H2M::PrimitiveTypeH2M type, bool depthTest)
{
	Log::GetLogger()->warn("EnvMapVulkanRenderer::DrawIndexed: Method not yet supported!");
}

void EnvMapVulkanRenderer::DrawLines(H2M::RefH2M<H2M::VertexArrayH2M> vertexArray, uint32_t vertexCount)
{
	Log::GetLogger()->warn("EnvMapVulkanRenderer::DrawLines: Method not yet supported!");
}

void EnvMapVulkanRenderer::SetLineWidth(float width)
{
	Log::GetLogger()->warn("EnvMapVulkanRenderer::SetLineWidth: Method not yet supported!");
}

void EnvMapVulkanRenderer::SubmitFullscreenQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material)
{
	H2M::RefH2M<H2M::VulkanMaterialH2M> vulkanMaterial = material.As<H2M::VulkanMaterialH2M>();
	vulkanMaterial->UpdateForRendering();

	// H2M::RendererH2M::Submit([pipeline, vulkanMaterial]() mutable {});
	{
		H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_CompositePipeline.As<H2M::VulkanPipelineH2M>();

		VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

		auto vulkanMeshVB = s_Data.QuadVertexBuffer.As<H2M::VulkanVertexBufferH2M>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(s_Data.ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = s_Data.QuadIndexBuffer.As<H2M::VulkanIndexBufferH2M>();
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(s_Data.ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
		vkCmdBindPipeline(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Bind descriptor sets describing shader binding points
		// vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)s_Data.QuadDescriptorSet.DescriptorSets.size(), s_Data.QuadDescriptorSet.DescriptorSets.data(), 0, nullptr);
		vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)vulkanMaterial->GetDescriptorSet().DescriptorSets.size(), vulkanMaterial->GetDescriptorSet().DescriptorSets.data(), 0, nullptr);

		H2M::BufferH2M uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

		vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
		vkCmdDrawIndexed(s_Data.ActiveCommandBuffer, s_Data.QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
	}
}

H2M::RendererCapabilitiesH2M EnvMapVulkanRenderer::GetCapabilities()
{
	return s_Data.RenderCaps;
}

void EnvMapVulkanRenderer::UpdateImGuizmo(Window* mainWindow)
{
	// BEGIN ImGuizmo

	// Gizmo mode: 1 translate, 2 rotate, 3 scale, 4 hide (not while typing into an ImGui text field)
	if (!ImGui::GetIO().WantTextInput)
	{
		if (Input::IsKeyPressed(KeyH2M::D1))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

		if (Input::IsKeyPressed(KeyH2M::D2))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;

		if (Input::IsKeyPressed(KeyH2M::D3))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;

		if (Input::IsKeyPressed(KeyH2M::D4))
			Scene::s_ImGuizmoType = -1;
	}

	// The gizmo moves the selected point or spot light, or else the model or mesh selected in the Models and Meshes panel (or picked with the mouse)
	if (Scene::s_ImGuizmoType != -1 && s_SelectedLightKind != LightKind::None && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
	{
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);
		if (s_SelectedLightKind == LightKind::Sun)
		{
			ManipulateSun(Scene::s_ImGuizmoType, Input::IsKeyPressed(KeyH2M::LeftControl));
		}
		else
		{
			ManipulateSelectedLight(Scene::s_ImGuizmoType, Input::IsKeyPressed(KeyH2M::LeftControl));
		}
		return;
	}

	// The water: translate moves it (its position and height), scale resizes it. It doesn't turn: it is an axis-aligned rectangle.
	if (Scene::s_ImGuizmoType != -1 && s_WaterSelected && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
	{
		if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE)
		{
			return;
		}
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);
		float snapValues[3] = { 1.0f, 1.0f, 1.0f };
		glm::mat4 transform = s_WaterSettings.GetTransform();
		if (ImGuizmo::Manipulate(
			glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix()),
			glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix()),
			(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
			ImGuizmo::WORLD,
			glm::value_ptr(transform),
			nullptr,
			Input::IsKeyPressed(KeyH2M::LeftControl) ? snapValues : nullptr))
		{
			glm::vec3 translation, rotation, scale;
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), &translation.x, &rotation.x, &scale.x);
			s_WaterSettings.Center = glm::vec2(translation.x, translation.z);
			s_WaterSettings.Height = translation.y;
			s_WaterSettings.Size = glm::max(glm::vec2(scale.x, scale.z), glm::vec2(0.1f));
		}
		return;
	}

	if (Scene::s_ImGuizmoType == -1 || s_SelectedModelIndex < 0 || s_SelectedModelIndex >= (int)s_LoadedModels.size() ||
		s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	LoadedModelVulkan& entry = s_LoadedModels[s_SelectedModelIndex];

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	// The scene image, not the whole window (which includes the tab bar)
	ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);

	// Snapping with Ctrl: 1 unit for translation/scale, 45 degrees for rotation
	bool snap = Input::IsKeyPressed(KeyH2M::LeftControl);
	float snapValue = Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE ? 45.0f : 1.0f;
	float snapValues[3] = { snapValue, snapValue, snapValue };

	// A selected part of a multi-part model: the gizmo moves the part. It sits at the center of the part's bounding box
	// (as in SceneHazelEnvMap): imported parts often have their origin at the model's origin, far from the part, and would
	// turn around it. world = model * part * T(center); after the gizmo: part = model^-1 * world * T(-center)
	if (CanManipulateMesh(entry, s_SelectedMeshIndex))
	{
		H2M::RefH2M<H2M::MeshH2M> part = entry.Model->GetMeshes()[s_SelectedMeshIndex];
		const glm::mat4 modelMatrix = entry.GetTransform();
		const glm::vec3 center = (part->BoundingBox.Min + part->BoundingBox.Max) * 0.5f;
		glm::mat4 world = modelMatrix * part->Transform * glm::translate(glm::mat4(1.0f), center);
		if (ImGuizmo::Manipulate(
			glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix()),
			glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix()),
			(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
			ImGuizmo::WORLD,
			glm::value_ptr(world),
			nullptr,
			snap ? snapValues : nullptr))
		{
			part->Transform = glm::inverse(modelMatrix) * world * glm::translate(glm::mat4(1.0f), -center);
		}
		return;
	}

	glm::mat4 transform = entry.GetTransform();
	if (ImGuizmo::Manipulate(
		glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix()),
		glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix()),
		(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
		ImGuizmo::WORLD,
		glm::value_ptr(transform),
		nullptr,
		snap ? snapValues : nullptr))
	{
		// Back into the values shown (and editable) in the Models and Meshes panel
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), &entry.Translation.x, &entry.Rotation.x, &entry.Scale.x);
	}
}

/**** BEGIN to be removed from VulkanRenderer ****/
uint32_t EnvMapVulkanRenderer::GetViewportWidth()
{
	return s_ViewportWidth;
}
	
uint32_t EnvMapVulkanRenderer::GetViewportHeight()
{
	return s_ViewportHeight;
}
/**** END to be removed from VulkanRenderer ****/

int32_t& EnvMapVulkanRenderer::GetSelectedDrawCall()
{
	return s_Data.SelectedDrawCall;
}


void EnvMapVulkanRenderer::SetCamera(H2M::CameraH2M& camera)
{
	s_Data.SceneData.SceneCamera.Camera = camera;
}

/**** BEGIN code moved from VulkanTestLayer to VulkanRenderer****/
H2M::SceneRendererOptionsH2M& EnvMapVulkanRenderer::GetOptions()
{
	return s_Data.Options;
}
/**** END code moved from VulkanTestLayer to VulkanRenderer****/

void EnvMapVulkanRenderer::MapUniformBuffersVTL(H2M::RefH2M<H2M::ModelH2M> model, const H2M::EditorCameraH2M& camera)
{
	// Temporary code
	s_Data.SceneData.SceneCamera.Camera = camera;

	H2M::RendererH2M::BeginRenderPass(s_Data.GeoPass);

	// Camera and scene data are per frame (set 0), shared by every mesh
	UpdateFrameUniforms();
	UpdateObjectUniforms(model);
}

namespace Utils
{

	void InsertImageMemoryBarrier(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkImageSubresourceRange subresourceRange)
	{
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		imageMemoryBarrier.srcAccessMask = srcAccessMask;
		imageMemoryBarrier.dstAccessMask = dstAccessMask;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}

	void SetImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask)
	{
		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Host writes are only covered by the HOST pipeline stage
		if (imageMemoryBarrier.srcAccessMask & VK_ACCESS_HOST_WRITE_BIT)
		{
			srcStageMask |= VK_PIPELINE_STAGE_HOST_BIT;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}

	void SetImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask)
	{
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
	}

}
