#include "EnvMapVulkanMaterialBuffer.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"

#include <algorithm>
#include <cmath>
#include <cstring>


static_assert(sizeof(EnvMapVulkanMaterialBuffer::GPUMaterial) == 64, "std430 layout mismatch with GPUMaterial in Include/FrameSet.glslh");

static constexpr uint32_t InitialCapacity = 64; // materials

void EnvMapVulkanMaterialBuffer::Create()
{
	CreateBuffer(InitialCapacity);
}

void EnvMapVulkanMaterialBuffer::Destroy()
{
	DestroyBuffer();
	m_Entries.clear();
	m_AverageColors.clear();
}

void EnvMapVulkanMaterialBuffer::CreateBuffer(uint32_t capacity)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = (VkDeviceSize)capacity * sizeof(GPUMaterial);
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	VK_CHECK_RESULT_H2M(vkCreateBuffer(device, &bufferInfo, nullptr, &m_Buffer));
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(device, m_Buffer, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("MaterialBuffer"));
	allocator.Allocate(requirements, &m_Memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK_RESULT_H2M(vkBindBufferMemory(device, m_Buffer, m_Memory, 0));
	m_Capacity = capacity;

	// Every entry has defined values, also the ones no material uses yet
	std::vector<GPUMaterial> defaults(capacity);
	void* mapped;
	VK_CHECK_RESULT_H2M(vkMapMemory(device, m_Memory, 0, bufferInfo.size, 0, &mapped));
	memcpy(mapped, defaults.data(), bufferInfo.size);
	vkUnmapMemory(device, m_Memory);
}

void EnvMapVulkanMaterialBuffer::DestroyBuffer()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (m_Buffer) vkDestroyBuffer(device, m_Buffer, nullptr);
	if (m_Memory) vkFreeMemory(device, m_Memory, nullptr);
	m_Buffer = VK_NULL_HANDLE;
	m_Memory = VK_NULL_HANDLE;
	m_Capacity = 0;
}

bool EnvMapVulkanMaterialBuffer::Update()
{
	const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();
	std::vector<GPUMaterial> entries(materials.size());
	for (size_t i = 0; i < materials.size(); i++)
	{
		H2M::RefH2M<EnvMapVulkanMaterial> material = materials[i];
		material->Get<float>("u_MaterialUniforms.MaterialIndex") = (float)i;
		entries[i] = Pack(material);
	}

	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	bool replaced = false;
	if (entries.size() > m_Capacity)
	{
		vkDeviceWaitIdle(device); // the last frame's command buffer may still read the old buffer
		const uint32_t capacity = std::max((uint32_t)entries.size(), m_Capacity * 2);
		DestroyBuffer();
		CreateBuffer(capacity);
		m_Entries.clear();
		replaced = true;
	}

	const bool changed = entries.size() != m_Entries.size() ||
		(!entries.empty() && memcmp(entries.data(), m_Entries.data(), entries.size() * sizeof(GPUMaterial)) != 0);
	if (changed)
	{
		if (!entries.empty())
		{
			const VkDeviceSize size = entries.size() * sizeof(GPUMaterial);
			void* mapped;
			VK_CHECK_RESULT_H2M(vkMapMemory(device, m_Memory, 0, size, 0, &mapped));
			memcpy(mapped, entries.data(), size);
			vkUnmapMemory(device, m_Memory);
		}
		m_Entries = std::move(entries);
	}
	return replaced;
}

EnvMapVulkanMaterialBuffer::GPUMaterial EnvMapVulkanMaterialBuffer::Pack(const H2M::RefH2M<EnvMapVulkanMaterial>& materialRef)
{
	H2M::RefH2M<EnvMapVulkanMaterial> material = materialRef;
	// A map counts when it is bound and its toggle is on: what the shaders test too (Include/MeshMaterial.glslh)
	auto usedMap = [&](uint32_t slot) -> H2M::RefH2M<H2M::Texture2D_H2M>
	{
		const bool on = material->Get<float>(EnvMapVulkanMaterial::GetMapToggleName(slot)) > 0.5f;
		return on ? material->GetMap(slot) : H2M::RefH2M<H2M::Texture2D_H2M>();
	};
	const bool packed = material->Get<float>("u_MaterialUniforms.MetalRoughPacked") > 0.5f; // glTF: roughness in G, metalness in B

	GPUMaterial entry;
	entry.AlbedoColor = glm::vec4(material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor"), 1.0f);
	if (auto map = usedMap(EnvMapVulkanMaterial::Albedo))
	{
		entry.AlbedoColor = glm::vec4(glm::vec3(GetAverageColor(map, true)), 1.0f);
	}
	if (auto map = usedMap(EnvMapVulkanMaterial::Emissive))
	{
		entry.Emissive = glm::vec4(glm::vec3(GetAverageColor(map, true)) * material->Get<float>("u_MaterialUniforms.EmissiveIntensity"), 0.0f);
	}
	entry.Metalness = material->Get<float>("u_MaterialUniforms.Metalness");
	if (auto map = usedMap(EnvMapVulkanMaterial::Metalness))
	{
		const glm::vec4 average = GetAverageColor(map, false);
		entry.Metalness = packed ? average.b : average.r;
	}
	entry.Roughness = material->Get<float>("u_MaterialUniforms.Roughness");
	if (auto map = usedMap(EnvMapVulkanMaterial::Roughness))
	{
		const glm::vec4 average = GetAverageColor(map, false);
		entry.Roughness = packed ? average.g : average.r;
	}
	entry.Roughness = std::max(entry.Roughness, 0.05f); // as MaterialRoughness in the shaders
	entry.Flags = material->IsGlass() ? FlagGlass : 0;
	return entry;
}

glm::vec4 EnvMapVulkanMaterialBuffer::GetAverageColor(const H2M::RefH2M<H2M::Texture2D_H2M>& textureRef, bool srgb)
{
	H2M::RefH2M<H2M::Texture2D_H2M> texture = textureRef;
	// By file (the same image as a color map and as a data map averages differently); a texture without a file by itself
	std::string key = texture->GetPath();
	if (key.empty())
	{
		key = std::to_string((uintptr_t)texture.Raw());
	}
	key += srgb ? "|srgb" : "|linear";
	auto cached = m_AverageColors.find(key);
	if (cached != m_AverageColors.end())
	{
		return cached->second;
	}

	// The pixels the texture was created from (kept by the texture): 8 bits or a float per channel, 4 channels
	glm::vec4 average(1.0f);
	const H2M::BufferH2M pixels = texture->GetWriteableBuffer();
	const uint32_t width = texture->GetWidth(), height = texture->GetHeight();
	const uint64_t pixelCount = (uint64_t)width * height;
	const bool bytes = pixels.Data && pixelCount > 0 && pixels.Size == pixelCount * 4;
	const bool floats = pixels.Data && pixelCount > 0 && pixels.Size == pixelCount * 4 * sizeof(float);
	if (bytes || floats)
	{
		// A grid of up to 128 x 128 samples: plenty for an average
		const uint32_t stepX = std::max(width / 128u, 1u), stepY = std::max(height / 128u, 1u);
		glm::dvec4 sum(0.0);
		uint64_t samples = 0;
		for (uint32_t y = stepY / 2; y < height; y += stepY)
		{
			for (uint32_t x = stepX / 2; x < width; x += stepX)
			{
				const uint64_t index = ((uint64_t)y * width + x) * 4;
				glm::vec4 color;
				if (bytes)
				{
					const uint8_t* p = (const uint8_t*)pixels.Data + index;
					color = glm::vec4(p[0], p[1], p[2], p[3]) / 255.0f;
					if (srgb)
					{
						// sRGB to linear, as the sampler does for a color map (not the alpha)
						for (int c = 0; c < 3; c++)
						{
							color[c] = color[c] <= 0.04045f ? color[c] / 12.92f : std::pow((color[c] + 0.055f) / 1.055f, 2.4f);
						}
					}
				}
				else
				{
					const float* p = (const float*)pixels.Data + index;
					color = glm::vec4(p[0], p[1], p[2], p[3]);
				}
				sum += glm::dvec4(color);
				samples++;
			}
		}
		if (samples > 0)
		{
			average = glm::vec4(sum / (double)samples);
		}
	}
	m_AverageColors[key] = average;
	return average;
}
