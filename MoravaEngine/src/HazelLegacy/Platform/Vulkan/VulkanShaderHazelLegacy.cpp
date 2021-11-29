#include "VulkanShaderHazelLegacy.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/ShaderCache.h"

#include "HazelLegacy/Platform/Vulkan/VulkanAllocatorHazelLegacy.h"

#include "Core/Log.h"

#include <shaderc/shaderc.hpp>
#include <spirv_glsl.hpp>

#include <filesystem>


namespace Hazel {

	namespace Utils {

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "Resources/Cache/Shader/Vulkan";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

		static ShaderUniformType SPIRTypeToShaderUniformType(spirv_cross::SPIRType type)
		{
			switch (type.basetype)
			{
			case spirv_cross::SPIRType::Boolean:  return ShaderUniformType::Bool;
			case spirv_cross::SPIRType::Int:
				if (type.vecsize == 1)            return ShaderUniformType::Int;
				if (type.vecsize == 2)            return ShaderUniformType::IVec2;
				if (type.vecsize == 3)            return ShaderUniformType::IVec3;
				if (type.vecsize == 4)            return ShaderUniformType::IVec4;

			case spirv_cross::SPIRType::UInt:     return ShaderUniformType::UInt;
			case spirv_cross::SPIRType::Float:
				if (type.columns == 3)            return ShaderUniformType::Mat3;
				if (type.columns == 4)            return ShaderUniformType::Mat4;

				if (type.vecsize == 1)            return ShaderUniformType::Float;
				if (type.vecsize == 2)            return ShaderUniformType::Vec2;
				if (type.vecsize == 3)            return ShaderUniformType::Vec3;
				if (type.vecsize == 4)            return ShaderUniformType::Vec4;
				break;
			}
			HZ_CORE_ASSERT(false, "Unknown type!");
			return ShaderUniformType::None;
		}

	}

	static std::unordered_map<uint32_t, std::unordered_map<uint32_t, VulkanShaderHazelLegacy::UniformBuffer*>> s_UniformBuffers; // set -> binding point -> buffer
	static std::unordered_map<uint32_t, std::unordered_map<uint32_t, VulkanShaderHazelLegacy::StorageBuffer*>> s_StorageBuffers; // set -> binding point -> buffer

	// Very temporary attribute in Vulkan Week Day 5 Part 1
	// Hazel::Ref<HazelLegacy::Texture2DHazelLegacy> VulkanShaderHazelLegacy::s_AlbedoTexture;
	// Hazel::Ref<HazelLegacy::Texture2DHazelLegacy> VulkanShaderHazelLegacy::s_NormalTexture;

	VulkanShaderHazelLegacy::VulkanShaderHazelLegacy(const std::string& path, bool forceCompile)
		: m_AssetPath(path)
	{
		// TODO: This should be more "general"
		size_t found = path.find_last_of("/\\");
		m_Name = found != std::string::npos ? path.substr(found + 1) : path;
		found = m_Name.find_last_of(".");
		m_Name = found != std::string::npos ? m_Name.substr(0, found) : m_Name;

		Reload(forceCompile);
	}

	VulkanShaderHazelLegacy::~VulkanShaderHazelLegacy()
	{
	}

	void VulkanShaderHazelLegacy::ClearUniformBuffers()
	{
		s_UniformBuffers.clear();
		s_StorageBuffers.clear();
	}

	static std::string ReadShaderFromFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
		}
		else
		{
			HZ_CORE_VERIFY(false, "Could not load shader!");
		}
		in.close();
		return result;
	}

	void VulkanShaderHazelLegacy::Reload(bool forceCompile)
	{
		// Ref<VulkanShaderHazelLegacy> instance = this;
		// HazelRenderer::Submit([instance, forceCompile]() mutable {});
		{
			// Clear old shader
			m_ShaderDescriptorSets.clear();
			m_Resources.clear();
			m_PushConstantRanges.clear();
			m_PipelineShaderStageCreateInfos.clear();
			m_DescriptorSetLayouts.clear();
			m_ShaderSource.clear();
			m_Buffers.clear();
			m_TypeCounts.clear();


			Utils::CreateCacheDirectoryIfNeeded();

			// Vertex and Fragment for now
			std::string source = ReadShaderFromFile(m_AssetPath);
			forceCompile = ShaderCache::HasChanged(m_AssetPath, source);

			m_ShaderSource = PreProcess(source);
			std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;
			CompileOrGetVulkanBinary(shaderData, forceCompile);
			LoadAndCreateShaders(shaderData);
			ReflectAllShaderStages(shaderData);
			CreateDescriptors();

			HazelRenderer::OnShaderReloaded(GetHash());
		}
	}

	size_t VulkanShaderHazelLegacy::GetHash() const
	{
		return std::hash<std::string>{}(m_AssetPath);
	}

	void VulkanShaderHazelLegacy::LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		m_PipelineShaderStageCreateInfos.clear();

		for (auto [stage, data] : shaderData)
		{
			HZ_CORE_ASSERT(data.size());

			// Create a new shader module that will be used for pipeline creation
			VkShaderModuleCreateInfo moduleCreateInfo{};
			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.codeSize = data.size() * sizeof(uint32_t);
			moduleCreateInfo.pCode = data.data();

			VkShaderModule shaderModule;
			VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

			VkPipelineShaderStageCreateInfo& shaderStage = m_PipelineShaderStageCreateInfos.emplace_back();
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.stage = stage;
			shaderStage.module = shaderModule;
			shaderStage.pName = "main";
		}
	}

	void VulkanShaderHazelLegacy::ReflectAllShaderStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		m_Resources.clear();

		for (auto [stage, data] : shaderData)
		{
			Reflect(stage, data);
		}
	}

	void VulkanShaderHazelLegacy::Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		std::string shaderStageName = "UNKNOWN";
		switch (shaderStage)
		{
			case VK_SHADER_STAGE_VERTEX_BIT:   shaderStageName = "VERTEX";   break;
			case VK_SHADER_STAGE_FRAGMENT_BIT: shaderStageName = "FRAGMENT"; break;
			case VK_SHADER_STAGE_COMPUTE_BIT:  shaderStageName = "COMPUTE";  break;
		}

		MORAVA_CORE_TRACE("==========================");
		MORAVA_CORE_TRACE(" Vulkan Shader Reflection (Stage: " + shaderStageName  + ")");
		MORAVA_CORE_TRACE(" {0}", m_AssetPath);
		MORAVA_CORE_TRACE("==========================");

		// Vertex Shader
		spirv_cross::Compiler compiler(shaderData);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		MORAVA_CORE_TRACE("Uniform Buffers:");
		for (const spirv_cross::Resource& resource : resources.uniform_buffers)
		{
			const auto& name = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			int memberCount = static_cast<uint32_t>(bufferType.member_types.size());
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t size = static_cast<uint32_t>(compiler.get_declared_struct_size(bufferType));

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			HZ_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end());

			// UniformBuffer& buffer = shaderDescriptorSet.UniformBuffers[bindingPoint];
			UniformBuffer& buffer = shaderDescriptorSet.UniformBuffers[binding];
			// UniformBuffer buffer;
			buffer.BindingPoint = binding;
			buffer.DescriptorSet = descriptorSet;
			buffer.Size = size;
			// AllocateUniformBuffer(buffer);
			buffer.Name = name;
			buffer.ShaderStage = shaderStage;
			// m_UniformBuffers.insert(std::pair(bindingPoint, buffer));

			MORAVA_CORE_TRACE("    {0} ({1}, {2})", name, descriptorSet, binding);

			MORAVA_CORE_TRACE("  Name: {0}", name);
			MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
			MORAVA_CORE_TRACE("  Descriptor Set: {0}", descriptorSet);
			MORAVA_CORE_TRACE("  Binding Point: {0}", binding);
			MORAVA_CORE_TRACE("  Size: {0}", size);
			MORAVA_CORE_TRACE("--------------------------");
		}

		MORAVA_CORE_TRACE("Storage Buffers:");
		for (const auto& resource : resources.storage_buffers)
		{
			const auto& name = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t memberCount = (uint32_t)bufferType.member_types.size();
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);

			if (descriptorSet >= m_ShaderDescriptorSets.size())
			{
				// m_ShaderDescriptorSets.resize(descriptorSet + 1);
			}

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			if (s_StorageBuffers[descriptorSet].find(binding) == s_StorageBuffers[descriptorSet].end())
			{
				StorageBuffer* storageBuffer = new StorageBuffer();
				storageBuffer->BindingPoint = binding;
				storageBuffer->Size = size;
				storageBuffer->Name = name;
				storageBuffer->ShaderStage = VK_SHADER_STAGE_ALL;
				s_StorageBuffers.at(descriptorSet)[binding] = storageBuffer;
			}
			else
			{
				StorageBuffer* storageBuffer = s_StorageBuffers.at(descriptorSet).at(binding);
				if (size > storageBuffer->Size)
					storageBuffer->Size = size;
			}

			shaderDescriptorSet.StorageBuffers[binding] = *s_StorageBuffers.at(descriptorSet).at(binding);

			MORAVA_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
			MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
			MORAVA_CORE_TRACE("  Size: {0}", size);
			MORAVA_CORE_TRACE("-------------------");
		}

		MORAVA_CORE_TRACE("Push Constant Buffers:");
		for (const auto& resource : resources.push_constant_buffers)
		{
			const auto& bufferName = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			auto bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
			uint32_t memberCount = uint32_t(bufferType.member_types.size());
			uint32_t bufferOffset = 0;
			if (m_PushConstantRanges.size())
			{
				bufferOffset = m_PushConstantRanges.back().Offset + m_PushConstantRanges.back().Size;
			}

			auto& pushConstantRange = m_PushConstantRanges.emplace_back();
			pushConstantRange.ShaderStage = shaderStage;
			pushConstantRange.Size = bufferSize - bufferOffset;
			pushConstantRange.Offset = bufferOffset;

			// Skip empty push constant buffers - these are for the renderer only
			if (bufferName.empty() || bufferName == "u_Renderer")
			{
				continue;
			}

			ShaderBuffer& buffer = m_Buffers[bufferName];
			buffer.Name = bufferName;
			buffer.Size = bufferSize - bufferOffset;

			// MORAVA_CORE_TRACE("    {0} ({1}, {2})", name, descriptorSet, binding);

			MORAVA_CORE_TRACE("  Name: {0}", bufferName);
			MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
			// MORAVA_CORE_TRACE("  Binding Point: {0}", bindingPoint);
			MORAVA_CORE_TRACE("  Buffer size: {0}", bufferSize);
			MORAVA_CORE_TRACE("--------------------------");

			for (uint32_t i = 0; i < memberCount; i++)
			{
				auto type = compiler.get_type(bufferType.member_types[i]);
				const auto& memberName = compiler.get_member_name(bufferType.self, i);
				auto size = (uint32_t)compiler.get_declared_struct_member_size(bufferType, i);
				auto offset = compiler.type_struct_member_offset(bufferType, i) - bufferOffset;

				std::string uniformName = fmt::format("{}.{}", bufferName, memberName);
				buffer.Uniforms[uniformName] = ShaderUniform(uniformName, Utils::SPIRTypeToShaderUniformType(type), size, offset);
			}
		}

		MORAVA_CORE_TRACE("Sampled Images:");
		for (const auto& resource : resources.sampled_images)
		{
			const auto& name = resource.name;
			auto& baseType = compiler.get_type(resource.base_type_id);
			auto& type = compiler.get_type(resource.type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t dimension = baseType.image.dim;
			uint32_t arraySize = type.array[0];
			if (arraySize == 0)
				arraySize = 1;
			if (descriptorSet >= m_ShaderDescriptorSets.size())
			{
				// m_ShaderDescriptorSets.resize(descriptorSet + 1);
			}

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			HZ_CORE_ASSERT(shaderDescriptorSet.ImageSamplers.find(binding) == shaderDescriptorSet.ImageSamplers.end());
			// ImageSampler imageSampler;
			auto& imageSampler = shaderDescriptorSet.ImageSamplers[binding];
			imageSampler.BindingPoint = binding;
			imageSampler.DescriptorSet = descriptorSet;
			imageSampler.Name = name;
			imageSampler.ShaderStage = shaderStage;
			// m_ImageSamplers.insert(std::pair(bindingPoint, imageSampler));
			imageSampler.ArraySize = arraySize;

			m_Resources[name] = ShaderResourceDeclaration(name, binding, 1);

			MORAVA_CORE_TRACE("    {0} ({1}, {2})", name, descriptorSet, binding);

			MORAVA_CORE_TRACE("  Name: {0}", name);
			// MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
			MORAVA_CORE_TRACE("  Descriptor Set: {0}", descriptorSet);
			MORAVA_CORE_TRACE("  Binding Point: {0}", binding);
			// MORAVA_CORE_TRACE("  Size: {0}", size);
			MORAVA_CORE_TRACE("--------------------------");
		}

		MORAVA_CORE_TRACE("Storage Images:");
		for (const auto& resource : resources.storage_images)
		{
			const auto& name = resource.name;
			auto& type = compiler.get_type(resource.base_type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t dimension = type.image.dim;

			if (descriptorSet >= m_ShaderDescriptorSets.size())
			{
				// m_ShaderDescriptorSets.resize(descriptorSet + 1);
			}

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			// ImageSampler imageSampler;
			auto& imageSampler = shaderDescriptorSet.StorageImages[binding];
			imageSampler.BindingPoint = binding;
			imageSampler.DescriptorSet = descriptorSet;
			imageSampler.Name = name;
			imageSampler.ShaderStage = shaderStage;
			// m_ImageSamplers.insert(std::pair(bindingPoint, imageSampler));

			MORAVA_CORE_TRACE("    {0} ({1}, {2})", name, descriptorSet, binding);

			MORAVA_CORE_TRACE("  Name: {0}", name);
			// MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
			MORAVA_CORE_TRACE("  Descriptor Set: {0}", descriptorSet);
			MORAVA_CORE_TRACE("  Binding Point: {0}", binding);
			// MORAVA_CORE_TRACE("  Size: {0}", size);
			MORAVA_CORE_TRACE("--------------------------");
		}

		MORAVA_CORE_TRACE("==========================");
	}

	void VulkanShaderHazelLegacy::CreateDescriptors()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		//////////////////////////////////////////////////////////////////////
		// Descriptor Pool
		//////////////////////////////////////////////////////////////////////

		// We need to tell the API the number of max. requested descriptors per type
		m_TypeCounts.clear();
		for (uint32_t set = 0; set < m_ShaderDescriptorSets.size(); set++)
		{
			auto& shaderDescriptorSet = m_ShaderDescriptorSets[set];

			if (shaderDescriptorSet.UniformBuffers.size())
			{
				VkDescriptorPoolSize& typeCount = m_TypeCounts[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				typeCount.descriptorCount = static_cast<uint32_t>(shaderDescriptorSet.UniformBuffers.size());
			}

			if (shaderDescriptorSet.ImageSamplers.size())
			{
				VkDescriptorPoolSize& typeCount = m_TypeCounts[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				typeCount.descriptorCount = static_cast<uint32_t>(shaderDescriptorSet.ImageSamplers.size());
			}

			if (shaderDescriptorSet.StorageImages.size())
			{
				VkDescriptorPoolSize& typeCount = m_TypeCounts[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				typeCount.descriptorCount = static_cast<uint32_t>(shaderDescriptorSet.StorageImages.size());
			}

#if 0
			// TODO: Move this to the centralized renderer
			// Create the global descriptor pool
			// All descriptors used in this example are allocated from this pool
			VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			// Once you bind a descriptor set and use it in a vkCmdDraw() function, you can no longer modify it unless you specify the
			// descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			descriptorPoolInfo.pNext = nullptr;
			descriptorPoolInfo.poolSizeCount = (uint32_t)m_TypeCounts.at(set).size();
			descriptorPoolInfo.pPoolSizes = m_TypeCounts.at(set).data();
			descriptorPoolInfo.maxSets = 1;

			VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &m_DescriptorPool));
#endif

			//////////////////////////////////////////////////////////////////////
			// Descriptor Set Layout
			//////////////////////////////////////////////////////////////////////

			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			for (auto& [binding, uniformBuffer] : shaderDescriptorSet.UniformBuffers)
			{
				VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = uniformBuffer.ShaderStage;
				layoutBinding.pImmutableSamplers = nullptr;
				layoutBinding.binding = binding;

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[uniformBuffer.Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType; // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				set.descriptorCount = 1;
				set.dstBinding = layoutBinding.binding;

				AllocateUniformBuffer(uniformBuffer);
			}

			for (auto& [binding, imageSampler] : shaderDescriptorSet.ImageSamplers)
			{
				VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = imageSampler.ShaderStage;
				layoutBinding.pImmutableSamplers = nullptr;
				layoutBinding.binding = binding;

				HZ_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end(), "Binding is already present in m_UniformBuffers!");

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[imageSampler.Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType; // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				set.descriptorCount = 1;
				set.dstBinding = layoutBinding.binding;
			}

			for (auto& [binding, storageImage] : shaderDescriptorSet.StorageImages)
			{
				VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = storageImage.ShaderStage;
				layoutBinding.pImmutableSamplers = nullptr;

				// uint32_t binding = bindingAndSet & 0xffffffff;
				// uint32_t descriptorSet = (bindingAndSet >> 32);
				layoutBinding.binding = binding;

				HZ_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end(), "Binding is already present in m_UniformBuffers!");
				HZ_CORE_ASSERT(shaderDescriptorSet.ImageSamplers.find(binding) == shaderDescriptorSet.ImageSamplers.end(), "Binding is already present in m_ImageSamplers!");

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[storageImage.Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType; // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
				set.descriptorCount = 1;
				set.dstBinding = layoutBinding.binding;
				// set.dstSet = descriptorSet;
			}

			VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
			descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayout.pNext = nullptr;
			descriptorLayout.bindingCount = static_cast<uint32_t>(layoutBindings.size());
			descriptorLayout.pBindings = layoutBindings.data();

			MORAVA_CORE_INFO("Creating descriptor set {0} with {1} ubos, {2} samplers and {3} storage images", set,
				shaderDescriptorSet.UniformBuffers.size(),
				shaderDescriptorSet.ImageSamplers.size(),
				shaderDescriptorSet.StorageImages.size());

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &m_DescriptorSetLayouts[set]));
		}
	}

	void VulkanShaderHazelLegacy::AllocateUniformBuffer(UniformBuffer& dst)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		UniformBuffer& uniformBuffer = dst;

		// Prepare and initialize an uniform buffer block containing shader uniforms
		// Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks

		// Vertex shader uniform buffer block
		VkBufferCreateInfo bufferInfo = {};
		VkMemoryAllocateInfo allocInfo = {};

		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = uniformBuffer.Size;
		// This buffer will be used as an uniform buffer
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VulkanAllocatorHazelLegacy allocator(std::string("UniformBuffer"));

		// Create a new buffer
		VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer.Buffer));

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, uniformBuffer.Buffer, &memoryRequirements);
		allocInfo.allocationSize = memoryRequirements.size;

		allocator.Allocate(memoryRequirements, &uniformBuffer.Memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffer.Buffer, uniformBuffer.Memory, 0));

		// Store information in the uniform's descriptor that is used by the descriptor set
		uniformBuffer.Descriptor.buffer = uniformBuffer.Buffer;
		uniformBuffer.Descriptor.offset = 0;
		uniformBuffer.Descriptor.range = uniformBuffer.Size;
	}

	//	ShaderMaterialDescriptorSet VulkanShaderHazelLegacy::AllocateDescriptorSet(uint32_t set)
	//	{
	//		return ShaderMaterialDescriptorSet();
	//	}

	VulkanShaderHazelLegacy::ShaderMaterialDescriptorSet VulkanShaderHazelLegacy::CreateDescriptorSets(uint32_t set)
	{
		ShaderMaterialDescriptorSet result;

		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		HZ_CORE_ASSERT(m_TypeCounts.find(set) != m_TypeCounts.end());

		// TODO: Move this to the centralized renderer
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = (uint32_t)m_TypeCounts.at(set).size();
		descriptorPoolInfo.pPoolSizes = m_TypeCounts.at(set).data();
		descriptorPoolInfo.maxSets = 1;

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &result.Pool));

		// Allocate a new descriptor set from the global descriptor pool
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = result.Pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayouts[set];

		result.DescriptorSets.emplace_back();
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, result.DescriptorSets.data()));
		return result;
	}

	VulkanShaderHazelLegacy::ShaderMaterialDescriptorSet VulkanShaderHazelLegacy::CreateDescriptorSets(uint32_t set, uint32_t numberOfSets)
	{
		ShaderMaterialDescriptorSet result{};

		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> poolSizes;
		for (auto&& [set, shaderDescriptorSet] : m_ShaderDescriptorSets)
		{
			if (shaderDescriptorSet.UniformBuffers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.UniformBuffers.size() * numberOfSets;
			}
			if (shaderDescriptorSet.StorageBuffers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageBuffers.size() * numberOfSets;
			}
			if (shaderDescriptorSet.ImageSamplers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				typeCount.descriptorCount = static_cast<uint32_t>(shaderDescriptorSet.ImageSamplers.size()) * numberOfSets;
			}
			if (shaderDescriptorSet.StorageImages.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageImages.size() * numberOfSets;
			}

		}

		if (poolSizes.find(set) == poolSizes.end())
		{
			// HZ_CORE_ASSERT(poolSizes.find(set) != poolSizes.end());
			Log::GetLogger()->error("VulkanShaderHazelLegacy::CreateDescriptorSets('{0}, {1}') - descriptor set not found in 'poolSizes'!", set, numberOfSets);
		}

		// TODO: Move this to the centralized renderer
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.at(set).size());
		descriptorPoolInfo.pPoolSizes = poolSizes.at(set).data();
		descriptorPoolInfo.maxSets = numberOfSets;

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &result.Pool));

		result.DescriptorSets.resize(numberOfSets);

		for (uint32_t i = 0; i < numberOfSets; i++)
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = result.Pool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &m_DescriptorSetLayouts[set];

			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result.DescriptorSets[i]));
		}
		return result;
	}

	VulkanShaderHazelLegacy::ShaderMaterialDescriptorSet VulkanShaderHazelLegacy::AllocateDescriptorSet(uint32_t set)
	{
		HZ_CORE_ASSERT(set < m_DescriptorSetLayouts.size());
		ShaderMaterialDescriptorSet result;

		if (m_ShaderDescriptorSets.empty())
			return result;

#if 0
		if (!m_DescriptorPool)
		{
			std::vector<VkDescriptorPoolSize> poolSizes;
			for (uint32_t set = 0; set < m_ShaderDescriptorSets.size(); set++)
			{
				auto& shaderDescriptorSet = m_ShaderDescriptorSets[set];
				if (!shaderDescriptorSet) // Empty descriptor set
					continue;

				if (shaderDescriptorSet.UniformBuffers.size())
				{
					VkDescriptorPoolSize& typeCount = poolSizes.emplace_back();
					typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					typeCount.descriptorCount = shaderDescriptorSet.UniformBuffers.size();
				}
				if (shaderDescriptorSet.ImageSamplers.size())
				{
					VkDescriptorPoolSize& typeCount = poolSizes.emplace_back();
					typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					typeCount.descriptorCount = shaderDescriptorSet.ImageSamplers.size();
				}
				if (shaderDescriptorSet.StorageImages.size())
				{
					VkDescriptorPoolSize& typeCount = poolSizes.emplace_back();
					typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					typeCount.descriptorCount = shaderDescriptorSet.StorageImages.size();
				}
			}

			VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.pNext = nullptr;
			descriptorPoolInfo.poolSizeCount = poolSizes.size();
			descriptorPoolInfo.pPoolSizes = poolSizes.data();
			descriptorPoolInfo.maxSets = m_ShaderDescriptorSets.size() * 1000;

			VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &m_DescriptorPool));
		}
#endif

		// TODO: remove
		result.Pool = nullptr;


		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayouts[set];
		VkDescriptorSet descriptorSet = VulkanRenderer::RT_AllocateDescriptorSet(allocInfo);
		HZ_CORE_ASSERT(descriptorSet);
		result.DescriptorSets.push_back(descriptorSet);
		return result;
	}

	const VkWriteDescriptorSet* VulkanShaderHazelLegacy::GetDescriptorSet(const std::string& name, uint32_t set) const
	{
		HZ_CORE_ASSERT(set < m_ShaderDescriptorSets.size());
		// HZ_CORE_ASSERT(m_ShaderDescriptorSets[set]);
		if (m_ShaderDescriptorSets.at(set).WriteDescriptorSets.find(name) == m_ShaderDescriptorSets.at(set).WriteDescriptorSets.end())
		{
			// HZ_CORE_WARN("Shader {0} does not contain requested descriptor set {1}", m_Name, name);
			Log::GetLogger()->warn("Shader {0} does not contain requested descriptor set {1}", m_Name, name);
			return nullptr;
		}
		return &m_ShaderDescriptorSets.at(set).WriteDescriptorSets.at(name);
	}

	// does not exist in Vulkan Week version, added later
	std::vector<VkDescriptorSetLayout> VulkanShaderHazelLegacy::GetAllDescriptorSetLayouts()
	{
		std::vector<VkDescriptorSetLayout> result;
		result.reserve(m_DescriptorSetLayouts.size());
		for (auto [set, layout] : m_DescriptorSetLayouts)
		{
			result.emplace_back(layout);
		}

		return result;
	}

	VulkanShaderHazelLegacy::UniformBuffer& VulkanShaderHazelLegacy::GetUniformBuffer(uint32_t binding, uint32_t set)
	{
		HZ_CORE_ASSERT(m_ShaderDescriptorSets.at(set).UniformBuffers.size() > binding);
		return m_ShaderDescriptorSets.at(set).UniformBuffers[binding];
	}

	static const char* VkShaderStageCachedFileExtension(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:   return ".cached_vulkan.vert";
		case VK_SHADER_STAGE_FRAGMENT_BIT: return ".cached_vulkan.frag";
		case VK_SHADER_STAGE_COMPUTE_BIT:  return ".cached_vulkan.comp";
		}
		HZ_CORE_ASSERT(false, "Invalid VkShaderStageFlagBits value!");
		Log::GetLogger()->error("Invalid VkShaderStageFlagBits value '{0}'!", stage);
		return "";
	}

	static shaderc_shader_kind VkShaderStageToShaderC(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:   return shaderc_vertex_shader;
		case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
		case VK_SHADER_STAGE_COMPUTE_BIT:  return shaderc_compute_shader;
		}
		Log::GetLogger()->error("Invalid VkShaderStageFlagBits value '{0}'!", stage);
		HZ_CORE_ASSERT(false, "Invalid VkShaderStageFlagBits value!");
		return (shaderc_shader_kind)-1;
	}

	void VulkanShaderHazelLegacy::CompileOrGetVulkanBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile)
	{
		for (auto [stage, source] : m_ShaderSource)
		{
			auto extension = VkShaderStageCachedFileExtension(stage);

			if (!forceCompile)
			{
				// Retrieve shader code from cache, if available
				std::filesystem::path p = m_AssetPath;
				auto path = p.parent_path() / "cached" / (p.filename().string() + extension);
				std::string cachedFilePath = path.string();

				FILE* f = fopen(cachedFilePath.c_str(), "rb");
				if (f)
				{
					fseek(f, 0, SEEK_END);
					uint64_t size = ftell(f);
					fseek(f, 0, SEEK_SET);
					outputBinary[stage] = std::vector<uint32_t>(size / sizeof(uint32_t));
					fread(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}

			if (outputBinary[stage].size() == 0)
			{
				// TODO: Do we need to init a compiler for each stage?
				shaderc::Compiler compiler;
				shaderc::CompileOptions options;
				options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

				const bool optimize = false;
				if (optimize)
				{
					options.SetOptimizationLevel(shaderc_optimization_level_performance);
				}

				if (m_ShaderSource.find(stage) == m_ShaderSource.end()) return;

				// Compile shader
				{
					auto& shaderSource = m_ShaderSource.at(stage); // e.g. VK_SHADER_STAGE_VERTEX_BIT
					shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderSource, VkShaderStageToShaderC(stage), m_AssetPath.c_str(), options);

					if (module.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						HZ_CORE_ERROR(module.GetErrorMessage());
						HZ_CORE_ASSERT(false);
					}

					const uint8_t* begin = (const uint8_t*)module.cbegin();
					const uint8_t* end = (const uint8_t*)module.cend();
					const ptrdiff_t size = end - begin;

					outputBinary[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
				}

				// Cache compiled shader
				{
					std::filesystem::path p = m_AssetPath;
					auto path = p.parent_path() / "cached" / (p.filename().string() + extension);
					std::string cachedFilePath = path.string();

					FILE* f = fopen(cachedFilePath.c_str(), "wb");
					fwrite(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}
		}
	}

	static VkShaderStageFlagBits ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex")                       return VK_SHADER_STAGE_VERTEX_BIT;
		if (type == "fragment" || type == "pixel")  return VK_SHADER_STAGE_FRAGMENT_BIT;
		if (type == "compute")                      return VK_SHADER_STAGE_COMPUTE_BIT;
		
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}

	std::unordered_map<VkShaderStageFlagBits, std::string> VulkanShaderHazelLegacy::PreProcess(const std::string& source)
	{
		std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			HZ_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			HZ_CORE_ASSERT(type == "vertex" || type == "fragment" || type == "pixel" || type == "compute", "Invalid shader type specified");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			auto shaderType = ShaderTypeFromString(type);
			shaderSources[shaderType] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void VulkanShaderHazelLegacy::SetUniformBuffer(const std::string& name, const void* data, uint32_t size) {}

	/****
	const std::vector<VkPipelineShaderStageCreateInfo>& VulkanShaderHazelLegacy::GetShaderStages() const
	{
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (auto [stage, pipelineShaderStageCreateInfo] : m_ShaderStages)
		{
			shaderStages.push_back(pipelineShaderStageCreateInfo);
		}
		return shaderStages;
	}
	****/

	void VulkanShaderHazelLegacy::SetUniform(const std::string& fullname, float value) {}

	void VulkanShaderHazelLegacy::SetUniform(const std::string& fullname, int value) {}

	void VulkanShaderHazelLegacy::SetUniform(const std::string& fullname, const glm::vec2& value) {}

	void VulkanShaderHazelLegacy::SetUniform(const std::string& fullname, const glm::vec3& value) {}

	void VulkanShaderHazelLegacy::SetUniform(const std::string& fullname, const glm::vec4& value) {}

	void VulkanShaderHazelLegacy::SetUniform(const std::string& fullname, const glm::mat3& value) {}

	void VulkanShaderHazelLegacy::SetUniform(const std::string& fullname, const glm::mat4& value) {}

	void VulkanShaderHazelLegacy::SetUniform(const std::string& fullname, uint32_t value) {}

	void VulkanShaderHazelLegacy::SetInt(const std::string& name, int value) {}

	void VulkanShaderHazelLegacy::SetUInt(const std::string& name, uint32_t value) {}

	void VulkanShaderHazelLegacy::SetFloat(const std::string& name, float value) {}

	void VulkanShaderHazelLegacy::SetFloat2(const std::string& name, const glm::vec2& value) {}

	void VulkanShaderHazelLegacy::SetFloat3(const std::string& name, const glm::vec3& value) {}

	void VulkanShaderHazelLegacy::SetFloat4(const std::string& name, const glm::vec4& value) {}

	void VulkanShaderHazelLegacy::SetMat4(const std::string& name, const glm::mat4& value) {}

	void VulkanShaderHazelLegacy::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind /*= true*/) {}

	void VulkanShaderHazelLegacy::SetIntArray(const std::string& name, int* values, uint32_t size) {}

	// const std::unordered_map<std::string, Hazel::ShaderBuffer>& VulkanShaderHazelLegacy::GetShaderBuffers() const { return {}; }

	const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& VulkanShaderHazelLegacy::GetResources() const
	{
		return m_Resources;
	}

	void VulkanShaderHazelLegacy::AddShaderReloadedCallback(const ShaderReloadedCallback& callback) {}

	void* VulkanShaderHazelLegacy::MapUniformBuffer(uint32_t bindingPoint, uint32_t set)
	{
		HZ_CORE_ASSERT(m_ShaderDescriptorSets.find(set) != m_ShaderDescriptorSets.end());
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		uint8_t* pData;
		VK_CHECK_RESULT(vkMapMemory(device, m_ShaderDescriptorSets.at(set).UniformBuffers.at(bindingPoint).Memory, 0, m_ShaderDescriptorSets.at(set).UniformBuffers.at(bindingPoint).Size, 0, (void**)&pData));
		return pData;
	}

	void VulkanShaderHazelLegacy::UnmapUniformBuffer(uint32_t bindingPoint, uint32_t set)
	{
		HZ_CORE_ASSERT(m_ShaderDescriptorSets.find(set) != m_ShaderDescriptorSets.end());
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkUnmapMemory(device, m_ShaderDescriptorSets.at(set).UniformBuffers.at(bindingPoint).Memory);
	}

}
