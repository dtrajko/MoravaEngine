#include "VulkanShader.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Renderer/HazelRenderer.h"

#include "Core/Log.h"

#include <shaderc/shaderc.hpp>
#include <spirv_glsl.hpp>

#include <filesystem>


namespace Hazel {

	static ShaderUniformType SPIRTypeToShaderUniformType(spirv_cross::SPIRType type)
	{
		switch (type.basetype)
		{
			case spirv_cross::SPIRType::Boolean:  return ShaderUniformType::Bool;
			case spirv_cross::SPIRType::Int:      return ShaderUniformType::Int;
			case spirv_cross::SPIRType::Float:
				if (type.vecsize == 1)            return ShaderUniformType::Float;
				if (type.vecsize == 2)            return ShaderUniformType::Vec2;
				if (type.vecsize == 3)            return ShaderUniformType::Vec3;
				if (type.vecsize == 4)            return ShaderUniformType::Vec4;

				if (type.columns == 3)            return ShaderUniformType::Mat3;
				if (type.columns == 4)            return ShaderUniformType::Mat4;
				break;
		}
		HZ_CORE_ASSERT(false, "Unknown type!");
		return ShaderUniformType::None;
	}

	static std::unordered_map<uint32_t, std::unordered_map<uint32_t, VulkanShader::UniformBuffer*>> s_UniformBuffers; // set -> binding point -> buffer

	// Very temporary attribute in Vulkan Week Day 5 Part 1
	// Hazel::Ref<Hazel::HazelTexture2D> VulkanShader::s_AlbedoTexture;
	// Hazel::Ref<Hazel::HazelTexture2D> VulkanShader::s_NormalTexture;

	VulkanShader::VulkanShader(const std::string& path, bool forceCompile)
		: m_AssetPath(path)
	{
		// TODO: This should be more "general"
		size_t found = path.find_last_of("/\\");
		m_Name = found != std::string::npos ? path.substr(found + 1) : path;
		found = m_Name.find_last_of(".");
		m_Name = found != std::string::npos ? m_Name.substr(0, found) : m_Name;

		Reload();
	}

	VulkanShader::~VulkanShader() {}

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
			HZ_CORE_ASSERT(false, "Could not load shader!");
		}
		in.close();
		return result;
	}

	void VulkanShader::Reload(bool forceCompile)
	{
		// Ref<VulkanShader> instance = this;
		// HazelRenderer::Submit([instance]() mutable
		// {
		// });
		{
			// Vertex and Fragment for now
			std::string source = ReadShaderFromFile(m_AssetPath);
			m_ShaderSource = PreProcess(source);
			std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;
			CompileOrGetVulkanBinary(shaderData, forceCompile);
			LoadAndCreateShaders(shaderData);
			ReflectAllShaderStages(shaderData);
			CreateDescriptors();
		}
	}

	void VulkanShader::LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
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

	void VulkanShader::ReflectAllShaderStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		for (auto [stage, data] : shaderData)
		{
			Reflect(stage, data);
		}
	}

	void VulkanShader::Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData)
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
			uint32_t bindingPoint = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t size = static_cast<uint32_t>(compiler.get_declared_struct_size(bufferType));

			HZ_CORE_ASSERT(m_UniformBuffers.find(bindingPoint) == m_UniformBuffers.end());

			UniformBuffer& buffer = m_UniformBuffers[bindingPoint];
			// UniformBuffer buffer;
			buffer.BindingPoint = bindingPoint;
			buffer.Size = size;
			// AllocateUniformBuffer(buffer);
			buffer.Name = name;
			buffer.ShaderStage = shaderStage;
			// m_UniformBuffers.insert(std::pair(bindingPoint, buffer));

			MORAVA_CORE_TRACE("  Name: {0}", name);
			MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
			MORAVA_CORE_TRACE("  Binding Point: {0}", bindingPoint);
			MORAVA_CORE_TRACE("  Size: {0}", size);
			MORAVA_CORE_TRACE("--------------------------");
		}

		MORAVA_CORE_TRACE("Push Constant Buffers:");
		for (const auto& resource : resources.push_constant_buffers)
		{
			const auto& bufferName = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			auto bufferSize = compiler.get_declared_struct_size(bufferType);
			int memberCount = static_cast<int>(bufferType.member_types.size());
			uint32_t size = static_cast<uint32_t>(compiler.get_declared_struct_size(bufferType));

			uint32_t offset = 0;
			if (m_PushConstantRanges.size())
			{
				offset = m_PushConstantRanges.back().Offset + m_PushConstantRanges.back().Size;
			}

			auto& pushConstantRange = m_PushConstantRanges.emplace_back();
			pushConstantRange.ShaderStage = shaderStage;
			pushConstantRange.Size = static_cast<uint32_t>(bufferSize);
			pushConstantRange.Offset = offset;

			// Skip empty push constant buffers - these are for the renderer only
			if (bufferName.empty())
			{
				continue;
			}

			ShaderBuffer& buffer = m_Buffers[bufferName];
			buffer.Name = bufferName;
			buffer.Size = static_cast<uint32_t>(bufferSize);

			MORAVA_CORE_TRACE("  Name: {0}", bufferName);
			MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
			// MORAVA_CORE_TRACE("  Binding Point: {0}", bindingPoint);
			MORAVA_CORE_TRACE("  Size: {0}", size);
			MORAVA_CORE_TRACE("--------------------------");

			for (int i = 0; i < memberCount; i++)
			{
				auto type = compiler.get_type(bufferType.member_types[i]);
				const auto& memberName = compiler.get_member_name(bufferType.self, i);
				auto size = compiler.get_declared_struct_member_size(bufferType, i);
				auto offset = compiler.type_struct_member_offset(bufferType, i);

				std::string uniformName = bufferName + "." + memberName;
				buffer.Uniforms[uniformName] = ShaderUniform(uniformName, SPIRTypeToShaderUniformType(type), static_cast<uint32_t>(size), offset);
			}
		}

		MORAVA_CORE_TRACE("Sampled Images:");
		for (const auto& resource : resources.sampled_images)
		{
			const auto& name = resource.name;
			auto& type = compiler.get_type(resource.base_type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t dimension = type.image.dim;

			// HZ_CORE_ASSERT(m_ImageSamplers.find(binding) == m_ImageSamplers.end());

			auto& imageSampler = m_ImageSamplers[binding];
			// ImageSampler imageSampler;
			imageSampler.BindingPoint = binding;
			imageSampler.Name = name;
			imageSampler.ShaderStage = shaderStage;
			// m_ImageSamplers.insert(std::pair(bindingPoint, imageSampler));

			MORAVA_CORE_TRACE("  Name: {0}", name);
			// MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
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
			uint32_t dimension = type.image.dim;

			// HZ_CORE_ASSERT(m_StorageImages.find(binding) == m_StorageImages.end());

			auto& storageImage = m_StorageImages[binding];
			// ImageSampler imageSampler;
			storageImage.BindingPoint = binding;
			storageImage.Name = name;
			storageImage.ShaderStage = shaderStage;
			// m_ImageSamplers.insert(std::pair(bindingPoint, imageSampler));

			MORAVA_CORE_TRACE("  Name: {0}", name);
			// MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
			MORAVA_CORE_TRACE("  Binding Point: {0}", binding);
			// MORAVA_CORE_TRACE("  Size: {0}", size);
			MORAVA_CORE_TRACE("--------------------------");
		}

		MORAVA_CORE_TRACE("==========================");
	}

	void VulkanShader::CreateDescriptors()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		//////////////////////////////////////////////////////////////////////
		// Descriptor Pool
		//////////////////////////////////////////////////////////////////////

		// We need to tell the API the number of max. requested descriptors per type
		std::vector<VkDescriptorPoolSize> typeCounts;

		if (m_UniformBuffers.size())
		{
			VkDescriptorPoolSize& typeCount = typeCounts.emplace_back();
			typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			typeCount.descriptorCount = static_cast<uint32_t>(m_UniformBuffers.size());
		}

		if (m_ImageSamplers.size())
		{
			VkDescriptorPoolSize& typeCount = typeCounts.emplace_back();
			typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			typeCount.descriptorCount = static_cast<uint32_t>(m_ImageSamplers.size());
		}

		// TODO: Move this to the centralized renderer
		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		// Once you bind a descriptor set and use it in a vkCmdDraw() function, you can no longer modify it unless you specify the
		// descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(typeCounts.size());
		descriptorPoolInfo.pPoolSizes = typeCounts.data();
		descriptorPoolInfo.maxSets = 1;

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &m_DescriptorPool));

		//////////////////////////////////////////////////////////////////////
		// Descriptor Set Layout
		//////////////////////////////////////////////////////////////////////
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		for (auto& [binding, uniformBuffer] : m_UniformBuffers)
		{
			auto& layoutBinding = layoutBindings.emplace_back();
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = uniformBuffer.ShaderStage;
			layoutBinding.pImmutableSamplers = nullptr;
			layoutBinding.binding = binding;

			VkWriteDescriptorSet& set = m_WriteDescriptorSets[uniformBuffer.Name];
			set = {};
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.descriptorType = layoutBinding.descriptorType; // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			set.descriptorCount = 1;
			set.dstBinding = layoutBinding.binding;

			AllocateUniformBuffer(uniformBuffer);
		}

		for (auto& [binding, imageSampler] : m_ImageSamplers)
		{
			auto& layoutBinding = layoutBindings.emplace_back();
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = imageSampler.ShaderStage;
			layoutBinding.pImmutableSamplers = nullptr;
			layoutBinding.binding = binding;

			HZ_CORE_ASSERT(m_UniformBuffers.find(binding) == m_UniformBuffers.end(), "Binding is already present!");

			VkWriteDescriptorSet& set = m_WriteDescriptorSets[imageSampler.Name];
			set = {};
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.descriptorType = layoutBinding.descriptorType; // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			set.descriptorCount = 1;
			set.dstBinding = layoutBinding.binding;
		}

		VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = nullptr;
		descriptorLayout.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		descriptorLayout.pBindings = layoutBindings.data();

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &m_DescriptorSetLayout));
	}

	VkDescriptorSet VulkanShader::CreateDescriptorSet()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		VkDescriptorSet descriptorSet;

		// Allocate a new descriptor set from the global descriptor pool
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayout;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		return descriptorSet;
	}

	const VkWriteDescriptorSet* VulkanShader::GetDescriptorSet(const std::string& name, uint32_t set) const
	{
		// HZ_CORE_ASSERT(m_WriteDescriptorSets.find(name) != m_WriteDescriptorSets.end());
		if (m_WriteDescriptorSets.find(name) == m_WriteDescriptorSets.end())
		{
			// HZ_CORE_WARN("Shader {0} does not contain requested descriptor set {1}", m_Name, name);
			Log::GetLogger()->warn("Shader {0} does not contain requested descriptor set {1}", m_Name, name);
			return nullptr;
		}
		return &m_WriteDescriptorSets.at(name);

		//	HZ_CORE_ASSERT(m_ShaderDescriptorSets.find(set) != m_ShaderDescriptorSets.end());
		//	if (m_ShaderDescriptorSets.at(set).WriteDescriptorSets.find(name) == m_ShaderDescriptorSets.at(set).WriteDescriptorSets.end())
		//	{
		//		MORAVA_CORE_WARN("Shader {0} does not contain requested descriptor set {1}", m_Name, name);
		//		return nullptr;
		//	}
		//	return &m_ShaderDescriptorSets.at(set).WriteDescriptorSets.at(name);
	}

	void VulkanShader::AllocateUniformBuffer(UniformBuffer& dst)
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

		VulkanAllocator allocator(std::string("UniformBuffer"));

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

	//	ShaderMaterialDescriptorSet VulkanShader::AllocateDescriptorSet(uint32_t set)
	//	{
	//		return ShaderMaterialDescriptorSet();
	//	}

	// TODO: does not exist in Vulkan Week version, added later
	VulkanShader::ShaderMaterialDescriptorSet VulkanShader::CreateDescriptorSets(uint32_t set)
	{
		ShaderMaterialDescriptorSet result{};

		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		if (m_TypeCounts.find(set) == m_TypeCounts.end())
		{
			// HZ_CORE_ASSERT(m_TypeCounts.find(set) != m_TypeCounts.end());
			Log::GetLogger()->error("VulkanShader::CreateDescriptorSets('{0}') - descriptor set not found!", set);
			return result;
		}

		// TODO: Move this to the centralized renderer
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = m_TypeCounts.at(set).size();
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

	// TODO: does not exist in Vulkan Week version, added later
	VulkanShader::ShaderMaterialDescriptorSet VulkanShader::CreateDescriptorSets(uint32_t set, uint32_t numberOfSets)
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
				typeCount.descriptorCount = shaderDescriptorSet.UniformBuffers.size() * numberOfSets;
			}
			if (shaderDescriptorSet.ImageSamplers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				typeCount.descriptorCount = shaderDescriptorSet.ImageSamplers.size() * numberOfSets;
			}
			if (shaderDescriptorSet.StorageImages.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				typeCount.descriptorCount = shaderDescriptorSet.StorageImages.size() * numberOfSets;
			}

		}

		if (poolSizes.find(set) == poolSizes.end())
		{
			// HZ_CORE_ASSERT(poolSizes.find(set) != poolSizes.end());
			Log::GetLogger()->error("VulkanShader::CreateDescriptorSets('{0}, {1}') - descriptor set not found in 'poolSizes'!", set, numberOfSets);
		}

		// TODO: Move this to the centralized renderer
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = poolSizes.at(set).size();
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

	// does not exist in Vulkan Week version, added later
	std::vector<VkDescriptorSetLayout> VulkanShader::GetAllDescriptorSetLayouts()
	{
		std::vector<VkDescriptorSetLayout> result;
		result.reserve(m_DescriptorSetLayouts.size());
		for (auto [set, layout] : m_DescriptorSetLayouts)
			result.emplace_back(layout);

		return result;
	}

	static const char* VkShaderStageCachedFileExtension(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:   return ".cached_vulkan.vert";
		case VK_SHADER_STAGE_FRAGMENT_BIT: return ".cached_vulkan.frag";
		case VK_SHADER_STAGE_COMPUTE_BIT:  return ".cached_vulkan.comp";
		}
		Log::GetLogger()->error("Invalid VkShaderStageFlagBits value '{0}'!", stage);
		HZ_CORE_ASSERT(false, "Invalid VkShaderStageFlagBits value!");
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

	void VulkanShader::CompileOrGetVulkanBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile)
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

	std::unordered_map<VkShaderStageFlagBits, std::string> VulkanShader::PreProcess(const std::string& source)
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

	void VulkanShader::Bind() {}

	RendererID VulkanShader::GetRendererID() const { return 0; }

	void VulkanShader::ClearUniformBuffers()
	{
		s_UniformBuffers.clear();
	}

	size_t VulkanShader::GetHash() const
	{
		return std::hash<std::string>{}(m_AssetPath);
	}

	void VulkanShader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size) {}

	/****
	const std::vector<VkPipelineShaderStageCreateInfo>& VulkanShader::GetShaderStages() const
	{
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (auto [stage, pipelineShaderStageCreateInfo] : m_ShaderStages)
		{
			shaderStages.push_back(pipelineShaderStageCreateInfo);
		}
		return shaderStages;
	}
	****/

	void VulkanShader::SetUniform(const std::string& fullname, float value) {}

	void VulkanShader::SetUniform(const std::string& fullname, int value) {}

	void VulkanShader::SetUniform(const std::string& fullname, const glm::vec2& value) {}

	void VulkanShader::SetUniform(const std::string& fullname, const glm::vec3& value) {}

	void VulkanShader::SetUniform(const std::string& fullname, const glm::vec4& value) {}

	void VulkanShader::SetUniform(const std::string& fullname, const glm::mat3& value) {}

	void VulkanShader::SetUniform(const std::string& fullname, const glm::mat4& value) {}

	void VulkanShader::SetUniform(const std::string& fullname, uint32_t value) {}

	void VulkanShader::SetInt(const std::string& name, int value) {}

	void VulkanShader::SetUInt(const std::string& name, uint32_t value) {}

	void VulkanShader::SetFloat(const std::string& name, float value) {}

	void VulkanShader::SetFloat2(const std::string& name, const glm::vec2& value) {}

	void VulkanShader::SetFloat3(const std::string& name, const glm::vec3& value) {}

	void VulkanShader::SetFloat4(const std::string& name, const glm::vec4& value) {}

	void VulkanShader::SetMat4(const std::string& name, const glm::mat4& value) {}

	void VulkanShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind /*= true*/) {}

	void VulkanShader::SetIntArray(const std::string& name, int* values, uint32_t size) {}

	// const std::unordered_map<std::string, Hazel::ShaderBuffer>& VulkanShader::GetShaderBuffers() const { return {}; }

	const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& VulkanShader::GetResources() const { return {}; }

	void VulkanShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback) {}

	void* VulkanShader::MapUniformBuffer(uint32_t bindingPoint)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		uint8_t* pData;
		VK_CHECK_RESULT(vkMapMemory(device, m_UniformBuffers[bindingPoint].Memory, 0, m_UniformBuffers[bindingPoint].Size, 0, (void**)&pData));
		return pData;
	}

	void VulkanShader::UnmapUniformBuffer(uint32_t bindingPoint)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		vkUnmapMemory(device, m_UniformBuffers[bindingPoint].Memory);
	}

}
