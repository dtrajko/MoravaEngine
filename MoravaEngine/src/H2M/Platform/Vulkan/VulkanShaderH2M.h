/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/ShaderH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include "VulkanH2M.h"
// #include "VulkanMemoryAllocator/vk_mem_alloc.h"


namespace H2M
{

	class VulkanShaderH2M : public ShaderH2M
	{
	public:
		struct UniformBufferH2M
		{
			VkDeviceMemory Memory;
			VkBuffer Buffer;
			VkDescriptorBufferInfo Descriptor;
			uint32_t Size = 0;
			uint32_t BindingPoint = 0;
			uint32_t DescriptorSet = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		};

		struct StorageBufferH2M
		{
			// VmaAllocation MemoryAlloc = nullptr;
			VkDescriptorBufferInfo Descriptor;
			uint32_t Size = 0;
			uint32_t BindingPoint = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		};

		struct ImageSamplerH2M
		{
			uint32_t BindingPoint = 0;
			uint32_t DescriptorSet = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		};

		struct PushConstantRangeH2M
		{
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			uint32_t Offset = 0;
			uint32_t Size = 0;
		};

	private:
		struct ShaderDescriptorSetH2M;

	public:
		VulkanShaderH2M(const std::string& path, bool forceCompile);
		virtual ~VulkanShaderH2M();

		virtual void Bind() override;
		virtual void Reload(bool forceCompile = false) override;

		virtual size_t GetHash() const override;

		virtual const std::string& GetName() const override { return m_Name; }
		virtual const std::unordered_map<std::string, ShaderBufferH2M>& GetShaderBuffers() const override { return m_Buffers; }
		virtual const std::unordered_map<std::string, ShaderResourceDeclarationH2M>& GetResources() const override;
		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

		// Vulkan-specific
		const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() const { return m_PipelineShaderStageCreateInfos; }

		void* MapUniformBuffer(uint32_t bindingPoint, uint32_t set = 0);
		void UnmapUniformBuffer(uint32_t bindingPoint, uint32_t set = 0);

		//	UniformBuffer& GetUniformBuffer(uint32_t binding = 0, uint32_t set = 0)
		//	{
		//		HZ_CORE_ASSERT(m_ShaderDescriptorSets.at(set).UniformBuffers.size() > binding);
		//		return *m_ShaderDescriptorSets.at(set).UniformBuffers[binding];
		//	}

		VkDescriptorSet GetDescriptorSet() { return m_DescriptorSet; }
		VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t set) { return m_DescriptorSetLayouts.at(set); }
		// VkDescriptorSetLayout GetDescriptorSetLayout() { return m_DescriptorSetLayout; } // TODO: obsolete, it should be removed
		std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayouts();

		UniformBufferH2M& GetUniformBuffer(uint32_t binding = 0, uint32_t set = 0);
		uint32_t GetUniformBufferCount(uint32_t set = 0)
		{
			if (m_ShaderDescriptorSets.find(set) == m_ShaderDescriptorSets.end())
			{
				return 0;
			}
			return (uint32_t)m_ShaderDescriptorSets.at(set).UniformBuffers.size();
		}

		const std::unordered_map<uint32_t, ShaderDescriptorSetH2M>& GetShaderDescriptorSets() const { return m_ShaderDescriptorSets; }
		bool HasDescriptorSet(uint32_t set) const { return m_TypeCounts.find(set) != m_TypeCounts.end(); }

		const std::vector<PushConstantRangeH2M>& GetPushConstantRanges() const { return m_PushConstantRanges; }

		struct ShaderMaterialDescriptorSet
		{
			VkDescriptorPool Pool = nullptr;
			std::vector<VkDescriptorSet> DescriptorSets;
			// VkDescriptorSet DescriptorSet; // redundant or obsolete?
		};

		ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set = 0);
		ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set, uint32_t numberOfSets);
		const VkWriteDescriptorSet* GetDescriptorSet(const std::string& name, uint32_t set = 0) const;
		ShaderMaterialDescriptorSet AllocateDescriptorSet(uint32_t set = 0) {}; // TODO - fix syntax error

		static void ClearUniformBuffers();

		// Vulkan-specific
		virtual RendererID_H2M GetRendererID() const override;
		virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) override;

		virtual void SetUniform(const std::string& fullname, uint32_t value) override;
		virtual void SetUniform(const std::string& fullname, float value) override;
		virtual void SetUniform(const std::string& fullname, int value) override;
		virtual void SetUniform(const std::string& fullname, const glm::vec2& value) override;
		virtual void SetUniform(const std::string& fullname, const glm::vec3& value) override;
		virtual void SetUniform(const std::string& fullname, const glm::vec4& value) override;
		virtual void SetUniform(const std::string& fullname, const glm::mat3& value) override;
		virtual void SetUniform(const std::string& fullname, const glm::mat4& value) override;

		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetUInt(const std::string& name, uint32_t value) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
		virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t size) override;
		
	private:
		std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& source);
		void CompileOrGetVulkanBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile);

		void LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void ReflectAllShaderStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData); // same as Reflect()
		// void LoadAndCreateShader(VkShaderStageFlagBits shaderStage, VkPipelineShaderStageCreateInfo& pipelineShaderStageCreateInfo, const std::vector<uint32_t>& shaderData);
		void CreateDescriptors(); // same as CreateDescriptorsVulkanWeek();

		void AllocateUniformBuffer(UniformBufferH2M& dst);

	private:
		std::vector<VkPipelineShaderStageCreateInfo> m_PipelineShaderStageCreateInfos;
		// std::unordered_map<VkShaderStageFlagBits, VkPipelineShaderStageCreateInfo> m_ShaderStages;
		std::unordered_map<VkShaderStageFlagBits, std::string> m_ShaderSource;
		std::string m_AssetPath;
		std::string m_Name;

		struct ShaderDescriptorSetH2M // added in Hazel Live 19.02.2021
		{
			std::unordered_map<uint32_t, UniformBufferH2M> UniformBuffers;
			std::unordered_map<uint32_t, StorageBufferH2M> StorageBuffers;
			std::unordered_map<uint32_t, ImageSamplerH2M> ImageSamplers;
			std::unordered_map<uint32_t, ImageSamplerH2M> StorageImages;

			std::unordered_map<std::string, VkWriteDescriptorSet> WriteDescriptorSets;

			operator bool() const { return !(StorageBuffers.empty() && UniformBuffers.empty() && ImageSamplers.empty() && StorageImages.empty()); }
		};

		std::unordered_map<uint32_t, ShaderDescriptorSetH2M> m_ShaderDescriptorSets; // added in Hazel Live 19.02.2021

		std::vector<PushConstantRangeH2M> m_PushConstantRanges;

		std::unordered_map<uint32_t, VkDescriptorSetLayout> m_DescriptorSetLayouts; // added in Hazel Live 19.02.2021
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorSet m_DescriptorSet;
		VkDescriptorPool m_DescriptorPool;

		std::unordered_map<std::string, ShaderBufferH2M> m_Buffers;

		std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> m_TypeCounts;

	};

}
