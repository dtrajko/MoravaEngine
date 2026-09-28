/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EnvMapVulkanRenderer.h"

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

#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "ImGuizmo.h"

#include "stb_image.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <filesystem>
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

// Editor grid on the ground plane (Resources/Shaders/Grid.glsl), as in SceneHazelEnvMap
static H2M::RefH2M<H2M::PipelineH2M> s_GridPipeline;
static H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet s_GridDescriptorSet; // allocated and written once (camera uniform buffer)
static bool s_DisplayGrid = true;
static float s_GridScale = 16.025f; // cells across the grid (it spans 32 x 32 units)
static float s_GridSize = 0.025f;   // line width, as a fraction of a cell
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipeline;                 // to be removed from VulkanRenderer
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
static std::vector<std::pair<H2M::RefH2M<H2M::MeshH2M>, glm::mat4>> s_Meshes; // meshes submitted for this frame, with their transforms

static H2M::RefH2M<H2M::SubmeshH2M> s_SelectedSubmesh;
static glm::mat4* s_Transform_ImGuizmo = nullptr;

struct VulkanRendererData
{
	VkCommandBuffer ActiveCommandBuffer = nullptr;
	H2M::RefH2M<H2M::Texture2D_H2M> BRDFLut;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet RendererDescriptorSetFeb2021;
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
	std::vector<std::array<bool, 4>> MaterialHasMap; // per material: albedo, normal, metalness, roughness map available (from the model or assigned)
	std::vector<std::array<H2M::RefH2M<H2M::Texture2D_H2M>, 4>> MaterialTextures; // maps assigned in the Material Editor (kept alive here)

	glm::mat4 GetTransform() const
	{
		return glm::translate(glm::mat4(1.0f), Translation) *
			glm::toMat4(glm::quat(glm::radians(Rotation))) *
			glm::scale(glm::mat4(1.0f), Scale);
	}
};
static std::vector<LoadedMeshVulkan> s_LoadedMeshes;
static int s_SelectedMeshIndex = -1;
static int s_SelectedSubmeshIndex = -1; // submesh of the selected mesh; -1 = none (the Material Editor then lists all materials)
static std::string s_PendingMeshFilename;  // requested from the UI, loaded at the start of the next Draw
static int s_PendingRemoveMeshIndex = -1;  // requested from the UI, removed at the start of the next Draw

static const char* s_MaterialMapToggles[4] = {
	"u_MaterialUniforms.AlbedoTexToggle",
	"u_MaterialUniforms.NormalTexToggle",
	"u_MaterialUniforms.MetalnessTexToggle",
	"u_MaterialUniforms.RoughnessTexToggle",
};

// Material texture bindings in HazelPBR_Static.glsl (set 0), in the same order as s_MaterialMapToggles
static const char* s_MaterialTextureNames[4] = { "u_AlbedoTexture", "u_NormalTexture", "u_MetalnessTexture", "u_RoughnessTexture" };

// A map assigned in the Material Editor (button or drag & drop), applied at the start of the next Draw
struct PendingMaterialTexture
{
	int MeshIndex;
	uint32_t MaterialIndex;
	uint32_t Slot; // 0 albedo, 1 normal, 2 metalness, 3 roughness
	std::string FilePath;
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

// Loads a model file and adds it to the scene (called at the start of a frame, see Draw)
static void LoadMesh(const std::string& filepath)
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

	// On Vulkan, MeshH2M loads animated models with the static vertex layout (see MeshH2M::Create)
	if (mesh->HasAnimations())
	{
		Log::GetLogger()->warn("Mesh '{0}' has animations: shown in its bind pose (skinning is not supported by the Vulkan PBR pipeline yet).", filepath);
	}

	LoadedMeshVulkan entry;
	entry.Mesh = mesh;
	entry.FilePath = filepath;

	// Which texture maps the model provided (the loader turns the toggle on for each map it found)
	for (auto& material : mesh->GetMaterials())
	{
		H2M::RefH2M<H2M::VulkanMaterialH2M> vulkanMaterial = material.As<H2M::VulkanMaterialH2M>();
		std::array<bool, 4> hasMap = {};
		for (uint32_t i = 0; i < 4; i++)
		{
			hasMap[i] = vulkanMaterial->Get<float>(s_MaterialMapToggles[i]) > 0.5f;
		}
		entry.MaterialHasMap.push_back(hasMap);
	}
	entry.MaterialTextures.resize(entry.MaterialHasMap.size());

	s_LoadedMeshes.push_back(entry);
	s_SelectedMeshIndex = (int)s_LoadedMeshes.size() - 1;
	s_SelectedSubmeshIndex = -1;
	Log::GetLogger()->info("Mesh '{0}' loaded: {1} submeshes, {2} materials", filepath, mesh->GetSubmeshes().size(), mesh->GetMaterials().size());
}

// Loads an image and binds it as a material's albedo/normal/metalness/roughness map (called at the start of a frame, see Draw)
static void ApplyMaterialTexture(const PendingMaterialTexture& request)
{
	if (request.MeshIndex < 0 || request.MeshIndex >= (int)s_LoadedMeshes.size() || request.Slot >= 4)
	{
		return;
	}
	LoadedMeshVulkan& entry = s_LoadedMeshes[request.MeshIndex];
	auto& materials = entry.Mesh->GetMaterials();
	if (request.MaterialIndex >= materials.size())
	{
		return;
	}

	if (!std::filesystem::exists(request.FilePath) || !IsImageFile(request.FilePath))
	{
		Log::GetLogger()->error("Map '{0}' was not loaded: the file does not exist or is not a supported image.", request.FilePath);
		return;
	}

	const H2M::MeshH2M::MaterialDescriptor* materialDescriptor = entry.Mesh->FindDescriptorSet(request.MaterialIndex);
	if (!materialDescriptor)
	{
		Log::GetLogger()->error("Map '{0}' was not loaded: material {1} has no descriptor set.", request.FilePath, request.MaterialIndex);
		return;
	}

	// Albedo is color data (sRGB); normal, metalness and roughness maps are linear data
	H2M::RefH2M<H2M::Texture2D_H2M> texture;
	try
	{
		texture = H2M::Texture2D_H2M::Create(request.FilePath, request.Slot == 0);
	}
	catch (...)
	{
		Log::GetLogger()->error("Map '{0}' could not be loaded.", request.FilePath);
		return;
	}

	H2M::RefH2M<H2M::VulkanShaderH2M> shader = entry.Mesh->GetMeshShader().As<H2M::VulkanShaderH2M>();
	const VkWriteDescriptorSet* binding = shader->GetDescriptorSet(s_MaterialTextureNames[request.Slot]);
	if (!binding)
	{
		Log::GetLogger()->error("Map '{0}' was not loaded: '{1}' not found in the shader.", request.FilePath, s_MaterialTextureNames[request.Slot]);
		return;
	}

	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	// The descriptor set may be used by frames in flight: updating it is only allowed when the GPU no longer uses it
	vkDeviceWaitIdle(device);

	VkWriteDescriptorSet write = *binding;
	write.dstSet = materialDescriptor->DescriptorSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pImageInfo = &texture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
	vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

	entry.MaterialTextures[request.MaterialIndex][request.Slot] = texture; // keeps the image alive while the set uses it
	entry.MaterialHasMap[request.MaterialIndex][request.Slot] = true;
	materials[request.MaterialIndex].As<H2M::VulkanMaterialH2M>()->Get<float>(s_MaterialMapToggles[request.Slot]) = 1.0f;

	Log::GetLogger()->info("Map '{0}' assigned to {1} of material '{2}'", request.FilePath, s_MaterialTextureNames[request.Slot], materials[request.MaterialIndex]->GetName());
}

// Every mesh has its own shader instance (see MeshH2M::Create), so the camera and light uniform buffers
// referenced by its material descriptor sets are updated per mesh
static void UpdateMeshUniforms(const H2M::RefH2M<H2M::MeshH2M>& mesh)
{
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = const_cast<H2M::RefH2M<H2M::MeshH2M>&>(mesh)->GetMeshShader().As<H2M::VulkanShaderH2M>();
	H2M::CameraH2M& camera = s_Data.SceneData.SceneCamera.Camera;

	// set 0, binding 0: Camera
	glm::mat4 viewProjection = camera.GetViewProjection();
	void* ubPtr = shader->MapUniformBuffer(0, 0);
	memcpy(ubPtr, &viewProjection, sizeof(glm::mat4));
	shader->UnmapUniformBuffer(0, 0);

	// set 0, binding 1: Environment (std140: Light { vec3 Direction; vec3 Radiance; float Multiplier; }, vec3 u_CameraPosition)
	struct Light
	{
		glm::vec3 Direction;
		float Padding = 0.0f;
		glm::vec3 Radiance;
		float Multiplier;
	};
	struct EnvironmentUB
	{
		Light Lights;
		glm::vec3 CameraPosition;
	};
	EnvironmentUB ub;
	ub.Lights.Direction = s_Data.SceneData.LightDirectionTemp;
	ub.Lights.Radiance = s_LightRadiance;
	ub.Lights.Multiplier = s_LightMultiplier;
	ub.CameraPosition = camera.GetPosition();

	ubPtr = shader->MapUniformBuffer(1, 0);
	memcpy(ubPtr, &ub, sizeof(EnvironmentUB));
	shader->UnmapUniformBuffer(1, 0);
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
	// A model file dropped from the Content Browser onto the button is loaded too
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
			if (IsModelFile(itemPath))
			{
				s_PendingMeshFilename = itemPath;
			}
			else
			{
				Log::GetLogger()->warn("'{0}' is not a supported model file", itemPath);
			}
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(or drop a model here from the Content Browser)");

	ImGui::Separator();

	if (s_LoadedMeshes.empty())
	{
		ImGui::TextDisabled("No meshes loaded");
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

		// Submeshes (as in SceneHazelEnvMap): select one to edit its material in the Material Editor,
		// and choose which of the model's materials it is drawn with
		auto& submeshes = entry.Mesh->GetSubmeshes();
		auto& materials = entry.Mesh->GetMaterials();

		ImGui::Separator();
		ImGui::Text("Submeshes (%d)", (int)submeshes.size());

		for (int s = 0; s < (int)submeshes.size(); s++)
		{
			H2M::RefH2M<H2M::SubmeshH2M> submesh = submeshes[s];
			ImGui::PushID(1000 + s);

			std::string submeshName = !submesh->MeshName.empty() ? submesh->MeshName :
				(!submesh->NodeName.empty() ? submesh->NodeName : "Submesh " + std::to_string(s));
			if (ImGui::Selectable(submeshName.c_str(), s_SelectedSubmeshIndex == s, 0, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f)))
			{
				s_SelectedSubmeshIndex = (s_SelectedSubmeshIndex == s) ? -1 : s; // click again to show all materials
			}

			// Material used by this submesh
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1.0f);
			auto materialName = [&](uint32_t index) {
				return index < materials.size() && !materials[index]->GetName().empty() ? materials[index]->GetName() : "Material " + std::to_string(index);
			};
			if (ImGui::BeginCombo("##material", materialName(submesh->MaterialIndex).c_str()))
			{
				for (uint32_t m = 0; m < (uint32_t)materials.size(); m++)
				{
					ImGui::PushID((int)m);
					if (ImGui::Selectable(materialName(m).c_str(), submesh->MaterialIndex == m))
					{
						submesh->MaterialIndex = m; // used by RenderMeshVulkan from the next frame
					}
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}

			ImGui::PopID();
		}
	}

	ImGui::End();
}

// Edits the selected mesh's material values in place (they are pushed as push constants for every draw)
static void OnImGuiRenderMaterialEditor()
{
	ImGui::Begin("Material Editor");

	if (s_SelectedMeshIndex < 0 || s_SelectedMeshIndex >= (int)s_LoadedMeshes.size())
	{
		ImGui::TextDisabled("Select a mesh in the Meshes panel");
		ImGui::End();
		return;
	}

	LoadedMeshVulkan& entry = s_LoadedMeshes[s_SelectedMeshIndex];
	auto& materials = entry.Mesh->GetMaterials();

	static const char* s_MapLabels[4] = { "Use Albedo Map", "Use Normal Map", "Use Metalness Map", "Use Roughness Map" };

	// With a submesh selected (Meshes panel), only the material that submesh is drawn with is shown
	auto& submeshes = entry.Mesh->GetSubmeshes();
	int selectedMaterial = -1;
	if (s_SelectedSubmeshIndex >= 0 && s_SelectedSubmeshIndex < (int)submeshes.size())
	{
		H2M::RefH2M<H2M::SubmeshH2M> submesh = submeshes[s_SelectedSubmeshIndex];
		selectedMaterial = (int)submesh->MaterialIndex;

		std::string submeshName = !submesh->MeshName.empty() ? submesh->MeshName :
			(!submesh->NodeName.empty() ? submesh->NodeName : "Submesh " + std::to_string(s_SelectedSubmeshIndex));
		ImGui::Text("Submesh: %s", submeshName.c_str());

		int sharedBy = 0;
		for (auto& other : submeshes)
		{
			sharedBy += other->MaterialIndex == submesh->MaterialIndex ? 1 : 0;
		}
		if (sharedBy > 1)
		{
			ImGui::TextDisabled("This material is used by %d submeshes: changes apply to all of them.", sharedBy);
		}
	}
	else
	{
		ImGui::TextDisabled("All materials (select a submesh in the Meshes panel to edit only its material)");
	}

	for (uint32_t m = 0; m < (uint32_t)materials.size(); m++)
	{
		if (selectedMaterial >= 0 && (int)m != selectedMaterial)
		{
			continue;
		}

		ImGui::PushID((int)m);

		H2M::RefH2M<H2M::VulkanMaterialH2M> material = materials[m].As<H2M::VulkanMaterialH2M>();
		std::string name = material->GetName().empty() ? "(unnamed)" : material->GetName();

		if (ImGui::CollapsingHeader(name.c_str(), (m == 0 || selectedMaterial >= 0) ? ImGuiTreeNodeFlags_DefaultOpen : 0))
		{
			glm::vec3& albedoColor = material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
			float& metalness = material->Get<float>("u_MaterialUniforms.Metalness");
			float& roughness = material->Get<float>("u_MaterialUniforms.Roughness");

			ImGui::ColorEdit3("Albedo Color", &albedoColor.x);
			ImGui::SliderFloat("Metalness", &metalness, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f);

			for (uint32_t i = 0; i < 4; i++)
			{
				ImGui::PushID((int)i);
				ImGui::Separator();

				bool hasMap = m < entry.MaterialHasMap.size() && entry.MaterialHasMap[m][i];
				H2M::RefH2M<H2M::Texture2D_H2M> assignedMap = m < entry.MaterialTextures.size() ? entry.MaterialTextures[m][i] : H2M::RefH2M<H2M::Texture2D_H2M>();

				// Thumbnail of a map assigned here (maps that came with the model have no texture object in this list);
				// also a drop target for images from the Content Browser
				const ImVec2 thumbnailSize(64.0f, 64.0f);
				ImTextureID thumbnail = assignedMap ? assignedMap->GetImTextureID() : ImTextureID{};
				if (thumbnail)
				{
					ImGui::Image(thumbnail, thumbnailSize, ImVec2(0, 1), ImVec2(1, 0)); // the Vulkan loader flips LDR images
				}
				else
				{
					ImGui::Button(hasMap ? "model\nmap" : "no map", thumbnailSize);
				}
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
						if (IsImageFile(itemPath))
						{
							s_PendingMaterialTextures.push_back({ s_SelectedMeshIndex, m, i, itemPath });
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
					ImGui::SetTooltip("%s", assignedMap ? assignedMap->GetPath().c_str() : "Drop an image from the Content Browser here");
				}

				ImGui::SameLine();
				ImGui::BeginGroup();
				{
					float& toggle = material->Get<float>(s_MaterialMapToggles[i]);
					bool enabled = toggle > 0.5f;

					ImGui::BeginDisabled(!hasMap);
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
							s_PendingMaterialTextures.push_back({ s_SelectedMeshIndex, m, i, filepath }); // applied at the start of the next frame
						}
					}
					if (assignedMap)
					{
						ImGui::SameLine();
						ImGui::TextDisabled("%s", std::filesystem::path(assignedMap->GetPath()).filename().string().c_str());
					}
					else if (!hasMap)
					{
						ImGui::SameLine();
						ImGui::TextDisabled("(no map in the model)");
					}
				}
				ImGui::EndGroup();

				ImGui::PopID();
			}
		}

		ImGui::PopID();
	}

	ImGui::End();
}

/**** BEGIN to be removed from VulkanRenderer ****/
void EnvMapVulkanRenderer::SubmitMeshTemp(const H2M::RefH2M<H2M::MeshH2M>& mesh, const glm::mat4& transform)
{
	// Temporary code - populate selected submesh
	// std::vector<Submesh> submeshes = mesh->GetSubmeshes();
	// s_SelectedSubmesh = &submeshes.at(0);

	s_Meshes.push_back({ mesh, transform });

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
		pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("SceneComposite");

		H2M::RenderPassSpecificationH2M renderPassSpec;
		renderPassSpec.TargetFramebuffer = s_ViewportCompositeFramebuffer;
		pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
		pipelineSpecification.DebugName = "ViewportComposite";
		s_ViewportCompositePipeline = H2M::PipelineH2M::Create(pipelineSpecification);
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
		auto shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");
		H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = shader.As<H2M::VulkanShaderH2M>();
		s_Data.RendererDescriptorSetFeb2021 = pbrShader->CreateDescriptorSets(1);
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
	s_Data.SceneData.LightDirectionTemp = { 0.5f, 0.5f, 0.5f };

	OnResize(s_ViewportWidth, s_ViewportHeight); // to be removed from VulkanRenderer
}

void EnvMapVulkanRenderer::Shutdown()
{
	H2M::VulkanShaderH2M::ClearUniformBuffers();
	// delete s_Data;
}

void EnvMapVulkanRenderer::RenderMeshVulkan(H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform, VkCommandBuffer commandBuffer)
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
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = s_MeshPipeline.As<H2M::VulkanPipelineH2M>(); // to be removed from VulkanRenderer
	/**** END Composite ****/

	VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

	auto vulkanMeshVB = mesh->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>();
	VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

	auto vulkanMeshIB = H2M::RefH2M<H2M::VulkanIndexBufferH2M>(mesh->GetIndexBuffer());
	VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
	vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

	auto& submeshes = mesh->GetSubmeshes();
	for (H2M::RefH2M<H2M::SubmeshH2M> submesh : submeshes)
	{
		auto& material = mesh->GetMaterials()[submesh->MaterialIndex];
		material->Set("u_MaterialUniforms.EnvMapRotation", s_EnvMapRotation); // global setting (Environment panel)
		H2M::BufferH2M uniformStorageBuffer = material->GetUniformStorageBuffer();

		// The PBR pipeline needs the submesh's material descriptor set (set 0). Skip submeshes without one
		// (drawing with a missing/null descriptor set is undefined behavior in Vulkan).
		const H2M::MeshH2M::MaterialDescriptor* materialDescriptor = mesh->FindDescriptorSet(submesh->MaterialIndex);
		if (!materialDescriptor || materialDescriptor->DescriptorSet.DescriptorSets[0] == VK_NULL_HANDLE)
		{
			static std::set<std::string> s_ReportedMeshes;
			if (s_ReportedMeshes.insert(mesh->GetFilePath()).second)
			{
				Log::GetLogger()->warn("EnvMapVulkanRenderer: mesh '{0}' (submesh '{1}', material index {2}) has no material descriptor set - not drawn",
					mesh->GetFilePath(), submesh->MeshName, submesh->MaterialIndex);
			}
			continue;
		}

		VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Bind descriptor sets describing shader binding points
		const std::vector<VkDescriptorSet>& descriptorSet = materialDescriptor->DescriptorSet.DescriptorSets;
		// std::vector<VkDescriptorSet> descriptorSet = material.As<VulkanMaterialH2M>()->GetDescriptorSet().DescriptorSets;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet rendererDescriptorSet = s_Data.RendererDescriptorSetFeb2021;

		std::array<VkDescriptorSet, 2> descriptorSets = {
			*descriptorSet.data(),
			*rendererDescriptorSet.DescriptorSets.data(),
		};

		// VkDescriptorSet* descriptorSet = (VkDescriptorSet*)mesh->GetDescriptorSet();
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

		// Push Constants
		// glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		// vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
		glm::mat4 submeshTransform = transform * submesh->Transform; // mesh transform (Meshes panel) * node transform from the model
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
	float skyboxLod = s_Data.SceneData.SkyboxLod;
	vkCmdPushConstants(commandBuffer, skyboxPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &skyboxLod);

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

		writeDescriptors[0] = *pbrShader->GetDescriptorSet("u_EnvRadianceTex", 1);
		writeDescriptors[0].dstSet = *s_Data.RendererDescriptorSetFeb2021.DescriptorSets.data();
		writeDescriptors[0].descriptorCount = (uint32_t)s_Data.RendererDescriptorSetFeb2021.DescriptorSets.size();
		auto& radianceMapImageInfo = radianceMap->GetVulkanDescriptorInfo();
		writeDescriptors[0].pImageInfo = &radianceMapImageInfo;

		writeDescriptors[1] = *pbrShader->GetDescriptorSet("u_EnvIrradianceTex", 1);
		writeDescriptors[1].dstSet = *s_Data.RendererDescriptorSetFeb2021.DescriptorSets.data();
		writeDescriptors[1].descriptorCount = (uint32_t)s_Data.RendererDescriptorSetFeb2021.DescriptorSets.size();
		auto& irradianceMapImageInfo = irradianceMap->GetVulkanDescriptorInfo();
		writeDescriptors[1].pImageInfo = &irradianceMapImageInfo;

		writeDescriptors[2] = *pbrShader->GetDescriptorSet("u_BRDFLUTTexture", 1);
		writeDescriptors[2].dstSet = *s_Data.RendererDescriptorSetFeb2021.DescriptorSets.data();
		writeDescriptors[2].descriptorCount = (uint32_t)s_Data.RendererDescriptorSetFeb2021.DescriptorSets.size();
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

		for (auto& [mesh, transform] : s_Meshes)
		{
			RenderMeshVulkan(mesh, transform, drawCommandBuffer);
		}

		s_Meshes.clear();

		// Transparent, so after the opaque meshes
		if (s_DisplayGrid)
		{
			RenderGrid(drawCommandBuffer);
		}

		vkCmdEndRenderPass(drawCommandBuffer);

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

	float exposure = s_Exposure * (s_AutoExposureEnabled ? s_EnvMapAutoExposure : 1.0f);
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &exposure);

	// QuadDescriptorSet samples s_Framebuffer's color image (rewritten when s_Framebuffer is resized)
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)s_Data.QuadDescriptorSet.DescriptorSets.size(), s_Data.QuadDescriptorSet.DescriptorSets.data(), 0, nullptr);

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
						ImGuiWrapper::Property("Env Map Rotation", s_EnvMapRotation, 1.0f, -360.0f, 360.0f, PropertyFlag::DragProperty);
						ImGuiWrapper::Property("Display Grid", s_DisplayGrid);
						ImGuiWrapper::Property("Grid Scale", s_GridScale, 0.1f, 1.0f, 256.0f, PropertyFlag::DragProperty);
						ImGuiWrapper::Property("Grid Line Width", s_GridSize, 0.001f, 0.001f, 0.5f, PropertyFlag::DragProperty);

						ImGui::Columns(1);
					}
				}
			}
			ImGui::End();
			/**** END Environment ****/

			OnImGuiRenderMeshes();
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
			s_PendingMaterialTextures.clear(); // their mesh indices refer to the list before the removal
			s_SelectedMeshIndex = glm::min(s_SelectedMeshIndex, (int)s_LoadedMeshes.size() - 1);
			s_SelectedSubmeshIndex = -1;
		}
		s_PendingRemoveMeshIndex = -1;
	}
	if (!s_PendingMeshFilename.empty())
	{
		std::string filepath = s_PendingMeshFilename;
		s_PendingMeshFilename.clear();
		LoadMesh(filepath);
	}
	// Maps assigned in the Material Editor
	for (const PendingMaterialTexture& request : s_PendingMaterialTextures)
	{
		ApplyMaterialTexture(request);
	}
	s_PendingMaterialTextures.clear();

	s_Data.SceneData.SceneCamera.Camera = *camera;

	for (LoadedMeshVulkan& entry : s_LoadedMeshes)
	{
		UpdateMeshUniforms(entry.Mesh);
		SubmitMeshTemp(entry.Mesh, entry.GetTransform());
	}

	if (s_ViewportFBNeedsResize)
	{
		s_Framebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		s_ViewportCompositeFramebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
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

			// Bind descriptor sets describing shader binding points
			std::array<VkDescriptorSet, 2> descriptorSets = {
				// mesh->GetDescriptorSet(submesh->MaterialIndex).DescriptorSet.DescriptorSets[0],
				material->GetDescriptorSet().DescriptorSets[0],
				s_Data.RendererDescriptorSetFeb2021.DescriptorSets[0],
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

	// ImGizmo switching modes
	if (Input::IsKeyPressed(KeyH2M::D1))
		Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

	if (Input::IsKeyPressed(KeyH2M::D2))
		Scene::s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;

	if (Input::IsKeyPressed(KeyH2M::D3))
		Scene::s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;

	if (Input::IsKeyPressed(KeyH2M::D4))
		Scene::s_ImGuizmoType = -1;

	// ImGuizmo
	if (Scene::s_ImGuizmoType != -1)
	{
		float rw = (float)ImGui::GetWindowWidth();
		float rh = (float)ImGui::GetWindowHeight();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

		if (s_SelectedSubmesh) {
			s_Transform_ImGuizmo = &s_SelectedSubmesh->Transform; // Connect to model transform
		}

		// Snapping
		bool snap = Input::IsKeyPressed(KeyH2M::LeftControl);
		float snapValue = 1.0f; // Snap to 0.5m for translation/scale
		// Snap to 45 degrees for rotation
		if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE) {
			snapValue = 45.0f;
		}

		float snapValues[3] = { snapValue, snapValue, snapValue };

		if (s_Transform_ImGuizmo != nullptr) // TODO: specify display criteria here
		{
			ImGuizmo::Manipulate(
				glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix()),
				glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix()),
				(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
				ImGuizmo::WORLD,
				glm::value_ptr(*s_Transform_ImGuizmo),
				nullptr,
				snap ? snapValues : nullptr);
		}
	}
	// END ImGuizmo
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

	auto viewProjection = s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix() * s_Data.SceneData.SceneCamera.ViewMatrix;
	// glm::vec3 cameraPosition = glm::inverse(s_Data.SceneData.SceneCamera.ViewMatrix)[3];
	glm::vec3 cameraPosition = camera.GetPosition();

	// float skyboxLod = s_Data.ActiveScene->GetSkyboxLod();
	// H2M::RendererH2M::Submit([viewProjection, cameraPosition]() {});
	{
		auto inverseVP = glm::inverse(viewProjection);
		// auto shader = s_Data.GridMaterial->GetShader().As<H2M::VulkanShaderH2M>();
		// void* ubPtr = shader->MapUniformBuffer(0);
		struct ViewProj
		{
			glm::mat4 ViewProjection;
			glm::mat4 InverseViewProjection;
		};
		ViewProj viewProj;
		viewProj.ViewProjection = viewProjection;
		viewProj.InverseViewProjection = inverseVP;
		// memcpy(ubPtr, &viewProj, sizeof(ViewProj));
		// shader->UnmapUniformBuffer(0);

		// shader = s_Data.SkyboxMaterial->GetShader().As<H2M::VulkanShaderH2M>();
		// ubPtr = shader->MapUniformBuffer(0);
		// memcpy(ubPtr, &viewProj, sizeof(ViewProj));
		// shader->UnmapUniformBuffer(0);

		// shader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
		// ubPtr = shader->MapUniformBuffer(0);
		// memcpy(ubPtr, &viewProj, sizeof(ViewProj));
		// shader->UnmapUniformBuffer(0);

		H2M::RefH2M<H2M::VulkanShaderH2M> shader = mesh->GetMeshShader().As<H2M::VulkanShaderH2M>();

		{
			void* ubPtr = shader->MapUniformBuffer(0, 0);
			glm::mat4 viewProj = camera.GetViewProjection();
			memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
			shader->UnmapUniformBuffer(0, 0);
		}

		struct Light
		{
			glm::vec3 Direction;
			float Padding = 0.0f;
			glm::vec3 Radiance;
			float Multiplier;
		};

		struct UB
		{
			Light lights;
			glm::vec3 u_CameraPosition;
			// glm::vec4 u_AlbedoColorUB;
		};

		UB ub;
		ub.lights =
		{
			{ 0.5f, 0.5f, 0.5f },
			0.0f,
			{ 1.0f, 1.0f, 1.0f },
			1.0f
		};

		ub.lights.Direction = EnvMapVulkanRenderer::GetLightDirectionTemp();
		ub.u_CameraPosition = cameraPosition;
		// ub.u_AlbedoColorUB = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

		// Log::GetLogger()->info("Light Direction: {0}, {1}, {2}", ub.lights.Direction.x, ub.lights.Direction.y, ub.lights.Direction.z);

		void* ubPtr = shader->MapUniformBuffer(1, 0);
		memcpy(ubPtr, &ub, sizeof(UB));
		shader->UnmapUniformBuffer(1, 0);
	}
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
