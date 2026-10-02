/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EnvMapVulkanRenderer.h"
#include "EnvMapVulkanMaterialLibrary.h"

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
#include "imgui_internal.h" // BeginDragDropTargetCustom (the Meshes panel as one drop area)

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
// - selection mask: silhouette of the selected mesh or submesh; the composite draws the outline around it
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
	glm::vec4 SelectedBoundingBoxColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); // the selected submesh's box (all boxes of the selected mesh when no submesh is selected)
	float LineWidth = 1.0f; // wireframe and bounding boxes (pixels, 1..10)
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
static H2M::RefH2M<H2M::VertexBufferH2M> s_BoundingBoxVertexBuffer;
static const uint32_t s_BoundingBoxVertexCount = 24; // 12 edges

static void CreateEditorOverlayResources();
static void RecordEditorOverlayPasses(VkCommandBuffer commandBuffer);
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipeline;                 // to be removed from VulkanRenderer
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipelineAnim; // skinned meshes (HazelPBR_Anim.glsl): vertex layout with bone IDs and weights
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
// Meshes submitted for this frame, with their transforms and the material of each submesh
struct SubmittedMesh
{
	H2M::RefH2M<H2M::MeshH2M> Mesh;
	glm::mat4 Transform;
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> Materials;
};
static std::vector<SubmittedMesh> s_Meshes;

static H2M::RefH2M<H2M::SubmeshH2M> s_SelectedSubmesh;
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
		glm::vec3 LightDirectionTemp;
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
		H2M::RefH2M<H2M::MeshH2M> Mesh;
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

// Light (Environment panel) and environment map rotation, used by the PBR shader
static glm::vec3 s_LightRadiance = glm::vec3(1.0f);
static float s_LightMultiplier = 1.0f;
static float s_EnvMapRotation = 0.0f; // degrees, applied to the PBR environment lookups (as in SceneHazelEnvMap)

// Meshes loaded from the Meshes panel: the Vulkan counterpart of the mesh entities in SceneHazelEnvMap
struct LoadedMeshVulkan
{
	H2M::RefH2M<H2M::MeshH2M> Mesh;
	std::string FilePath;
	glm::vec3 Translation = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f); // degrees
	glm::vec3 Scale = glm::vec3(1.0f);
	// Material slots: the Material Library material each submesh is drawn with (same order as the mesh's submeshes)
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> SubmeshMaterials;

	// Composed with ImGuizmo's own convention (Euler angles in degrees), so the gizmo (Manipulate +
	// DecomposeMatrixToComponents) and the values in the Meshes panel round-trip exactly
	glm::mat4 GetTransform() const
	{
		glm::mat4 transform;
		ImGuizmo::RecomposeMatrixFromComponents(&Translation.x, &Rotation.x, &Scale.x, glm::value_ptr(transform));
		return transform;
	}
};
static std::vector<LoadedMeshVulkan> s_LoadedMeshes;
static int s_SelectedMeshIndex = -1;
static int s_SelectedSubmeshIndex = -1; // submesh of the selected mesh; -1 = none
static std::string s_PendingMeshFilename;  // requested from the UI, loaded at the start of the next Draw
static std::optional<glm::vec3> s_PendingMeshGroundPosition; // dropped on the viewport: where the model is placed (see LoadMesh)
static int s_PendingRemoveMeshIndex = -1;  // requested from the UI, removed at the start of the next Draw

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

// Finds the mesh and submesh under the mouse: a ray through the cursor is tested against every submesh's bounding box,
// then its triangles (as in SceneHazelEnvMap); the nearest hit wins. hitMesh/hitSubmesh are -1 when nothing is hit.
// ndcX, ndcY: cursor position in normalized device coordinates of the viewport (-1..1, +y up)
static void RaycastSubmesh(float ndcX, float ndcY, int& hitMesh, int& hitSubmesh)
{
	glm::vec3 origin, direction;
	GetCameraRay(ndcX, ndcY, origin, direction);

	float nearestT = std::numeric_limits<float>::max();
	hitMesh = -1;
	hitSubmesh = -1;

	for (int m = 0; m < (int)s_LoadedMeshes.size(); m++)
	{
		LoadedMeshVulkan& entry = s_LoadedMeshes[m];
		glm::mat4 meshTransform = entry.GetTransform();
		auto& submeshes = entry.Mesh->GetSubmeshes();

		for (int s = 0; s < (int)submeshes.size(); s++)
		{
			// Ray in the submesh's local space; the direction is not renormalized, so t is the same distance
			// along the world ray for every submesh and the hits can be compared
			glm::mat4 toLocal = glm::inverse(meshTransform * submeshes[s]->Transform);
			H2M::RayH2M ray = { glm::vec3(toLocal * glm::vec4(origin, 1.0f)), glm::mat3(toLocal) * direction };

			float t;
			if (!ray.IntersectsAABB(submeshes[s]->BoundingBox, t) || t < 0.0f || t >= nearestT)
			{
				continue;
			}

			const auto triangles = entry.Mesh->GetTriangleCache((uint32_t)s);
			if (triangles.empty())
			{
				nearestT = t; hitMesh = m; hitSubmesh = s; // no triangle data: the bounding box has to do
				continue;
			}
			for (const auto& triangle : triangles)
			{
				if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t) && t >= 0.0f && t < nearestT)
				{
					nearestT = t; hitMesh = m; hitSubmesh = s;
				}
			}
		}
	}

}

// Selects the mesh and submesh under the mouse. Nothing hit clears the selection.
static void PickMesh(float ndcX, float ndcY)
{
	RaycastSubmesh(ndcX, ndcY, s_SelectedMeshIndex, s_SelectedSubmeshIndex);
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

// Number of submeshes, over all loaded meshes, drawn with the material
static int CountMaterialUsers(const H2M::RefH2M<EnvMapVulkanMaterial>& material)
{
	int count = 0;
	for (const LoadedMeshVulkan& entry : s_LoadedMeshes)
	{
		count += (int)std::count(entry.SubmeshMaterials.begin(), entry.SubmeshMaterials.end(), material);
	}
	return count;
}

// The Material Editor follows the selection: selecting a submesh (Meshes panel or viewport) selects the material it is drawn with
static void SyncSelectedMaterial()
{
	static int s_LastMeshIndex = -1;
	static int s_LastSubmeshIndex = -1;
	if (s_SelectedMeshIndex == s_LastMeshIndex && s_SelectedSubmeshIndex == s_LastSubmeshIndex)
	{
		return;
	}
	s_LastMeshIndex = s_SelectedMeshIndex;
	s_LastSubmeshIndex = s_SelectedSubmeshIndex;

	if (s_SelectedMeshIndex >= 0 && s_SelectedMeshIndex < (int)s_LoadedMeshes.size())
	{
		const auto& slots = s_LoadedMeshes[s_SelectedMeshIndex].SubmeshMaterials;
		if (s_SelectedSubmeshIndex >= 0 && s_SelectedSubmeshIndex < (int)slots.size())
		{
			s_SelectedMaterial = slots[s_SelectedSubmeshIndex];
		}
	}
}

static glm::mat4 GetSubmeshTransform(H2M::RefH2M<H2M::MeshH2M> mesh, const H2M::RefH2M<H2M::SubmeshH2M>& submesh, const glm::mat4& transform);

// Loads a model file and adds it to the scene (called at the start of a frame, see Draw). With a ground position (a model dropped
// on the viewport), the model stands on that point: the bottom center of its bounding box is placed there, rather than its origin.
// A model far too large or too small for the view (e.g. modeled in millimeters) is also scaled to fit (see below).
static void LoadMesh(const std::string& filepath, std::optional<glm::vec3> groundPosition = std::nullopt)
{
	if (!std::filesystem::exists(filepath) || !IsModelFile(filepath))
	{
		Log::GetLogger()->error("Mesh '{0}' was not loaded: the file does not exist or is not a supported model file.", filepath);
		return;
	}

	H2M::RefH2M<H2M::MeshH2M> mesh = H2M::RefH2M<H2M::MeshH2M>::Create(filepath);
	if (!mesh || mesh->GetSubmeshes().empty())
	{
		Log::GetLogger()->error("Mesh '{0}' was not loaded: no geometry found.", filepath);
		return;
	}

	if (mesh->HasAnimations())
	{
		Log::GetLogger()->info("Mesh '{0}': {1} animation(s), {2} bones", filepath, mesh->GetAnimationCount(), mesh->GetBoneCount());
	}

	LoadedMeshVulkan entry;
	entry.Mesh = mesh;
	entry.FilePath = filepath;

	// The model's materials go into the Material Library (reused if this model was loaded before); each submesh starts
	// with the material the model assigns to it, or the library's Default material if the model has none for it
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> modelMaterials = EnvMapVulkanMaterialLibrary::ImportMeshMaterials(mesh);
	for (auto& submesh : mesh->GetSubmeshes())
	{
		entry.SubmeshMaterials.push_back(submesh->MaterialIndex < modelMaterials.size() ?
			modelMaterials[submesh->MaterialIndex] : EnvMapVulkanMaterialLibrary::GetDefaultMaterial());
	}

	if (groundPosition)
	{
		// Bounding box of the model at rest: the corners of every submesh's bounding box, placed as they are drawn
		glm::vec3 boundsMin(std::numeric_limits<float>::max());
		glm::vec3 boundsMax(-std::numeric_limits<float>::max());
		for (auto& submesh : mesh->GetSubmeshes())
		{
			glm::mat4 submeshTransform = GetSubmeshTransform(mesh, submesh, glm::mat4(1.0f));
			const H2M::AABB_H2M& box = submesh->BoundingBox;
			for (int corner = 0; corner < 8; corner++)
			{
				glm::vec3 point((corner & 1) ? box.Max.x : box.Min.x, (corner & 2) ? box.Max.y : box.Min.y, (corner & 4) ? box.Max.z : box.Min.z);
				glm::vec3 placed = glm::vec3(submeshTransform * glm::vec4(point, 1.0f));
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
				Log::GetLogger()->info("Mesh '{0}' is {1} units across: scaled by {2} to fit the view (set Scale to 1 for its original size)",
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

	s_LoadedMeshes.push_back(entry);
	s_SelectedMeshIndex = (int)s_LoadedMeshes.size() - 1;
	s_SelectedSubmeshIndex = -1;
	Log::GetLogger()->info("Mesh '{0}' loaded: {1} submeshes, {2} materials", filepath, mesh->GetSubmeshes().size(), modelMaterials.size());
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

	H2M::RefH2M<H2M::Texture2D_H2M> texture;
	try
	{
		texture = H2M::Texture2D_H2M::Create(request.FilePath, EnvMapVulkanMaterial::IsColorMap(request.Slot));
	}
	catch (...)
	{
		Log::GetLogger()->error("Map '{0}' could not be loaded.", request.FilePath);
		return;
	}

	vkDeviceWaitIdle(device); // the descriptor set may be used by frames in flight
	material->SetMap(request.Slot, texture);

	Log::GetLogger()->info("Map '{0}' assigned to {1} of material '{2}'", request.FilePath, EnvMapVulkanMaterial::GetMapTextureName(request.Slot), material->GetName());
}

// Deletes a material from the library (called at the start of a frame, see Draw). Submeshes that used it get the Default material.
static void DeleteMaterial(H2M::RefH2M<EnvMapVulkanMaterial> material)
{
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice()); // its descriptor set may be used by frames in flight

	int users = CountMaterialUsers(material);
	EnvMapVulkanMaterialLibrary::Remove(material);
	if (users > 0)
	{
		H2M::RefH2M<EnvMapVulkanMaterial> replacement = EnvMapVulkanMaterialLibrary::GetDefaultMaterial(); // a new one if the Default was deleted
		for (LoadedMeshVulkan& entry : s_LoadedMeshes)
		{
			std::replace(entry.SubmeshMaterials.begin(), entry.SubmeshMaterials.end(), material, replacement);
		}
	}
	if (s_SelectedMaterial == material)
	{
		s_SelectedMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
	}
	Log::GetLogger()->info("Material '{0}' deleted ({1} submeshes now use the Default material)", material->GetName(), users);
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

	// binding 1: SceneData (fragment stage), std140: Light { vec3 Direction; vec3 Radiance; float Multiplier; },
	// vec3 u_CameraPosition, float u_EnvMapRotation
	struct Light
	{
		glm::vec3 Direction;
		float Padding = 0.0f;
		glm::vec3 Radiance;
		float Multiplier;
	};
	struct SceneDataUB
	{
		Light Lights;
		glm::vec3 CameraPosition;
		float EnvMapRotation;
	};
	SceneDataUB ub;
	ub.Lights.Direction = s_Data.SceneData.LightDirectionTemp;
	ub.Lights.Radiance = s_LightRadiance;
	ub.Lights.Multiplier = s_LightMultiplier;
	ub.CameraPosition = camera.GetPosition();
	ub.EnvMapRotation = s_EnvMapRotation;

	ubPtr = shader->MapUniformBuffer(1, frameSet);
	memcpy(ubPtr, &ub, sizeof(SceneDataUB));
	shader->UnmapUniformBuffer(1, frameSet);
}

// Per-object uniform buffers (set 2): the bone matrices of the current animation frame of a skinned mesh (up to 128).
// Every mesh has its own shader instance (see MeshH2M::Create), so every skinned mesh has its own bone buffer.
static void UpdateObjectUniforms(const H2M::RefH2M<H2M::MeshH2M>& mesh)
{
	H2M::RefH2M<H2M::MeshH2M> meshRef = mesh;
	if (!meshRef->IsSkinned() || meshRef->GetObjectDescriptorSet() == VK_NULL_HANDLE)
	{
		return;
	}

	const std::vector<glm::mat4>& boneTransforms = meshRef->GetBoneTransforms();
	size_t boneCount = std::min<size_t>(boneTransforms.size(), 128);
	if (boneCount > 0)
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = meshRef->GetMeshShader().As<H2M::VulkanShaderH2M>();
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

static void OnImGuiRenderMeshes()
{
	ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Meshes");

	if (ImGui::Button("Load Mesh"))
	{
		std::string filepath = Util::ToUtf8(Application::Get()->OpenFile());
		if (!filepath.empty())
		{
			s_PendingMeshFilename = filepath; // loaded at the start of the next frame (see Draw)
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
	if (s_LoadedMeshes.empty())
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

	for (int i = 0; i < (int)s_LoadedMeshes.size(); i++)
	{
		ImGui::PushID(i);
		std::string name = std::filesystem::path(s_LoadedMeshes[i].FilePath).filename().string();
		if (ImGui::Selectable(name.c_str(), s_SelectedMeshIndex == i))
		{
			if (s_SelectedMeshIndex != i)
			{
				s_SelectedSubmeshIndex = -1;
			}
			s_SelectedMeshIndex = i;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", s_LoadedMeshes[i].FilePath.c_str());
		}
		ImGui::PopID();
	}

	if (s_SelectedMeshIndex >= 0 && s_SelectedMeshIndex < (int)s_LoadedMeshes.size())
	{
		LoadedMeshVulkan& entry = s_LoadedMeshes[s_SelectedMeshIndex];

		ImGui::Separator();
		ImGui::Text("Transform");
		ImGui::DragFloat3("Translation", &entry.Translation.x, 0.1f);
		ImGui::DragFloat3("Rotation", &entry.Rotation.x, 1.0f);
		ImGui::DragFloat3("Scale", &entry.Scale.x, 0.01f, 0.001f, 1000.0f);

		if (ImGui::Button("Remove Mesh"))
		{
			s_PendingRemoveMeshIndex = s_SelectedMeshIndex; // removed at the start of the next frame (see Draw)
		}

		// Animation playback (skinned meshes), like the Animation section of the Mesh Debug panel in SceneHazelEnvMap
		H2M::RefH2M<H2M::MeshH2M> mesh = entry.Mesh;
		if (mesh->HasAnimations() && mesh->IsSkinned())
		{
			ImGui::Separator();
			ImGui::Text("Animation");

			ImGui::Checkbox("Animated", &mesh->IsAnimated());
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Off: the model is shown in its bind pose");
			}

			ImGui::BeginDisabled(!mesh->IsAnimated());
			{
				uint32_t animationCount = mesh->GetAnimationCount();
				if (animationCount > 1)
				{
					if (ImGui::BeginCombo("Clip", mesh->GetAnimationName(mesh->GetAnimationIndex()).c_str()))
					{
						for (uint32_t a = 0; a < animationCount; a++)
						{
							ImGui::PushID((int)a);
							if (ImGui::Selectable(mesh->GetAnimationName(a).c_str(), mesh->GetAnimationIndex() == a))
							{
								mesh->SetAnimationIndex(a);
							}
							ImGui::PopID();
						}
						ImGui::EndCombo();
					}
				}
				else
				{
					ImGui::TextDisabled("Clip: %s", mesh->GetAnimationName(0).c_str());
				}

				if (ImGui::Button(mesh->AnimationPlaying() ? "Pause" : "Play", ImVec2(60.0f, 0.0f)))
				{
					mesh->AnimationPlaying() = !mesh->AnimationPlaying();
				}
				ImGui::SameLine();
				if (ImGui::Button("Restart"))
				{
					mesh->AnimationTime() = 0.0f;
				}

				// Scrub through the animation (in seconds; MeshH2M keeps the time in animation ticks)
				float ticksPerSecond = mesh->GetAnimationTicksPerSecond();
				float durationSeconds = mesh->GetAnimationDuration() / ticksPerSecond;
				float timeSeconds = mesh->AnimationTime() / ticksPerSecond;
				if (ImGui::SliderFloat("Time", &timeSeconds, 0.0f, durationSeconds, "%.2f s"))
				{
					mesh->AnimationTime() = timeSeconds * ticksPerSecond;
				}
				ImGui::DragFloat("Time Scale", &mesh->TimeMultiplier(), 0.01f, 0.0f, 10.0f, "%.2fx");
				ImGui::TextDisabled("Duration %.2f s (%.0f ticks at %.0f/s), %u bones", durationSeconds, mesh->GetAnimationDuration(), ticksPerSecond, mesh->GetBoneCount());
			}
			ImGui::EndDisabled();
		}

		// Material slots, one per submesh: select a submesh to edit its material in the Material Editor. The dropdown, or a
		// material dropped from the Material Library, chooses which library material the submesh is drawn with.
		auto& submeshes = entry.Mesh->GetSubmeshes();
		const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();

		// Assigns a library material to a submesh; the Material Editor follows if that submesh is selected
		auto assignMaterial = [&](int s, H2M::RefH2M<EnvMapVulkanMaterial> material) {
			entry.SubmeshMaterials[s] = material; // used by RenderMeshVulkan from the next frame
			if (s == s_SelectedSubmeshIndex)
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
		ImGui::Text("Submeshes (%d)", (int)submeshes.size());

		for (int s = 0; s < (int)submeshes.size() && s < (int)entry.SubmeshMaterials.size(); s++)
		{
			H2M::RefH2M<H2M::SubmeshH2M> submesh = submeshes[s];
			ImGui::PushID(1000 + s);

			std::string submeshName = !submesh->MeshName.empty() ? submesh->MeshName :
				(!submesh->NodeName.empty() ? submesh->NodeName : "Submesh " + std::to_string(s));
			if (ImGui::Selectable(submeshName.c_str(), s_SelectedSubmeshIndex == s, 0, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f)))
			{
				s_SelectedSubmeshIndex = (s_SelectedSubmeshIndex == s) ? -1 : s; // click again to deselect
			}
			acceptMaterialDrop(s);

			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::BeginCombo("##material", entry.SubmeshMaterials[s]->GetName().c_str()))
			{
				for (uint32_t m = 0; m < (uint32_t)materials.size(); m++)
				{
					ImGui::PushID((int)m);
					if (ImGui::Selectable(materials[m]->GetName().c_str(), entry.SubmeshMaterials[s] == materials[m]))
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
			std::string label = "Apply '" + s_SelectedMaterial->GetName() + "' to all submeshes";
			if (ImGui::Button(label.c_str()))
			{
				std::fill(entry.SubmeshMaterials.begin(), entry.SubmeshMaterials.end(), s_SelectedMaterial);
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
	if (ImGui::BeginDragDropTargetCustom(ImRect(panelMin, panelMax), ImGui::GetID("##MeshesPanelDropArea")))
	{
		std::string filepath;
		if (AcceptFileDrop(panelMin, panelMax, IsModelFile, "a model file", filepath))
		{
			s_PendingMeshFilename = filepath; // loaded at the start of the next frame (see Draw)
			s_PendingMeshGroundPosition.reset();
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::End();
}

// All materials of the scene: create, duplicate, delete, and drag onto a submesh (Meshes panel or viewport) to assign
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
		ImGui::SetTooltip("Submeshes that use the selected material get the Default material");
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
			ImGui::SetTooltip("%s\nDrag onto a submesh in the Meshes panel or the viewport to assign it", origin.c_str());
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

// Edits the selected library material in place: every submesh drawn with it changes (values are push constants of every
// draw; maps are written to the material's descriptor set at the start of the next frame)
static void OnImGuiRenderMaterialEditor()
{
	ImGui::Begin("Material Editor");

	SyncSelectedMaterial();

	H2M::RefH2M<EnvMapVulkanMaterial> material = s_SelectedMaterial;
	if (!material)
	{
		ImGui::TextDisabled("Select a material in the Material Library,\nor a submesh in the Meshes panel or the viewport");
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
		ImGui::TextDisabled("Used by %d submeshes: changes apply to all of them.", users);
	}
	else if (users == 0)
	{
		ImGui::TextDisabled("Not used by any submesh.");
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
			ImGui::SetTooltip("%s", map ? map->GetPath().c_str() : "Drop an image from the Content Browser here");
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

// Where a submesh is drawn: mesh transform (Meshes panel) * node transform from the model. Rigged submeshes: the bone matrices
// already place the skinned vertices relative to the root node, so the root node transform replaces the node transform.
static glm::mat4 GetSubmeshTransform(H2M::RefH2M<H2M::MeshH2M> mesh, const H2M::RefH2M<H2M::SubmeshH2M>& submesh, const glm::mat4& transform)
{
	return (mesh->IsSkinned() && submesh->IsRigged) ? transform * mesh->GetRootTransform() : transform * submesh->Transform;
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

	// Vertex layouts of MeshH2M's Vertex and AnimatedVertex (the overlay shaders read the position, and the bones)
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

// Draws a loaded mesh (all submeshes, or only submesh onlySubmesh) in one color with an overlay pipeline.
// lineWidth: for the wireframe pipelines (dynamic state), 0 for the others.
static void DrawMeshOverlay(VkCommandBuffer commandBuffer, LoadedMeshVulkan& entry, int onlySubmesh, const H2M::RefH2M<H2M::PipelineH2M>& staticPipeline,
	const H2M::RefH2M<H2M::PipelineH2M>& animPipeline, const glm::vec4& color, const glm::mat4& viewProjection, float lineWidth = 0.0f)
{
	H2M::RefH2M<H2M::MeshH2M> mesh = entry.Mesh;
	bool skinned = mesh->IsSkinned();
	// EditorOverlay_Anim.glsl declares the bone matrices exactly like set 2 of HazelPBR_Anim.glsl (one uniform buffer at
	// binding 0, vertex stage), only as its set 0: the set layouts are identical, so the mesh's per-object set is bound directly
	VkDescriptorSet boneDescriptorSet = skinned ? mesh->GetObjectDescriptorSet() : VK_NULL_HANDLE;
	if (skinned && !boneDescriptorSet)
	{
		return; // no bone buffer (the mesh isn't drawn by RenderMeshVulkan either)
	}
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = (skinned ? animPipeline : staticPipeline).As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

	VkBuffer vertexBuffer = mesh->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, mesh->GetIndexBuffer().As<H2M::VulkanIndexBufferH2M>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
	auto& submeshes = mesh->GetSubmeshes();
	for (int s = 0; s < (int)submeshes.size(); s++)
	{
		if (onlySubmesh >= 0 && s != onlySubmesh)
		{
			continue;
		}
		glm::mat4 mvp = viewProjection * GetSubmeshTransform(mesh, submeshes[s], transform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
		vkCmdDrawIndexed(commandBuffer, submeshes[s]->IndexCount, 1, submeshes[s]->BaseIndex, submeshes[s]->BaseVertex, 0);
	}
}

// Draws the editor overlays into s_OverlayFramebuffer and s_SelectionMaskFramebuffer. Both are rendered every frame (cleared
// when there is nothing to show): the viewport composite samples them, so they always need valid content.
static void RecordEditorOverlayPasses(VkCommandBuffer commandBuffer)
{
	const glm::mat4 viewProjection = s_Data.SceneData.SceneCamera.Camera.GetViewProjection();
	const bool hasSelection = s_SelectedMeshIndex >= 0 && s_SelectedMeshIndex < (int)s_LoadedMeshes.size();
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
	auto inScope = [hasSelection](int scope, int meshIndex)
	{
		return scope == OverlayScopeAll || (scope == OverlayScopeSelected && hasSelection && meshIndex == s_SelectedMeshIndex);
	};

	// Wireframe and bounding boxes
	beginPass(s_OverlayFramebuffer);
	{
		if (settings.Wireframe != OverlayScopeOff)
		{
			// Depth of every mesh first (color alpha 0 leaves the image unchanged), so edges hidden behind a mesh are hidden
			for (LoadedMeshVulkan& entry : s_LoadedMeshes)
			{
				DrawMeshOverlay(commandBuffer, entry, -1, s_OverlayDepthPipeline, s_OverlayDepthPipelineAnim, glm::vec4(0.0f), viewProjection);
			}

			// The edges are pulled slightly towards the camera (clip z - bias * w), so they win the depth test against their own faces
			glm::mat4 depthBias(1.0f);
			depthBias[3][2] = -2.0e-5f;
			for (int m = 0; m < (int)s_LoadedMeshes.size(); m++)
			{
				if (inScope(settings.Wireframe, m))
				{
					// "Selected": the selected submesh, or the whole mesh when no submesh is selected (like the selection outline)
					int onlySubmesh = -1;
					if (settings.Wireframe == OverlayScopeSelected && s_SelectedSubmeshIndex < (int)s_LoadedMeshes[m].Mesh->GetSubmeshes().size())
					{
						onlySubmesh = s_SelectedSubmeshIndex;
					}
					DrawMeshOverlay(commandBuffer, s_LoadedMeshes[m], onlySubmesh, s_WireframePipeline, s_WireframePipelineAnim, settings.WireframeColor,
						depthBias * viewProjection, lineWidth);
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

			for (int m = 0; m < (int)s_LoadedMeshes.size(); m++)
			{
				if (!inScope(settings.BoundingBoxes, m))
				{
					continue;
				}
				LoadedMeshVulkan& entry = s_LoadedMeshes[m];
				glm::mat4 transform = entry.GetTransform();
				auto& submeshes = entry.Mesh->GetSubmeshes();
				for (int s = 0; s < (int)submeshes.size(); s++)
				{
					// "Selected": the selected submesh's box, or all boxes of the mesh when no submesh is selected (like the wireframe)
					if (settings.BoundingBoxes == OverlayScopeSelected && s_SelectedSubmeshIndex >= 0 &&
						s_SelectedSubmeshIndex < (int)submeshes.size() && s != s_SelectedSubmeshIndex)
					{
						continue;
					}

					// Each submesh's box in its own space, so it turns with the mesh (as in SceneHazelEnvMap)
					const H2M::AABB_H2M& box = submeshes[s]->BoundingBox;
					glm::mat4 mvp = viewProjection * GetSubmeshTransform(entry.Mesh, submeshes[s], transform) *
						glm::translate(glm::mat4(1.0f), box.Min) * glm::scale(glm::mat4(1.0f), box.Max - box.Min);

					// The selection (the selected submesh, or the whole selected mesh when no submesh is selected) in its own color
					bool selected = m == s_SelectedMeshIndex && (s_SelectedSubmeshIndex < 0 || s == s_SelectedSubmeshIndex);
					glm::vec4 color = selected ? settings.SelectedBoundingBoxColor : settings.BoundingBoxColor;

					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
					vkCmdDraw(commandBuffer, s_BoundingBoxVertexCount, 1, 0, 0);
				}
			}
		}
	}
	vkCmdEndRenderPass(commandBuffer);

	// Silhouette of the selection: the selected submesh, or the whole mesh when no submesh is selected
	beginPass(s_SelectionMaskFramebuffer);
	if (settings.Outline && hasSelection)
	{
		LoadedMeshVulkan& entry = s_LoadedMeshes[s_SelectedMeshIndex];
		int submesh = s_SelectedSubmeshIndex < (int)entry.Mesh->GetSubmeshes().size() ? s_SelectedSubmeshIndex : -1;
		DrawMeshOverlay(commandBuffer, entry, submesh, s_SelectionMaskPipeline, s_SelectionMaskPipelineAnim, glm::vec4(1.0f), viewProjection);
	}
	vkCmdEndRenderPass(commandBuffer);
}

/**** BEGIN to be removed from VulkanRenderer ****/
void EnvMapVulkanRenderer::SubmitMeshTemp(const H2M::RefH2M<H2M::MeshH2M>& mesh, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials)
{
	// Temporary code - populate selected submesh
	// std::vector<Submesh> submeshes = mesh->GetSubmeshes();
	// s_SelectedSubmesh = &submeshes.at(0);

	s_Meshes.push_back({ mesh, transform, materials });

	// VulkanRendererData::DrawCommand drawCommand = {};
	// drawCommand.Mesh = mesh;
	// drawCommand.Transform = transform;
	s_Data.DrawList.push_back({ mesh, H2M::RefH2M<H2M::MaterialH2M>(), transform });
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
		framebufferSpec.Attachments = { H2M::ImageFormatH2M::RGBA32F, H2M::ImageFormatH2M::Depth };
		framebufferSpec.Samples = 1;
		framebufferSpec.ClearOnLoad = false;
		framebufferSpec.ClearColor = { 0.1f, 0.5f, 0.5f, 1.0f };
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

		// Skinned meshes: same render pass, vertex layout of MeshH2M's AnimatedVertex
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
	s_EnvMapFilename = "Textures/HDR/newport_loft.hdr";
	s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap(s_EnvMapFilename);

	s_Data.BRDFLut = H2M::Texture2D_H2M::Create("assets/textures/BRDF_LUT.tga", false);

	// H2M::RendererH2M::Submit([environment]() mutable {});
	{
		// The per-frame set (set 0) is shared by the static and the skinned mesh pipelines: set 0 is declared identically in
		// HazelPBR_Static.glsl and HazelPBR_Anim.glsl, so a set allocated with one shader's layout is valid for both
		auto shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");
		H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = shader.As<H2M::VulkanShaderH2M>();
		const uint32_t frameSet = H2M::VulkanShaderH2M::FrameDescriptorSet;
		s_Data.FrameDescriptorSet = pbrShader->CreateDescriptorSets(frameSet);

		// Camera and SceneData uniform buffers (written every frame, see UpdateFrameUniforms); the environment maps are
		// written in SetSceneEnvironment
		std::array<VkWriteDescriptorSet, 2> writes = { *pbrShader->GetDescriptorSet("Camera", frameSet), *pbrShader->GetDescriptorSet("SceneData", frameSet) };
		writes[0].dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
		writes[0].descriptorCount = 1;
		writes[0].pBufferInfo = &pbrShader->GetUniformBuffer(0, frameSet).Descriptor;
		writes[1].dstSet = s_Data.FrameDescriptorSet.DescriptorSets[0];
		writes[1].descriptorCount = 1;
		writes[1].pBufferInfo = &pbrShader->GetUniformBuffer(1, frameSet).Descriptor;
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
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
	s_Data.SceneData.LightDirectionTemp = { 0.5f, 0.5f, 0.5f };

	OnResize(s_ViewportWidth, s_ViewportHeight); // to be removed from VulkanRenderer
}

void EnvMapVulkanRenderer::Shutdown()
{
	H2M::VulkanShaderH2M::ClearUniformBuffers();
	// delete s_Data;
}

void EnvMapVulkanRenderer::RenderMeshVulkan(H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials, VkCommandBuffer commandBuffer)
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
	// Skinned meshes have a different vertex layout (bone IDs and weights) and shader
	bool skinned = mesh->IsSkinned();
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = (skinned ? s_MeshPipelineAnim : s_MeshPipeline).As<H2M::VulkanPipelineH2M>(); // to be removed from VulkanRenderer
	/**** END Composite ****/

	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

	auto vulkanMeshVB = mesh->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>();
	VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

	auto vulkanMeshIB = H2M::RefH2M<H2M::VulkanIndexBufferH2M>(mesh->GetIndexBuffer());
	VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
	vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

	VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// Set 0 (per frame) was bound once for all meshes in GeometryPass. It stays bound across the static and skinned
	// pipelines: their layouts declare set 0 and the push constants identically ("compatible for set 0").

	// Set 2 (per object): the bone matrices of a skinned mesh
	if (skinned)
	{
		VkDescriptorSet objectDescriptorSet = mesh->GetObjectDescriptorSet();
		if (objectDescriptorSet == VK_NULL_HANDLE)
		{
			return; // no bone buffer: the skinned vertices can't be placed
		}
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, H2M::VulkanShaderH2M::ObjectDescriptorSet, 1, &objectDescriptorSet, 0, nullptr);
	}

	auto& submeshes = mesh->GetSubmeshes();
	for (size_t s = 0; s < submeshes.size() && s < materials.size(); s++)
	{
		H2M::RefH2M<H2M::SubmeshH2M> submesh = submeshes[s];
		H2M::RefH2M<EnvMapVulkanMaterial> material = materials[s];
		H2M::BufferH2M uniformStorageBuffer = material->GetUniformStorageBuffer();

		// Set 1 (per material): the texture maps of the submesh's library material
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
		glm::mat4 submeshTransform = GetSubmeshTransform(mesh, submesh, transform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &submeshTransform);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
		vkCmdDrawIndexed(commandBuffer, submesh->IndexCount, 1, submesh->BaseIndex, submesh->BaseVertex, 0);
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

		for (const SubmittedMesh& submitted : s_Meshes)
		{
			RenderMeshVulkan(submitted.Mesh, submitted.Transform, submitted.Materials, drawCommandBuffer);
		}

		s_Meshes.clear();

		// Transparent, so after the opaque meshes
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
	} uniforms;
	uniforms.Exposure = s_Exposure * (s_AutoExposureEnabled ? s_EnvMapAutoExposure : 1.0f);
	uniforms.BloomIntensity = s_BloomSettings.Enabled ? s_BloomSettings.Intensity : 0.0f;
	uniforms.BloomDirtIntensity = (s_BloomSettings.Enabled && s_BloomSettings.DirtEnabled) ? s_BloomSettings.DirtIntensity : 0.0f;
	uniforms.OutlineWidth = s_OverlaySettings.Outline ? s_OverlaySettings.OutlineWidth : 0.0f;
	uniforms.OutlineColor = s_OverlaySettings.OutlineColor;
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
			// - a material from the Material Library: the submesh under the cursor gets it and becomes the selection
			//   (so the Material Editor shows the material)
			// - a model file from the Content Browser: placed standing on the ground under the cursor (see LoadMesh)
			// - an .hdr file from the Content Browser: loaded as the environment map
			if (ImGui::BeginDragDropTarget())
			{
				glm::vec2 ndc = GetViewportMouseNdc();

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_MaterialPayload))
				{
					uint32_t index = *(const uint32_t*)payload->Data;
					const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();

					int hitMesh, hitSubmesh;
					RaycastSubmesh(ndc.x, ndc.y, hitMesh, hitSubmesh);

					if (index < materials.size() && hitMesh >= 0 && hitSubmesh >= 0 && hitSubmesh < (int)s_LoadedMeshes[hitMesh].SubmeshMaterials.size())
					{
						s_LoadedMeshes[hitMesh].SubmeshMaterials[hitSubmesh] = materials[index];
						s_SelectedMeshIndex = hitMesh;
						s_SelectedSubmeshIndex = hitSubmesh;
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
						s_PendingMeshFilename = droppedPath;
						s_PendingMeshGroundPosition = GetDropGroundPosition(ndc.x, ndc.y);
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

			Window* mainWindow = Application::Get()->GetWindow();
			UpdateImGuizmo(mainWindow);

			// Mouse picking: left click on the scene (not on the gizmo, not with Alt) selects the mesh/submesh under the cursor
			if (viewportImageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing() &&
				!Input::IsKeyPressed(KeyH2M::LeftAlt) && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
			{
				glm::vec2 ndc = GetViewportMouseNdc();
				PickMesh(ndc.x, ndc.y);
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
			// model files onto the Meshes panel's "Load Mesh" button
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

						ImGuiWrapper::Property("Light Direction", s_Data.SceneData.LightDirectionTemp, 0.01f, -1.0f, 1.0f, PropertyFlag::DragProperty);
						ImGuiWrapper::Property("Light Radiance", s_LightRadiance, PropertyFlag::ColorProperty);
						ImGuiWrapper::Property("Light Multiplier", s_LightMultiplier, 0.01f, 0.0f, 5.0f, PropertyFlag::DragProperty);
						ImGuiWrapper::Property("Exposure", s_Exposure, 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty);
						ImGuiWrapper::Property("Auto Exposure", s_AutoExposureEnabled);
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
						ImGui::SetTooltip("Selected: the selected submesh, or the whole model when no submesh is selected\nAll: every loaded model");
					}
					ImGui::ColorEdit4("Wireframe Color", &overlay.WireframeColor.x, ImGuiColorEditFlags_NoInputs);

					ImGui::Separator();
					ImGui::Combo("Bounding Boxes", &overlay.BoundingBoxes, s_OverlayScopeNames, IM_ARRAYSIZE(s_OverlayScopeNames));
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Selected: the selected submesh's box, or all boxes of the model when no submesh is selected\nAll: the boxes of every loaded model");
					}
					ImGui::ColorEdit4("Selected Box", &overlay.SelectedBoundingBoxColor.x, ImGuiColorEditFlags_NoInputs);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Box of the selected submesh\n(all boxes of the selected mesh when no submesh is selected)");
					}
					ImGui::SameLine();
					ImGui::ColorEdit4("Unselected Boxes", &overlay.BoundingBoxColor.x, ImGuiColorEditFlags_NoInputs);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("All other boxes: other meshes, and the other submeshes of the selected mesh.\n"
							"Only drawn with Bounding Boxes = All.");
					}

					ImGui::Separator();
					ImGui::DragFloat("Line Width", &overlay.LineWidth, 0.05f, 1.0f, 10.0f, "%.1f px", ImGuiSliderFlags_AlwaysClamp);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Wireframe and bounding box lines");
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

			OnImGuiRenderMeshes();
			OnImGuiRenderMaterialLibrary();
			OnImGuiRenderMaterialEditor();

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
	// An environment map requested from the UI (button or drag & drop) is loaded here, before any command buffer
	// of this frame is recorded, not in the middle of the ImGui pass that requested it
	if (!s_PendingEnvMapFilename.empty())
	{
		std::string filepath = s_PendingEnvMapFilename;
		s_PendingEnvMapFilename.clear();
		LoadEnvironmentMap(filepath);
	}

	// Mesh removal / loading requested from the Meshes panel
	if (s_PendingRemoveMeshIndex >= 0)
	{
		if (s_PendingRemoveMeshIndex < (int)s_LoadedMeshes.size())
		{
			// The mesh's buffers and descriptor sets may still be used by frames in flight
			vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
			s_LoadedMeshes.erase(s_LoadedMeshes.begin() + s_PendingRemoveMeshIndex);
			s_SelectedMeshIndex = glm::min(s_SelectedMeshIndex, (int)s_LoadedMeshes.size() - 1);
			s_SelectedSubmeshIndex = -1;
		}
		s_PendingRemoveMeshIndex = -1;
	}
	if (!s_PendingMeshFilename.empty())
	{
		std::string filepath = s_PendingMeshFilename;
		std::optional<glm::vec3> groundPosition = s_PendingMeshGroundPosition;
		s_PendingMeshFilename.clear();
		s_PendingMeshGroundPosition.reset();
		LoadMesh(filepath, groundPosition);
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
	}
	s_PendingMaterialTextures.clear();
	if (s_PendingDeleteMaterial)
	{
		DeleteMaterial(s_PendingDeleteMaterial);
		s_PendingDeleteMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
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

	for (LoadedMeshVulkan& entry : s_LoadedMeshes)
	{
		if (entry.Mesh->IsSkinned())
		{
			entry.Mesh->OnUpdate(H2M::TimestepH2M(deltaTime), false); // bone matrices of the current frame (bind pose when not animated)
		}
		UpdateObjectUniforms(entry.Mesh);
		SubmitMeshTemp(entry.Mesh, entry.GetTransform(), entry.SubmeshMaterials);
	}
	UpdateFrameUniforms();

	if (s_ViewportFBNeedsResize)
	{
		s_Framebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
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


void EnvMapVulkanRenderer::RenderMeshWithoutMaterial(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform)
{
}

void EnvMapVulkanRenderer::RenderMesh(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform)
{
	// H2M::RendererH2M::Submit([mesh, transform]() mutable {});
	{
		H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_MeshPipeline.As<H2M::VulkanPipelineH2M>();

		VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

		auto vulkanMeshVB = mesh->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(s_Data.ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = H2M::RefH2M<H2M::VulkanIndexBufferH2M>(mesh->GetIndexBuffer());
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(s_Data.ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		std::vector<H2M::RefH2M<H2M::SubmeshH2M>>& submeshes = mesh->GetSubmeshes();
		for (H2M::RefH2M<H2M::SubmeshH2M> submesh : submeshes)
		{
			auto& material = mesh->GetMaterials()[submesh->MaterialIndex].As<H2M::VulkanMaterialH2M>();
			material->UpdateForRendering();

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points: set 0 per frame, set 1 the material's own set
			std::array<VkDescriptorSet, 2> descriptorSets = {
				s_Data.FrameDescriptorSet.DescriptorSets[0],
				material->GetDescriptorSet().DescriptorSets[0],
			};
			vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

			glm::mat4 worldTransform = transform * submesh->Transform;
			H2M::BufferH2M uniformStorageBuffer = material->GetUniformStorageBuffer();
			vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
			vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
			vkCmdDrawIndexed(s_Data.ActiveCommandBuffer, submesh->IndexCount, 1, submesh->BaseIndex, submesh->BaseVertex, 0);
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

	// The gizmo moves the mesh selected in the Meshes panel (or picked with the mouse)
	if (Scene::s_ImGuizmoType == -1 || s_SelectedMeshIndex < 0 || s_SelectedMeshIndex >= (int)s_LoadedMeshes.size() ||
		s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	LoadedMeshVulkan& entry = s_LoadedMeshes[s_SelectedMeshIndex];

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	// The scene image, not the whole window (which includes the tab bar)
	ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);

	// Snapping with Ctrl: 1 unit for translation/scale, 45 degrees for rotation
	bool snap = Input::IsKeyPressed(KeyH2M::LeftControl);
	float snapValue = Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE ? 45.0f : 1.0f;
	float snapValues[3] = { snapValue, snapValue, snapValue };

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
		// Back into the values shown (and editable) in the Meshes panel
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

glm::vec3 EnvMapVulkanRenderer::GetLightDirectionTemp()
{
	return s_Data.SceneData.LightDirectionTemp;
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

void EnvMapVulkanRenderer::MapUniformBuffersVTL(H2M::RefH2M<H2M::MeshH2M> mesh, const H2M::EditorCameraH2M& camera)
{
	// Temporary code
	s_Data.SceneData.SceneCamera.Camera = camera;

	H2M::RendererH2M::BeginRenderPass(s_Data.GeoPass);

	// Camera and scene data are per frame (set 0), shared by every mesh
	UpdateFrameUniforms();
	UpdateObjectUniforms(mesh);
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
