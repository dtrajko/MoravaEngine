#pragma once

#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Renderer/HazelImage.h"
#include "Hazel/Renderer/HazelMaterial.h"


namespace Hazel {

	class VulkanMaterial : public HazelMaterial
	{
	public:
		VulkanMaterial(const Ref<HazelShader>& shader, const std::string& name = "");
		virtual ~VulkanMaterial();

		virtual void Invalidate() override;

		virtual void Set(const std::string& name, float value) override;
		virtual void Set(const std::string& name, int value) override;
		virtual void Set(const std::string& name, uint32_t value) override;
		virtual void Set(const std::string& name, bool value) override;
		virtual void Set(const std::string& name, const glm::vec2& value) override;
		virtual void Set(const std::string& name, const glm::vec3& value) override;
		virtual void Set(const std::string& name, const glm::vec4& value) override;
		virtual void Set(const std::string& name, const glm::mat3& value) override;
		virtual void Set(const std::string& name, const glm::mat4& value) override;

		virtual void Set(const std::string& name, const Ref<HazelTexture2D>& texture) override;
		virtual void Set(const std::string& name, const Ref<HazelTextureCube>& texture) override;
		virtual void Set(const std::string& name, const Ref<HazelImage2D>& image) override;

		virtual float& GetFloat(const std::string& name) override;
		virtual int32_t& GetInt(const std::string& name) override;
		virtual uint32_t& GetUInt(const std::string& name) override;
		virtual bool& GetBool(const std::string& name) override;
		virtual glm::vec2& GetVector2(const std::string& name) override;
		virtual glm::vec3& GetVector3(const std::string& name) override;
		virtual glm::vec4& GetVector4(const std::string& name) override;
		virtual glm::mat3& GetMatrix3(const std::string& name) override;
		virtual glm::mat4& GetMatrix4(const std::string& name) override;

		virtual Ref<HazelTexture2D> GetTexture2D(const std::string& name) override;
		virtual Ref<HazelTextureCube> GetTextureCube(const std::string& name) override;

		virtual Ref<HazelTexture2D> TryGetTexture2D(const std::string& name) override;
		virtual Ref<HazelTextureCube> TryGetTextureCube(const std::string& name) override;

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = FindUniformDeclaration(name);
			if (!decl)
			{
				// HZ_CORE_ASSERT(decl, "Could not find uniform!");
				Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
				return;
			}

			auto& buffer = m_UniformStorageBuffer;
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());
		}

		void Set(const std::string& name, const Ref<HazelTexture>& texture)
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
				// HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
				Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
			}
			auto& buffer = m_UniformStorageBuffer;
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		Ref<T> GetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			if (!decl)
			{
				// HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
				Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
			}
			uint32_t slot = decl->GetRegister();
			if (slot >= m_Textures.size())
			{
				// HZ_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
				Log::GetLogger()->error("Texture slot '{0}' is invalid!", slot);
			}
			return Ref<T>(m_Textures[slot]);
		}

		template<typename T>
		Ref<T> TryGetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			if (!decl) {
				return Ref<T>();
			}

			uint32_t slot = decl->GetRegister();
			if (slot >= m_Textures.size()) {
				return Ref<T>();
			}

			return Ref<T>(m_Textures[slot]);
		}

		virtual uint32_t GetFlags() const override { return m_MaterialFlags; }
		virtual bool GetFlag(HazelMaterialFlag flag) const override { return (uint32_t)flag & m_MaterialFlags; }
		virtual void SetFlag(HazelMaterialFlag flag, bool value = true) override
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

		virtual Ref<HazelShader> GetShader() override { return m_Shader; }
		virtual const std::string& GetName() const override { return m_Name; }

		// Buffer GetUniformStorageBuffer() { return m_UniformStorageBuffer; } // The method should be in parent HazelMaterial

		void UpdateForRendering();

		const VulkanShader::ShaderMaterialDescriptorSet& GetDescriptorSet() { return m_DescriptorSet; }

	private:
		void Init();
		void AllocateStorage();
		void OnShaderReloaded();

		void SetVulkanDescriptor(const std::string& name, const Ref<HazelTexture2D>& texture);
		void SetVulkanDescriptor(const std::string& name, const Ref<HazelTextureCube>& texture);
		void SetVulkanDescriptor(const std::string& name, const Ref<HazelImage2D>& image);
		void SetVulkanDescriptor(const std::string& name, const VkDescriptorImageInfo& imageInfo);

		const ShaderUniform* FindUniformDeclaration(const std::string& name);
		const ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);

	private:
		enum class PendingDescriptorType
		{
			None = 0, Texture2D, TextureCube, Image2D
		};

		struct PendingDescriptor
		{
			PendingDescriptorType Type = PendingDescriptorType::None;
			VkWriteDescriptorSet WDS;
			VkDescriptorImageInfo ImageInfo{};
			Ref<HazelTexture> Texture;
			Ref<HazelImage> Image;
			VkDescriptorImageInfo SubmittedImageInfo{};
		};

		std::vector<std::shared_ptr<PendingDescriptor>> m_ResidentDescriptors; // TODO: should this be a map (binding point)?
		std::vector<std::shared_ptr<PendingDescriptor>> m_PendingDescriptors;  // TODO: weak ref

		uint32_t m_MaterialFlags = 0;

		std::unordered_map<uint32_t, uint64_t> m_ImageHashes;

		VulkanShader::ShaderMaterialDescriptorSet m_DescriptorSet;
		std::vector<VkWriteDescriptorSet> m_WriteDescriptors;

		std::unordered_map<std::string, VkDescriptorImageInfo> m_ImageInfos;
	};

}
