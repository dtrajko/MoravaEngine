/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EnvMapVulkanRenderer.h"
#include "EnvMapVulkanLights.h"
#include "EnvMapVulkanShadows.h"
#include "EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkanProfiler.h"
#include "EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkanGlass.h"
#include "EnvMapVulkanGBuffer.h"
#include "EnvMapVulkanMaterialBuffer.h"
#include "EnvMapVulkanProbes.h"
#include "EnvMapVulkanRayTracing.h"
#include "EnvMapVulkanPathTracer.h"
#include "EnvMapVulkanReflections.h"
#include "EnvMapVulkanSSAO.h"
#include "EnvMapVulkanWater.h"
#include "EnvMapVulkanScene.h"
#include "EnvMapVulkanSceneData.h"
#include "EnvMapVulkanSceneLoader.h"
#include "EnvMapVulkanSceneSerializer.h"
#include "EnvMapVulkanViewOptions.h"


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
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/Renderer2D_H2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"

#include "H2M/Platform/Vulkan/VulkanUIImagesH2M.h"

#include "Platform/Vulkan/VulkanSkyboxCube.h"

#include "stb_image.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <set>
#include <unordered_map>


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

static VkCommandBuffer s_UserInterfaceCommandBuffer; // the user interface's commands (SetUserInterface)
static VkCommandBuffer s_CompositeCommandBuffer;     // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::FramebufferH2M> s_Framebuffer;          // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::FramebufferH2M> s_CompositeFramebuffer; // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::PipelineH2M> s_CompositePipeline;            // to be removed from VulkanRenderer
// The Viewport panel shows this image: s_Framebuffer (linear HDR scene) after exposure, tonemapping and gamma
static H2M::RefH2M<H2M::FramebufferH2M> s_ViewportCompositeFramebuffer;
static H2M::RefH2M<H2M::PipelineH2M> s_ViewportCompositePipeline;
static H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet s_ViewportCompositeDescriptorSet; // scene, bloom, lens dirt (ViewportComposite.glsl)

// How the scene is drawn: shadows, ray tracing, reflections, ambient occlusion, bloom (see EnvMapVulkanRenderSettings).
// This frame's copy of the scene owner's settings.
static EnvMapVulkanRenderSettings s_RenderSettings;

// Bloom (Resources/Shaders/BloomPass.glsl), with the settings of SceneHazelEnvMap's "Bloom Settings" panel
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
// The debug view, the grid, the selection and its overlays (see EnvMapVulkanViewOptions): this frame's copy of the scene
// owner's, with a ray-traced debug view turned off when no rays can be traced
static EnvMapVulkanViewOptions s_ViewOptions;

static H2M::RefH2M<H2M::FramebufferH2M> s_OverlayFramebuffer;
static H2M::RefH2M<H2M::FramebufferH2M> s_SelectionMaskFramebuffer;
static H2M::RefH2M<H2M::PipelineH2M> s_OverlayDepthPipeline;        // meshes with color alpha 0: depth only, hides the wireframe behind them
static H2M::RefH2M<H2M::PipelineH2M> s_OverlayDepthPipelineAnim;
static H2M::RefH2M<H2M::PipelineH2M> s_WireframePipeline;
static H2M::RefH2M<H2M::PipelineH2M> s_WireframePipelineAnim;
static H2M::RefH2M<H2M::PipelineH2M> s_BoundingBoxPipeline;         // line list: unit cube edges (s_BoundingBoxVertexBuffer)
static H2M::RefH2M<H2M::PipelineH2M> s_SelectionMaskPipeline;
static H2M::RefH2M<H2M::PipelineH2M> s_SelectionMaskPipelineAnim;
// The selection mask's second channel, the part of the selection that isn't hidden: the depth of what can hide it first
// (the models and the water's surface), then the selection again, depth tested
static H2M::RefH2M<H2M::PipelineH2M> s_SelectionMaskDepthPipeline;
static H2M::RefH2M<H2M::PipelineH2M> s_SelectionMaskDepthPipelineAnim;
static H2M::RefH2M<H2M::PipelineH2M> s_SelectionMaskVisiblePipeline;
static H2M::RefH2M<H2M::PipelineH2M> s_SelectionMaskVisiblePipelineAnim;
static H2M::RefH2M<H2M::VertexBufferH2M> s_SelectionMaskWaterVertexBuffer; // the water's unit square (two triangles)
static H2M::RefH2M<H2M::PipelineH2M> s_VectorsPipeline;             // line per vertex, instanced: the model's vertex buffer is per-instance data
static H2M::RefH2M<H2M::PipelineH2M> s_VectorsPipelineAnim;
static H2M::RefH2M<H2M::VertexBufferH2M> s_BoundingBoxVertexBuffer;
static const uint32_t s_BoundingBoxVertexCount = 24; // 12 edges

static void CreateEditorOverlayResources();
static void RecordEditorOverlayPasses(VkCommandBuffer commandBuffer);
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipeline;                 // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipelineAnim; // skinned models (HazelPBR_Anim.glsl): vertex layout with bone IDs and weights
// The same with depth EQUAL and no depth write: the scene pass, after the G-buffer prepass wrote the depth. The two above
// draw where there is no prepass (the water's reflection) and lend their layouts to the prepass pipelines.
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipelineDepthEqual;
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipelineAnimDepthEqual;
static uint64_t s_ViewportImageID = 0;                // the viewport image's ID in the user interface (GetViewportImage)
static bool s_ViewportTextureNeedsUpdate = false;     // the viewport framebuffer was recreated (resize)

// ImGui::Image() needs the viewport framebuffer's color image registered with the user interface (ImGui's Vulkan
// backend: as a descriptor set). After a resize the framebuffer has a new image, so the old registration is replaced.
static void RegisterViewportImage()
{
	if (s_ViewportImageID)
	{
		// The old descriptor set may still be used by frames in flight (only happens on resize)
		vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
		H2M::VulkanUIImagesH2M::Remove(s_ViewportImageID);
	}

	// The tonemapped image (not the linear HDR s_Framebuffer)
	auto vulkanFB = s_ViewportCompositeFramebuffer.As<H2M::VulkanFramebufferH2M>();
	const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
	s_ViewportImageID = H2M::VulkanUIImagesH2M::Add(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
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
	uint32_t FirstDraw = 0; // its first mesh in s_PreviousDrawTransforms (mesh s is at FirstDraw + s)
};
static std::vector<SubmittedModel> s_SubmittedModels;
// For the motion vectors: where every mesh of the submitted models was drawn in the previous frame (GetMeshTransform then),
// one matrix per mesh in the order of s_SubmittedModels. The G-buffer prepass reads them per draw (see EnvMapVulkanGBuffer).
static std::vector<glm::mat4> s_PreviousDrawTransforms;


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
	H2M::RefH2M<::VulkanSkyboxCube> VulkanSkyboxCube;
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

// The environment map loaded (shown in the Environment panel), and whether its sun was taken out of it (see ExtractSun).
// The scene's environment entity says which map it wants: another one is loaded at the start of the next Draw.
static std::string s_EnvMapFilename;
static bool s_EnvMapSunExtracted = true;
static std::string s_FailedEnvMapFilename; // a map that could not be loaded is not tried again every frame

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
		s_FailedEnvMapFilename = filepath;
		return;
	}
	s_FailedEnvMapFilename.clear();

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
// Cascaded shadow maps of the sun (see EnvMapVulkanShadows.h)
static EnvMapVulkanShadowMap s_ShadowMap;
static EnvMapVulkanShadowPipeline s_ShadowPipeline;     // ShadowDepth.glsl (static models)
static EnvMapVulkanShadowPipeline s_ShadowPipelineAnim; // ShadowDepth_Anim.glsl (skinned models)
// Shadow maps of the spot and point lights (see EnvMapVulkanShadows.h): a layer per shadowed spot light, a cube per
// shadowed point light (only when the GPU supports cube map arrays)
static EnvMapVulkanShadowMap s_SpotShadowMaps;
static EnvMapVulkanShadowMap s_PointShadowMaps;

// World bounds of each submitted model (same order as s_SubmittedModels), for the shadow passes: the cascades are fitted around all
// of them, and a spot or point light draws only the models within its range. Computed once per frame.
static std::vector<std::pair<glm::vec3, glm::vec3>> s_ShadowCasterBounds;
// Their union, this frame (min > max: no models)
static glm::vec3 s_ShadowCasterBoundsMin = glm::vec3(1.0f);
static glm::vec3 s_ShadowCasterBoundsMax = glm::vec3(-1.0f);
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
static void WriteShadowMapDescriptor();
static float s_EnvMapRotation = 0.0f; // degrees, applied to the PBR environment lookups (as in SceneHazelEnvMap)
static uint32_t s_EnvironmentVersion = 0; // changes whenever the environment's cube maps get new contents (CreateEnvironmentMap)

// The scene (EnvMapVulkanScene, see EnvMapVulkanComponents.h): the renderer reads it every frame. Terms: a model is a
// loaded model file (an H2M::ModelH2M), an entity with a ModelComponent; a mesh is one part of it (an H2M::MeshH2M), a
// child entity of the model with a MeshPartComponent. Every model entity has its own ModelH2M, so editing its parts
// changes only that model.
static EnvMapVulkanScene* s_Scene = nullptr; // the scene drawn, its owner's (see SetScene)
static const EnvMapVulkanRenderSettings* s_OwnerRenderSettings = nullptr; // copied into s_RenderSettings at the start of Draw
static const EnvMapVulkanViewOptions* s_OwnerViewOptions = nullptr;       // copied into s_ViewOptions at the start of Draw
static std::function<void(VkCommandBuffer)> s_RecordUserInterface; // the editor's windows (see SetUserInterface)
static EnvMapVulkanSceneData::LightEntities s_LightEntities; // the entities of s_Lights' lights (see ExtractLights)

template<typename T>
static bool Contains(const std::vector<T>& values, const T& value)
{
	return std::find(values.begin(), values.end(), value) != values.end();
}

// The environment is an entity with an EnvironmentComponent (one per scene). Every frame ExtractEnvironment reads it into
// the renderer's environment settings (s_EnvMapRotation, s_Exposure...), and UpdateEnvironmentMap loads the map it names
// when that isn't the map loaded. The Environment panel edits the component.
static EnvMapVulkanEntityID s_EnvironmentEntity = NoEntity;

static void ExtractEnvironment()
{
	s_EnvironmentEntity = s_Scene->FindFirst<EnvironmentComponent>();
	const EnvironmentComponent* environment = s_Scene->TryGet<EnvironmentComponent>(s_EnvironmentEntity);
	if (!environment)
	{
		return;
	}
	s_EnvMapRotation = environment->Rotation;
	s_Exposure = environment->Exposure;
	s_AutoExposureEnabled = environment->AutoExposure;
	s_TonemapHuePreservation = environment->HuePreservation;
	s_ExtractSunFromEnvironment = environment->ExtractSun;
	s_Data.SceneData.SkyboxLod = environment->SkyboxLod;
}

// The scene's environment map: loaded when the environment entity names another map, or the same one with or without its
// sun (Extract Sun). Called at the start of Draw, before any command buffer of the frame is recorded.
static void UpdateEnvironmentMap()
{
	const EnvironmentComponent* environment = s_Scene->TryGet<EnvironmentComponent>(s_EnvironmentEntity);
	if (!environment || environment->FilePath.empty() || environment->FilePath == s_FailedEnvMapFilename)
	{
		return;
	}
	if (environment->FilePath != s_EnvMapFilename || environment->ExtractSun != s_EnvMapSunExtracted)
	{
		LoadEnvironmentMap(environment->FilePath);
	}
}

// The parts' transforms (the scene's) into the model's meshes, which the drawing reads. Only a transform edited since it
// was last applied is written: an imported mesh transform the Euler decomposition can't reproduce exactly (a mirrored
// part, a shear) stays as it was loaded until the part is moved.
static void ApplyPartTransforms()
{
	for (EnvMapVulkanEntityID model : EnvMapVulkanSceneData::GetModels(*s_Scene))
	{
		H2M::RefH2M<H2M::ModelH2M> modelRef = EnvMapVulkanSceneData::GetModel(*s_Scene, model);
		for (EnvMapVulkanEntityID child : s_Scene->GetChildren(model))
		{
			MeshPartComponent* part = s_Scene->TryGet<MeshPartComponent>(child);
			if (!part || part->MeshIndex >= modelRef->GetMeshes().size())
			{
				continue;
			}
			const TransformComponent& transform = s_Scene->Get<TransformComponent>(child);
			if (transform.Translation != part->AppliedTransform.Translation || transform.Rotation != part->AppliedTransform.Rotation ||
				transform.Scale != part->AppliedTransform.Scale)
			{
				modelRef->GetMeshes()[part->MeshIndex]->Transform = transform.GetMatrix();
				part->AppliedTransform = transform;
			}
		}
	}
}

// The scene's lights into s_Lights, the lights the renderer draws with this frame (shadow slots, the Lights buffer), in
// the order of the hierarchy, with the entity of each light in s_LightEntities
static void ExtractLights()
{
	s_LightEntities = EnvMapVulkanSceneData::ReadLights(*s_Scene, s_Lights);
}

// The selection as the view draws it (EnvMapVulkanViewOptions::Selection)
static bool IsInViewSelection(EnvMapVulkanEntityID entity)
{
	return entity != NoEntity && Contains(s_ViewOptions.Selection, entity);
}

// The meshes of a model the overlays show as selected: { -1 } when the model itself is selected, else the mesh indices of
// its selected parts (empty: nothing of it is selected)
static std::vector<int> GetViewSelectedMeshes(EnvMapVulkanEntityID model)
{
	if (IsInViewSelection(model))
	{
		return { -1 };
	}
	std::vector<int> meshes;
	for (EnvMapVulkanEntityID child : s_Scene->GetChildren(model))
	{
		const MeshPartComponent* part = s_Scene->TryGet<MeshPartComponent>(child);
		if (part && IsInViewSelection(child))
		{
			meshes.push_back((int)part->MeshIndex);
		}
	}
	return meshes;
}

// The light the Shadows Only view shows: the view's primary selected entity, when it is a light of s_Lights (its kind and
// index; None: it isn't a light)
static EnvMapVulkanSceneData::LightKind GetShadowsOnlyLight(int& index)
{
	return s_LightEntities.Find(s_ViewOptions.PrimarySelection, index);
}

// The water plane (Water panel, see EnvMapVulkanWater.h)
static EnvMapVulkanWaterSettings s_WaterSettings; // this frame's, from the water entity (see ExtractWater)
static EnvMapVulkanWater s_Water;

// Glass (see EnvMapVulkanGlass): the meshes whose material is glass, drawn after the opaque meshes and the water. The
// scene pass collects them (CollectGlassDraws) and draws them sorted back to front (RecordGlassPass): each glass mesh
// shows what was drawn before it, so a farther one is drawn first. Static meshes only: a skinned mesh with a glass
// material is drawn with its material as opaque.
static EnvMapVulkanGlass s_Glass;
// The G-buffer (see EnvMapVulkanGBuffer): normals, roughness, motion and the scene's depth, written by a prepass before the
// scene pass (Phase A of docs/rendering/SSAO_GI_RayTracing_Guide.html). The scene pass loads that depth and draws the
// opaque meshes with depth EQUAL (s_MeshPipelineDepthEqual), so their PBR shader runs once per pixel.
static EnvMapVulkanGBuffer s_GBuffer;

// Screen-space ambient occlusion (Phase F of the guide): compute passes between the G-buffer prepass and the scene pass
// fill the AO image, u_AOImage in the mesh shaders' set 0 (binding 15). Settings: the Environment panel, saved with the scene.
static EnvMapVulkanSSAO s_SSAO;

// The Material Library's materials as plain values in one GPU buffer (see EnvMapVulkanMaterialBuffer): u_Materials in set 0,
// for shaders that light a point without its material's descriptor set
static EnvMapVulkanMaterialBuffer s_MaterialBuffer;

// The scene as rays see it (Phase G of the guide, see EnvMapVulkanRayTracing): acceleration structures over the meshes
// and the set a compute shader traces rays with. On a GPU without ray tracing it does nothing. Built in the frames that
// trace rays: so far the ray-traced debug views.
static EnvMapVulkanRayTracing s_RayTracing;
// Rays can be traced this frame: the GPU can, and the switch is on. What every ray-traced effect asks before it runs.
static bool IsRayTracingActive()
{
	return s_RenderSettings.RayTracing && EnvMapVulkanRayTracing::IsSupported();
}

// Ray-traced reflections (Phase H.3 of the guide, see EnvMapVulkanReflections): compute passes between the G-buffer
// prepass and the scene pass fill the reflection image, u_ReflectionImage in the mesh shaders' set 0 (binding 16).
// Without ray tracing the image stays empty and the meshes reflect the environment map, as before. Settings: the
// Environment panel, saved with the scene.
static EnvMapVulkanReflections s_Reflections;
// Reflections are traced this frame
static bool ReflectionsTraceRays()
{
	return IsRayTracingActive() && s_RenderSettings.Reflections.Enabled;
}

// The path tracer (Phase H.4 of the guide, see EnvMapVulkanPathTracer): the Debug View's Path Traced view, the scene by
// rays alone as a reference for everything above. It runs only while that view is shown.
static EnvMapVulkanPathTracer s_PathTracer;
// A number that changes whenever the scene does (OnUpdate: the models, their poses, the materials, the lights, the
// environment): the path tracer starts its image over then
static uint64_t s_PathTracerSceneVersion = 0;

// The probe volume on the GPU (see EnvMapVulkanProbes): its atlases and ProbeVolume block are in set 0 (bindings 11 to 14),
// read by IndirectDiffuse in Include/ShadeSurface.glslh
static EnvMapVulkanProbes s_Probes;
// The sun's shadow map of the whole probe volume, for the probes' capture (see RecordProbeUpdate): one orthographic map
// around the volume, drawn in the frames that update probes. The sun's cascades (s_ShadowMap) follow the camera, so they
// can't tell whether the sun reaches what a probe behind the camera sees.
static EnvMapVulkanShadowMap s_ProbeSunShadowMap;
static constexpr uint32_t ProbeSunShadowResolution = 2048;

// FNV-1a over bytes, continued from hash: for the numbers that tell whether something changed (the probes' scene version)
static constexpr uint64_t HashStart = 14695981039346656037ull;
static uint64_t HashBytes(uint64_t hash, const void* data, size_t size)
{
	const unsigned char* bytes = static_cast<const unsigned char*>(data);
	for (size_t i = 0; i < size; i++)
	{
		hash = (hash ^ bytes[i]) * 1099511628211ull;
	}
	return hash;
}
// The lights as packed for the shaders in the last frame (UpdateFrameUniforms), and whether the sun casts shadows
static uint64_t s_LightsVersion = 0;

// Points the per-frame set's probe atlas bindings to the atlases (at startup, and when the probe counts changed)
static void WriteProbeDescriptors()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	const VkDescriptorImageInfo imageInfos[3] = { s_Probes.GetIrradianceInfo(), s_Probes.GetVisibilityInfo(), s_Probes.GetProbeDataInfo() };
	const char* names[3] = { "u_ProbeIrradiance", "u_ProbeVisibility", "u_ProbeData" };
	std::array<VkWriteDescriptorSet, 3> writes;
	for (size_t i = 0; i < writes.size(); i++)
	{
		writes[i] = *pbrShader->GetDescriptorSet(names[i], H2M::VulkanShaderH2M::FrameDescriptorSet);
		writes[i].dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
		writes[i].descriptorCount = 1;
		writes[i].pImageInfo = &imageInfos[i];
	}
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

// Points the per-frame set's ambient occlusion binding to the AO image (at startup, and when the viewport was resized)
static void WriteAmbientOcclusionDescriptor()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	const VkDescriptorImageInfo imageInfo = s_SSAO.GetAOInfo();
	VkWriteDescriptorSet write = *pbrShader->GetDescriptorSet("u_AOImage", H2M::VulkanShaderH2M::FrameDescriptorSet);
	write.dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
}

// Points the per-frame set's reflection binding to the reflection image (at startup, and when the viewport was resized)
static void WriteReflectionDescriptor()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	const VkDescriptorImageInfo imageInfo = s_Reflections.GetReflectionInfo();
	VkWriteDescriptorSet write = *pbrShader->GetDescriptorSet("u_ReflectionImage", H2M::VulkanShaderH2M::FrameDescriptorSet);
	write.dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
}

// Points the per-frame set's material buffer binding to the buffer (at startup, and when a bigger buffer replaced it)
static void WriteMaterialBufferDescriptor()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	const VkDescriptorBufferInfo bufferInfo = s_MaterialBuffer.GetDescriptorInfo();
	VkWriteDescriptorSet write = *pbrShader->GetDescriptorSet("Materials", H2M::VulkanShaderH2M::FrameDescriptorSet);
	write.dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pBufferInfo = &bufferInfo;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
	s_RayTracing.SetMaterialBuffer(bufferInfo); // the ray tracing set has the same buffer
}
struct GlassDraw
{
	H2M::RefH2M<H2M::ModelH2M> Model; // keeps the buffers alive until the draw is recorded
	uint32_t MeshIndex = 0;
	glm::mat4 Transform = glm::mat4(1.0f); // the mesh's, as drawn
	H2M::RefH2M<EnvMapVulkanMaterial> Material;
	float Distance = 0.0f; // from the camera to the center of the mesh's bounds
};
static std::vector<GlassDraw> s_GlassDraws;
static EnvMapVulkanEntityID s_WaterEntity = NoEntity; // the scene's water (at most one), NoEntity: none

// The scene's water into s_WaterSettings (Enabled when the scene has one, see EnvMapVulkanSceneData::ReadWater)
static void ExtractWater()
{
	s_WaterEntity = EnvMapVulkanSceneData::ReadWater(*s_Scene, s_WaterSettings);
}

// The scene's probe volume into s_ProbeVolumeSettings (Exists when the scene has one, see EnvMapVulkanSceneData::ReadProbeVolume)
static EnvMapVulkanProbeVolumeSettings s_ProbeVolumeSettings;
static EnvMapVulkanEntityID s_ProbeVolumeEntity = NoEntity; // the scene's probe volume (at most one), NoEntity: none

static void ExtractProbeVolume()
{
	s_ProbeVolumeEntity = EnvMapVulkanSceneData::ReadProbeVolume(*s_Scene, s_ProbeVolumeSettings);
}

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

	// binding 0: Camera (vertex stage), see Include/FrameCamera.glslh: the view-projection, then the previous frame's (the
	// motion vectors of the G-buffer prepass; the first frame has no motion)
	static glm::mat4 s_PreviousViewProjection;
	static bool s_HasPreviousViewProjection = false;
	glm::mat4 viewProjection = camera.GetViewProjection();
	glm::mat4 cameraMatrices[2] = { viewProjection, s_HasPreviousViewProjection ? s_PreviousViewProjection : viewProjection };
	s_PreviousViewProjection = viewProjection;
	s_HasPreviousViewProjection = true;
	void* ubPtr = shader->MapUniformBuffer(0, frameSet);
	memcpy(ubPtr, cameraMatrices, sizeof(cameraMatrices));
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

	// binding 13: ProbeVolume (fragment stage), see EnvMapVulkanProbes
	const EnvMapVulkanProbes::ProbeVolumeUB probeVolume = s_Probes.GetUniforms();
	ubPtr = shader->MapUniformBuffer(13, frameSet);
	memcpy(ubPtr, &probeVolume, sizeof(probeVolume));
	shader->UnmapUniformBuffer(13, frameSet);

	// binding 5: Lights (fragment stage), see EnvMapVulkanLightsGPU
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

	const bool sunCastsShadows = s_Lights.Sun.CastShadows;
	s_LightsVersion = HashBytes(HashBytes(HashStart, &lights, sizeof(lights)), &sunCastsShadows, sizeof(sunCastsShadows));
}

// Per-object uniform buffers (set 2): the bone matrices of the current animation frame of a skinned model (up to 128), then
// the previous frame's for the motion vectors (previousBoneTransforms; empty: the pose didn't change). See the BoneTransforms
// block in Include/MeshVertex_Anim.glslh.
// Every model has its own shader instance (see ModelH2M::Create), so every skinned model has its own bone buffer.
static void UpdateObjectUniforms(const H2M::RefH2M<H2M::ModelH2M>& model, const std::vector<glm::mat4>& previousBoneTransforms = {})
{
	static constexpr size_t MaxBones = 128;
	H2M::RefH2M<H2M::ModelH2M> modelRef = model;
	if (!modelRef->IsSkinned() || modelRef->GetObjectDescriptorSet() == VK_NULL_HANDLE)
	{
		return;
	}

	const std::vector<glm::mat4>& boneTransforms = modelRef->GetBoneTransforms();
	size_t boneCount = std::min<size_t>(boneTransforms.size(), MaxBones);
	if (boneCount > 0)
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = modelRef->GetMeshShader().As<H2M::VulkanShaderH2M>();
		const bool hasPreviousArray = shader->GetUniformBuffer(0, H2M::VulkanShaderH2M::ObjectDescriptorSet).Size >= 2 * MaxBones * sizeof(glm::mat4);
		void* ubPtr = shader->MapUniformBuffer(0, H2M::VulkanShaderH2M::ObjectDescriptorSet);
		memcpy(ubPtr, boneTransforms.data(), boneCount * sizeof(glm::mat4));
		if (hasPreviousArray)
		{
			const std::vector<glm::mat4>& previous = previousBoneTransforms.size() == boneTransforms.size() ? previousBoneTransforms : boneTransforms;
			memcpy((uint8_t*)ubPtr + MaxBones * sizeof(glm::mat4), previous.data(), boneCount * sizeof(glm::mat4));
		}
		shader->UnmapUniformBuffer(0, H2M::VulkanShaderH2M::ObjectDescriptorSet);
	}
}

// Framebuffers, pipelines and the bounding box vertex buffer of the editor overlays (see EnvMapVulkanOverlaySettings)
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
	s_SelectionMaskDepthPipeline       = createPipeline("SelectionMaskDepth",        false, staticLayout, s_SelectionMaskFramebuffer, triangles, false, true, true);
	s_SelectionMaskDepthPipelineAnim   = createPipeline("SelectionMaskDepth-Anim",   true,  animLayout,   s_SelectionMaskFramebuffer, triangles, false, true, true);
	s_SelectionMaskVisiblePipeline     = createPipeline("SelectionMaskVisible",      false, staticLayout, s_SelectionMaskFramebuffer, triangles, false, true, false);
	s_SelectionMaskVisiblePipelineAnim = createPipeline("SelectionMaskVisible-Anim", true,  animLayout,   s_SelectionMaskFramebuffer, triangles, false, true, false);

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

	// The water's unit square (see EnvMapVulkanWaterSettings::GetTransform), as two triangles in the same layout
	const glm::vec3 square[6] = { { -0.5f, 0.0f, -0.5f }, { 0.5f, 0.0f, -0.5f }, { 0.5f, 0.0f, 0.5f }, { -0.5f, 0.0f, -0.5f }, { 0.5f, 0.0f, 0.5f }, { -0.5f, 0.0f, 0.5f } };
	LineVertex water[6] = {};
	for (uint32_t i = 0; i < 6; i++)
	{
		water[i].Position = square[i];
	}
	s_SelectionMaskWaterVertexBuffer = H2M::VertexBufferH2M::Create(water, (uint32_t)sizeof(water));
}

// Draws a loaded model (all meshes, or only mesh onlyMesh) in one color with an overlay pipeline.
// lineWidth: for the wireframe pipelines (dynamic state), 0 for the others.
static void DrawModelOverlay(VkCommandBuffer commandBuffer, EnvMapVulkanEntityID entry, int onlyMesh, const H2M::RefH2M<H2M::PipelineH2M>& staticPipeline,
	const H2M::RefH2M<H2M::PipelineH2M>& animPipeline, const glm::vec4& color, const glm::mat4& viewProjection, float lineWidth = 0.0f)
{
	H2M::RefH2M<H2M::ModelH2M> model = EnvMapVulkanSceneData::GetModel(*s_Scene, entry);
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

	glm::mat4 transform = s_Scene->GetWorldTransform(entry);
	auto& meshes = model->GetMeshes();
	for (int s = 0; s < (int)meshes.size(); s++)
	{
		if (onlyMesh >= 0 && s != onlyMesh)
		{
			continue;
		}
		glm::mat4 mvp = viewProjection * EnvMapVulkanSceneData::GetMeshTransform(model, meshes[s], transform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
		vkCmdDrawIndexed(commandBuffer, meshes[s]->IndexCount, 1, meshes[s]->BaseIndex, meshes[s]->BaseVertex, 0);
		EnvMapVulkanProfiler::CountDraw(meshes[s]->IndexCount / 3);
	}
}

// Draws a loaded model's normal, tangent or bitangent lines (vectorIndex 0, 1, 2): one line per vertex of every mesh (or
// only of mesh onlyMesh), lineLength long in world units
static void DrawModelVectors(VkCommandBuffer commandBuffer, EnvMapVulkanEntityID entry, int onlyMesh, uint32_t vectorIndex, float lineLength,
	int colorMode, const glm::mat4& viewProjection, float lineWidth)
{
	H2M::RefH2M<H2M::ModelH2M> model = EnvMapVulkanSceneData::GetModel(*s_Scene, entry);
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

	glm::mat4 transform = s_Scene->GetWorldTransform(entry);
	auto& meshes = model->GetMeshes();
	for (int s = 0; s < (int)meshes.size(); s++)
	{
		if ((onlyMesh >= 0 && s != onlyMesh) || meshes[s]->VertexCount == 0)
		{
			continue;
		}
		glm::mat4 meshMatrix = EnvMapVulkanSceneData::GetMeshTransform(model, meshes[s], transform);
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
		EnvMapVulkanProfiler::CountDraw(0);
	}
}

// Size of a loaded model as placed in the scene: the diagonal of the bounding box of all its mesh boxes (world space)
static float GetModelWorldSize(EnvMapVulkanEntityID entry)
{
	glm::vec3 boundsMin(std::numeric_limits<float>::max());
	glm::vec3 boundsMax(-std::numeric_limits<float>::max());
	glm::mat4 transform = s_Scene->GetWorldTransform(entry);
	H2M::RefH2M<H2M::ModelH2M> model = EnvMapVulkanSceneData::GetModel(*s_Scene, entry);
	for (auto& mesh : model->GetMeshes())
	{
		glm::mat4 meshTransform = EnvMapVulkanSceneData::GetMeshTransform(model, mesh, transform);
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
	const std::vector<EnvMapVulkanEntityID> models = EnvMapVulkanSceneData::GetModels(*s_Scene);
	// What of each model is selected: { -1 } the whole model, or mesh indices of its selected parts (see GetViewSelectedMeshes)
	std::map<EnvMapVulkanEntityID, std::vector<int>> selectedMeshes;
	for (EnvMapVulkanEntityID m : models)
	{
		std::vector<int> meshes = GetViewSelectedMeshes(m);
		if (!meshes.empty())
		{
			selectedMeshes[m] = std::move(meshes);
		}
	}
	auto getSelectedMeshes = [&selectedMeshes](EnvMapVulkanEntityID model) -> const std::vector<int>& {
		static const std::vector<int> s_None;
		auto it = selectedMeshes.find(model);
		return it != selectedMeshes.end() ? it->second : s_None;
	};
	const EnvMapVulkanOverlaySettings& settings = s_ViewOptions.Overlays;

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
		EnvMapVulkanProfiler::CountRenderPass(renderPassBeginInfo.renderArea.extent.width, renderPassBeginInfo.renderArea.extent.height);

		VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		VkRect2D scissor = { { 0, 0 }, { width, height } };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	};
	auto inScope = [&getSelectedMeshes](int scope, EnvMapVulkanEntityID model)
	{
		return scope == OverlayScopeAll || (scope == OverlayScopeSelected && !getSelectedMeshes(model).empty());
	};
	// The meshes of a model in scope to draw: "Selected" its selected meshes (-1: the whole model), "All" the whole model
	auto meshesInScope = [&getSelectedMeshes](int scope, EnvMapVulkanEntityID model)
	{
		return scope == OverlayScopeSelected ? getSelectedMeshes(model) : std::vector<int>{ -1 };
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
			for (EnvMapVulkanEntityID entry : models)
			{
				DrawModelOverlay(commandBuffer, entry, -1, s_OverlayDepthPipeline, s_OverlayDepthPipelineAnim, glm::vec4(0.0f), viewProjection);
			}
		}

		if (settings.Wireframe != OverlayScopeOff)
		{
			for (EnvMapVulkanEntityID m : models)
			{
				if (inScope(settings.Wireframe, m))
				{
					// "Selected": the selected meshes, or the whole model when it is selected (like the selection outline)
					for (int onlyMesh : meshesInScope(settings.Wireframe, m))
					{
						DrawModelOverlay(commandBuffer, m, onlyMesh, s_WireframePipeline, s_WireframePipelineAnim, settings.WireframeColor,
							depthBias * viewProjection, lineWidth);
					}
				}
			}
		}

		if (showVectors)
		{
			const bool show[3] = { settings.ShowNormals, settings.ShowTangents, settings.ShowBitangents };
			for (EnvMapVulkanEntityID m : models)
			{
				if (!inScope(settings.Vectors, m))
				{
					continue;
				}
				// "Selected": the selected meshes, or the whole model when it is selected (like the wireframe)
				float length = GetModelWorldSize(m) * settings.VectorLength * 0.01f;
				for (int onlyMesh : meshesInScope(settings.Vectors, m))
				{
					for (uint32_t v = 0; v < 3; v++)
					{
						if (show[v])
						{
							DrawModelVectors(commandBuffer, m, onlyMesh, v, length, settings.VectorColorMode, depthBias * viewProjection, lineWidth);
						}
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

			for (EnvMapVulkanEntityID m : models)
			{
				if (!inScope(settings.BoundingBoxes, m))
				{
					continue;
				}
				H2M::RefH2M<H2M::ModelH2M> model = EnvMapVulkanSceneData::GetModel(*s_Scene, m);
				glm::mat4 transform = s_Scene->GetWorldTransform(m);
				auto& meshes = model->GetMeshes();
				const std::vector<int>& modelSelection = getSelectedMeshes(m);
				const bool wholeModelSelected = Contains(modelSelection, -1);
				for (int s = 0; s < (int)meshes.size(); s++)
				{
					// The selection (the selected meshes, or the whole model when it is selected) in its own color
					const bool selected = wholeModelSelected || Contains(modelSelection, s);
					// "Selected": the selected meshes' boxes, or all boxes of a selected model (like the wireframe)
					if (settings.BoundingBoxes == OverlayScopeSelected && !selected)
					{
						continue;
					}

					// Each mesh's box in its own space, so it turns with the model (as in SceneHazelEnvMap)
					const H2M::AABB_H2M& box = meshes[s]->BoundingBox;
					glm::mat4 mvp = viewProjection * EnvMapVulkanSceneData::GetMeshTransform(model, meshes[s], transform) *
						glm::translate(glm::mat4(1.0f), box.Min) * glm::scale(glm::mat4(1.0f), box.Max - box.Min);

					glm::vec4 color = selected ? settings.SelectedBoundingBoxColor : settings.BoundingBoxColor;

					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
					vkCmdDraw(commandBuffer, s_BoundingBoxVertexCount, 1, 0, 0);
					EnvMapVulkanProfiler::CountDraw(0);
				}
			}
		}

		// The probe volume's box, outlined while the volume is selected (it has no surface of its own to see). The
		// overlay has its own depth, so the outline shows through the scene.
		if (s_ProbeVolumeSettings.Exists && s_ProbeVolumeEntity != NoEntity && s_ViewOptions.PrimarySelection == s_ProbeVolumeEntity)
		{
			H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_BoundingBoxPipeline.As<H2M::VulkanPipelineH2M>();
			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
			VkBuffer vertexBuffer = s_BoundingBoxVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());
			vkCmdSetLineWidth(commandBuffer, lineWidth);

			// The pipeline's box is the unit cube from 0 to 1
			const glm::mat4 mvp = viewProjection * glm::translate(glm::mat4(1.0f), s_ProbeVolumeSettings.GetOrigin()) *
				glm::scale(glm::mat4(1.0f), s_ProbeVolumeSettings.Size);
			const glm::vec4 color(0.75f, 0.51f, 1.0f, 1.0f); // the probe volume's color in the Scene Hierarchy
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
			vkCmdDraw(commandBuffer, s_BoundingBoxVertexCount, 1, 0, 0);
			EnvMapVulkanProfiler::CountDraw(0);
		}

		// The probes' moves, while their balls show their state (Show Probes, Show = State): a line from each moved
		// probe's place in the grid to where it stands now. Through the scene, as the outline above: the grid place of
		// a probe that was moved out of a wall is inside that wall.
		if (s_Probes.ShowsSpheres() && s_ProbeVolumeSettings.Show == EnvMapVulkanProbeVolumeSettings::ShowState)
		{
			H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_BoundingBoxPipeline.As<H2M::VulkanPipelineH2M>();
			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
			VkBuffer vertexBuffer = s_BoundingBoxVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());
			vkCmdSetLineWidth(commandBuffer, lineWidth);

			for (uint32_t probe = 0; probe < s_Probes.GetProbeCount(); probe++)
			{
				glm::vec3 onGrid, position;
				bool inUse = true;
				if (!s_Probes.GetProbeMove(probe, onGrid, position, inUse))
				{
					continue;
				}
				// The pipeline's unit cube with its y and z sides folded away: what is left of its edges is the line
				// from the cube's origin along its x side
				glm::mat4 line(0.0f);
				line[0] = glm::vec4(position - onGrid, 0.0f);
				line[3] = glm::vec4(onGrid, 1.0f);
				const glm::mat4 mvp = viewProjection * line;
				const glm::vec4 color = inUse ? glm::vec4(1.0f, 0.72f, 0.1f, 1.0f) : glm::vec4(0.8f, 0.08f, 0.08f, 1.0f); // as the balls
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
				vkCmdDraw(commandBuffer, s_BoundingBoxVertexCount, 1, 0, 0);
				EnvMapVulkanProfiler::CountDraw(0);
			}
		}
	}
	vkCmdEndRenderPass(commandBuffer);

	// Silhouette of the selection: the selected mesh, or the whole model when no mesh is selected. Red: all of it, also
	// where something is in front of it (the outline shows where a hidden model is); green: the part that isn't hidden.
	// The composite draws the outline of the hidden part faintly (see SelectionOutline in ViewportComposite.glsl).
	beginPass(s_SelectionMaskFramebuffer);
	if (settings.Outline && !selectedMeshes.empty())
	{
		// The depth of what can hide the selection (color alpha 0 leaves the image unchanged): every model, without its
		// glass (it shows what is behind it), and the water's surface, flat (without its waves and swell): what is under
		// it is tinted or lost in the water's color
		for (EnvMapVulkanEntityID entry : models)
		{
			const H2M::RefH2M<H2M::ModelH2M> model = EnvMapVulkanSceneData::GetModel(*s_Scene, entry);
			const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> materials = EnvMapVulkanSceneData::GetMeshMaterials(*s_Scene, entry);
			const int meshCount = (int)model->GetMeshes().size();
			bool hasGlass = false;
			for (int s = 0; s < meshCount && s < (int)materials.size(); s++)
			{
				hasGlass = hasGlass || (materials[s] && materials[s]->IsGlass() && !model->IsSkinned()); // skinned glass is drawn opaque
			}
			if (!hasGlass)
			{
				DrawModelOverlay(commandBuffer, entry, -1, s_SelectionMaskDepthPipeline, s_SelectionMaskDepthPipelineAnim, glm::vec4(0.0f), viewProjection);
				continue;
			}
			for (int s = 0; s < meshCount; s++)
			{
				if (s >= (int)materials.size() || !materials[s] || !materials[s]->IsGlass())
				{
					DrawModelOverlay(commandBuffer, entry, s, s_SelectionMaskDepthPipeline, s_SelectionMaskDepthPipelineAnim, glm::vec4(0.0f), viewProjection);
				}
			}
		}
		if (s_WaterSettings.Enabled)
		{
			H2M::RefH2M<H2M::VulkanPipelineH2M> pipeline = s_SelectionMaskDepthPipeline.As<H2M::VulkanPipelineH2M>();
			VkPipelineLayout layout = pipeline->GetVulkanPipelineLayout();
			VkBuffer vertexBuffer = s_SelectionMaskWaterVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			const glm::mat4 mvp = viewProjection * s_WaterSettings.GetTransform();
			const glm::vec4 color(0.0f);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVulkanPipeline());
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
			vkCmdDraw(commandBuffer, 6, 1, 0, 0);
			EnvMapVulkanProfiler::CountDraw(2);
		}
		// All of the selection, then (over it) its part that passes the depth test
		for (const auto& [model, meshes] : selectedMeshes)
		{
			for (int mesh : meshes)
			{
				DrawModelOverlay(commandBuffer, model, mesh, s_SelectionMaskPipeline, s_SelectionMaskPipelineAnim, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), viewProjection);
			}
		}
		for (const auto& [model, meshes] : selectedMeshes)
		{
			for (int mesh : meshes)
			{
				DrawModelOverlay(commandBuffer, model, mesh, s_SelectionMaskVisiblePipeline, s_SelectionMaskVisiblePipelineAnim, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), viewProjection);
			}
		}
	}
	vkCmdEndRenderPass(commandBuffer);
}

/**** BEGIN to be removed from VulkanRenderer ****/
void EnvMapVulkanRenderer::SubmitModelTemp(const H2M::RefH2M<H2M::ModelH2M>& model, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials,
	const std::vector<glm::mat4>& previousMeshTransforms)
{
	// Temporary code - populate selected mesh
	// std::vector<Submesh> submeshes = mesh->GetMeshes();
	// s_SelectedSubmesh = &submeshes.at(0);

	s_SubmittedModels.push_back({ model, transform, materials, (uint32_t)s_PreviousDrawTransforms.size() });
	H2M::RefH2M<H2M::ModelH2M> modelRef = model;
	const auto& meshes = modelRef->GetMeshes();
	for (size_t s = 0; s < meshes.size(); s++)
	{
		s_PreviousDrawTransforms.push_back(s < previousMeshTransforms.size() ? previousMeshTransforms[s] : EnvMapVulkanSceneData::GetMeshTransform(modelRef, meshes[s], transform));
	}

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
	s_SpotShadowMaps.Create(s_RenderSettings.LocalShadows.SpotResolution, MaxShadowedSpotLights);
	if (H2M::VulkanContextH2M::GetCurrentDevice()->GetEnabledFeatures().imageCubeArray)
	{
		s_PointShadowMaps.Create(s_RenderSettings.LocalShadows.PointResolution, 6 * MaxShadowedPointLights, true);
	}
	else
	{
		s_PointShadowMaps.Destroy();
		Log::GetLogger()->warn("Point light shadows are not available: the GPU doesn't support cube map arrays (imageCubeArray)");
	}
	Log::GetLogger()->info("Local light shadow maps: {0} spot lights at {1} x {1}, {2} point lights at {3} x {3} per cube face",
		MaxShadowedSpotLights, s_RenderSettings.LocalShadows.SpotResolution, s_PointShadowMaps.IsValid() ? MaxShadowedPointLights : 0,
		s_RenderSettings.LocalShadows.PointResolution);
}

void EnvMapVulkanRenderer::Init()
{
	/**** BEGIN: to be removed from VulkanRenderer ****/
	// H2M::RendererH2M::Submit([=]() {});
	{
		s_UserInterfaceCommandBuffer = H2M::VulkanContextH2M::GetCurrentDevice()->CreateSecondaryCommandBuffer();
		s_CompositeCommandBuffer = H2M::VulkanContextH2M::GetCurrentDevice()->CreateSecondaryCommandBuffer();
	}
	EnvMapVulkanProfiler::Init();
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
		framebufferSpec.LoadDepth = true;  // the G-buffer prepass writes the depth (see s_GBuffer)
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
				s_ViewportTextureNeedsUpdate = true; // re-register the new image (see RegisterViewportImage)

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

		// The scene pass: only the surfaces the G-buffer prepass kept in front (depth EQUAL, no write), so the PBR shader
		// runs once per pixel. Same shaders, so the layouts match the pipelines above.
		pipelineSpecification.DepthEqual = true;
		pipelineSpecification.DepthWrite = false;
		pipelineSpecification.DebugName = "PBR-Anim-DepthEqual";
		s_MeshPipelineAnimDepthEqual = H2M::PipelineH2M::Create(pipelineSpecification);
		pipelineSpecification.Layout = s_MeshPipeline->GetSpecification().Layout;
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");
		pipelineSpecification.DebugName = "PBR-Static-DepthEqual";
		s_MeshPipelineDepthEqual = H2M::PipelineH2M::Create(pipelineSpecification);
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
			s_ViewportTextureNeedsUpdate = true; // re-register the new image (see RegisterViewportImage)
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
	s_Glass.Create(s_Framebuffer);
	s_GBuffer.Create(s_Framebuffer);
	s_GBuffer.CreatePipelines(s_MeshPipeline, s_MeshPipelineAnim);
	s_SSAO.Create();
	s_SSAO.Resize(s_GBuffer, s_RenderSettings.AmbientOcclusion.HalfResolution);
	s_Reflections.Create();
	s_Reflections.Resize(s_GBuffer, s_RenderSettings.Reflections.HalfResolution);
	s_RayTracing.Create();
	s_RayTracing.ResizeView(s_GBuffer.GetWidth(), s_GBuffer.GetHeight());
	s_PathTracer.Create();
	s_PathTracer.Resize(s_GBuffer.GetWidth(), s_GBuffer.GetHeight());
	WriteBloomDescriptorSets(); // the viewport composite samples the G-buffer and the ray-traced views (debug views)

	// The scene (EnvMapVulkanScene): its operations are checked once at startup
	EnvMapVulkanScene::SelfTest();

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

	CreateShadowMap(s_RenderSettings.Shadows.Resolution, false); // the descriptor is written with the rest of set 0, below
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
		const char* bufferNames[] = { "Camera", "SceneData", "Lights", "Shadows", "ProbeVolume" };
		const uint32_t bufferBindings[] = { 0, 1, 5, 7, 13 };
		std::array<VkWriteDescriptorSet, 5> writes;
		for (size_t i = 0; i < writes.size(); i++)
		{
			writes[i] = *pbrShader->GetDescriptorSet(bufferNames[i], frameSet);
			writes[i].dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
			writes[i].descriptorCount = 1;
			writes[i].pBufferInfo = &pbrShader->GetUniformBuffer(bufferBindings[i], frameSet).Descriptor;
		}
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);

		WriteShadowMapDescriptor();

		// binding 17: the material buffer
		s_MaterialBuffer.Create();
		WriteMaterialBufferDescriptor();

		// bindings 11, 12, 14: the probe volume's atlases (binding 13, its uniform buffer, is written above)
		s_Probes.Create(s_Framebuffer);
		WriteProbeDescriptors();
		// The probes' own shadow map of the sun, in the capture's descriptor set
		s_ProbeSunShadowMap.Create(ProbeSunShadowResolution, 1);
		s_Probes.SetCaptureSunShadowMap({ s_ProbeSunShadowMap.GetCompareSampler(), s_ProbeSunShadowMap.GetArrayView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

		// binding 15: the ambient occlusion image (the water's reflection pass has none: a white texel)
		WriteAmbientOcclusionDescriptor();
		s_Water.SetReflectionAmbientOcclusion(s_SSAO.GetWhiteInfo());

		// binding 16: the ray-traced reflection image (the water's reflection pass has none: an empty texel)
		WriteReflectionDescriptor();
		s_Water.SetReflectionRayTracedReflections(s_Reflections.GetNoneInfo());

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

	s_Data.SceneData.SkyboxLod = 0.0f;

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
	s_Glass.Destroy();
	s_MaterialBuffer.Destroy();
	s_RayTracing.Destroy();
	s_Probes.Destroy();
	s_ProbeSunShadowMap.Destroy();
	s_SSAO.Destroy();
	s_Reflections.Destroy();
	s_PathTracer.Destroy();
	s_GBuffer.Destroy();
	EnvMapVulkanProfiler::Shutdown();
	H2M::VulkanShaderH2M::ClearUniformBuffers();

	// The file's static references, released here while the device works: as statics they would be destroyed after
	// main returns, when Vulkan (its validation layer) may already be torn down
	s_Scene = nullptr;
	EnvMapVulkanMaterialLibrary::Clear();
	s_SubmittedModels.clear();
	s_GlassDraws.clear();

	s_Framebuffer = {};
	s_CompositeFramebuffer = {};
	s_CompositePipeline = {};
	s_ViewportCompositeFramebuffer = {};
	s_ViewportCompositePipeline = {};
	for (auto& framebuffer : s_BloomDownFramebuffers) framebuffer = {};
	for (auto& framebuffer : s_BloomUpFramebuffers) framebuffer = {};
	s_BloomPipeline = {};
	s_BloomDirtTexture = {};
	s_GridPipeline = {};
	s_OverlayFramebuffer = {};
	s_SelectionMaskFramebuffer = {};
	s_OverlayDepthPipeline = {};
	s_OverlayDepthPipelineAnim = {};
	s_WireframePipeline = {};
	s_WireframePipelineAnim = {};
	s_BoundingBoxPipeline = {};
	s_SelectionMaskPipeline = {};
	s_SelectionMaskPipelineAnim = {};
	s_SelectionMaskDepthPipeline = {};
	s_SelectionMaskDepthPipelineAnim = {};
	s_SelectionMaskVisiblePipeline = {};
	s_SelectionMaskVisiblePipelineAnim = {};
	s_SelectionMaskWaterVertexBuffer = {};
	s_VectorsPipeline = {};
	s_VectorsPipelineAnim = {};
	s_BoundingBoxVertexBuffer = {};
	s_MeshPipeline = {};
	s_MeshPipelineAnim = {};
	s_MeshPipelineDepthEqual = {};
	s_MeshPipelineAnimDepthEqual = {};
	s_EnvMapCompute = EnvMapComputeResources();
	s_Data = VulkanRendererData();
}

void EnvMapVulkanRenderer::RenderModelVulkan(H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials, VkCommandBuffer commandBuffer,
	MeshPass pass, uint32_t firstDraw)
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
	 **** END Non-composite ****/
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

	// The pipelines of all passes have the PBR pipelines' layouts: everything bound below is valid for each of them
	VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline(); // MeshPass::WaterReflection
	if (pass == MeshPass::GBuffer)
	{
		pipeline = s_GBuffer.GetPipeline(skinned);
	}
	else if (pass == MeshPass::Scene)
	{
		pipeline = (skinned ? s_MeshPipelineAnimDepthEqual : s_MeshPipelineDepthEqual).As<H2M::VulkanPipelineH2M>()->GetVulkanPipeline();
	}
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
		if (material->IsGlass() && !skinned)
		{
			continue; // drawn after the opaque meshes and the water (see RecordGlassPass)
		}
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
		glm::mat4 meshTransform = EnvMapVulkanSceneData::GetMeshTransform(model, mesh, transform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &meshTransform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
		// The prepass reads the mesh's previous transform per instance: the instance index picks it (see EnvMapVulkanGBuffer)
		uint32_t firstInstance = pass == MeshPass::GBuffer ? firstDraw + (uint32_t)s : 0;
		vkCmdDrawIndexed(commandBuffer, mesh->IndexCount, 1, mesh->BaseIndex, mesh->BaseVertex, firstInstance);
		EnvMapVulkanProfiler::CountDraw(mesh->IndexCount / 3);
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
		glm::vec4 ShadowDebug;       // Shadows Only: x = 1 sun, 2 point, 3 spot, 4 light off (0: normal view); y = index in the packed lights;
		                             // z = debug view: 1 the direct light only, 2 the indirect light only, 3 diffuse surface shading
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
	ub.NormalBias = s_RenderSettings.Shadows.NormalBias;
	ub.Softness = s_RenderSettings.Shadows.Softness;
	ub.ShowCascades = s_ViewOptions.ShowCascades ? 1.0f : 0.0f;
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
	int shadowsOnlyIndex = 0;
	const EnvMapVulkanSceneData::LightKind shadowsOnlyKind = GetShadowsOnlyLight(shadowsOnlyIndex);
	if (s_ViewOptions.ShadowsOnly && shadowsOnlyKind != EnvMapVulkanSceneData::LightKind::None)
	{
		if (shadowsOnlyKind == EnvMapVulkanSceneData::LightKind::Sun)
		{
			ub.ShadowDebug = glm::vec4(s_Lights.Sun.Enabled ? 1.0f : 4.0f, 0.0f, 0.0f, 0.0f);
		}
		else
		{
			const bool point = shadowsOnlyKind == EnvMapVulkanSceneData::LightKind::Point;
			int packedIndex = 0;
			bool enabled = false;
			for (int i = 0; i <= shadowsOnlyIndex; i++)
			{
				bool on = point ? s_Lights.PointLights[i].Enabled : s_Lights.SpotLights[i].Enabled;
				if (i == shadowsOnlyIndex)
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
	ub.ShadowDebug.z = s_ViewOptions.DebugView >= DebugViewDirectLight && s_ViewOptions.DebugView <= DebugViewReflectionOcclusion ? (float)(s_ViewOptions.DebugView - DebugViewDirectLight + 1) : 0.0f;
	if (s_ViewOptions.DebugView >= DebugViewAO && s_ViewOptions.DebugView <= DebugViewAORaw)
	{
		ub.ShadowDebug.z = 6.0f; // the AO image: which of its steps it holds is the ambient occlusion's matter (see its Record)
	}
	if (IsReflectionDebugView(s_ViewOptions.DebugView))
	{
		ub.ShadowDebug.z = 7.0f; // the reflection image, likewise
	}
	ub.LocalShadowParams = glm::vec4(s_RenderSettings.LocalShadows.NormalBias, s_RenderSettings.LocalShadows.Softness,
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
			glm::mat4 transform = EnvMapVulkanSceneData::GetMeshTransform(submitted.Model, mesh, submitted.Transform);
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
	EnvMapVulkanProfiler::CountRenderPass(beginInfo.renderArea.extent.width, beginInfo.renderArea.extent.height);

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

		const auto& meshes = model->GetMeshes();
		const auto& materials = s_SubmittedModels[m].Materials;
		for (size_t s = 0; s < meshes.size(); s++)
		{
			const H2M::RefH2M<H2M::MeshH2M>& mesh = meshes[s];
			// Glass lets the light through, unless its material says it casts a shadow (as an opaque mesh would)
			if (!skinned && s < materials.size() && materials[s]->IsGlass() &&
				materials[s]->GetGlassValue(EnvMapVulkanMaterial::GlassCastShadows) < 0.5f)
			{
				continue;
			}
			glm::mat4 matrices[2] = { viewProjection, EnvMapVulkanSceneData::GetMeshTransform(model, mesh, s_SubmittedModels[m].Transform) };
			vkCmdPushConstants(commandBuffer, pipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), matrices);
			vkCmdDrawIndexed(commandBuffer, mesh->IndexCount, 1, mesh->BaseIndex, mesh->BaseVertex, 0);
			EnvMapVulkanProfiler::CountDraw(mesh->IndexCount / 3);
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

	ComputeShadowCascades(cameraPosition, cameraForward, cornerRays, camera.GetPerspectiveNearClip(), s_RenderSettings.Shadows, s_Lights.Sun.GetDirection(),
		boundsMin, boundsMax, s_ShadowCascades);

	EnvMapVulkanProfiler::Scope sunScope(commandBuffer, "Sun Shadows", "The sun's shadow map: the scene's depth seen from the sun, one cascade "
		"at a time (each covers a farther part of the view, at a lower density). Depth only, no color: the meshes' "
		"fragment shaders do almost nothing, so the cost is mostly vertices.");
	for (uint32_t cascade = 0; cascade < ShadowCascadeCount; cascade++)
	{
		EnvMapVulkanProfiler::Scope cascadeScope(commandBuffer, "Cascade " + std::to_string(cascade), "One layer of the sun's shadow map: every "
			"shadow caster drawn from the sun into it, with the cascade's light view and projection.");
		BeginShadowPass(commandBuffer, s_ShadowMap, cascade, s_RenderSettings.Shadows.DepthBias, s_RenderSettings.Shadows.SlopeBias);
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
	const EnvMapVulkanLocalShadowSettings& settings = s_RenderSettings.LocalShadows;

	if (slots.SpotCount > 0)
	{
		EnvMapVulkanProfiler::Scope spotScope(commandBuffer, "Spot Light Shadows", "A shadow map per shadow-casting spot light: depth seen "
			"from the light through its cone (one perspective view). Only the models within the light's range are drawn.");
		for (uint32_t slot = 0; slot < slots.SpotCount; slot++)
		{
			const EnvMapVulkanSpotLight& light = s_Lights.SpotLights[slots.SpotLight[slot]];
			EnvMapVulkanProfiler::Scope lightScope(commandBuffer, light.Name.empty() ? "Spot Light " + std::to_string(slot) : light.Name,
				"One spot light's shadow map (one render pass).");
			BeginShadowPass(commandBuffer, s_SpotShadowMaps, slot, settings.DepthBias, settings.SlopeBias);
			slots.ModelsDrawn += DrawShadowCasters(commandBuffer, slots.SpotViewProjection[slot], &light.Position, light.Range);
			vkCmdEndRenderPass(commandBuffer);
		}
	}

	if (slots.PointCount > 0)
	{
		EnvMapVulkanProfiler::Scope pointScope(commandBuffer, "Point Light Shadows", "A cube shadow map per shadow-casting point light: it "
			"shines in every direction, so the scene is drawn 6 times, once per cube face (90 degree views). The most "
			"expensive light to shadow.");
		for (uint32_t slot = 0; slot < slots.PointCount; slot++)
		{
			const EnvMapVulkanPointLight& light = s_Lights.PointLights[slots.PointLight[slot]];
			EnvMapVulkanProfiler::Scope lightScope(commandBuffer, light.Name.empty() ? "Point Light " + std::to_string(slot) : light.Name,
				"One point light's cube shadow map: 6 render passes, one per face.");
			for (uint32_t face = 0; face < 6; face++)
			{
				BeginShadowPass(commandBuffer, s_PointShadowMaps, slot * 6 + face, settings.DepthBias, settings.SlopeBias);
				slots.ModelsDrawn += DrawShadowCasters(commandBuffer, slots.PointFaceViewProjection[slot][face], &light.Position, light.Range);
				vkCmdEndRenderPass(commandBuffer);
			}
		}
	}
}

// Whether a box can be seen through a face of a cube map drawn around apex (face 0 to 5: +X, -X, +Y, -Y, +Z, -Z). A face
// sees the pyramid of directions whose largest coordinate is along its axis; the box is tested against the pyramid's four
// sides, so some boxes near its edges pass without being seen.
static bool BoxTouchesCubeFace(const glm::vec3& boxMin, const glm::vec3& boxMax, const glm::vec3& apex, uint32_t face)
{
	const int axis = (int)face / 2;
	const glm::vec3 low = boxMin - apex, high = boxMax - apex;
	// The farthest the box reaches along the face's direction
	const float along = face % 2 == 0 ? high[axis] : -low[axis];
	if (along <= 0.0f)
	{
		return false;
	}
	for (int other = 0; other < 3; other++)
	{
		// The sides: along >= p[other] and along >= -p[other]
		if (other != axis && (along < low[other] || along < -high[other]))
		{
			return false;
		}
	}
	return true;
}

// The probe volume's update of this frame (see EnvMapVulkanProbes): the probes of the batch capture the scene around
// them, then compute passes blend what they saw into the atlases. Recorded after the lights' shadow maps and the caustics
// (the captured surfaces are lit through them) and before everything that is lit by the probes.
static void RecordProbeUpdate(VkCommandBuffer commandBuffer)
{
	if (!s_Probes.HasCapture())
	{
		return;
	}
	EnvMapVulkanProfiler::Scope updateScope(commandBuffer, "Probe Update", "The probe volume's global illumination: this frame's probes "
		"look at the scene around them and take the light they see into their stored light. Only while something changed "
		"(or during a bake): when the probes are up to date this pass is gone.");

	// With ray tracing: no shadow map, no cube maps, no draw calls. Each probe's rays are traced against the scene's
	// acceleration structures (built earlier this frame), batch after batch.
	if (s_Probes.TracesRays())
	{
		EnvMapVulkanProfiler::Scope traceScope(commandBuffer, "Probe Trace", "Compute passes, per batch of up to 32 probes: every probe's "
			"rays are traced against the scene (ray queries) and bring back the light of what they hit, with shadow rays "
			"to the lights; then the rays are mixed into the probe's stored light and into how far it sees. No draw calls: "
			"the cost is rays, so many more probes fit into a frame than with cube maps.");
		const bool sunShadows = s_Lights.Sun.Enabled && s_Lights.Sun.CastShadows && s_Lights.Sun.Intensity > 0.0f;
		s_Probes.RecordTraced(commandBuffer, s_RayTracing.GetDescriptorSet(), sunShadows);
		return;
	}

	// The sun's shadow map of the whole volume: an orthographic view from the sun, as wide as a sphere around the volume
	// (so its size doesn't depend on the sun's direction) and deep enough for every shadow caster of the scene
	const bool sunShadows = s_ProbeSunShadowMap.IsValid() && s_Lights.Sun.Enabled && s_Lights.Sun.CastShadows && s_Lights.Sun.Intensity > 0.0f &&
		s_ShadowPipeline.Pipeline && s_ShadowPipelineAnim.Pipeline && !s_SubmittedModels.empty();
	glm::mat4 sunViewProjection(1.0f);
	float sunTexelWorldSize = 0.0f;
	if (sunShadows)
	{
		const glm::vec3 towardsSun = s_Lights.Sun.GetDirection();
		const glm::vec3 center = s_ProbeVolumeSettings.Center;
		const float radius = 0.5f * glm::length(s_ProbeVolumeSettings.Size) * 1.25f;
		// How far the casters and the sphere reach towards the sun and away from it, from the center
		float towards = radius, away = -radius;
		for (int c = 0; c < 8; c++)
		{
			const glm::vec3 corner((c & 1) ? s_ShadowCasterBoundsMax.x : s_ShadowCasterBoundsMin.x, (c & 2) ? s_ShadowCasterBoundsMax.y : s_ShadowCasterBoundsMin.y,
				(c & 4) ? s_ShadowCasterBoundsMax.z : s_ShadowCasterBoundsMin.z);
			const float distance = glm::dot(corner - center, towardsSun);
			towards = std::max(towards, distance);
			away = std::min(away, distance);
		}
		const float eyeDistance = towards + 1.0f;
		const glm::vec3 up = std::abs(towardsSun.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::mat4 view = glm::lookAt(center + towardsSun * eyeDistance, center, up);
		sunViewProjection = glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, eyeDistance - away + 1.0f) * view;
		sunTexelWorldSize = 2.0f * radius / (float)ProbeSunShadowResolution;

		EnvMapVulkanProfiler::Scope sunScope(commandBuffer, "Probe Sun Shadow", "The sun's shadow map of the whole probe volume: every shadow "
			"caster drawn once from the sun. The scene's own cascades only cover what the camera sees, and a probe also sees "
			"what is behind the camera.");
		BeginShadowPass(commandBuffer, s_ProbeSunShadowMap, 0, s_RenderSettings.Shadows.DepthBias, s_RenderSettings.Shadows.SlopeBias);
		DrawShadowCasters(commandBuffer, sunViewProjection);
		vkCmdEndRenderPass(commandBuffer);
	}
	s_Probes.SetCaptureSun(sunViewProjection, sunShadows, sunTexelWorldSize, ProbeSunShadowResolution, s_RenderSettings.Shadows.NormalBias);

	// The meshes the probes see: the opaque ones (glass lets the light through; on a skinned mesh it is drawn as opaque, as
	// in the scene pass). The water's surface is left out.
	struct CaptureDraw
	{
		SubmittedModel* Submitted = nullptr;
		uint32_t MeshIndex = 0;
		glm::vec4 TransformRows[3];
		float MaterialIndex = 0.0f;
		bool Cull = false; // by its bounds (not a skinned mesh: its bones move it out of them)
		glm::vec3 BoundsMin = glm::vec3(0.0f), BoundsMax = glm::vec3(0.0f); // in the world
	};
	std::vector<CaptureDraw> draws;
	for (SubmittedModel& submitted : s_SubmittedModels)
	{
		const bool skinned = submitted.Model->IsSkinned();
		if (skinned && submitted.Model->GetObjectDescriptorSet() == VK_NULL_HANDLE)
		{
			continue; // no bone buffer: the skinned vertices can't be placed
		}
		if (skinned && s_Probes.IsRelocating())
		{
			continue; // the probes are placed by what stands still: a probe isn't moved out of a model that walks on
		}
		const auto& meshes = submitted.Model->GetMeshes();
		for (uint32_t s = 0; s < (uint32_t)meshes.size() && s < (uint32_t)submitted.Materials.size(); s++)
		{
			if (submitted.Materials[s]->IsGlass() && !skinned)
			{
				continue;
			}
			const glm::mat4 transform = EnvMapVulkanSceneData::GetMeshTransform(submitted.Model, meshes[s], submitted.Transform);
			const glm::mat4 rows = glm::transpose(transform);
			CaptureDraw draw;
			draw.Submitted = &submitted;
			draw.MeshIndex = s;
			draw.TransformRows[0] = rows[0];
			draw.TransformRows[1] = rows[1];
			draw.TransformRows[2] = rows[2];
			draw.MaterialIndex = submitted.Materials[s]->Get<float>("u_MaterialUniforms.MaterialIndex");
			draw.Cull = !skinned;
			draw.BoundsMin = glm::vec3(std::numeric_limits<float>::max());
			draw.BoundsMax = glm::vec3(-std::numeric_limits<float>::max());
			for (int c = 0; c < 8; c++)
			{
				const glm::vec3 corner((c & 1) ? meshes[s]->BoundingBox.Max.x : meshes[s]->BoundingBox.Min.x, (c & 2) ? meshes[s]->BoundingBox.Max.y : meshes[s]->BoundingBox.Min.y,
					(c & 4) ? meshes[s]->BoundingBox.Max.z : meshes[s]->BoundingBox.Min.z);
				const glm::vec3 world = glm::vec3(transform * glm::vec4(corner, 1.0f));
				draw.BoundsMin = glm::min(draw.BoundsMin, world);
				draw.BoundsMax = glm::max(draw.BoundsMax, world);
			}
			draws.push_back(draw);
		}
	}

	{
		EnvMapVulkanProfiler::Scope captureScope(commandBuffer, "Probe Capture", "What each probe of this frame's batch sees: the meshes drawn "
			"into a small cube map around it (6 render passes of 32 x 32 pixels per probe), lit as matte surfaces by the "
			"lights and by the probes' own stored light. The cost is mostly draw calls: every mesh a face can see, per face.");
		const VkDescriptorSet sets[2] = { s_Data.FrameDescriptorSet.DescriptorSets[0], s_Probes.GetCaptureSet() };
		for (uint32_t slot = 0; slot < s_Probes.GetCaptureCount(); slot++)
		{
			if (!s_Probes.IsCaptureSlotUsed(slot))
			{
				continue; // its probe is switched off
			}
			const glm::vec3 probePosition = s_Probes.GetCaptureProbePosition(slot);
			for (uint32_t face = 0; face < 6; face++)
			{
				EnvMapVulkanProbes::CapturePush push;
				push.FaceViewProjection = s_Probes.BeginCaptureFace(commandBuffer, slot, face);
				// Sets 0 and 1 stay bound across the static and the skinned pipeline (their layouts agree in them)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Probes.GetCaptureLayout(false), 0, 2, sets, 0, nullptr);

				const SubmittedModel* boundModel = nullptr;
				for (const CaptureDraw& draw : draws)
				{
					if (draw.Cull && !BoxTouchesCubeFace(draw.BoundsMin, draw.BoundsMax, probePosition, face))
					{
						continue;
					}
					H2M::RefH2M<H2M::ModelH2M> model = draw.Submitted->Model;
					const bool skinned = model->IsSkinned();
					const VkPipelineLayout layout = s_Probes.GetCaptureLayout(skinned);
					if (boundModel != draw.Submitted)
					{
						vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Probes.GetCapturePipeline(skinned));
						if (skinned)
						{
							VkDescriptorSet boneDescriptorSet = model->GetObjectDescriptorSet();
							vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, H2M::VulkanShaderH2M::ObjectDescriptorSet, 1,
								&boneDescriptorSet, 0, nullptr);
						}
						VkBuffer vertexBuffer = model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
						VkDeviceSize offset = 0;
						vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
						vkCmdBindIndexBuffer(commandBuffer, H2M::RefH2M<H2M::VulkanIndexBufferH2M>(model->GetIndexBuffer())->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);
						boundModel = draw.Submitted;
					}
					push.TransformRows[0] = draw.TransformRows[0];
					push.TransformRows[1] = draw.TransformRows[1];
					push.TransformRows[2] = draw.TransformRows[2];
					push.ProbePositionMaterial = glm::vec4(probePosition, draw.MaterialIndex);
					vkCmdPushConstants(commandBuffer, layout, EnvMapVulkanProbes::CapturePushStages, 0, sizeof(push), &push);

					const H2M::RefH2M<H2M::MeshH2M>& mesh = model->GetMeshes()[draw.MeshIndex];
					vkCmdDrawIndexed(commandBuffer, mesh->IndexCount, 1, mesh->BaseIndex, mesh->BaseVertex, 0);
					EnvMapVulkanProfiler::CountDraw(mesh->IndexCount / 3);
				}
				s_Probes.EndCaptureFace(commandBuffer);
			}
		}
	}

	EnvMapVulkanProfiler::Scope blendScope(commandBuffer, "Probe Blend", "Compute passes: each probe's cube map is read in the directions of "
		"its rays, and the rays are mixed into the probe's stored light and into how far it sees (keeping part of the old "
		"values, so the light stays calm). No draw calls.");
	s_Probes.RecordBlend(commandBuffer);
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
	EnvMapVulkanProfiler::CountDraw(s_Data.VulkanSkyboxCube->m_IndexCount / 3);
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

// The glass meshes of the submitted models, sorted back to front (see s_GlassDraws)
static void CollectGlassDraws()
{
	s_GlassDraws.clear();
	const glm::vec3 cameraPosition = s_Data.SceneData.SceneCamera.Camera.GetPosition();
	for (const SubmittedModel& submitted : s_SubmittedModels)
	{
		if (submitted.Model->IsSkinned())
		{
			continue; // drawn as opaque (see RenderModelVulkan)
		}
		const auto& meshes = submitted.Model->GetMeshes();
		for (uint32_t s = 0; s < (uint32_t)meshes.size() && s < (uint32_t)submitted.Materials.size(); s++)
		{
			if (!submitted.Materials[s]->IsGlass())
			{
				continue;
			}
			GlassDraw draw;
			draw.Model = submitted.Model;
			draw.MeshIndex = s;
			draw.Transform = EnvMapVulkanSceneData::GetMeshTransform(submitted.Model, meshes[s], submitted.Transform);
			draw.Material = submitted.Materials[s];
			const glm::vec3 center = glm::vec3(draw.Transform * glm::vec4((meshes[s]->BoundingBox.Min + meshes[s]->BoundingBox.Max) * 0.5f, 1.0f));
			draw.Distance = glm::length(center - cameraPosition);
			s_GlassDraws.push_back(draw);
		}
	}
	std::sort(s_GlassDraws.begin(), s_GlassDraws.end(), [](const GlassDraw& a, const GlassDraw& b) { return a.Distance > b.Distance; });
}

// The glass meshes, inside the scene pass after the water: the pass ends, the scene is copied (EnvMapVulkanGlass::CopyScene),
// and the framebuffer's continue render pass takes over for the glass and what comes after it
static void RecordGlassPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo& sceneBeginInfo, const VkViewport& viewport, const VkRect2D& scissor)
{
	vkCmdEndRenderPass(commandBuffer);
	{
		EnvMapVulkanProfiler::Scope copyScope(commandBuffer, "Glass Copy", "The render pass ends and the scene so far (the water too) is "
			"copied for the glass to look through, then shrunk step by step into smaller copies (a mip chain): rough glass "
			"reads a smaller, blurrier one. Blits, no draw calls.");
		s_Glass.CopyScene(commandBuffer, s_Framebuffer);
	}

	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = s_Framebuffer.As<H2M::VulkanFramebufferH2M>();
	VkRenderPassBeginInfo continueBeginInfo = sceneBeginInfo;
	continueBeginInfo.renderPass = framebuffer->GetContinueRenderPass();
	continueBeginInfo.framebuffer = framebuffer->GetContinueVulkanFramebuffer();
	continueBeginInfo.clearValueCount = 0;
	continueBeginInfo.pClearValues = nullptr;
	vkCmdBeginRenderPass(commandBuffer, &continueBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	EnvMapVulkanProfiler::CountRenderPass(continueBeginInfo.renderArea.extent.width, continueBeginInfo.renderArea.extent.height);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	EnvMapVulkanProfiler::Scope glassScope(commandBuffer, "Glass", "The glass meshes, farthest first: each reads the scene copy where its "
		"refraction bends the view, tints it, blurs it by its roughness, and adds the environment's reflection and the "
		"lights' highlights (more at grazing angles). Glass behind glass isn't in the copy, so it isn't seen through.");
	H2M::RefH2M<H2M::VulkanPipelineH2M> pipeline = s_Glass.GetPipeline().As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = pipeline->GetVulkanPipelineLayout();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVulkanPipeline());
	// Set 0 again: the glass layout's push constants differ from the mesh pipelines', so their sets aren't "compatible"
	// (the set itself is: Glass_Static.glsl declares set 0 as the PBR shaders do)
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
		s_Data.FrameDescriptorSet.DescriptorSets.data(), 0, nullptr);
	VkDescriptorSet copySet = s_Glass.GetSceneCopySet();
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, EnvMapVulkanGlass::SceneCopySet, 1, &copySet, 0, nullptr);

	H2M::ModelH2M* boundModel = nullptr;
	for (GlassDraw& draw : s_GlassDraws)
	{
		VkDescriptorSet materialSet = draw.Material->GetDescriptorSet();
		if (materialSet == VK_NULL_HANDLE)
		{
			continue;
		}
		if (draw.Model.Raw() != boundModel)
		{
			VkBuffer vertexBuffer = draw.Model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
			vkCmdBindIndexBuffer(commandBuffer, H2M::RefH2M<H2M::VulkanIndexBufferH2M>(draw.Model->GetIndexBuffer())->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);
			boundModel = draw.Model.Raw();
		}
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, H2M::VulkanShaderH2M::MaterialDescriptorSet, 1,
			&materialSet, 0, nullptr);

		EnvMapVulkanMaterial& material = *draw.Material;
		GlassPushConstants values;
		values.TintColor = material.Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
		values.IOR = material.GetGlassValue(EnvMapVulkanMaterial::GlassIOR);
		values.Roughness = material.Get<float>("u_MaterialUniforms.Roughness");
		values.Thickness = material.GetGlassValue(EnvMapVulkanMaterial::GlassThickness);
		values.AlbedoTexToggle = material.Get<float>("u_MaterialUniforms.AlbedoTexToggle");
		values.NormalTexToggle = material.Get<float>("u_MaterialUniforms.NormalTexToggle");
		values.RoughnessTexToggle = material.Get<float>("u_MaterialUniforms.RoughnessTexToggle");
		values.TilingFactor = material.Get<float>("u_MaterialUniforms.TilingFactor");
		values.EmissiveTexToggle = material.Get<float>("u_MaterialUniforms.EmissiveTexToggle");
		values.EmissiveIntensity = material.Get<float>("u_MaterialUniforms.EmissiveIntensity");
		values.MetalRoughPacked = material.Get<float>("u_MaterialUniforms.MetalRoughPacked");
		values.SceneCopyLevels = (float)s_Glass.GetCopyLevels();
		values.Solid = material.GetGlassValue(EnvMapVulkanMaterial::GlassSolid);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &draw.Transform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(values), &values);

		const H2M::RefH2M<H2M::MeshH2M>& mesh = draw.Model->GetMeshes()[draw.MeshIndex];
		vkCmdDrawIndexed(commandBuffer, mesh->IndexCount, 1, mesh->BaseIndex, mesh->BaseVertex, 0);
		EnvMapVulkanProfiler::CountDraw(mesh->IndexCount / 3);
	}
	s_GlassDraws.clear();

	// The mesh pipelines' set 0 again, for what comes after (the grid binds its own sets)
	VkPipelineLayout meshLayout = s_MeshPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout();
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
		s_Data.FrameDescriptorSet.DescriptorSets.data(), 0, nullptr);
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
		EnvMapVulkanProfiler::BeginFrame(drawCommandBuffer, swapChain.GetCurrentBufferIndex());

		// The shadow maps first (the sun's, then the spot and point lights'): the geometry pass below samples them
		glm::vec3 casterBoundsMin, casterBoundsMax;
		ComputeShadowCasterBounds(casterBoundsMin, casterBoundsMax);
		RecordShadowPasses(drawCommandBuffer, casterBoundsMin, casterBoundsMax);
		RecordLocalShadowPasses(drawCommandBuffer);
		if (s_ShadowMapViewerRequest.Active)
		{
			const EnvMapVulkanShadowMap& map = s_ShadowMapViewerRequest.Cube ? s_PointShadowMaps : s_SpotShadowMaps;
			EnvMapVulkanProfiler::Scope viewerScope(drawCommandBuffer, "Shadow Map Viewer", "The Lights panel's preview of a spot or point "
				"light's shadow map, turned into a visible image (only while the preview is shown).");
			s_ShadowMapViewer.Record(drawCommandBuffer, map, s_ShadowMapViewerRequest.Cube, s_ShadowMapViewerRequest.BaseLayer,
				s_ShadowMapViewerRequest.Near, s_ShadowMapViewerRequest.Far);
			s_ShadowMapViewerRequest.Active = false; // requested again by the panel while it shows the viewer
		}

		// The scene for the rays, in the frames that trace any (the probes' update, a ray-traced debug view): the
		// acceleration structures follow the meshes as they are drawn this frame. Then the debug view, when one is shown.
		const bool probesTraceRays = s_Probes.HasCapture() && s_Probes.TracesRays();
		const bool occlusionTracesRays = s_RenderSettings.AmbientOcclusion.Enabled && s_RenderSettings.AmbientOcclusion.RayTraced;
		if (IsRayTracingActive() && (probesTraceRays || occlusionTracesRays || ReflectionsTraceRays() || IsRayTracedDebugView(s_ViewOptions.DebugView) ||
			s_ViewOptions.DebugView == DebugViewPathTraced))
		{
			{
				EnvMapVulkanProfiler::Scope sceneScope(drawCommandBuffer, "Ray Tracing Scene", "The scene as rays see it: the skinned models' "
					"vertices moved by their bones (a compute pass) and their acceleration structures refitted, then the top-level "
					"structure built from every mesh's transform. Static meshes' structures are built once, when a model is loaded.");
				s_RayTracing.BeginFrame();
				std::vector<glm::mat4> meshTransforms;
				for (const SubmittedModel& submitted : s_SubmittedModels)
				{
					meshTransforms.clear();
					for (const H2M::RefH2M<H2M::MeshH2M>& mesh : submitted.Model->GetMeshes())
					{
						meshTransforms.push_back(EnvMapVulkanSceneData::GetMeshTransform(submitted.Model, mesh, submitted.Transform));
					}
					s_RayTracing.Submit(submitted.Model, meshTransforms, submitted.Materials);
				}
				s_RayTracing.Record(drawCommandBuffer);
			}
		}
		if (IsRayTracingActive() && IsRayTracedDebugView(s_ViewOptions.DebugView))
		{
			EnvMapVulkanProfiler::Scope viewScope(drawCommandBuffer, "Ray Traced View", "The ray-traced debug view: a compute pass traces one "
				"ray per pixel from the camera (a ray query) and writes what it hit as a color.");
			const EnvMapVulkanRayTracing::View view = s_ViewOptions.DebugView == DebugViewRayTracedAlbedo ? EnvMapVulkanRayTracing::View::Albedo :
				(s_ViewOptions.DebugView == DebugViewRayTracedInstances ? EnvMapVulkanRayTracing::View::Instances : EnvMapVulkanRayTracing::View::Normals);
			s_RayTracing.RecordView(drawCommandBuffer, view, s_Data.SceneData.SceneCamera.Camera.GetViewProjection());
		}
		if (IsRayTracingActive() && s_ViewOptions.DebugView == DebugViewPathTraced)
		{
			EnvMapVulkanProfiler::Scope pathScope(drawCommandBuffer, "Path Tracer", "The Path Traced debug view: a compute pass follows a few "
				"random light paths per pixel through all their bounces (rays against the scene, a shadow ray per light at every "
				"hit) and adds them to the average of the paths so far. Nothing once the image has its Max Samples.");
			H2M::CameraH2M& pathCamera = s_Data.SceneData.SceneCamera.Camera;
			const bool sunShadows = s_Lights.Sun.Enabled && s_Lights.Sun.CastShadows && s_Lights.Sun.Intensity > 0.0f;
			s_PathTracer.Record(drawCommandBuffer, s_ViewOptions.PathTracer, pathCamera.GetViewProjection(), pathCamera.GetProjectionMatrix(),
				s_PathTracerSceneVersion, s_Data.SceneData.SkyboxLod, s_RayTracing.GetDescriptorSet(), sunShadows);
		}

		// The water's caustics: the meshes under the water (in the reflection and the scene pass) are lit through them
		if (s_WaterSettings.Enabled)
		{
			EnvMapVulkanProfiler::Scope causticsScope(drawCommandBuffer, "Water Caustics", "The light pattern the water's waves focus onto "
				"what's under it: a grid of rays from the sun bent by the surface, added up into a texture the mesh shaders sample.");
			s_Water.RecordCaustics(drawCommandBuffer);
		}

		// The probe volume: this frame's batch of probes captures the scene and is blended into the atlases
		RecordProbeUpdate(drawCommandBuffer);

		// The water's planar reflection: the meshes seen by the camera mirrored in the water plane, into the water's
		// reflection image (with the usual pipelines; its own per-frame set holds the mirrored camera)
		if (s_WaterSettings.Enabled && s_Water.IsReflectionActive())
		{
			EnvMapVulkanProfiler::BeginScope(drawCommandBuffer, "Water Reflection", "The scene mirrored in the water plane, drawn into its "
				"own image with the usual mesh pipelines (a second camera below the water). Costs about as much as the scene's "
				"meshes again, at the reflection image's size.");
			VkDescriptorSet reflectionFrameSet = s_Water.BeginReflectionPass(drawCommandBuffer);
			VkPipelineLayout reflectionLayout = s_MeshPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout();
			vkCmdBindDescriptorSets(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, reflectionLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
				&reflectionFrameSet, 0, nullptr);
			for (const SubmittedModel& submitted : s_SubmittedModels)
			{
				RenderModelVulkan(submitted.Model, submitted.Transform, submitted.Materials, drawCommandBuffer, MeshPass::WaterReflection);
			}
			s_Water.EndReflectionPass(drawCommandBuffer);
			EnvMapVulkanProfiler::EndScope(drawCommandBuffer);
		}

		// The G-buffer prepass: the opaque meshes' normals, roughness, motion and depth, before the scene pass
		{
			EnvMapVulkanProfiler::Scope prepassScope(drawCommandBuffer, "G-Buffer Prepass", "The surface data of every pixel (normal, "
				"roughness, motion, depth) for the effects that need it before or after the lighting, such as ambient occlusion: "
				"the opaque meshes drawn once more, with a cheap shader that only reads their normal and roughness maps.");
			s_GBuffer.SetPreviousTransforms(s_PreviousDrawTransforms);
			s_GBuffer.BeginPass(drawCommandBuffer);
			EnvMapVulkanProfiler::CountRenderPass(s_GBuffer.GetWidth(), s_GBuffer.GetHeight());
			VkPipelineLayout prepassLayout = s_MeshPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout();
			vkCmdBindDescriptorSets(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prepassLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
				s_Data.FrameDescriptorSet.DescriptorSets.data(), 0, nullptr);
			for (const SubmittedModel& submitted : s_SubmittedModels)
			{
				RenderModelVulkan(submitted.Model, submitted.Transform, submitted.Materials, drawCommandBuffer, MeshPass::GBuffer, submitted.FirstDraw);
			}
			s_GBuffer.EndPass(drawCommandBuffer);
		}

		// The ambient occlusion, from the G-buffer just drawn, for the scene pass's mesh shaders
		{
			EnvMapVulkanProfiler::Scope occlusionScope(drawCommandBuffer, "Ambient Occlusion", "How much of the light from all around reaches "
				"each pixel's surface past the geometry close to it: compute passes at half the viewport's size (linear depth, the "
				"occlusion, a blur that keeps edges, the average over the frames), then one at full size. The occlusion itself: "
				"with ray tracing a few short rays per pixel against the scene, otherwise from the G-buffer's depth (GTAO).");
			H2M::CameraH2M& occlusionCamera = s_Data.SceneData.SceneCamera.Camera;
			const EnvMapVulkanSSAO::Stage stage = s_ViewOptions.DebugView == DebugViewAORaw ? EnvMapVulkanSSAO::Stage::Raw :
				(s_ViewOptions.DebugView == DebugViewAODenoised ? EnvMapVulkanSSAO::Stage::Denoised : EnvMapVulkanSSAO::Stage::Final);
			// Traced when rays can be traced this frame (the scene for them was built above)
			s_SSAO.Record(drawCommandBuffer, s_RenderSettings.AmbientOcclusion, s_GBuffer, occlusionCamera.GetViewMatrix(), occlusionCamera.GetProjectionMatrix(), stage,
				IsRayTracingActive() ? s_RayTracing.GetDescriptorSet() : VK_NULL_HANDLE);
		}

		// The ray-traced reflections, from the G-buffer too, for the scene pass's mesh shaders. Without ray tracing (or
		// with the setting off) nothing is traced and the reflection image is empty: the environment's reflection.
		{
			std::optional<EnvMapVulkanProfiler::Scope> reflectionScope;
			if (ReflectionsTraceRays())
			{
				reflectionScope.emplace(drawCommandBuffer, "Reflections", "Ray-traced reflections: a ray per pixel (at half the viewport's size, "
					"or at its full size) leaves the surface as a reflection does and brings back the light of what it hits, lit "
					"with shadow rays and the probes; then the average over the frames, and a pass to the viewport's size that "
					"blurs by the surface's roughness. Surfaces above the max roughness get no ray.");
			}
			H2M::CameraH2M& reflectionCamera = s_Data.SceneData.SceneCamera.Camera;
			const bool sunShadows = s_Lights.Sun.Enabled && s_Lights.Sun.CastShadows && s_Lights.Sun.Intensity > 0.0f;
			s_Reflections.Record(drawCommandBuffer, s_RenderSettings.Reflections, s_GBuffer, reflectionCamera.GetViewProjection(), reflectionCamera.GetProjectionMatrix(),
				reflectionCamera.GetPosition(), s_ViewOptions.DebugView == DebugViewReflectionsRaw ? EnvMapVulkanReflections::Stage::Raw : EnvMapVulkanReflections::Stage::Final,
				ReflectionsTraceRays() ? s_RayTracing.GetDescriptorSet() : VK_NULL_HANDLE, sunShadows);
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

		EnvMapVulkanProfiler::BeginScope(drawCommandBuffer, "Scene", "The main view, in HDR (16-bit float color): the skybox, the meshes "
			"with full PBR lighting (image-based light, the sun, point and spot lights, shadows), then the water and the grid.");
		vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		EnvMapVulkanProfiler::CountRenderPass(renderPassBeginInfo.renderArea.extent.width, renderPassBeginInfo.renderArea.extent.height);

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

		{
			EnvMapVulkanProfiler::Scope skyboxScope(drawCommandBuffer, "Skybox", "The environment map on a cube around the camera: the "
				"background wherever no mesh is drawn (only those pixels are shaded: the prepass's depth hides the rest).");
			EnvMapVulkanRenderer::RenderSkybox(drawCommandBuffer); // in progress
		}

		// Set 0 (per frame: camera, scene data, environment maps) is bound once, for all meshes
		VkPipelineLayout meshPipelineLayout = s_MeshPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout();
		vkCmdBindDescriptorSets(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
			s_Data.FrameDescriptorSet.DescriptorSets.data(), 0, nullptr);

		{
			EnvMapVulkanProfiler::Scope meshesScope(drawCommandBuffer, "Opaque Meshes", "Every mesh of every model, one draw call per mesh, "
				"with its material: the PBR fragment shader runs once for each covered pixel (the depth test keeps only the surface "
				"the G-buffer prepass found in front). Usually the biggest part of the frame.");
			for (const SubmittedModel& submitted : s_SubmittedModels)
			{
				RenderModelVulkan(submitted.Model, submitted.Transform, submitted.Materials, drawCommandBuffer, MeshPass::Scene);
			}
		}
		if (s_Probes.ShowsSpheres())
		{
			EnvMapVulkanProfiler::Scope probesScope(drawCommandBuffer, "Probe Balls", "The probe volume's probes shown as balls (Show Probes): "
				"one camera-facing square per probe, cut to a ball and lit by its probe in the fragment shader.");
			s_Probes.RecordSpheres(drawCommandBuffer, s_Data.FrameDescriptorSet.DescriptorSets[0], s_Data.SceneData.SceneCamera.Camera.GetViewMatrix());
		}
		CollectGlassDraws();

		s_SubmittedModels.clear();
		s_PreviousDrawTransforms.clear();

		// The water: it looks into the opaque scene drawn so far. The render pass ends, its color and depth are copied
		// for the water to read, and the framebuffer's continue render pass (which loads the attachments instead of
		// clearing them) takes over for the water and the rest.
		if (s_WaterSettings.Enabled)
		{
			vkCmdEndRenderPass(drawCommandBuffer);
			{
				EnvMapVulkanProfiler::Scope copyScope(drawCommandBuffer, "Scene Copy", "The render pass ends and the scene's color and depth are "
					"copied, so the water can read what's behind and under it (a shader can't read the image it is drawing into). "
					"A plain memory copy: no draw calls.");
				s_Water.CopyScene(drawCommandBuffer, s_Framebuffer);
			}

			VkRenderPassBeginInfo continueBeginInfo = renderPassBeginInfo;
			continueBeginInfo.renderPass = framebuffer->GetContinueRenderPass();
			continueBeginInfo.framebuffer = framebuffer->GetContinueVulkanFramebuffer();
			continueBeginInfo.clearValueCount = 0;
			continueBeginInfo.pClearValues = nullptr;
			vkCmdBeginRenderPass(drawCommandBuffer, &continueBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			EnvMapVulkanProfiler::CountRenderPass(continueBeginInfo.renderArea.extent.width, continueBeginInfo.renderArea.extent.height);
			vkCmdSetViewport(drawCommandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(drawCommandBuffer, 0, 1, &scissor);

			EnvMapVulkanProfiler::BeginScope(drawCommandBuffer, "Water Volume", "Underwater fog and the water seen from the side: a full-screen "
				"triangle that rewrites every pixel from the scene copy, darkening what's under the water by its depth.");
			s_Water.RecordVolume(drawCommandBuffer, s_Data.FrameDescriptorSet.DescriptorSets[0]); // underwater fog, the water seen from the side
			EnvMapVulkanProfiler::EndScope(drawCommandBuffer);
			// The water's wireframe: its own toggle (Water panel), or the editor's (Environment panel, Wireframe: All, or
			// Selected while the water is selected), with the editor's line width
			EnvMapVulkanWaterSettings waterDrawSettings = s_WaterSettings;
			waterDrawSettings.Wireframe = s_WaterSettings.Wireframe || s_ViewOptions.Overlays.Wireframe == OverlayScopeAll ||
				(s_ViewOptions.Overlays.Wireframe == OverlayScopeSelected && s_WaterEntity != NoEntity && s_ViewOptions.PrimarySelection == s_WaterEntity);
			const float maxLineWidth = H2M::VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetProperties().limits.lineWidthRange[1];
			EnvMapVulkanProfiler::Scope surfaceScope(drawCommandBuffer, "Water Surface", "The water's mesh, moved by the swell: refraction from "
				"the scene copy, the planar reflection, foam and the sun's highlight, mixed by the Fresnel factor. Transparent "
				"without blending: it computes what's behind it.");
			s_Water.Record(drawCommandBuffer, s_Data.FrameDescriptorSet.DescriptorSets[0], waterDrawSettings,
				glm::clamp(s_ViewOptions.Overlays.LineWidth, 1.0f, maxLineWidth));
		}

		// Glass: it looks into everything drawn so far, the water too
		if (!s_GlassDraws.empty())
		{
			RecordGlassPass(drawCommandBuffer, renderPassBeginInfo, viewport, scissor);
		}

		// Transparent, so after the opaque meshes and the water
		if (s_ViewOptions.Grid)
		{
			EnvMapVulkanProfiler::Scope gridScope(drawCommandBuffer, "Grid", "The editor's ground grid: one quad, blended over the scene "
				"without writing depth.");
			RenderGrid(drawCommandBuffer);
		}

		vkCmdEndRenderPass(drawCommandBuffer);
		EnvMapVulkanProfiler::EndScope(drawCommandBuffer); // Scene
		s_GBuffer.MakeDepthReadable(drawCommandBuffer); // the viewport composite samples it (the Linear Depth debug view)

		if (s_RenderSettings.Bloom.Enabled || !s_BloomChainRendered)
		{
			EnvMapVulkanProfiler::Scope bloomScope(drawCommandBuffer, "Bloom", "The glow around bright light: the brightest parts of the "
				"scene are blurred by shrinking them step by step to small images and growing them back (a mip chain), one "
				"full-screen quad per step.");
			RecordBloomPasses(drawCommandBuffer);
		}
		{
			EnvMapVulkanProfiler::Scope overlayScope(drawCommandBuffer, "Editor Overlay", "Editor drawings that aren't part of the scene: "
				"wireframes, normals, bounding boxes, and the selection's silhouette (a mask the composite turns into the outline).");
			RecordEditorOverlayPasses(drawCommandBuffer);
		}
		{
			EnvMapVulkanProfiler::Scope compositeScope(drawCommandBuffer, "Viewport Composite", "The HDR scene becomes the image you see: "
				"bloom and lens dirt added, exposure, tonemapping to the screen's range, the overlay and the selection outline on top.");
			ViewportCompositePass(drawCommandBuffer);
		}
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
	} settings = { s_ViewOptions.GridScale, s_ViewOptions.GridSize };
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(GridSettings), &settings);

	vkCmdDrawIndexed(commandBuffer, s_Data.QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
	EnvMapVulkanProfiler::CountDraw(s_Data.QuadIndexBuffer->GetCount() / 3);
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
	// The G-buffer, for the debug views (created after the bloom resources: Init writes the sets again once it exists)
	const VkDescriptorImageInfo gbufferImages[3] = { s_GBuffer.GetNormalRoughnessInfo(), s_GBuffer.GetMotionInfo(), s_GBuffer.GetDepthInfo() };
	const char* gbufferBindings[3] = { "u_GBufferNormalRoughness", "u_GBufferMotion", "u_GBufferDepth" };
	for (uint32_t i = 0; i < 3 && s_GBuffer.IsValid(); i++)
	{
		VkWriteDescriptorSet write = *compositeShader->GetDescriptorSet(gbufferBindings[i]);
		write.dstSet = s_ViewportCompositeDescriptorSet.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = &gbufferImages[i];
		writes.push_back(write);
	}
	// The ray-traced debug views' image (black without ray tracing)
	const VkDescriptorImageInfo rayTracedView = s_RayTracing.GetViewInfo();
	if (rayTracedView.imageView)
	{
		VkWriteDescriptorSet write = *compositeShader->GetDescriptorSet("u_RayTracedView");
		write.dstSet = s_ViewportCompositeDescriptorSet.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = &rayTracedView;
		writes.push_back(write);
	}

	// The path tracer's image (a black texel without ray tracing)
	const VkDescriptorImageInfo pathTracedImage = s_PathTracer.GetImageInfo();
	if (pathTracedImage.imageView)
	{
		VkWriteDescriptorSet write = *compositeShader->GetDescriptorSet("u_PathTracedImage");
		write.dstSet = s_ViewportCompositeDescriptorSet.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = &pathTracedImage;
		writes.push_back(write);
	}

	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

// Prefilter, downsample and upsample passes (BloomPass.glsl), each a fullscreen quad drawn into one level of the chain.
// Every render pass ends with its image in SHADER_READ_ONLY layout, ready for the next pass to sample.
static void RecordBloomPasses(VkCommandBuffer commandBuffer)
{
	if (!s_RenderSettings.Bloom.Enabled && s_BloomChainRendered)
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
	float knee = std::max(s_RenderSettings.Bloom.Knee, 1.0e-4f);
	float threshold = s_RenderSettings.Bloom.Threshold;
	uniforms.Params = { threshold, threshold - knee, knee * 2.0f, 0.25f / knee };
	uniforms.UpsampleScale = s_RenderSettings.Bloom.UpsampleScale;
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
		EnvMapVulkanProfiler::CountRenderPass(renderPassBeginInfo.renderArea.extent.width, renderPassBeginInfo.renderArea.extent.height);

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
		EnvMapVulkanProfiler::CountDraw(s_Data.QuadIndexBuffer->GetCount() / 3);
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
	EnvMapVulkanProfiler::CountRenderPass(renderPassBeginInfo.renderArea.extent.width, renderPassBeginInfo.renderArea.extent.height);

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
		float DebugView;
		float Padding;
		glm::vec4 DebugParams;
	} uniforms;
	static_assert(sizeof(CompositeUniforms) == 64, "layout mismatch with the Uniforms block of ViewportComposite.glsl");
	uniforms.Exposure = s_Exposure * (s_AutoExposureEnabled ? s_EnvMapAutoExposure : 1.0f);
	uniforms.BloomIntensity = s_RenderSettings.Bloom.Enabled ? s_RenderSettings.Bloom.Intensity : 0.0f;
	uniforms.BloomDirtIntensity = (s_RenderSettings.Bloom.Enabled && s_RenderSettings.Bloom.DirtEnabled) ? s_RenderSettings.Bloom.DirtIntensity : 0.0f;
	uniforms.OutlineWidth = s_ViewOptions.Overlays.Outline ? s_ViewOptions.Overlays.OutlineWidth : 0.0f;
	uniforms.OutlineColor = s_ViewOptions.Overlays.OutlineColor;
	uniforms.HuePreservation = s_TonemapHuePreservation;
	int shadowsOnlyIndex = 0;
	uniforms.RawScene = s_ViewOptions.ShadowsOnly && GetShadowsOnlyLight(shadowsOnlyIndex) != EnvMapVulkanSceneData::LightKind::None ? 1.0f : 0.0f;
	// The G-buffer views have the numbers of ViewportComposite.glsl's GBufferView (the lighting views are the PBR shaders')
	const glm::mat4& projection = s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix();
	uniforms.DebugView = s_ViewOptions.DebugView >= DebugViewNormals && s_ViewOptions.DebugView <= DebugViewMotion ? (float)s_ViewOptions.DebugView : 0.0f;
	if (IsRayTracedDebugView(s_ViewOptions.DebugView))
	{
		uniforms.DebugView = 5.0f; // the ray-traced view's image, whichever of the views it holds
	}
	if (s_ViewOptions.DebugView == DebugViewPathTraced)
	{
		uniforms.DebugView = 6.0f; // the path tracer's image in the scene's place (exposed and tonemapped like it)
	}
	uniforms.Padding = 0.0f;
	uniforms.DebugParams = glm::vec4(projection[2][2], projection[3][2], s_ViewOptions.DebugViewDepthRange, s_ViewOptions.DebugViewMotionScale);
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositeUniforms), &uniforms);

	// Scene, bloom, lens dirt, overlay and selection mask images (rewritten when the framebuffers are resized, see WriteBloomDescriptorSets)
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, s_ViewportCompositeDescriptorSet.DescriptorSets.data(), 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, s_Data.QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
	EnvMapVulkanProfiler::CountDraw(s_Data.QuadIndexBuffer->GetCount() / 3);

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

		// No pipeline statistics query here: its secondary command buffers can't run while one is active
		EnvMapVulkanProfiler::BeginScope(drawCommandBuffer, "Window + UI", "The window's image: the viewport image and every ImGui panel "
			"(text, buttons, this one too) drawn into the swapchain image that goes to the screen.", false);
		vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		EnvMapVulkanProfiler::CountRenderPass(renderPassBeginInfo.renderArea.extent.width, renderPassBeginInfo.renderArea.extent.height);
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
		EnvMapVulkanProfiler::CountDraw(s_Data.QuadIndexBuffer->GetCount() / 3);

		VK_CHECK_RESULT_H2M(vkEndCommandBuffer(commandBuffer));

		/**** END CompositeRenderPass(inheritanceInfo) ****/

		commandBuffers.push_back(s_CompositeCommandBuffer);

		RecordUserInterface(inheritanceInfo, commandBuffers);

		vkCmdExecuteCommands(drawCommandBuffer, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkCmdEndRenderPass(drawCommandBuffer);
		EnvMapVulkanProfiler::EndScope(drawCommandBuffer); // Window + UI
		EnvMapVulkanProfiler::EndFrame(drawCommandBuffer);

		VK_CHECK_RESULT_H2M(vkEndCommandBuffer(drawCommandBuffer));
	}
}

// The window's user interface (SetUserInterface), recorded by its owner into a secondary command buffer of the window's
// render pass, after the composite of the scene. Without one the window shows only the scene.
void EnvMapVulkanRenderer::RecordUserInterface(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers)
{
	if (!s_RecordUserInterface)
	{
		return;
	}

	H2M::VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();
	uint32_t width = swapChain.GetWidth();
	uint32_t height = swapChain.GetHeight();

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	cmdBufInfo.pInheritanceInfo = &inheritanceInfo;
	VK_CHECK_RESULT_H2M(vkBeginCommandBuffer(s_UserInterfaceCommandBuffer, &cmdBufInfo));

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = (float)height;
	viewport.height = -(float)height;
	viewport.width = (float)width;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(s_UserInterfaceCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.width = width;
	scissor.extent.height = height;
	vkCmdSetScissor(s_UserInterfaceCommandBuffer, 0, 1, &scissor);

	s_RecordUserInterface(s_UserInterfaceCommandBuffer);

	VK_CHECK_RESULT_H2M(vkEndCommandBuffer(s_UserInterfaceCommandBuffer));
	commandBuffers.push_back(s_UserInterfaceCommandBuffer);
}

// A frame without a scene (the player before it opens one): the window is cleared. Every frame records the window's
// command buffer, which the swapchain submits and presents.
static void RecordEmptyFrame()
{
	H2M::VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();
	VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_CHECK_RESULT_H2M(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));

	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
	renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();
	renderPassBeginInfo.renderArea.extent.width = swapChain.GetWidth();
	renderPassBeginInfo.renderArea.extent.height = swapChain.GetHeight();
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(drawCommandBuffer);

	VK_CHECK_RESULT_H2M(vkEndCommandBuffer(drawCommandBuffer));
}

void EnvMapVulkanRenderer::Draw(H2M::CameraH2M* camera)
{
	if (!s_Scene)
	{
		RecordEmptyFrame(); // no scene yet (see SetScene)
		return;
	}
	s_RenderSettings = s_OwnerRenderSettings ? *s_OwnerRenderSettings : EnvMapVulkanRenderSettings();
	s_ViewOptions = s_OwnerViewOptions ? *s_OwnerViewOptions : EnvMapVulkanViewOptions();

	// This frame's environment, lights, water and probe volume, from the scene (see ExtractEnvironment, ExtractLights,
	// ExtractWater, ExtractProbeVolume)
	ExtractEnvironment();
	ExtractLights();
	ExtractWater();
	ExtractProbeVolume();

	// Shadow map resolutions that differ from the settings (changed in the panel, a scene opened): the maps are recreated
	// before any command buffer of this frame is recorded (their descriptors in set 0 can't change while a recorded frame
	// uses them)
	if (s_RenderSettings.Shadows.Resolution != s_ShadowMap.GetResolution())
	{
		vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
		CreateShadowMap(s_RenderSettings.Shadows.Resolution, true);
	}
	const EnvMapVulkanLocalShadowSettings& localShadows = s_RenderSettings.LocalShadows;
	if (localShadows.SpotResolution != s_SpotShadowMaps.GetResolution() ||
		(s_PointShadowMaps.IsValid() && localShadows.PointResolution != s_PointShadowMaps.GetResolution()))
	{
		vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
		CreateLocalShadowMaps();
		WriteShadowMapDescriptor();
	}

	// The environment map the scene asks for (the panel, a file dropped, a scene opened) is loaded here, before any command
	// buffer of this frame is recorded, not in the middle of the ImGui pass that requested it
	UpdateEnvironmentMap();

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
	// Set when a change below may leave cached textures unused (a material's inherited map replaced)
	bool texturesMayBeUnused = false;
	// Material variants get their parents' inherited values and maps (after every change above), and an inherited
	// property edited since the last frame becomes an override
	if (EnvMapVulkanMaterialLibrary::ResolveVariants([] { vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice()); }))
	{
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

	// The scene into the frame: the parts' transforms into their meshes, then every model with its materials. For the motion
	// vectors each model is submitted with where its meshes and bones were in the previous frame (a model that's new, or
	// was replaced, didn't move).
	struct ModelMotionState
	{
		const H2M::ModelH2M* Model = nullptr;
		std::vector<glm::mat4> MeshTransforms; // GetMeshTransform of each mesh
		std::vector<glm::mat4> BoneTransforms; // skinned models
	};
	static std::unordered_map<EnvMapVulkanEntityID, ModelMotionState> s_PreviousModelStates;
	std::unordered_map<EnvMapVulkanEntityID, ModelMotionState> modelStates; // this frame's: deleted models drop out

	// For the probe volume: numbers that change whenever something its probes capture changed. Apart: what moves by
	// itself (the skinned models), which the probes follow more calmly than an edit (see EnvMapVulkanProbes).
	// And what decides where the probes can stand: the models that stand still, where their meshes are and which
	// materials they have (glass isn't captured).
	uint64_t probeSceneVersion = HashStart, probeAnimationVersion = HashStart, probeGeometryVersion = HashStart;

	ApplyPartTransforms();
	for (EnvMapVulkanEntityID entity : EnvMapVulkanSceneData::GetModels(*s_Scene))
	{
		H2M::RefH2M<H2M::ModelH2M> model = EnvMapVulkanSceneData::GetModel(*s_Scene, entity);
		if (model->IsSkinned())
		{
			model->OnUpdate(H2M::TimestepH2M(deltaTime), false); // bone matrices of the current frame (bind pose when not animated)
		}
		glm::mat4 worldTransform = s_Scene->GetWorldTransform(entity);

		ModelMotionState state;
		state.Model = model.Raw();
		for (const H2M::RefH2M<H2M::MeshH2M>& mesh : model->GetMeshes())
		{
			state.MeshTransforms.push_back(EnvMapVulkanSceneData::GetMeshTransform(model, mesh, worldTransform));
		}
		if (model->IsSkinned())
		{
			state.BoneTransforms = model->GetBoneTransforms();
		}
		auto previousIt = s_PreviousModelStates.find(entity);
		const bool hasPrevious = previousIt != s_PreviousModelStates.end() && previousIt->second.Model == state.Model &&
			previousIt->second.MeshTransforms.size() == state.MeshTransforms.size();
		const ModelMotionState& previousState = hasPrevious ? previousIt->second : state;

		UpdateObjectUniforms(model, previousState.BoneTransforms);
		const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> meshMaterials = EnvMapVulkanSceneData::GetMeshMaterials(*s_Scene, entity);
		SubmitModelTemp(model, worldTransform, meshMaterials, previousState.MeshTransforms);

		// The model, where its meshes are, its pose and which materials they have (the materials' values: below)
		probeSceneVersion = HashBytes(probeSceneVersion, &state.Model, sizeof(state.Model));
		uint64_t& motionVersion = model->IsSkinned() ? probeAnimationVersion : probeSceneVersion;
		motionVersion = HashBytes(motionVersion, state.MeshTransforms.data(), state.MeshTransforms.size() * sizeof(glm::mat4));
		motionVersion = HashBytes(motionVersion, state.BoneTransforms.data(), state.BoneTransforms.size() * sizeof(glm::mat4));
		for (const H2M::RefH2M<EnvMapVulkanMaterial>& material : meshMaterials)
		{
			const EnvMapVulkanMaterial* materialPointer = material.Raw();
			probeSceneVersion = HashBytes(probeSceneVersion, &materialPointer, sizeof(materialPointer));
		}
		if (!model->IsSkinned())
		{
			probeGeometryVersion = HashBytes(probeGeometryVersion, &state.Model, sizeof(state.Model));
			probeGeometryVersion = HashBytes(probeGeometryVersion, state.MeshTransforms.data(), state.MeshTransforms.size() * sizeof(glm::mat4));
			for (const H2M::RefH2M<EnvMapVulkanMaterial>& material : meshMaterials)
			{
				const EnvMapVulkanMaterial* materialPointer = material.Raw();
				probeGeometryVersion = HashBytes(probeGeometryVersion, &materialPointer, sizeof(materialPointer));
			}
		}
		modelStates[entity] = std::move(state);
	}
	s_PreviousModelStates = std::move(modelStates);
	// The materials into their GPU buffer, each with its index (after this frame's edits, before anything is recorded)
	// (with ray tracing, the maps they use get their elements in the array of all textures)
	if (s_MaterialBuffer.Update(s_RayTracing.GetTextureTable()))
	{
		WriteMaterialBufferDescriptor(); // a bigger buffer: Update waited for the GPU
	}
	s_RayTracing.UpdateTextures();
	// The probe volume: atlases for its probe counts, filled from the environment while new, and this frame's batch of
	// probes to update when what they capture changed: the models (above), the materials' values, the lights (as of the
	// last frame), the environment, the water the light passes through
	{
		const std::vector<EnvMapVulkanMaterialBuffer::GPUMaterial>& materialEntries = s_MaterialBuffer.GetEntries();
		probeSceneVersion = HashBytes(probeSceneVersion, materialEntries.data(), materialEntries.size() * sizeof(EnvMapVulkanMaterialBuffer::GPUMaterial));
		probeSceneVersion = HashBytes(probeSceneVersion, &s_LightsVersion, sizeof(s_LightsVersion));
		probeSceneVersion = HashBytes(probeSceneVersion, &s_EnvironmentVersion, sizeof(s_EnvironmentVersion));
		probeSceneVersion = HashBytes(probeSceneVersion, &s_EnvMapRotation, sizeof(s_EnvMapRotation));
		const float waterHeight = s_WaterSettings.Enabled ? s_WaterSettings.Height : -std::numeric_limits<float>::max();
		probeSceneVersion = HashBytes(probeSceneVersion, &waterHeight, sizeof(waterHeight));
	}
	s_PathTracerSceneVersion = HashBytes(probeSceneVersion, &probeAnimationVersion, sizeof(probeAnimationVersion));
	if (s_Data.irradianceMap && s_Data.envFiltered &&
		s_Probes.Update(s_ProbeVolumeSettings, s_Data.irradianceMap.As<H2M::VulkanTextureCubeH2M>()->GetVulkanDescriptorInfo(),
			s_Data.envFiltered.As<H2M::VulkanTextureCubeH2M>()->GetVulkanDescriptorInfo(), s_EnvMapRotation, s_EnvironmentVersion, probeSceneVersion,
			probeGeometryVersion, probeAnimationVersion, IsRayTracingActive()))
	{
		WriteProbeDescriptors(); // new atlases: Update waited for the GPU
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
		s_Glass.Resize(s_Framebuffer->GetWidth(), s_Framebuffer->GetHeight());
		s_GBuffer.Resize(s_Framebuffer); // after s_Framebuffer: it shares its depth image
		s_SSAO.Resize(s_GBuffer, s_RenderSettings.AmbientOcclusion.HalfResolution); // after the G-buffer: it reads its images
		WriteAmbientOcclusionDescriptor();
		s_Reflections.Resize(s_GBuffer, s_RenderSettings.Reflections.HalfResolution);
		WriteReflectionDescriptor();
		s_RayTracing.ResizeView(s_GBuffer.GetWidth(), s_GBuffer.GetHeight()); // the composite's set is written below
		s_PathTracer.Resize(s_GBuffer.GetWidth(), s_GBuffer.GetHeight());     // the same
		s_ViewportCompositeFramebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		s_OverlayFramebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		s_SelectionMaskFramebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		ResizeBloomResources(); // also rewrites the composite descriptor set
		s_ViewportFBNeedsResize = false;
	}
	if (s_SSAO.IsHalfResolution() != s_RenderSettings.AmbientOcclusion.HalfResolution)
	{
		s_SSAO.Resize(s_GBuffer, s_RenderSettings.AmbientOcclusion.HalfResolution); // the setting changed (the panel, a scene opened)
		WriteAmbientOcclusionDescriptor();
	}

	if (s_Reflections.IsHalfResolution() != s_RenderSettings.Reflections.HalfResolution)
	{
		s_Reflections.Resize(s_GBuffer, s_RenderSettings.Reflections.HalfResolution); // the setting changed (the panel, a scene opened)
		WriteReflectionDescriptor();
	}

	// A ray-traced debug view needs rays (the editor tells why it isn't shown)
	if (DebugViewNeedsRayTracing(s_ViewOptions.DebugView) && !IsRayTracingActive())
	{
		s_ViewOptions.DebugView = DebugViewOff;
	}

	// With ray tracing the probes' update traces rays: its set 0 gets what the per-frame set holds (as it is now, after
	// everything above that may have rewritten it)
	s_Probes.PrepareTrace(s_Data.FrameDescriptorSet.DescriptorSets[0]);
	// The same for the reflections' trace
	if (ReflectionsTraceRays())
	{
		s_Reflections.Prepare(s_Data.FrameDescriptorSet.DescriptorSets[0]);
	}
	// And for the path tracer
	if (IsRayTracingActive() && s_ViewOptions.DebugView == DebugViewPathTraced)
	{
		s_PathTracer.Prepare(s_Data.FrameDescriptorSet.DescriptorSets[0]);
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
	s_EnvMapSunExtracted = s_ExtractSunFromEnvironment;
	{
		H2M::BufferH2M pixels = s_Data.envEquirect->GetWriteableBuffer();
		uint32_t width = s_Data.envEquirect->GetWidth(), height = s_Data.envEquirect->GetHeight();
		if (s_ExtractSunFromEnvironment && pixels.Data && s_Data.envEquirect->GetFormat() == H2M::ImageFormatH2M::RGBA32F &&
			pixels.Size >= (uint64_t)width * height * 4 * sizeof(float) && ExtractSun((float*)pixels.Data, width, height, s_ExtractedSun))
		{
			s_Data.envEquirect = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA32F, width, height, pixels.Data);
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

	s_EnvironmentVersion++; // what was made from the old maps (the probe volume's fill) is out of date
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
			auto material = model->GetMaterials()[mesh->MaterialIndex].As<H2M::VulkanMaterialH2M>();
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

/**** BEGIN to be removed from VulkanRenderer ****/
void EnvMapVulkanRenderer::SetScene(EnvMapVulkanScene* scene, const EnvMapVulkanRenderSettings* renderSettings, const EnvMapVulkanViewOptions* viewOptions)
{
	s_Scene = scene;
	s_OwnerRenderSettings = renderSettings;
	s_OwnerViewOptions = viewOptions;
}

void EnvMapVulkanRenderer::SetUserInterface(const std::function<void(VkCommandBuffer)>& recordUserInterface)
{
	s_RecordUserInterface = recordUserInterface;
}

uint64_t EnvMapVulkanRenderer::GetViewportImage()
{
	if (!s_ViewportImageID || s_ViewportTextureNeedsUpdate)
	{
		RegisterViewportImage();
	}
	return s_ViewportImageID;
}

void EnvMapVulkanRenderer::SetViewportSize(uint32_t width, uint32_t height)
{
	if (width > 0 && height > 0 && (s_ViewportWidth != width || s_ViewportHeight != height))
	{
		s_ViewportWidth = width;
		s_ViewportHeight = height;
		s_ViewportFBNeedsResize = true;
	}
}

void EnvMapVulkanRenderer::WaitIdle()
{
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
}

void EnvMapVulkanRenderer::OnSceneReplaced()
{
	s_Probes.RequestReset(); // what the probes captured belongs to the scene that went
}

const std::string& EnvMapVulkanRenderer::GetEnvironmentMapFile()
{
	return s_EnvMapFilename;
}

bool EnvMapVulkanRenderer::HasEnvironmentMap()
{
	return (bool)s_Data.envEquirect;
}

uint32_t EnvMapVulkanRenderer::GetEnvironmentVersion()
{
	return s_EnvironmentVersion;
}

float EnvMapVulkanRenderer::GetEnvironmentMaxLod()
{
	return s_Data.envUnfiltered ? (float)(s_Data.envUnfiltered->GetMipLevelCount() - 1) : 10.0f;
}

const EnvMapVulkanExtractedSun& EnvMapVulkanRenderer::GetExtractedSun()
{
	return s_ExtractedSun;
}

bool EnvMapVulkanRenderer::FindEnvironmentSun(EnvMapVulkanExtractedSun& sun)
{
	H2M::RefH2M<H2M::Texture2D_H2M> equirect = s_Data.envEquirect;
	if (!equirect)
	{
		return false;
	}
	if (s_ExtractedSun.Found)
	{
		sun = s_ExtractedSun;
		return true;
	}
	// No sun in the map: the brightest region (a window, bright sky), direction and color only
	sun = EnvMapVulkanExtractedSun();
	H2M::BufferH2M pixels = equirect->GetWriteableBuffer();
	uint32_t width = equirect->GetWidth(), height = equirect->GetHeight();
	return pixels.Data && equirect->GetFormat() == H2M::ImageFormatH2M::RGBA32F && pixels.Size >= (uint64_t)width * height * 4 * sizeof(float) &&
		FindBrightestDirection((const float*)pixels.Data, width, height, sun.Direction, sun.Color);
}

bool EnvMapVulkanRenderer::IsRayTracingSupported()
{
	return EnvMapVulkanRayTracing::IsSupported();
}

bool EnvMapVulkanRenderer::IsRayTracingActive()
{
	return (s_OwnerRenderSettings ? s_OwnerRenderSettings->RayTracing : s_RenderSettings.RayTracing) && EnvMapVulkanRayTracing::IsSupported();
}

bool EnvMapVulkanRenderer::AreShadowsRendered()
{
	return s_ShadowsRendered;
}

const EnvMapVulkanShadowCascade& EnvMapVulkanRenderer::GetShadowCascade(uint32_t cascade)
{
	return s_ShadowCascades[glm::min(cascade, ShadowCascadeCount - 1)];
}

uint64_t EnvMapVulkanRenderer::GetShadowCascadeImage(uint32_t cascade)
{
	static std::array<uint64_t, ShadowCascadeCount> s_ShadowMapTextureIDs = {};
	static uint32_t s_ShadowMapTexturesGeneration = 0; // the shadow map the IDs were registered for (EnvMapVulkanShadowMap::GetGeneration)
	if (!s_ShadowMap.IsValid() || cascade >= ShadowCascadeCount)
	{
		return 0;
	}
	// Registered with the user interface once per shadow map (again after it is recreated)
	if (s_ShadowMapTexturesGeneration != s_ShadowMap.GetGeneration())
	{
		for (uint32_t i = 0; i < ShadowCascadeCount; i++)
		{
			H2M::VulkanUIImagesH2M::Remove(s_ShadowMapTextureIDs[i]);
			s_ShadowMapTextureIDs[i] = H2M::VulkanUIImagesH2M::Add(s_ShadowMap.GetDisplaySampler(), s_ShadowMap.GetDisplayView(i),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		s_ShadowMapTexturesGeneration = s_ShadowMap.GetGeneration();
	}
	return s_ShadowMapTextureIDs[cascade];
}

void EnvMapVulkanRenderer::GetShadowCasterBounds(glm::vec3& boundsMin, glm::vec3& boundsMax)
{
	boundsMin = s_ShadowCasterBoundsMin;
	boundsMax = s_ShadowCasterBoundsMax;
}

void EnvMapVulkanRenderer::GetShadowSlotsPerLight(std::vector<int>& pointSlots, std::vector<int>& spotSlots)
{
	::GetShadowSlotsPerLight(pointSlots, spotSlots);
}

void EnvMapVulkanRenderer::GetShadowedLightCounts(uint32_t& pointLights, uint32_t& spotLights)
{
	pointLights = s_LocalShadowSlots.PointCount;
	spotLights = s_LocalShadowSlots.SpotCount;
}

bool EnvMapVulkanRenderer::CanPointLightsCastShadows()
{
	return s_PointShadowMaps.IsValid();
}

bool EnvMapVulkanRenderer::CanSpotLightsCastShadows()
{
	return s_SpotShadowMaps.IsValid();
}

uint64_t EnvMapVulkanRenderer::ShowLocalShadowMap(EnvMapVulkanSceneData::LightKind kind, int index)
{
	const bool spot = kind == EnvMapVulkanSceneData::LightKind::Spot;
	if (!spot && kind != EnvMapVulkanSceneData::LightKind::Point)
	{
		return 0;
	}
	int slot = -1;
	float range = 1.0f;
	if (spot)
	{
		for (uint32_t i = 0; i < s_LocalShadowSlots.SpotCount; i++)
		{
			slot = s_LocalShadowSlots.SpotLight[i] == index ? (int)i : slot;
		}
		range = index >= 0 && index < (int)s_Lights.SpotLights.size() ? s_Lights.SpotLights[index].Range : range;
	}
	else
	{
		for (uint32_t i = 0; i < s_LocalShadowSlots.PointCount; i++)
		{
			slot = s_LocalShadowSlots.PointLight[i] == index ? (int)i : slot;
		}
		range = index >= 0 && index < (int)s_Lights.PointLights.size() ? s_Lights.PointLights[index].Range : range;
	}
	if (slot < 0 || !s_ShadowMapViewer.IsValid())
	{
		return 0;
	}

	s_ShadowMapViewerRequest.Active = true;
	s_ShadowMapViewerRequest.Cube = !spot;
	s_ShadowMapViewerRequest.BaseLayer = spot ? (uint32_t)slot : (uint32_t)slot * 6;
	s_ShadowMapViewerRequest.Near = GetLocalShadowNearPlane(range);
	s_ShadowMapViewerRequest.Far = std::max(range, 0.02f);

	static uint64_t s_ViewerTextureID = 0;
	if (!s_ViewerTextureID)
	{
		s_ViewerTextureID = H2M::VulkanUIImagesH2M::Add(s_ShadowMapViewer.GetSampler(), s_ShadowMapViewer.GetView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	return s_ViewerTextureID;
}

void EnvMapVulkanRenderer::SetBloomDirtTexture(const std::string& filepath)
{
	s_PendingBloomDirtFilename = filepath;
}

uint64_t EnvMapVulkanRenderer::GetBloomDirtImage()
{
	return s_BloomDirtTexture ? s_BloomDirtTexture->GetImTextureID() : 0;
}

std::string EnvMapVulkanRenderer::GetBloomDirtTextureFile()
{
	return s_BloomDirtTexture ? s_BloomDirtTexture->GetPath() : std::string();
}

EnvMapVulkanProbes::UpdateStatus EnvMapVulkanRenderer::GetProbeStatus()
{
	return s_Probes.GetStatus();
}

uint32_t EnvMapVulkanRenderer::GetProbeCount()
{
	return s_Probes.GetProbeCount();
}

void EnvMapVulkanRenderer::BakeProbes()
{
	s_Probes.RequestBake();
}

void EnvMapVulkanRenderer::ResetProbes()
{
	s_Probes.RequestReset();
}

void EnvMapVulkanRenderer::RestartPathTracer()
{
	s_PathTracer.Restart();
}

uint32_t EnvMapVulkanRenderer::GetPathTracerSamples()
{
	return s_PathTracer.GetSamples();
}

const EnvMapVulkanRayTracing::Stats& EnvMapVulkanRenderer::GetRayTracingStats()
{
	return s_RayTracing.GetStats();
}

bool EnvMapVulkanRenderer::AreReflectionsTraced()
{
	return s_Reflections.IsTraced();
}

glm::uvec2 EnvMapVulkanRenderer::GetReflectionsSize()
{
	return glm::uvec2(s_Reflections.GetWidth(), s_Reflections.GetHeight());
}

bool EnvMapVulkanRenderer::IsAmbientOcclusionRayTraced()
{
	return s_SSAO.IsRayTraced();
}

glm::uvec2 EnvMapVulkanRenderer::GetAmbientOcclusionSize()
{
	return glm::uvec2(s_SSAO.GetWidth(), s_SSAO.GetHeight());
}

EnvMapVulkanRenderer::GPUInfo EnvMapVulkanRenderer::GetGPUInfo()
{
	const VkPhysicalDeviceProperties& properties = H2M::VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
	GPUInfo info;
	info.Name = properties.deviceName;
	info.API = "Vulkan " + std::to_string(VK_API_VERSION_MAJOR(properties.apiVersion)) + "." + std::to_string(VK_API_VERSION_MINOR(properties.apiVersion)) +
		"." + std::to_string(VK_API_VERSION_PATCH(properties.apiVersion));
	if (properties.vendorID == 0x10DE) // NVIDIA's driver version encoding
	{
		info.Driver = std::to_string((properties.driverVersion >> 22) & 0x3FF) + "." + std::to_string((properties.driverVersion >> 14) & 0xFF);
	}
	return info;
}

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
