/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanMaterialH2M.h"

#include "H2M/Renderer/RendererH2M.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Platform/Vulkan/VulkanImageH2M.h"


namespace H2M
{

	VulkanMaterialH2M::VulkanMaterialH2M(const RefH2M<HazelShader>& shader, const std::string& name)
		: MaterialH2M(shader, name)
	{
		m_WriteDescriptors = std::vector<VkWriteDescriptorSet>();

		m_Shader->AddShaderReloadedCallback(std::bind(&VulkanMaterialH2M::OnShaderReloaded, this));
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;

		// RefH2M<VulkanMaterialH2M> instance = this;
		// HazelRenderer::Submit([instance]() mutable {});
		{
			auto shader = GetShader().As<VulkanShaderH2M>();
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
		// HazelRenderer::RegisterShaderDependency(shader, RefH2M<MaterialH2M>(this));
	}

	VulkanMaterialH2M::~VulkanMaterialH2M()
	{
		Log::GetLogger()->debug("Destroy VulkanMaterialH2M!");
	}

	void VulkanMaterialH2M::AllocateStorage()
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		if (shaderBuffers.size() > 0)
		{
			uint32_t size = 0;
			for (auto [name, shaderBuffer] : shaderBuffers)
			{
				size += shaderBuffer.Size;
			}

			const ShaderBufferH2M& buffer = (*shaderBuffers.begin()).second;
			m_UniformStorageBuffer.Allocate(size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void VulkanMaterialH2M::Init()
	{
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;

		// TODO: Maybe get the descriptor set from the pipeline
		RefH2M<VulkanShaderH2M> vulkanShader = m_Shader.As<VulkanShaderH2M>();
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

		// RefH2M<VulkanMaterialH2M> instance = this;
		// HazelRenderer::Submit([=]() mutable
		// {
		// });
		{
			auto shader = GetShader().As<VulkanShaderH2M>();
			m_DescriptorSet = shader->CreateDescriptorSets();
		}
	}

	void VulkanMaterialH2M::Invalidate()
	{
		auto shader = m_Shader.As<VulkanShaderH2M>();
		m_DescriptorSet = shader->CreateDescriptorSets();

		RefH2M<VulkanShaderH2M> vulkanShader = m_Shader.As<VulkanShaderH2M>();
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

	void VulkanMaterialH2M::OnShaderReloaded()
	{
		return;
		AllocateStorage();
	}

	const ShaderUniformH2M* VulkanMaterialH2M::FindUniformDeclaration(const std::string& name)
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		H2M_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBufferH2M& buffer = (*shaderBuffers.begin()).second;
			if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
			{
				return nullptr;
			}

			return &buffer.Uniforms.at(name);
		}
		return nullptr;
	}

	const ShaderResourceDeclarationH2M* VulkanMaterialH2M::FindResourceDeclaration(const std::string& name)
	{
		auto& resources = m_Shader->GetResources();

		if (!resources.size())
		{
			Log::GetLogger()->error("VulkanMaterialH2M::FindResourceDeclaration - no resources found (name '{0}')!", name);
			return nullptr;
		}

		for (const auto& [n, resource] : resources)
		{
			if (resource.GetName() == name)
				return &resource;
		}
		return nullptr;
	}

	void VulkanMaterialH2M::SetVulkanDescriptor(const std::string& name, const RefH2M<Texture2D_H2M>& texture)
	{
		// RefH2M<VulkanMaterialH2M> instance = this;
		m_Textures.push_back(texture);
		// HazelRenderer::Submit([instance, name, texture]() mutable {});
		{
			const VkWriteDescriptorSet* wds = GetShader().As<VulkanShaderH2M>()->GetDescriptorSet(name);
			H2M_CORE_ASSERT(wds);

			VkWriteDescriptorSet descriptorSet = *wds;
			descriptorSet.dstSet = m_DescriptorSet.DescriptorSets[0];
			auto& imageInfo = texture.As<VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
			descriptorSet.pImageInfo = &imageInfo;
			m_WriteDescriptors.push_back(descriptorSet);
		}

		/**** BEGIN a more recent version ****/
		//	const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
		//	
		//	if (!resource)
		//	{
		//		// H2M_CORE_ASSERT(resource);
		//		Log::GetLogger()->error("VulkanMaterialH2M::SetVulkanDescriptor - resource not found (name '{0}')!", name);
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
		//	const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShaderH2M>()->GetDescriptorSet(name);
		//	H2M_CORE_ASSERT(wds);
		//	m_ResidentDescriptors.push_back(std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::Texture2D, *wds, {}, texture.As<HazelTexture>() /*, nullptr */ }));
		//	m_PendingDescriptors.push_back(m_ResidentDescriptors.back());
		/**** END a more recent version ****/
	}

	void VulkanMaterialH2M::SetVulkanDescriptor(const std::string& name, const RefH2M<Texture2D_H2M>& texture, uint32_t arrayIndex)
	{
		const ShaderResourceDeclarationH2M* resource = FindResourceDeclaration(name);
		H2M_CORE_ASSERT(resource);

		uint32_t binding = resource->GetRegister();
		// Texture is already set
		if (binding < m_TextureArrays.size() && m_TextureArrays[binding].size() < arrayIndex && texture->GetHash() == m_TextureArrays[binding][arrayIndex]->GetHash())
			return;

		if (binding >= m_TextureArrays.size())
			m_TextureArrays.resize(binding + 1);

		if (arrayIndex >= m_TextureArrays[binding].size())
			m_TextureArrays[binding].resize(arrayIndex + 1);

		m_TextureArrays[binding][arrayIndex] = texture;

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShaderH2M>()->GetDescriptorSet(name);
		H2M_CORE_ASSERT(wds);
		if (m_ResidentDescriptorArrays.find(binding) == m_ResidentDescriptorArrays.end())
		{
			m_ResidentDescriptorArrays[binding] = std::make_shared<PendingDescriptorArray>(PendingDescriptorArray{ PendingDescriptorType::Texture2D, *wds, {}, {}, {} });
		}

		auto& residentDesriptorArray = m_ResidentDescriptorArrays.at(binding);
		if (arrayIndex >= residentDesriptorArray->Textures.size())
			residentDesriptorArray->Textures.resize(arrayIndex + 1);

		residentDesriptorArray->Textures[arrayIndex] = texture;

		//m_PendingDescriptors.push_back(m_ResidentDescriptors.at(binding));

		InvalidateDescriptorSets();
	}

	void VulkanMaterialH2M::SetVulkanDescriptor(const std::string& name, const RefH2M<TextureCubeH2M>& texture)
	{
		const ShaderResourceDeclarationH2M* resource = FindResourceDeclaration(name);
		H2M_CORE_ASSERT(resource);

		// Texture is already set
		if (resource->GetRegister() < m_Textures.size() && m_Textures[resource->GetRegister()] && texture->GetHash() == m_Textures[resource->GetRegister()]->GetHash())
			return;

		if (resource->GetRegister() >= m_Textures.size())
			m_Textures.resize(resource->GetRegister() + 1);
		m_Textures[resource->GetRegister()] = texture;

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShaderH2M>()->GetDescriptorSet(name);
		H2M_CORE_ASSERT(wds);
		m_ResidentDescriptors.push_back(std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::TextureCube, *wds, {}, texture.As<TextureH2M>() /*, nullptr */ }));
		m_PendingDescriptors.push_back(m_ResidentDescriptors.back());
	}

	void VulkanMaterialH2M::SetVulkanDescriptor(const std::string& name, const VkDescriptorImageInfo& imageInfo)
	{
		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShaderH2M>()->GetDescriptorSet(name);
		H2M_CORE_ASSERT(wds);

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

	void VulkanMaterialH2M::SetVulkanDescriptor(const std::string& name, const RefH2M<Image2D_H2M>& image)
	{
		const ShaderResourceDeclarationH2M* resource = FindResourceDeclaration(name);
		H2M_CORE_ASSERT(resource);

		// TODO: replace with set/map
		if (resource->GetRegister() < m_Images.size() && m_Images[resource->GetRegister()] && m_ImageHashes.at(resource->GetRegister()) == image->GetHash())
			return;

		if (resource->GetRegister() >= m_Images.size())
			m_Images.resize(resource->GetRegister() + 1);
		m_Images[resource->GetRegister()] = image;
		m_ImageHashes[resource->GetRegister()] = image->GetHash();

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShaderH2M>()->GetDescriptorSet(name);
		H2M_CORE_ASSERT(wds);
		// m_ResidentDescriptors.push_back(std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::TextureCube, *wds, {}, texture.As<HazelTexture>() /* , nullptr */ }));
		m_PendingDescriptors.push_back(m_ResidentDescriptors.back());
	}

	void VulkanMaterialH2M::Set(const std::string& name, float value)
	{
		Set<float>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, int value)
	{
		Set<int>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, uint32_t value)
	{
		Set<uint32_t>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, bool value)
	{
		// Bools are 4-byte ints
		Set<int>(name, (int)value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const glm::ivec2& value)
	{
		Set<glm::ivec2>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const glm::ivec3& value)
	{
		Set<glm::ivec3>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const glm::ivec4& value)
	{
		Set<glm::ivec4>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const glm::vec2& value)
	{
		Set<glm::vec2>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const glm::vec3& value)
	{
		Set<glm::vec3>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const glm::vec4& value)
	{
		Set<glm::vec4>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const glm::mat3& value)
	{
		Set<glm::mat3>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const glm::mat4& value)
	{
		Set<glm::mat4>(name, value);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const RefH2M<Texture2D_H2M>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const RefH2M<Texture2D_H2M>& texture, uint32_t arrayIndex)
	{
		SetVulkanDescriptor(name, texture, arrayIndex);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const RefH2M<TextureCubeH2M>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterialH2M::Set(const std::string& name, const RefH2M<Image2D_H2M>& image)
	{
		SetVulkanDescriptor(name, image);
	}

	float& VulkanMaterialH2M::GetFloat(const std::string& name)
	{
		return Get<float>(name);
	}

	int32_t& VulkanMaterialH2M::GetInt(const std::string& name)
	{
		return Get<int32_t>(name);
	}

	uint32_t& VulkanMaterialH2M::GetUInt(const std::string& name)
	{
		return Get<uint32_t>(name);
	}

	bool& VulkanMaterialH2M::GetBool(const std::string& name)
	{
		return Get<bool>(name);
	}

	glm::vec2& VulkanMaterialH2M::GetVector2(const std::string& name)
	{
		return Get<glm::vec2>(name);
	}

	glm::vec3& VulkanMaterialH2M::GetVector3(const std::string& name)
	{
		return Get<glm::vec3>(name);
	}

	glm::vec4& VulkanMaterialH2M::GetVector4(const std::string& name)
	{
		return Get<glm::vec4>(name);
	}

	glm::mat3& VulkanMaterialH2M::GetMatrix3(const std::string& name)
	{
		return Get<glm::mat3>(name);
	}

	glm::mat4& VulkanMaterialH2M::GetMatrix4(const std::string& name)
	{
		return Get<glm::mat4>(name);
	}

	RefH2M<Texture2D_H2M> VulkanMaterialH2M::GetTexture2D(const std::string& name)
	{
		return GetResource<Texture2D_H2M>(name);
	}

	RefH2M<TextureCubeH2M> VulkanMaterialH2M::TryGetTextureCube(const std::string& name)
	{
		return TryGetResource<TextureCubeH2M>(name);
	}

	RefH2M<Texture2D_H2M> VulkanMaterialH2M::TryGetTexture2D(const std::string& name)
	{
		return TryGetResource<Texture2D_H2M>(name);
	}

	RefH2M<TextureCubeH2M> VulkanMaterialH2M::GetTextureCube(const std::string& name)
	{
		return GetResource<TextureCubeH2M>(name);
	}

	void VulkanMaterialH2M::UpdateForRendering()
	{
		// RefH2M<VulkanMaterialH2M> instance = this;
		// HazelRenderer::Submit([instance]() mutable {});
		{
			auto vulkanDevice = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

			if (m_WriteDescriptors.size())
			{
				H2M_CORE_WARN("Updating {0} descriptor sets", m_WriteDescriptors.size());
				vkUpdateDescriptorSets(vulkanDevice, (uint32_t)m_WriteDescriptors.size(), m_WriteDescriptors.data(), 0, nullptr);
				m_WriteDescriptors.clear();
			}
		}
	}

	void VulkanMaterialH2M::InvalidateDescriptorSets()
	{
		const uint32_t framesInFlight = RendererH2M::GetConfig().FramesInFlight;
		for (uint32_t i = 0; i < framesInFlight; i++)
			m_DirtyDescriptorSets[i] = true;
	}

}
