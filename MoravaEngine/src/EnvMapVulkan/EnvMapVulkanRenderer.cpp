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
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/Renderer2D_H2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"

#include "Platform/Vulkan/VulkanSkyboxCube.h"

#include "imgui.h"

#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan_with_textures.h"

#include "ImGuizmo.h"


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
static H2M::RefH2M<H2M::PipelineH2M> s_MeshPipeline;                 // to be removed from VulkanRenderer
static ImTextureID s_TextureID;                      // to be removed from VulkanRenderer
static uint32_t s_ViewportWidth = 1280;              // to be removed from VulkanRenderer
static uint32_t s_ViewportHeight = 720;              // to be removed from VulkanRenderer
static std::vector<H2M::RefH2M<H2M::MeshH2M>> s_Meshes;         // to be removed from VulkanRenderer

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

/**** BEGIN to be removed from VulkanRenderer ****/
void EnvMapVulkanRenderer::SubmitMeshTemp(const H2M::RefH2M<H2M::MeshH2M>& mesh, const glm::mat4& transform)
{
	// Temporary code - populate selected submesh
	// std::vector<Submesh> submeshes = mesh->GetSubmeshes();
	// s_SelectedSubmesh = &submeshes.at(0);

	s_Meshes.push_back(mesh);

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
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap.glsl", true);
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentMipFilter.glsl", true);
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance.glsl", true);
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/HazelPBR_Static.glsl");
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/Texture.glsl");
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/SceneComposite.glsl");
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/Grid.glsl");
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/Outline.glsl");

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
				Log::GetLogger()->warn("Resizing framebuffer; image layout is {0}", imageInfo.imageLayout);
				// s_TextureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
				s_TextureID = ImGui_ImplVulkan_UpdateTextureInfo((VkDescriptorSet)s_TextureID, imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);

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
				H2M_CORE_WARN("Resizing framebuffer; image layout is {0}", imageInfo.imageLayout);
				s_TextureID = ImGui_ImplVulkan_UpdateTextureInfo((VkDescriptorSet)s_TextureID, imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
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
		s_TextureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
	}

	// s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap("Textures/HDR/pink_sunrise_4k.hdr");
	// s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap("Textures/HDR/umhlanga_sunrise_4k.hdr");
	// s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap("Textures/HDR/venice_dawn_1_4k.hdr");
	s_Data.EnvironmentMap = H2M::RendererH2M::CreateEnvironmentMap("Textures/HDR/newport_loft.hdr");

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

void EnvMapVulkanRenderer::RenderMeshVulkan(H2M::RefH2M<H2M::MeshH2M> mesh, VkCommandBuffer commandBuffer)
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
		H2M::BufferH2M uniformStorageBuffer = material->GetUniformStorageBuffer();

		VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Bind descriptor sets describing shader binding points
		std::vector<VkDescriptorSet> descriptorSet = mesh->GetDescriptorSet(submesh->MaterialIndex).DescriptorSet.DescriptorSets;
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
		glm::mat4 submeshTransform = submesh->Transform;
		submeshTransform = glm::scale(submeshTransform, glm::vec3(0.2f));
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

	std::array<VkWriteDescriptorSet, 2> writeDescriptors;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet descriptorSet = vulkanSkyboxShader->CreateDescriptorSets();

	writeDescriptors[0] = *vulkanSkyboxShader->GetDescriptorSet("Camera");
	writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
	writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
	writeDescriptors[0].pBufferInfo = &vulkanSkyboxShader->GetUniformBuffer(0, 0).Descriptor;

	// H2M::RefH2M<H2M::VulkanTextureCubeH2M> envUnfilteredCubemap = s_Data.envUnfiltered.As<H2M::VulkanTextureCubeH2M>();
	H2M::RefH2M<H2M::VulkanTextureCubeH2M> envFilteredCubemap = s_Data.envFiltered.As<H2M::VulkanTextureCubeH2M>();
	writeDescriptors[1] = *vulkanSkyboxShader->GetDescriptorSet("u_Texture");
	writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
	writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
	writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

	vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

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

		for (auto& mesh : s_Meshes)
		{
			RenderMeshVulkan(mesh, drawCommandBuffer);
		}

		s_Meshes.clear();

		vkCmdEndRenderPass(drawCommandBuffer);
	}
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

		float exposure = 0.8f; // s_Data.Exposure;
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
			auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
			auto viewportSize = ImGui::GetContentRegionAvail();
			ImGui::Image(s_TextureID, viewportSize, { 0, 1 }, { 1, 0 });

			if (s_ViewportWidth != viewportSize.x || s_ViewportHeight != viewportSize.y)
			{
				s_ViewportWidth = (uint32_t)viewportSize.x;
				s_ViewportHeight = (uint32_t)viewportSize.y;
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

			bool showContentBrowserPanel = true;
			// VulkanTestLayer::s_ContentBrowserPanel->OnImGuiRender(&showContentBrowserPanel);

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

						ImGui::InputText("##envmapfilepath", "", 256, ImGuiInputTextFlags_ReadOnly);

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
							{
								std::wstring itemPath = std::wstring((const wchar_t*)payload->Data);
								size_t itemSize = payload->DataSize;
								Log::GetLogger()->debug("END DRAG & DROP FILE '{0}', size: {1}", Util::to_str(itemPath.c_str()).c_str(), itemSize);

								//	m_EnvMapFilename = std::string{ itemPath.begin(), itemPath.end() };
								//	if (m_EnvMapFilename != "")
								//	{
								//		EnvMapSceneRenderer::SetEnvironment(EnvMapSceneRenderer::Load(m_EnvMapFilename));
								//	}
							}
							ImGui::EndDragDropTarget();
						}

						ImGui::NextColumn();

						if (ImGui::Button("Load Environment Map"))
						{
							//	m_EnvMapFilename = Application::Get()->OpenFile("*.hdr");
							//	if (m_EnvMapFilename != "")
							//	{
							//		EnvMapSceneRenderer::SetEnvironment(EnvMapSceneRenderer::Load(m_EnvMapFilename));
							//	}
						}

						ImGui::NextColumn();

						ImGui::AlignTextToFramePadding();

						if (ImGuiWrapper::Property("Skybox LOD", s_Data.SceneData.SkyboxLod, 0.01f, 0.0f, 4.0f, PropertyFlag::DragProperty))
						{
							// SetSkyboxLOD(skyboxLOD);
						}

						H2M::LightH2M light; // = EnvMapSceneRenderer::GetActiveLight();
						H2M::LightH2M lightPrev = light;

						ImGuiWrapper::Property("Light Direction", s_Data.SceneData.LightDirectionTemp, 0.01f, -1.0f, 1.0f, PropertyFlag::DragProperty);
						ImGuiWrapper::Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
						ImGuiWrapper::Property("Light Multiplier", light.Multiplier, 0.01f, 0.0f, 5.0f, PropertyFlag::DragProperty);
						float exposure = 0.8f; // s_Data.Exposure;
						ImGuiWrapper::Property("Exposure", exposure, 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty);

						float radiancePrefilter = 1.0f; // EnvMapSharedData::s_RadiancePrefilter
						ImGuiWrapper::Property("Radiance Prefiltering", radiancePrefilter);
						float envMapRotation = 0.0f; // EnvMapSharedData::s_EnvMapRotation;
						ImGuiWrapper::Property("Env Map Rotation", envMapRotation, 1.0f, -360.0f, 360.0f, PropertyFlag::DragProperty);

						ImGui::Columns(1);
					}

					ImGui::Separator();

					{
						ImGui::Text("Mesh");

						// H2M::RefH2M<Hazel::Entity> meshEntity;
						std::string meshFullPath = "None";

						std::string fileName = Util::GetFileNameFromFullPath(meshFullPath);
						ImGui::Text(fileName.c_str()); ImGui::SameLine();
						if (ImGui::Button("...##Mesh"))
						{
							std::string fullPath = Application::Get()->OpenFile();
							if (fullPath != "")
							{
								// Hazel::Entity entity = LoadEntity(fullPath);
							}
						}

						//	auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
						//	if (meshEntities.size())
						//	{
						//		meshEntity = GetMeshEntity();
						//		auto& meshComponent = meshEntity->GetComponent<Hazel::MeshComponent>();
						//		if (meshComponent.Mesh) {
						//			ImGui::SameLine();
						//			ImGui::Checkbox("Is Animated", &meshComponent.Mesh->IsAnimated());
						//		}
						//	}
					}
				}
			}
			ImGui::End();
			/**** END Environment ****/

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
	s_Data.SceneData.SceneCamera.Camera = *camera;

	if (s_ViewportFBNeedsResize)
	{
		s_Framebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
		s_ViewportFBNeedsResize = false;
	}

	GeometryPass();
	CompositePass();
}

std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> EnvMapVulkanRenderer::CreateEnvironmentMap(const std::string& filepath)
{
	const uint32_t cubemapSize = 1024;
	const uint32_t irradianceMapSize = 32;

	if (!s_Data.envUnfiltered)
	{
		s_Data.envUnfiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize);
	}

	s_Data.envEquirect = H2M::Texture2D_H2M::Create(filepath, false);
	// ImageFormatH2M envEquirectImageFormat = s_Data.envEquirect->GetFormat(); // Vulkan Live 18.03.2021 #2: s_Data.envEquirect->GetImage()->GetFormat();

	/****
	H2M_CORE_ASSERT(s_Data.envEquirect->GetFormat() == ImageFormatH2M::RGBA16F, "Texture is not HDR!");
	if (envEquirectImageFormat != ImageFormatH2M::RGBA16F)
	{
		Log::GetLogger()->error("Texture '{0}' is not HDR (format: '{1}')!", filepath, envEquirectImageFormat);
		return std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>>();
	}
	****/

	// Convert equirectangular to cubemap
	H2M::RefH2M<H2M::ShaderH2M> equirectangularConversionShader = H2M::RendererH2M::GetShaderLibrary()->Get("EquirectangularToCubeMap");
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> equirectangularConversionPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(equirectangularConversionShader);

	// H2M::RendererH2M::Submit([equirectangularConversionPipeline, envUnfiltered, envEquirect, cubemapSize]() mutable {});
	{
		VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = equirectangularConversionPipeline->GetShader();

		std::array<VkWriteDescriptorSet, 2> writeDescriptors;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet descriptorSet = shader->CreateDescriptorSets();

		H2M::RefH2M<H2M::VulkanTextureCubeH2M> envUnfilteredCubemap = s_Data.envUnfiltered.As<H2M::VulkanTextureCubeH2M>();
		writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
		writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
		writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

		H2M::RefH2M<H2M::VulkanTexture2D_H2M> envEquirectVK = s_Data.envEquirect.As<H2M::VulkanTexture2D_H2M>();
		writeDescriptors[1] = *shader->GetDescriptorSet("u_EquirectangularTex");
		writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
		writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[1].pImageInfo = &envEquirectVK->GetVulkanDescriptorInfo();

		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		equirectangularConversionPipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), cubemapSize / 32, cubemapSize / 32, 6);

		VkQueue computeQueue = H2M::VulkanContextH2M::GetCurrentDevice()->GetComputeQueue();
		vkQueueWaitIdle(computeQueue);

		envUnfilteredCubemap->GenerateMips(true);
	}

	// MipFiltering
	H2M::RefH2M<H2M::ShaderH2M> environmentMipFilterShader = H2M::RendererH2M::GetShaderLibrary()->Get("EnvironmentMipFilter");
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> environmentMipFilterPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(environmentMipFilterShader);

	if (!s_Data.envFiltered)
	{
		s_Data.envFiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize);
	}

	// H2M::RendererH2M::Submit([environmentMipFilterPipeline, cubemapSize, envFiltered, envUnfiltered]() mutable {});
	{
		VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = environmentMipFilterPipeline->GetShader();

		H2M::RefH2M<H2M::VulkanTextureCubeH2M> envFilteredCubemap = s_Data.envFiltered.As<H2M::VulkanTextureCubeH2M>();
		VkDescriptorImageInfo imageInfo = envFilteredCubemap->GetVulkanDescriptorInfo();

		uint32_t totalMipLevels = s_MipMapsEnabled ? 11 : 1;

		std::vector<VkWriteDescriptorSet> writeDescriptors;
		std::vector<VkDescriptorImageInfo> mipImageInfos;

		writeDescriptors.resize(totalMipLevels * 2);
		mipImageInfos.resize(totalMipLevels);

		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet descriptorSet = shader->CreateDescriptorSets(0, totalMipLevels);

		for (uint32_t i = 0; i < totalMipLevels; i++)
		{
			VkDescriptorImageInfo& mipImageInfo = mipImageInfos[i];
			mipImageInfo = imageInfo;
			mipImageInfo.imageView = envFilteredCubemap->CreateImageViewSingleMip(i);
			mipImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			writeDescriptors[i * 2 + 0] = *shader->GetDescriptorSet("outputTexture");
			writeDescriptors[i * 2 + 0].dstSet = descriptorSet.DescriptorSets[i];
			writeDescriptors[i * 2 + 0].pImageInfo = &mipImageInfo;

			H2M::RefH2M<H2M::VulkanTextureCubeH2M> envUnfilteredCubemap = s_Data.envUnfiltered.As<H2M::VulkanTextureCubeH2M>();
			writeDescriptors[i * 2 + 1] = *shader->GetDescriptorSet("inputTexture");
			writeDescriptors[i * 2 + 1].dstSet = descriptorSet.DescriptorSets[i];
			writeDescriptors[i * 2 + 1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
		}

		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

		environmentMipFilterPipeline->Begin(); // begin compute pass
		const float deltaRoughness = 1.0f / glm::max((float)s_Data.envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
		for (uint32_t i = 0, size = cubemapSize; i < totalMipLevels; i++, size /= 2)
		{
			uint32_t numGroups = glm::max(1u, size / 32);
			float roughness = i * deltaRoughness;
			roughness = glm::max(roughness, 0.05f);
			environmentMipFilterPipeline->SetPushConstants(&roughness, sizeof(float));
			environmentMipFilterPipeline->Dispatch(descriptorSet.DescriptorSets[i], numGroups, numGroups, 6);
		}
		environmentMipFilterPipeline->End();

		VkQueue computeQueue = H2M::VulkanContextH2M::GetCurrentDevice()->GetComputeQueue();
		vkQueueWaitIdle(computeQueue);
	}

	// Irradiance map
	H2M::RefH2M<H2M::ShaderH2M> environmentIrradianceShader = H2M::RendererH2M::GetShaderLibrary()->Get("EnvironmentIrradiance");
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> environmentIrradiancePipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(environmentIrradianceShader);


	if (!s_Data.irradianceMap)
	{
		// s_Data.irradianceMap = TextureCubeH2M::Create(ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize);
		s_Data.irradianceMap = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, irradianceMapSize, irradianceMapSize);
	}

	// H2M::RendererH2M::Submit([environmentIrradiancePipeline, envFilteredCubemap, s_Data.irradianceMap, irradianceMapSize]() mutable {});
	{
		VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = environmentIrradiancePipeline->GetShader();

		std::array<VkWriteDescriptorSet, 2> writeDescriptors;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet descriptorSet = shader->CreateDescriptorSets();

		H2M::RefH2M<H2M::VulkanTextureCubeH2M> irradianceCubemap = s_Data.irradianceMap.As<H2M::VulkanTextureCubeH2M>();
		writeDescriptors[0] = *shader->GetDescriptorSet("o_IrradianceMap");
		writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data();
		writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[0].pImageInfo = &irradianceCubemap->GetVulkanDescriptorInfo();

		H2M::RefH2M<H2M::VulkanTextureCubeH2M> envFilteredCubemap = s_Data.envFiltered.As<H2M::VulkanTextureCubeH2M>();
		writeDescriptors[1] = *shader->GetDescriptorSet("u_RadianceMap");
		writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data();
		writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		environmentIrradiancePipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), irradianceCubemap->GetWidth() / 32, irradianceCubemap->GetHeight() / 32, 6);

		VkQueue computeQueue = H2M::VulkanContextH2M::GetCurrentDevice()->GetComputeQueue();
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
			vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, &uniformStorageBuffer.Data);
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

H2M::RendererCapabilitiesH2M& EnvMapVulkanRenderer::GetCapabilities()
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
	int32_t v;
	return v; // TODO: s_Data.SelectedDrawCall;
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
