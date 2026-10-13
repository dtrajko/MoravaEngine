#include "EnvMapVulkanRayTracing.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanIndexBufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanVertexBufferH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Log.h"

#include <algorithm>
#include <cstring>


static_assert(sizeof(EnvMapVulkanRayTracing::GPUInstance) == 24, "std430 layout mismatch with RayTracingInstance in Include/RayTracing.glslh");
static_assert(sizeof(EnvMapVulkanRayTracing::ViewPush) == 80, "layout mismatch with the RayTracingViewPush block in RayTracingView.glsl");
static_assert(sizeof(H2M::VertexH2M) == 56, "RayTracingVertex in Include/RayTracing.glslh must match H2M::VertexH2M");
static_assert(sizeof(H2M::AnimatedVertex) == 88, "SkinningSourceVertex in RayTracingSkinning.glsl must match H2M::AnimatedVertex");

static constexpr uint32_t InitialInstanceCapacity = 256;
// Frames (calls of Record) a mesh may go unsubmitted before its structure is destroyed. More than the frames a command
// buffer that used it can still be running.
static constexpr uint64_t RetireAfterFrames = 3;

// A mesh that never changes: built once, so the build may take longer for rays that are traced faster, and it can be
// made smaller afterwards
static constexpr VkBuildAccelerationStructureFlagsKHR StaticMeshFlags =
	VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
// A skinned mesh: refitted to its new vertex positions every frame
static constexpr VkBuildAccelerationStructureFlagsKHR SkinnedMeshFlags =
	VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
// The TLAS: built from scratch every frame
static constexpr VkBuildAccelerationStructureFlagsKHR TopLevelFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;

// The ray tracing functions belong to device extensions: the Vulkan library has no entry points for them, they are
// asked from the device (Create)
struct RayTracingFunctions
{
	PFN_vkGetBufferDeviceAddress GetBufferDeviceAddress = nullptr;
	PFN_vkCreateAccelerationStructureKHR CreateAccelerationStructure = nullptr;
	PFN_vkDestroyAccelerationStructureKHR DestroyAccelerationStructure = nullptr;
	PFN_vkGetAccelerationStructureBuildSizesKHR GetAccelerationStructureBuildSizes = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR GetAccelerationStructureDeviceAddress = nullptr;
	PFN_vkCmdBuildAccelerationStructuresKHR CmdBuildAccelerationStructures = nullptr;
	PFN_vkCmdCopyAccelerationStructureKHR CmdCopyAccelerationStructure = nullptr;
	PFN_vkCmdWriteAccelerationStructuresPropertiesKHR CmdWriteAccelerationStructuresProperties = nullptr;

	bool Load(VkDevice device)
	{
		GetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddress)vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddress");
		CreateAccelerationStructure = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
		DestroyAccelerationStructure = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
		GetAccelerationStructureBuildSizes = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
		GetAccelerationStructureDeviceAddress = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
		CmdBuildAccelerationStructures = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
		CmdCopyAccelerationStructure = (PFN_vkCmdCopyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR");
		CmdWriteAccelerationStructuresProperties = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)vkGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR");
		return GetBufferDeviceAddress && CreateAccelerationStructure && DestroyAccelerationStructure && GetAccelerationStructureBuildSizes &&
			GetAccelerationStructureDeviceAddress && CmdBuildAccelerationStructures && CmdCopyAccelerationStructure && CmdWriteAccelerationStructuresProperties;
	}
};
static RayTracingFunctions s_Vk;

static VkDevice GetDevice()
{
	return H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
}

static VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment)
{
	return (value + alignment - 1) / alignment * alignment;
}

static VkDeviceAddress GetBufferAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	info.buffer = buffer;
	return s_Vk.GetBufferDeviceAddress(GetDevice(), &info);
}

// The triangles of one mesh, as a build or an update reads them
static VkAccelerationStructureGeometryKHR MakeTriangles(VkDeviceAddress vertexAddress, VkDeviceAddress indexAddress, uint32_t maxVertex)
{
	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // a hit is final: no shader decides about it
	VkAccelerationStructureGeometryTrianglesDataKHR& triangles = geometry.geometry.triangles;
	triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT; // the position, first in H2M::VertexH2M
	triangles.vertexData.deviceAddress = vertexAddress;
	triangles.vertexStride = sizeof(H2M::VertexH2M);
	triangles.maxVertex = maxVertex;
	triangles.indexType = VK_INDEX_TYPE_UINT32;
	triangles.indexData.deviceAddress = indexAddress;
	return geometry;
}

static VkAccelerationStructureBuildGeometryInfoKHR MakeBuildInfo(VkAccelerationStructureTypeKHR type, VkBuildAccelerationStructureFlagsKHR flags,
	const VkAccelerationStructureGeometryKHR* geometry)
{
	VkAccelerationStructureBuildGeometryInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	info.type = type;
	info.flags = flags;
	info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	info.geometryCount = 1;
	info.pGeometries = geometry;
	return info;
}

// How big the structure and the working memory of its build can get, for up to primitiveCount triangles or instances
static VkAccelerationStructureBuildSizesInfoKHR GetBuildSizes(const VkAccelerationStructureBuildGeometryInfoKHR& buildInfo, uint32_t primitiveCount)
{
	VkAccelerationStructureBuildSizesInfoKHR sizes = {};
	sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	s_Vk.GetAccelerationStructureBuildSizes(GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCount, &sizes);
	return sizes;
}

static void RecordMemoryBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags sourceStages, VkAccessFlags sourceAccess,
	VkPipelineStageFlags destinationStages, VkAccessFlags destinationAccess)
{
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = sourceAccess;
	memoryBarrier.dstAccessMask = destinationAccess;
	vkCmdPipelineBarrier(commandBuffer, sourceStages, destinationStages, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void EnvMapVulkanRayTracing::Buffer::Create(VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible)
{
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkDevice device = vulkanDevice->GetVulkanDevice();
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VK_CHECK_RESULT_H2M(vkCreateBuffer(device, &bufferInfo, nullptr, &Handle));

	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(device, Handle, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("RayTracing"));
	allocator.Allocate(requirements, &Memory, hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
	VK_CHECK_RESULT_H2M(vkBindBufferMemory(device, Handle, Memory, 0));
	Size = size;
	Address = GetBufferAddress(Handle);
	if (hostVisible)
	{
		VK_CHECK_RESULT_H2M(vkMapMemory(device, Memory, 0, size, 0, &Mapped));
	}
}

void EnvMapVulkanRayTracing::Buffer::Destroy()
{
	VkDevice device = GetDevice();
	if (Handle) vkDestroyBuffer(device, Handle, nullptr);
	if (Memory) vkFreeMemory(device, Memory, nullptr); // unmaps it
	*this = Buffer();
}

void EnvMapVulkanRayTracing::Structure::Create(VkDeviceSize size, VkAccelerationStructureTypeKHR type)
{
	Storage.Create(size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, false);
	VkAccelerationStructureCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.buffer = Storage.Handle;
	createInfo.size = size;
	createInfo.type = type;
	VK_CHECK_RESULT_H2M(s_Vk.CreateAccelerationStructure(GetDevice(), &createInfo, nullptr, &Handle));

	VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
	addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addressInfo.accelerationStructure = Handle;
	Address = s_Vk.GetAccelerationStructureDeviceAddress(GetDevice(), &addressInfo);
}

void EnvMapVulkanRayTracing::Structure::Destroy()
{
	if (Handle) s_Vk.DestroyAccelerationStructure(GetDevice(), Handle, nullptr);
	Storage.Destroy();
	Handle = VK_NULL_HANDLE;
	Address = 0;
}

bool EnvMapVulkanRayTracing::IsSupported()
{
	return H2M::VulkanContextH2M::GetCurrentDevice()->IsRayTracingSupported();
}

void EnvMapVulkanRayTracing::Create()
{
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkDevice device = vulkanDevice->GetVulkanDevice();

	// The view image is shown pixel for pixel
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxAnisotropy = 1.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler));
	m_ViewImage.Create(ViewFormat, 1, 1, 1, 0.0f); // until the first ResizeView

	if (!IsSupported())
	{
		return;
	}
	if (!s_Vk.Load(device))
	{
		Log::GetLogger()->error("EnvMapVulkanRayTracing: the device has the ray tracing extensions but not their functions. Ray tracing stays off.");
		return;
	}

	// Offsets into the working memory of a build must be multiples of this
	VkPhysicalDeviceAccelerationStructurePropertiesKHR structureProperties = {};
	structureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 properties = {};
	properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	properties.pNext = &structureProperties;
	vkGetPhysicalDeviceProperties2(vulkanDevice->GetPhysicalDevice()->GetVulkanPhysicalDevice(), &properties);
	m_ScratchAlignment = std::max<VkDeviceSize>(structureProperties.minAccelerationStructureScratchOffsetAlignment, 1);

	// These shaders only compile for a device with ray tracing: loaded here, not with the others (RendererH2M::Init)
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/RayTracingSkinning.glsl");
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/RayTracingView.glsl");
	m_SkinningPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("RayTracingSkinning"));
	m_ViewPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("RayTracingView"));
	// Every shader that includes Include/RayTracing.glslh declares set 3 the same way, so this set is valid for all of them
	m_SceneSet = m_ViewPipeline->GetShader()->CreateDescriptorSets(3);
	m_ViewSet = m_ViewPipeline->GetShader()->CreateDescriptorSets(0);

	m_Created = true;
	CreateTopLevel(InitialInstanceCapacity);
	ResizeView(m_ViewImage.Width, m_ViewImage.Height); // the view set's image
}

void EnvMapVulkanRayTracing::Destroy()
{
	VkDevice device = GetDevice();
	if (m_Created)
	{
		for (auto& [mesh, entry] : m_Meshes)
		{
			entry.BLAS.Destroy();
		}
		m_Meshes.clear();
		for (auto& [model, skinned] : m_SkinnedModels)
		{
			skinned.Vertices.Destroy();
			vkDestroyDescriptorPool(device, skinned.Set.Pool, nullptr);
		}
		m_SkinnedModels.clear();
		DestroyTopLevel();
		m_Scratch.Destroy();
		vkDestroyDescriptorPool(device, m_SceneSet.Pool, nullptr);
		vkDestroyDescriptorPool(device, m_ViewSet.Pool, nullptr);
		m_SceneSet = {};
		m_ViewSet = {};
		m_SkinningPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
		m_ViewPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
		m_Textures.Clear();
		m_FrameInstances.clear();
		m_Created = false;
	}
	m_ViewImage.Destroy();
	if (m_Sampler)
	{
		vkDestroySampler(device, m_Sampler, nullptr);
		m_Sampler = VK_NULL_HANDLE;
	}
}

void EnvMapVulkanRayTracing::CreateTopLevel(uint32_t instanceCapacity)
{
	m_InstanceCapacity = instanceCapacity;
	m_InstanceInput.Create((VkDeviceSize)instanceCapacity * sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true);
	m_InstanceTable.Create((VkDeviceSize)instanceCapacity * sizeof(GPUInstance), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
	memset(m_InstanceTable.Mapped, 0, (size_t)m_InstanceTable.Size);

	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.data.deviceAddress = m_InstanceInput.Address;
	const VkAccelerationStructureBuildSizesInfoKHR sizes = GetBuildSizes(MakeBuildInfo(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, TopLevelFlags, &geometry), instanceCapacity);
	m_TLAS.Create(sizes.accelerationStructureSize, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
	m_TLASScratchSize = sizes.buildScratchSize;
	WriteSceneDescriptors();
}

void EnvMapVulkanRayTracing::DestroyTopLevel()
{
	m_TLAS.Destroy();
	m_InstanceInput.Destroy();
	m_InstanceTable.Destroy();
	m_InstanceCapacity = 0;
}

void EnvMapVulkanRayTracing::WriteSceneDescriptors()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_ViewPipeline->GetShader();
	std::vector<VkWriteDescriptorSet> writes;

	VkWriteDescriptorSetAccelerationStructureKHR structureInfo = {};
	structureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	structureInfo.accelerationStructureCount = 1;
	structureInfo.pAccelerationStructures = &m_TLAS.Handle;
	VkWriteDescriptorSet write = *shader->GetDescriptorSet("u_TLAS", 3);
	write.dstSet = m_SceneSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pNext = &structureInfo;
	writes.push_back(write);

	const VkDescriptorBufferInfo instanceTable = { m_InstanceTable.Handle, 0, VK_WHOLE_SIZE };
	write = *shader->GetDescriptorSet("RayTracingInstances", 3);
	write.dstSet = m_SceneSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pBufferInfo = &instanceTable;
	writes.push_back(write);

	if (m_MaterialBuffer.buffer)
	{
		write = *shader->GetDescriptorSet("RayTracingMaterials", 3);
		write.dstSet = m_SceneSet.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pBufferInfo = &m_MaterialBuffer;
		writes.push_back(write);
	}
	vkUpdateDescriptorSets(GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void EnvMapVulkanRayTracing::SetMaterialBuffer(const VkDescriptorBufferInfo& materialBuffer)
{
	m_MaterialBuffer = materialBuffer;
	if (m_Created)
	{
		WriteSceneDescriptors();
	}
}

void EnvMapVulkanRayTracing::UpdateTextures()
{
	if (!m_Created)
	{
		return;
	}
	// The array's binding may be written while the set is in use: only elements no shader reads yet are written
	const std::vector<EnvMapVulkanTextureTable::Write> textureWrites = m_Textures.TakeWrites();
	if (textureWrites.empty())
	{
		return;
	}
	const VkWriteDescriptorSet* binding = m_ViewPipeline->GetShader()->GetDescriptorSet("u_Textures", 3);
	std::vector<VkWriteDescriptorSet> writes(textureWrites.size());
	for (size_t i = 0; i < textureWrites.size(); i++)
	{
		writes[i] = *binding;
		writes[i].dstSet = m_SceneSet.DescriptorSets[0];
		writes[i].dstArrayElement = textureWrites[i].Slot;
		writes[i].descriptorCount = 1;
		writes[i].pImageInfo = &textureWrites[i].Info;
	}
	vkUpdateDescriptorSets(GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void EnvMapVulkanRayTracing::ReserveScratch(VkDeviceSize size)
{
	if (size <= m_Scratch.Size)
	{
		return;
	}
	vkDeviceWaitIdle(GetDevice()); // the last frame's builds may still use the old buffer
	m_Scratch.Destroy();
	m_Scratch.Create(size + size / 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false);
}

void EnvMapVulkanRayTracing::BeginFrame()
{
	m_FrameInstances.clear();
}

void EnvMapVulkanRayTracing::Submit(const H2M::RefH2M<H2M::ModelH2M>& modelRef, const std::vector<glm::mat4>& meshTransforms,
	const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials)
{
	if (!m_Created)
	{
		return;
	}
	H2M::RefH2M<H2M::ModelH2M> model = modelRef;
	const bool skinned = model->IsSkinned();
	// What the mesh passes can't draw has no instance either (see EnvMapVulkanRenderer::RenderModelVulkan)
	if (skinned && model->GetObjectDescriptorSet() == VK_NULL_HANDLE)
	{
		return;
	}
	if (!model->GetVertexBuffer() || !model->GetIndexBuffer() || !model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer())
	{
		return;
	}
	const auto& meshes = model->GetMeshes();
	for (size_t s = 0; s < meshes.size() && s < materials.size() && s < meshTransforms.size(); s++)
	{
		H2M::RefH2M<EnvMapVulkanMaterial> material = materials[s];
		if (meshes[s]->IndexCount < 3)
		{
			continue;
		}
		FrameInstance& instance = m_FrameInstances.emplace_back();
		instance.Model = model;
		instance.Mesh = meshes[s];
		instance.Transform = meshTransforms[s];
		instance.MaterialIndex = (uint32_t)material->Get<float>("u_MaterialUniforms.MaterialIndex");
		// As the passes draw them: glass on a skinned model is drawn with the opaque meshes
		instance.Mask = material->IsGlass() && !skinned ? MaskGlass : MaskOpaque;
	}
}

EnvMapVulkanRayTracing::SkinnedModel& EnvMapVulkanRayTracing::GetSkinnedModel(const H2M::RefH2M<H2M::ModelH2M>& modelRef)
{
	H2M::RefH2M<H2M::ModelH2M> model = modelRef;
	auto [found, isNew] = m_SkinnedModels.try_emplace(model.Raw());
	SkinnedModel& skinned = found->second;
	if (!isNew)
	{
		return skinned;
	}
	skinned.Model = model;
	skinned.VertexCount = model->GetVertexBuffer()->GetSize() / (uint32_t)sizeof(H2M::AnimatedVertex);
	skinned.Vertices.Create((VkDeviceSize)std::max(skinned.VertexCount, 1u) * sizeof(H2M::VertexH2M),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, false);

	// The skinning pass's set: the model's own bone buffer (written every frame by the renderer), its vertex buffer, and
	// the buffer of moved vertices
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_SkinningPipeline->GetShader();
	skinned.Set = shader->CreateDescriptorSets();
	const VkDescriptorBufferInfo bones = model->GetMeshShader().As<H2M::VulkanShaderH2M>()->GetUniformBuffer(0, H2M::VulkanShaderH2M::ObjectDescriptorSet).Descriptor;
	const VkDescriptorBufferInfo source = { model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer(), 0, VK_WHOLE_SIZE };
	const VkDescriptorBufferInfo destination = { skinned.Vertices.Handle, 0, VK_WHOLE_SIZE };
	const char* names[3] = { "BoneTransforms", "SourceVertices", "SkinnedVertices" };
	const VkDescriptorBufferInfo* infos[3] = { &bones, &source, &destination };
	VkWriteDescriptorSet writes[3];
	for (uint32_t i = 0; i < 3; i++)
	{
		writes[i] = *shader->GetDescriptorSet(names[i]);
		writes[i].dstSet = skinned.Set.DescriptorSets[0];
		writes[i].descriptorCount = 1;
		writes[i].pBufferInfo = infos[i];
	}
	vkUpdateDescriptorSets(GetDevice(), 3, writes, 0, nullptr);
	return skinned;
}

EnvMapVulkanRayTracing::MeshEntry* EnvMapVulkanRayTracing::GetMeshEntry(const FrameInstance& instance, std::vector<MeshEntry*>& newStaticMeshes)
{
	H2M::RefH2M<H2M::ModelH2M> model = instance.Model;
	H2M::RefH2M<H2M::MeshH2M> mesh = instance.Mesh;
	auto [found, isNew] = m_Meshes.try_emplace(mesh.Raw());
	MeshEntry& entry = found->second;
	if (!isNew)
	{
		return &entry;
	}

	const bool skinned = model->IsSkinned();
	const uint32_t vertexSize = skinned ? (uint32_t)sizeof(H2M::AnimatedVertex) : (uint32_t)sizeof(H2M::VertexH2M);
	const uint32_t modelVertexCount = model->GetVertexBuffer()->GetSize() / vertexSize;
	const uint32_t meshVertexCount = mesh->VertexCount > 0 ? mesh->VertexCount : (modelVertexCount > mesh->BaseVertex ? modelVertexCount - mesh->BaseVertex : 0);
	if (meshVertexCount == 0)
	{
		m_Meshes.erase(found);
		return nullptr;
	}
	entry.Mesh = mesh;
	entry.MaxVertex = meshVertexCount - 1;
	entry.TriangleCount = mesh->IndexCount / 3;
	entry.IndexAddress = GetBufferAddress(H2M::RefH2M<H2M::VulkanIndexBufferH2M>(model->GetIndexBuffer())->GetVulkanBuffer()) +
		(VkDeviceAddress)mesh->BaseIndex * sizeof(uint32_t);
	if (!skinned)
	{
		entry.VertexAddress = GetBufferAddress(model->GetVertexBuffer().As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer()) +
			(VkDeviceAddress)mesh->BaseVertex * sizeof(H2M::VertexH2M);
		newStaticMeshes.push_back(&entry); // built by BuildStaticMeshes
		return &entry;
	}

	// A skinned mesh reads the model's moved vertices. Its structure is built in the frame's command buffer, after the
	// skinning pass has filled them for the first time.
	entry.Skinned = &GetSkinnedModel(model);
	entry.VertexAddress = entry.Skinned->Vertices.Address + (VkDeviceAddress)mesh->BaseVertex * sizeof(H2M::VertexH2M);
	const VkAccelerationStructureGeometryKHR geometry = MakeTriangles(entry.VertexAddress, entry.IndexAddress, entry.MaxVertex);
	const VkAccelerationStructureBuildSizesInfoKHR sizes = GetBuildSizes(MakeBuildInfo(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, SkinnedMeshFlags, &geometry), entry.TriangleCount);
	entry.BLAS.Create(sizes.accelerationStructureSize, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
	entry.ScratchSize = std::max(sizes.buildScratchSize, sizes.updateScratchSize);
	return &entry;
}

// The structures of meshes that never change: all built in one command buffer, then each copied into a structure of the
// size it really needs (compaction: usually about half). Waits for the GPU twice; meshes are new when a model was loaded.
void EnvMapVulkanRayTracing::BuildStaticMeshes(const std::vector<MeshEntry*>& meshes)
{
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkDevice device = vulkanDevice->GetVulkanDevice();
	const uint32_t count = (uint32_t)meshes.size();

	std::vector<VkAccelerationStructureGeometryKHR> geometries(count);
	std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(count);
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(count);
	std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> rangePointers(count);
	std::vector<Structure> uncompacted(count);
	std::vector<VkAccelerationStructureKHR> handles(count);
	std::vector<VkDeviceSize> scratchOffsets(count);
	VkDeviceSize scratchSize = m_ScratchAlignment; // room to align the buffer's own address
	for (uint32_t i = 0; i < count; i++)
	{
		const MeshEntry& entry = *meshes[i];
		geometries[i] = MakeTriangles(entry.VertexAddress, entry.IndexAddress, entry.MaxVertex);
		buildInfos[i] = MakeBuildInfo(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, StaticMeshFlags, &geometries[i]);
		const VkAccelerationStructureBuildSizesInfoKHR sizes = GetBuildSizes(buildInfos[i], entry.TriangleCount);
		uncompacted[i].Create(sizes.accelerationStructureSize, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
		handles[i] = uncompacted[i].Handle;
		buildInfos[i].dstAccelerationStructure = uncompacted[i].Handle;
		scratchOffsets[i] = scratchSize;
		scratchSize += AlignUp(sizes.buildScratchSize, m_ScratchAlignment);
		ranges[i] = {};
		ranges[i].primitiveCount = entry.TriangleCount;
		rangePointers[i] = &ranges[i];
	}
	Buffer scratch;
	scratch.Create(scratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false);
	const VkDeviceAddress scratchBase = AlignUp(scratch.Address, m_ScratchAlignment) - m_ScratchAlignment;
	for (uint32_t i = 0; i < count; i++)
	{
		buildInfos[i].scratchData.deviceAddress = scratchBase + scratchOffsets[i];
	}

	VkQueryPoolCreateInfo queryPoolInfo = {};
	queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
	queryPoolInfo.queryCount = count;
	VkQueryPool queryPool;
	VK_CHECK_RESULT_H2M(vkCreateQueryPool(device, &queryPoolInfo, nullptr, &queryPool));

	// Build, and ask how small each structure can get
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	vkCmdResetQueryPool(commandBuffer, queryPool, 0, count);
	s_Vk.CmdBuildAccelerationStructures(commandBuffer, count, buildInfos.data(), rangePointers.data());
	RecordMemoryBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
	s_Vk.CmdWriteAccelerationStructuresProperties(commandBuffer, count, handles.data(), VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, 0);
	vulkanDevice->FlushCommandBuffer(commandBuffer);

	std::vector<VkDeviceSize> compactedSizes(count);
	VK_CHECK_RESULT_H2M(vkGetQueryPoolResults(device, queryPool, 0, count, count * sizeof(VkDeviceSize), compactedSizes.data(), sizeof(VkDeviceSize),
		VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));

	// Copy each into a structure of that size
	commandBuffer = vulkanDevice->GetCommandBuffer(true);
	VkDeviceSize builtBytes = 0, compactedBytes = 0;
	for (uint32_t i = 0; i < count; i++)
	{
		MeshEntry& entry = *meshes[i];
		entry.BLAS.Create(compactedSizes[i], VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
		entry.Built = true;
		VkCopyAccelerationStructureInfoKHR copyInfo = {};
		copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
		copyInfo.src = uncompacted[i].Handle;
		copyInfo.dst = entry.BLAS.Handle;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
		s_Vk.CmdCopyAccelerationStructure(commandBuffer, &copyInfo);
		builtBytes += uncompacted[i].Storage.Size;
		compactedBytes += compactedSizes[i];
	}
	vulkanDevice->FlushCommandBuffer(commandBuffer);

	for (Structure& structure : uncompacted)
	{
		structure.Destroy();
	}
	scratch.Destroy();
	vkDestroyQueryPool(device, queryPool, nullptr);
	Log::GetLogger()->info("Ray tracing: {0} mesh acceleration structure(s) built, {1} KB compacted to {2} KB", count, builtBytes / 1024, compactedBytes / 1024);
}

void EnvMapVulkanRayTracing::RetireUnused()
{
	VkDevice device = GetDevice();
	// The meshes first: a skinned mesh was last used no later than its model
	for (auto it = m_Meshes.begin(); it != m_Meshes.end();)
	{
		if (it->second.LastUsedFrame + RetireAfterFrames < m_Frame)
		{
			it->second.BLAS.Destroy();
			it = m_Meshes.erase(it);
		}
		else
		{
			++it;
		}
	}
	for (auto it = m_SkinnedModels.begin(); it != m_SkinnedModels.end();)
	{
		if (it->second.LastUsedFrame + RetireAfterFrames < m_Frame)
		{
			it->second.Vertices.Destroy();
			vkDestroyDescriptorPool(device, it->second.Set.Pool, nullptr);
			it = m_SkinnedModels.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void EnvMapVulkanRayTracing::Record(VkCommandBuffer commandBuffer)
{
	if (!m_Created)
	{
		return;
	}
	m_Frame++;

	// Every submitted mesh's structure; the new static ones are built now
	std::vector<MeshEntry*> entries(m_FrameInstances.size());
	std::vector<MeshEntry*> newStaticMeshes, skinnedMeshes;
	std::vector<SkinnedModel*> skinnedModels;
	for (size_t i = 0; i < m_FrameInstances.size(); i++)
	{
		MeshEntry* entry = GetMeshEntry(m_FrameInstances[i], newStaticMeshes);
		entries[i] = entry;
		if (!entry || entry->LastUsedFrame == m_Frame)
		{
			continue; // no structure, or another instance of a mesh already seen this frame
		}
		entry->LastUsedFrame = m_Frame;
		if (entry->Skinned)
		{
			skinnedMeshes.push_back(entry);
			if (entry->Skinned->LastUsedFrame != m_Frame)
			{
				entry->Skinned->LastUsedFrame = m_Frame;
				skinnedModels.push_back(entry->Skinned);
			}
		}
	}
	if (!newStaticMeshes.empty())
	{
		BuildStaticMeshes(newStaticMeshes);
	}
	RetireUnused();

	// The instances: what the TLAS is built from, and what a hit reads by the instance's number
	std::vector<VkAccelerationStructureInstanceKHR> instanceInputs;
	std::vector<GPUInstance> instanceTable;
	instanceInputs.reserve(entries.size());
	instanceTable.reserve(entries.size());
	m_Stats = Stats();
	for (size_t i = 0; i < m_FrameInstances.size(); i++)
	{
		const MeshEntry* entry = entries[i];
		if (!entry)
		{
			continue;
		}
		const FrameInstance& frameInstance = m_FrameInstances[i];
		VkAccelerationStructureInstanceKHR& input = instanceInputs.emplace_back();
		input = {};
		for (int row = 0; row < 3; row++)
		{
			for (int column = 0; column < 4; column++)
			{
				input.transform.matrix[row][column] = frameInstance.Transform[column][row]; // glm is column-major, this is 3 rows of 4
			}
		}
		input.instanceCustomIndex = (uint32_t)instanceTable.size() & 0xFFFFFF;
		input.mask = frameInstance.Mask;
		input.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; // both sides, as the mesh passes draw
		input.accelerationStructureReference = entry->BLAS.Address;

		GPUInstance& tableEntry = instanceTable.emplace_back();
		tableEntry.VertexAddress = entry->VertexAddress;
		tableEntry.IndexAddress = entry->IndexAddress;
		tableEntry.MaterialIndex = frameInstance.MaterialIndex;
		m_Stats.Triangles += entry->TriangleCount;
	}
	const uint32_t instanceCount = (uint32_t)instanceInputs.size();
	if (instanceCount > m_InstanceCapacity)
	{
		vkDeviceWaitIdle(GetDevice()); // the last frame's command buffer may still use the old TLAS and buffers
		const uint32_t capacity = std::max(instanceCount, m_InstanceCapacity * 2);
		DestroyTopLevel();
		CreateTopLevel(capacity);
	}
	if (instanceCount > 0)
	{
		memcpy(m_InstanceInput.Mapped, instanceInputs.data(), instanceCount * sizeof(VkAccelerationStructureInstanceKHR));
		memcpy(m_InstanceTable.Mapped, instanceTable.data(), instanceCount * sizeof(GPUInstance));
	}

	// Working memory: a part for each skinned mesh's build or update, then the TLAS's
	std::vector<VkDeviceSize> scratchOffsets(skinnedMeshes.size());
	VkDeviceSize scratchSize = m_ScratchAlignment;
	for (size_t i = 0; i < skinnedMeshes.size(); i++)
	{
		scratchOffsets[i] = scratchSize;
		scratchSize += AlignUp(skinnedMeshes[i]->ScratchSize, m_ScratchAlignment);
	}
	const VkDeviceSize topLevelScratchOffset = scratchSize;
	scratchSize += AlignUp(m_TLASScratchSize, m_ScratchAlignment);
	ReserveScratch(scratchSize);
	const VkDeviceAddress scratchBase = AlignUp(m_Scratch.Address, m_ScratchAlignment) - m_ScratchAlignment;

	const VkPipelineStageFlags compute = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	const VkPipelineStageFlags build = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	const VkAccessFlags structureRead = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	const VkAccessFlags structureWrite = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

	// After the last frame's rays and builds, which read what is written now
	RecordMemoryBarrier(commandBuffer, compute | build, VK_ACCESS_SHADER_READ_BIT | structureRead, compute | build, VK_ACCESS_SHADER_WRITE_BIT | structureWrite);

	if (!skinnedModels.empty())
	{
		// 1. The skinned models' vertices into this frame's pose
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_SkinningPipeline->GetVulkanPipeline());
		for (const SkinnedModel* skinned : skinnedModels)
		{
			const glm::ivec4 counts((int)skinned->VertexCount, 0, 0, 0);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_SkinningPipeline->GetVulkanPipelineLayout(), 0, 1,
				skinned->Set.DescriptorSets.data(), 0, nullptr);
			vkCmdPushConstants(commandBuffer, m_SkinningPipeline->GetVulkanPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(counts), &counts);
			vkCmdDispatch(commandBuffer, (skinned->VertexCount + 63) / 64, 1, 1);
		}
		// The builds read the moved vertices, and so do the rays' hits later
		RecordMemoryBarrier(commandBuffer, compute, VK_ACCESS_SHADER_WRITE_BIT, build | compute, VK_ACCESS_SHADER_READ_BIT);

		// 2. Their meshes' structures: built the first time, from then on refitted to the new positions
		const uint32_t count = (uint32_t)skinnedMeshes.size();
		std::vector<VkAccelerationStructureGeometryKHR> geometries(count);
		std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(count);
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(count);
		std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> rangePointers(count);
		for (uint32_t i = 0; i < count; i++)
		{
			MeshEntry& entry = *skinnedMeshes[i];
			geometries[i] = MakeTriangles(entry.VertexAddress, entry.IndexAddress, entry.MaxVertex);
			buildInfos[i] = MakeBuildInfo(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, SkinnedMeshFlags, &geometries[i]);
			if (entry.Built)
			{
				buildInfos[i].mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
				buildInfos[i].srcAccelerationStructure = entry.BLAS.Handle; // in place
			}
			buildInfos[i].dstAccelerationStructure = entry.BLAS.Handle;
			buildInfos[i].scratchData.deviceAddress = scratchBase + scratchOffsets[i];
			ranges[i] = {};
			ranges[i].primitiveCount = entry.TriangleCount;
			rangePointers[i] = &ranges[i];
			entry.Built = true;
		}
		s_Vk.CmdBuildAccelerationStructures(commandBuffer, count, buildInfos.data(), rangePointers.data());
		// The TLAS build reads them
		RecordMemoryBarrier(commandBuffer, build, structureWrite, build, structureRead);
	}

	// 3. The TLAS, from this frame's instances
	{
		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		geometry.geometry.instances.data.deviceAddress = m_InstanceInput.Address;
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo = MakeBuildInfo(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, TopLevelFlags, &geometry);
		buildInfo.dstAccelerationStructure = m_TLAS.Handle;
		buildInfo.scratchData.deviceAddress = scratchBase + topLevelScratchOffset;
		VkAccelerationStructureBuildRangeInfoKHR range = {};
		range.primitiveCount = instanceCount;
		const VkAccelerationStructureBuildRangeInfoKHR* rangePointer = &range;
		s_Vk.CmdBuildAccelerationStructures(commandBuffer, 1, &buildInfo, &rangePointer);
	}
	// The compute passes that trace rays read it
	RecordMemoryBarrier(commandBuffer, build, structureWrite, compute, structureRead);

	m_Stats.Instances = instanceCount;
	m_Stats.Meshes = (uint32_t)m_Meshes.size();
	m_Stats.SkinnedModels = (uint32_t)skinnedModels.size();
	m_Stats.Textures = m_Textures.GetCount();
	m_Stats.StructureBytes = m_TLAS.Storage.Size;
	for (const auto& [mesh, entry] : m_Meshes)
	{
		m_Stats.StructureBytes += entry.BLAS.Storage.Size;
	}
}

void EnvMapVulkanRayTracing::ResizeView(uint32_t width, uint32_t height)
{
	vkDeviceWaitIdle(GetDevice()); // the old image may still be in use
	m_ViewImage.Destroy();
	m_ViewImage.Create(ViewFormat, std::max(width, 1u), std::max(height, 1u), 1, 0.0f);
	if (!m_Created)
	{
		return;
	}
	const VkDescriptorImageInfo target = m_ViewImage.GetStorageInfo();
	VkWriteDescriptorSet write = *m_ViewPipeline->GetShader()->GetDescriptorSet("o_View");
	write.dstSet = m_ViewSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pImageInfo = &target;
	vkUpdateDescriptorSets(GetDevice(), 1, &write, 0, nullptr);
}

void EnvMapVulkanRayTracing::RecordView(VkCommandBuffer commandBuffer, View view, const glm::mat4& viewProjection)
{
	if (!m_Created)
	{
		return;
	}
	ViewPush push;
	push.InverseViewProjection = glm::inverse(viewProjection);
	push.Params = glm::ivec4((int)view, (int)MaskOpaque, 0, 0); // the opaque meshes: what the G-buffer's views show

	const VkPipelineStageFlags compute = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	const VkPipelineStageFlags fragment = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	const VkPipelineLayout layout = m_ViewPipeline->GetVulkanPipelineLayout();
	// After the last frame's composite, which sampled the view image
	RecordMemoryBarrier(commandBuffer, fragment, VK_ACCESS_SHADER_READ_BIT, compute, VK_ACCESS_SHADER_WRITE_BIT);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ViewPipeline->GetVulkanPipeline());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, m_ViewSet.DescriptorSets.data(), 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 3, 1, m_SceneSet.DescriptorSets.data(), 0, nullptr);
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
	vkCmdDispatch(commandBuffer, (m_ViewImage.Width + 7) / 8, (m_ViewImage.Height + 7) / 8, 1);
	RecordMemoryBarrier(commandBuffer, compute, VK_ACCESS_SHADER_WRITE_BIT, fragment, VK_ACCESS_SHADER_READ_BIT);
}
