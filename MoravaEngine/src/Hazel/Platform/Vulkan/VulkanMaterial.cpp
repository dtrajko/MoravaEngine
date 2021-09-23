#include "VulkanMaterial.h"

#include "Hazel/Renderer/HazelRenderer.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Platform/Vulkan/VulkanImage.h"


namespace Hazel {

	VulkanMaterial::VulkanMaterial(const Ref<HazelShader>& shader, const std::string& name)
		: HazelMaterial(shader, name)
	{
		m_WriteDescriptors = std::vector<VkWriteDescriptorSet>();

		m_Shader->AddShaderReloadedCallback(std::bind(&VulkanMaterial::OnShaderReloaded, this));
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;

		// Ref<VulkanMaterial> instance = this;
		// HazelRenderer::Submit([instance]() mutable {});
		{
			auto shader = GetShader().As<VulkanShader>();
			m_DescriptorSet = shader->CreateDescriptorSets();

			for (uint32_t i = 0; i < shader->GetUniformBufferCount(); i++)
			{
				auto& uniformBuffer = shader->GetUniformBuffer(i);
				VkWriteDescriptorSet writeDescriptorSet = {};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = *m_DescriptorSet.DescriptorSets.data();
				writeDescriptorSet.descriptorCount = (uint32_t)m_DescriptorSet.DescriptorSets.size();
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSet.pBufferInfo = &uniformBuffer.Descriptor;
				writeDescriptorSet.dstBinding = uniformBuffer.BindingPoint;
				m_WriteDescriptors.push_back(writeDescriptorSet);
			}
		}

		// Init();
		// HazelRenderer::RegisterShaderDependency(shader, Ref<HazelMaterial>(this));
	}

	VulkanMaterial::~VulkanMaterial()
	{
		Log::GetLogger()->debug("Destroy VulkanMaterial!");
	}

	void VulkanMaterial::AllocateStorage()
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		if (shaderBuffers.size() > 0)
		{
			uint32_t size = 0;
			for (auto [name, shaderBuffer] : shaderBuffers)
			{
				size += shaderBuffer.Size;
			}

			const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
			m_UniformStorageBuffer.Allocate(size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void VulkanMaterial::Init()
	{
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;

		// TODO: Maybe get the descriptor set from the pipeline
		Ref<VulkanShader> vulkanShader = m_Shader.As<VulkanShader>();
		for (uint32_t i = 0; i < vulkanShader->GetUniformBufferCount(); i++)
		{
			// auto& uniformBuffer = vulkanShader->GetUniformBuffer(i);
			auto& uniformBuffer = vulkanShader->GetUniformBuffer();
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &uniformBuffer.Descriptor;
			writeDescriptorSet.dstBinding = uniformBuffer.BindingPoint;
			m_WriteDescriptors.push_back(writeDescriptorSet);
		}

		// Ref<VulkanMaterial> instance = this;
		// HazelRenderer::Submit([=]() mutable
		// {
		// });
		{
			auto shader = GetShader().As<VulkanShader>();
			m_DescriptorSet = shader->CreateDescriptorSets();
		}
	}

	void VulkanMaterial::Invalidate()
	{
		auto shader = m_Shader.As<VulkanShader>();
		m_DescriptorSet = shader->CreateDescriptorSets();

		Ref<VulkanShader> vulkanShader = m_Shader.As<VulkanShader>();
		for (uint32_t i = 0; i < vulkanShader->GetUniformBufferCount(); i++)
		{
			// auto& uniformBuffer = vulkanShader->GetUniformBuffer(i);
			auto& uniformBuffer = vulkanShader->GetUniformBuffer();
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &uniformBuffer.Descriptor;
			writeDescriptorSet.dstBinding = uniformBuffer.BindingPoint;
			m_WriteDescriptors.push_back(writeDescriptorSet);
		}

		for (auto& descriptor : m_ResidentDescriptors) {
			m_PendingDescriptors.push_back(descriptor);
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
			{
				return nullptr;
			}

			return &buffer.Uniforms.at(name);
		}
		return nullptr;
	}

	const ShaderResourceDeclaration* VulkanMaterial::FindResourceDeclaration(const std::string& name)
	{
		auto& resources = m_Shader->GetResources();

		if (!resources.size())
		{
			Log::GetLogger()->error("VulkanMaterial::FindResourceDeclaration - no resources found (name '{0}')!", name);
			return nullptr;
		}

		for (const auto& [n, resource] : resources)
		{
			if (resource.GetName() == name)
				return &resource;
		}
		return nullptr;
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<HazelTexture2D>& texture)
	{
		// Ref<VulkanMaterial> instance = this;
		m_Textures.push_back(texture);
		// HazelRenderer::Submit([instance, name, texture]() mutable {});
		{
			const VkWriteDescriptorSet* wds = GetShader().As<VulkanShader>()->GetDescriptorSet(name);
			HZ_CORE_ASSERT(wds);

			VkWriteDescriptorSet descriptorSet = *wds;
			descriptorSet.dstSet = m_DescriptorSet.DescriptorSets[0];
			auto& imageInfo = texture.As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
			descriptorSet.pImageInfo = &imageInfo;
			m_WriteDescriptors.push_back(descriptorSet);
		}

		/**** BEGIN a more recent version ****/
		//	const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
		//	
		//	if (!resource)
		//	{
		//		// HZ_CORE_ASSERT(resource);
		//		Log::GetLogger()->error("VulkanMaterial::SetVulkanDescriptor - resource not found (name '{0}')!", name);
		//		return;
		//	}
		//	
		//	// Texture is already set
		//	if (resource->GetRegister() < m_Textures.size() && m_Textures[resource->GetRegister()] && texture->GetHash() == m_Textures[resource->GetRegister()]->GetHash())
		//		return;
		//	
		//	if (resource->GetRegister() >= m_Textures.size())
		//		m_Textures.resize(resource->GetRegister() + 1);
		//	m_Textures[resource->GetRegister()] = texture;
		//	
		//	const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		//	HZ_CORE_ASSERT(wds);
		//	m_ResidentDescriptors.push_back(std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::Texture2D, *wds, {}, texture.As<HazelTexture>() /*, nullptr */ }));
		//	m_PendingDescriptors.push_back(m_ResidentDescriptors.back());
		/**** END a more recent version ****/
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
		m_ResidentDescriptors.push_back(std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::TextureCube, *wds, {}, texture.As<HazelTexture>() /*, nullptr */ }));
		m_PendingDescriptors.push_back(m_ResidentDescriptors.back());
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
		// m_ResidentDescriptors.push_back(std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::TextureCube, *wds, {}, texture.As<HazelTexture>() /* , nullptr */ }));
		m_PendingDescriptors.push_back(m_ResidentDescriptors.back());
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
		// Ref<VulkanMaterial> instance = this;
		// HazelRenderer::Submit([instance]() mutable {});
		{
			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			if (m_WriteDescriptors.size())
			{
				HZ_CORE_WARN("Updating {0} descriptor sets", m_WriteDescriptors.size());
				vkUpdateDescriptorSets(vulkanDevice, (uint32_t)m_WriteDescriptors.size(), m_WriteDescriptors.data(), 0, nullptr);
				m_WriteDescriptors.clear();
			}
		}
	}

}
