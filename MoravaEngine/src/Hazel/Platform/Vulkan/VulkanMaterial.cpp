#include "VulkanMaterial.h"

#include "Hazel/Renderer/HazelRenderer.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Platform/Vulkan/VulkanImage.h"


namespace Hazel {

	VulkanMaterial::VulkanMaterial(const Ref<HazelShader>& shader, const std::string& name)
		: m_Shader(shader), m_Name(name)
	{
		m_Shader->AddShaderReloadedCallback(std::bind(&VulkanMaterial::OnShaderReloaded, this));
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;

		Ref<VulkanShader> vulkanShader = m_Shader.As<VulkanShader>();
		for (uint32_t i = 0; i < vulkanShader->GetUniformBufferCount(); i++)
		{
			auto& uniformBuffer = vulkanShader->GetUniformBuffer(i);
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &uniformBuffer.Descriptor;
			writeDescriptorSet.dstBinding = uniformBuffer.BindingPoint;
			m_WriteDescriptors.push_back(writeDescriptorSet);
		}

		Ref<VulkanMaterial> instance = this;
		HazelRenderer::Submit([instance]() mutable
			{
				auto shader = instance->GetShader().As<VulkanShader>();
				instance->m_DescriptorSet = shader->CreateDescriptorSets();
			});
	}

	VulkanMaterial::~VulkanMaterial()
	{
	}

	void VulkanMaterial::AllocateStorage()
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		if (shaderBuffers.size() > 0)
		{
			uint32_t size = 0;
			for (auto [name, shaderBuffer] : shaderBuffers)
				size += shaderBuffer.Size;

			m_UniformStorageBuffer.Allocate(size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void VulkanMaterial::OnShaderReloaded()
	{
		return;
		AllocateStorage();
	}

	const ShaderUniform* VulkanMaterial::FindUniformDeclaration(const std::string& name)
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
			if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
				return nullptr;

			return &buffer.Uniforms.at(name);
		}
		return nullptr;
	}

	const ShaderResourceDeclaration* VulkanMaterial::FindResourceDeclaration(const std::string& name)
	{
		auto& resources = m_Shader->GetResources();
		for (const auto& [n, resource] : resources)
		{
			if (resource.GetName() == name)
				return &resource;
		}
		return nullptr;
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<HazelTexture2D>& texture)
	{
		const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
		HZ_CORE_ASSERT(resource);

		// Texture is already set
		if (resource->GetRegister() < m_Textures.size() && m_Textures[resource->GetRegister()] && texture->GetHash() == m_Textures[resource->GetRegister()]->GetHash())
			return;

		if (resource->GetRegister() >= m_Textures.size())
			m_Textures.resize(resource->GetRegister() + 1);
		m_Textures[resource->GetRegister()] = texture;

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		HZ_CORE_ASSERT(wds);
		m_PendingDescriptors.push_back({ PendingDescriptorType::Texture2D, *wds, texture.As<HazelTexture>(), nullptr });
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<HazelTextureCube>& texture)
	{
		const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
		HZ_CORE_ASSERT(resource);

		// Texture is already set
		if (resource->GetRegister() < m_Textures.size() && m_Textures[resource->GetRegister()] && texture->GetHash() == m_Textures[resource->GetRegister()]->GetHash())
			return;

		if (resource->GetRegister() >= m_Textures.size())
			m_Textures.resize(resource->GetRegister() + 1);
		m_Textures[resource->GetRegister()] = texture;

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		HZ_CORE_ASSERT(wds);
		m_PendingDescriptors.push_back({ PendingDescriptorType::TextureCube, *wds, texture.As<HazelTexture>(), nullptr });
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const VkDescriptorImageInfo& imageInfo)
	{
		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		HZ_CORE_ASSERT(wds);

		if (m_ImageInfos.find(name) != m_ImageInfos.end())
		{
			if (m_ImageInfos.at(name).imageView == imageInfo.imageView)
				return;
		}

		m_ImageInfos[name] = imageInfo;

		VkWriteDescriptorSet descriptorSet = *wds;
		descriptorSet.pImageInfo = &m_ImageInfos.at(name);
		m_WriteDescriptors.push_back(descriptorSet);
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<HazelImage2D>& image)
	{
		const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
		HZ_CORE_ASSERT(resource);

		// TODO: replace with set/map
		if (resource->GetRegister() < m_Images.size() && m_Images[resource->GetRegister()] && m_ImageHashes.at(resource->GetRegister()) == image->GetHash())
			return;

		if (resource->GetRegister() >= m_Images.size())
			m_Images.resize(resource->GetRegister() + 1);
		m_Images[resource->GetRegister()] = image;
		m_ImageHashes[resource->GetRegister()] = image->GetHash();

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		HZ_CORE_ASSERT(wds);
		m_PendingDescriptors.push_back({ PendingDescriptorType::Image2D, *wds, nullptr, image.As<HazelImage>() });
	}

	void VulkanMaterial::Set(const std::string& name, float value)
	{
		Set<float>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, int value)
	{
		Set<int>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, uint32_t value)
	{
		Set<uint32_t>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, bool value)
	{
		// Bools are 4-byte ints
		Set<int>(name, (int)value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec2& value)
	{
		Set<glm::vec2>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec3& value)
	{
		Set<glm::vec3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec4& value)
	{
		Set<glm::vec4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::mat3& value)
	{
		Set<glm::mat3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::mat4& value)
	{
		Set<glm::mat4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<HazelTexture2D>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<HazelTextureCube>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<HazelImage2D>& image)
	{
		SetVulkanDescriptor(name, image);
	}

	float& VulkanMaterial::GetFloat(const std::string& name)
	{
		return Get<float>(name);
	}

	int32_t& VulkanMaterial::GetInt(const std::string& name)
	{
		return Get<int32_t>(name);
	}

	uint32_t& VulkanMaterial::GetUInt(const std::string& name)
	{
		return Get<uint32_t>(name);
	}

	bool& VulkanMaterial::GetBool(const std::string& name)
	{
		return Get<bool>(name);
	}

	glm::vec2& VulkanMaterial::GetVector2(const std::string& name)
	{
		return Get<glm::vec2>(name);
	}

	glm::vec3& VulkanMaterial::GetVector3(const std::string& name)
	{
		return Get<glm::vec3>(name);
	}

	glm::vec4& VulkanMaterial::GetVector4(const std::string& name)
	{
		return Get<glm::vec4>(name);
	}

	glm::mat3& VulkanMaterial::GetMatrix3(const std::string& name)
	{
		return Get<glm::mat3>(name);
	}

	glm::mat4& VulkanMaterial::GetMatrix4(const std::string& name)
	{
		return Get<glm::mat4>(name);
	}

	Ref<HazelTexture2D> VulkanMaterial::GetTexture2D(const std::string& name)
	{
		return GetResource<HazelTexture2D>(name);
	}

	Ref<HazelTextureCube> VulkanMaterial::TryGetTextureCube(const std::string& name)
	{
		return TryGetResource<HazelTextureCube>(name);
	}

	Ref<HazelTexture2D> VulkanMaterial::TryGetTexture2D(const std::string& name)
	{
		return TryGetResource<HazelTexture2D>(name);
	}

	Ref<HazelTextureCube> VulkanMaterial::GetTextureCube(const std::string& name)
	{
		return GetResource<HazelTextureCube>(name);
	}

	void VulkanMaterial::UpdateForRendering()
	{
		Ref<VulkanMaterial> instance = this;
		std::vector<VkWriteDescriptorSet>& writeDescriptors = m_WriteDescriptors;
		std::vector<PendingDescriptor>& pendingDescriptors = m_PendingDescriptors;
		HazelRenderer::Submit([instance, writeDescriptors, pendingDescriptors]() mutable
			{
				auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

				for (auto& pd : pendingDescriptors)
				{
					if (pd.Type == PendingDescriptorType::Texture2D)
					{
						Ref<VulkanTexture2D> texture = pd.Texture.As<VulkanTexture2D>();
						pd.WDS.pImageInfo = &texture->GetVulkanDescriptorInfo();
					}
					else if (pd.Type == PendingDescriptorType::TextureCube)
					{
						Ref<VulkanTextureCube> texture = pd.Texture.As<VulkanTextureCube>();
						pd.WDS.pImageInfo = &texture->GetVulkanDescriptorInfo();
					}
					else if (pd.Type == PendingDescriptorType::Image2D)
					{
						Ref<VulkanImage2D> image = pd.Image.As<VulkanImage2D>();
						pd.WDS.pImageInfo = &image->GetDescriptor();
					}

					writeDescriptors.push_back(pd.WDS);
				}

				if (writeDescriptors.size())
				{
					for (auto& writeDescriptor : writeDescriptors)
						writeDescriptor.dstSet = instance->m_DescriptorSet.DescriptorSets[0];

					HZ_CORE_WARN("Updating {0} descriptor sets", writeDescriptors.size());
					vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
				}
			});
		pendingDescriptors.clear();
		writeDescriptors.clear();
	}

}
