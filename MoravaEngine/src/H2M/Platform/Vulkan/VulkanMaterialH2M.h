/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/ImageH2M.h"
#include "H2M/Renderer/MaterialH2M.h"


namespace H2M
{

	class VulkanMaterialH2M : public MaterialH2M
	{
	public:
		VulkanMaterialH2M(const RefH2M<ShaderH2M>& shader, const std::string& name = "");
		VulkanMaterialH2M(RefH2M<MaterialH2M> material, const std::string& name = "");
		virtual ~VulkanMaterialH2M();

		virtual void Invalidate() override;

		virtual void Set(const std::string& name, float value) override;
		virtual void Set(const std::string& name, int value) override;
		virtual void Set(const std::string& name, uint32_t value) override;
		virtual void Set(const std::string& name, bool value) override;
		virtual void Set(const std::string& name, const glm::ivec2& value) override;
		virtual void Set(const std::string& name, const glm::ivec3& value) override;
		virtual void Set(const std::string& name, const glm::ivec4& value) override;
		virtual void Set(const std::string& name, const glm::vec2& value) override;
		virtual void Set(const std::string& name, const glm::vec3& value) override;
		virtual void Set(const std::string& name, const glm::vec4& value) override;
		virtual void Set(const std::string& name, const glm::mat3& value) override;
		virtual void Set(const std::string& name, const glm::mat4& value) override;

		virtual void Set(const std::string& name, const RefH2M<Texture2D_H2M>& texture) override;
		virtual void Set(const std::string& name, const RefH2M<Texture2D_H2M>& texture, uint32_t arrayIndex) override;
		virtual void Set(const std::string& name, const RefH2M<TextureCubeH2M>& texture) override;
		virtual void Set(const std::string& name, const RefH2M<Image2D_H2M>& image) override;

		virtual float& GetFloat(const std::string& name) override;
		virtual int32_t& GetInt(const std::string& name) override;
		virtual uint32_t& GetUInt(const std::string& name) override;
		virtual bool& GetBool(const std::string& name) override;
		virtual glm::vec2& GetVector2(const std::string& name) override;
		virtual glm::vec3& GetVector3(const std::string& name) override;
		virtual glm::vec4& GetVector4(const std::string& name) override;
		virtual glm::mat3& GetMatrix3(const std::string& name) override;
		virtual glm::mat4& GetMatrix4(const std::string& name) override;

		virtual RefH2M<Texture2D_H2M> GetTexture2D(const std::string& name) override;
		virtual RefH2M<TextureCubeH2M> GetTextureCube(const std::string& name) override;

		virtual RefH2M<Texture2D_H2M> TryGetTexture2D(const std::string& name) override;
		virtual RefH2M<TextureCubeH2M> TryGetTextureCube(const std::string& name) override;

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = FindUniformDeclaration(name);
			if (!decl)
			{
				// H2M_CORE_ASSERT(decl, "Could not find uniform!");
				Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
				return;
			}

			auto& buffer = m_UniformStorageBuffer;
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());
		}

		void Set(const std::string& name, const RefH2M<TextureH2M>& texture)
		{
			auto decl = FindResourceDeclaration(name);
			if (!decl)
			{
				// HZ_CORE_WARN("Cannot find material property: ", name);
				Log::GetLogger()->error("Cannot find material property: ", name);
				return;
			}
			uint32_t slot = decl->GetRegister();
			if (m_Textures.size() <= slot)
				m_Textures.resize((size_t)slot + 1);
			m_Textures[slot] = texture;
		}

		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = FindUniformDeclaration(name);
			if (!decl)
			{
				// H2M_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
				Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
			}
			auto& buffer = m_UniformStorageBuffer;
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		RefH2M<T> GetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			if (!decl)
			{
				// H2M_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
				Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
			}
			uint32_t slot = decl->GetRegister();
			if (slot >= m_Textures.size())
			{
				// H2M_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
				Log::GetLogger()->error("Texture slot '{0}' is invalid!", slot);
			}
			return RefH2M<T>(m_Textures[slot]);
		}

		template<typename T>
		RefH2M<T> TryGetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			if (!decl) {
				return RefH2M<T>();
			}

			uint32_t slot = decl->GetRegister();
			if (slot >= m_Textures.size()) {
				return RefH2M<T>();
			}

			return RefH2M<T>(m_Textures[slot]);
		}

		virtual uint32_t GetFlags() const override { return m_MaterialFlags; }
		virtual bool GetFlag(MaterialFlagH2M flag) const override { return (uint32_t)flag & m_MaterialFlags; }
		virtual void SetFlag(MaterialFlagH2M flag, bool value = true) override
		{
			if (value)
			{
				m_MaterialFlags |= (uint32_t)flag;
			}
			else
			{
				m_MaterialFlags &= ~(uint32_t)flag;
			}
		}

		virtual RefH2M<ShaderH2M> GetShader() override { return m_Shader; }
		virtual const std::string& GetName() const override { return m_Name; }

		BufferH2M GetUniformStorageBuffer() { return m_UniformStorageBuffer; }

		void UpdateForRendering();
		void InvalidateDescriptorSets();

		const VulkanShaderH2M::ShaderMaterialDescriptorSet& GetDescriptorSet() { return m_DescriptorSet; }

	private:
		void Init();
		void AllocateStorage();
		void OnShaderReloaded();

		void SetVulkanDescriptor(const std::string& name, const RefH2M<Texture2D_H2M>& texture);
		void SetVulkanDescriptor(const std::string& name, const RefH2M<Texture2D_H2M>& texture, uint32_t arrayIndex);
		void SetVulkanDescriptor(const std::string& name, const RefH2M<TextureCubeH2M>& texture);
		void SetVulkanDescriptor(const std::string& name, const RefH2M<Image2D_H2M>& image);
		void SetVulkanDescriptor(const std::string& name, const VkDescriptorImageInfo& imageInfo);

		const ShaderUniformH2M* FindUniformDeclaration(const std::string& name);
		const ShaderResourceDeclarationH2M* FindResourceDeclaration(const std::string& name);

	private:
		// RefH2M<ShaderH2M> m_Shader;
		// std::string m_Name;

		enum class PendingDescriptorType
		{
			None = 0, Texture2D, TextureCube, Image2D
		};

		struct PendingDescriptor
		{
			PendingDescriptorType Type = PendingDescriptorType::None;
			VkWriteDescriptorSet WDS;
			VkDescriptorImageInfo ImageInfo{};
			RefH2M<TextureH2M> Texture;
			RefH2M<ImageH2M> Image;
			VkDescriptorImageInfo SubmittedImageInfo{};
		};

		struct PendingDescriptorArray
		{
			PendingDescriptorType Type = PendingDescriptorType::None;
			VkWriteDescriptorSet WDS;
			std::vector<VkDescriptorImageInfo> ImageInfos;
			std::vector<RefH2M<TextureH2M>> Textures;
			std::vector<RefH2M<ImageH2M>> Images;
			VkDescriptorImageInfo SubmittedImageInfo{};
		};

		std::vector<std::shared_ptr<PendingDescriptor>> m_ResidentDescriptors; // TODO: should this be a map (binding point)?
		std::unordered_map<uint32_t, std::shared_ptr<PendingDescriptorArray>> m_ResidentDescriptorArrays;
		std::vector<std::shared_ptr<PendingDescriptor>> m_PendingDescriptors;  // TODO: weak ref

		uint32_t m_MaterialFlags = 0;

		BufferH2M m_UniformStorageBuffer;
		std::vector<RefH2M<TextureH2M>> m_Textures; // TODO: Texture should only be stored as images
		std::vector<std::vector<RefH2M<TextureH2M>>> m_TextureArrays;
		std::vector<RefH2M<ImageH2M>> m_Images;

		VulkanShaderH2M::ShaderMaterialDescriptorSet m_DescriptorSets[3];

		std::unordered_map<uint32_t, uint64_t> m_ImageHashes;

		VulkanShaderH2M::ShaderMaterialDescriptorSet m_DescriptorSet;

		std::vector<VkWriteDescriptorSet> m_WriteDescriptors;
		std::vector<bool> m_DirtyDescriptorSets;

		std::unordered_map<std::string, VkDescriptorImageInfo> m_ImageInfos;
	};

}
