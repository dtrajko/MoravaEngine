#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Ref.h"

#include "HazelLegacy/Renderer/TextureHazelLegacy.h"
#include "HazelLegacy/Renderer/ShaderHazelLegacy.h"

#include "Core/Log.h"

#include <unordered_set>


namespace HazelLegacy {

	enum class HazelMaterialFlag
	{
		None       = BIT(0),
		DepthTest  = BIT(1),
		Blend      = BIT(2),
		TwoSided   = BIT(3),
	};

	class MaterialHazelLegacy : public Hazel::RefCounted
	{
		friend class MaterialInstanceHazelLegacy; // Removed in more recent commits in Vulkan branch?

	public:
		MaterialHazelLegacy();
		MaterialHazelLegacy(const Hazel::Ref<ShaderHazelLegacy>& shader, const std::string& name = "");
		static Hazel::Ref<MaterialHazelLegacy> Create(const Hazel::Ref<ShaderHazelLegacy>& shader, const std::string& name = "");
		virtual ~MaterialHazelLegacy();

		virtual void Invalidate() = 0;

		virtual void Set(const std::string& name, float value) = 0;
		virtual void Set(const std::string& name, int value) = 0;
		virtual void Set(const std::string& name, uint32_t value) = 0;
		virtual void Set(const std::string& name, bool value) = 0;
		virtual void Set(const std::string& name, const glm::ivec2& value) = 0;
		virtual void Set(const std::string& name, const glm::ivec3& value) = 0;
		virtual void Set(const std::string& name, const glm::ivec4& value) = 0;
		virtual void Set(const std::string& name, const glm::vec2& value) = 0;
		virtual void Set(const std::string& name, const glm::vec3& value) = 0;
		virtual void Set(const std::string& name, const glm::vec4& value) = 0;
		virtual void Set(const std::string& name, const glm::mat3& value) = 0;
		virtual void Set(const std::string& name, const glm::mat4& value) = 0;

		virtual void Set(const std::string& name, const Hazel::Ref<Texture2DHazelLegacy>& texture) = 0;
		virtual void Set(const std::string& name, const Hazel::Ref<Texture2DHazelLegacy>& texture, uint32_t arrayIndex) = 0;
		virtual void Set(const std::string& name, const Hazel::Ref<TextureCubeHazelLegacy>& texture) = 0;
		virtual void Set(const std::string& name, const Hazel::Ref<Image2DHazelLegacy>& image) = 0;

		virtual float& GetFloat(const std::string& name) = 0;
		virtual int32_t& GetInt(const std::string& name) = 0;
		virtual uint32_t& GetUInt(const std::string& name) = 0;
		virtual bool& GetBool(const std::string& name) = 0;
		virtual glm::vec2& GetVector2(const std::string& name) = 0;
		virtual glm::vec3& GetVector3(const std::string& name) = 0;
		virtual glm::vec4& GetVector4(const std::string& name) = 0;
		virtual glm::mat3& GetMatrix3(const std::string& name) = 0;
		virtual glm::mat4& GetMatrix4(const std::string& name) = 0;

		virtual Hazel::Ref<Texture2DHazelLegacy> GetTexture2D(const std::string& name) = 0;
		virtual Hazel::Ref<TextureCubeHazelLegacy> GetTextureCube(const std::string& name) = 0;

		virtual Hazel::Ref<Texture2DHazelLegacy> TryGetTexture2D(const std::string& name) = 0;
		virtual Hazel::Ref<TextureCubeHazelLegacy> TryGetTextureCube(const std::string& name) = 0;

#if 0
		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = FindUniformDeclaration(name);
			HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		Ref<T> GetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			HZ_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
			return m_Textures[slot];
		}
#endif

		virtual uint32_t GetFlags() const = 0;
		virtual bool GetFlag(HazelMaterialFlag flag) const = 0;
		virtual void SetFlag(HazelMaterialFlag flag, bool value = true) = 0;

		virtual Hazel::Ref<ShaderHazelLegacy> GetShader() = 0;
		virtual const std::string& GetName() const = 0;

		Hazel::Buffer GetUniformStorageBuffer() { return m_UniformStorageBuffer; }; // should it be located in HazelMaterial or VulkanMaterial?

		// TODO: obsolete?
		void Bind(); // Removed in more recent commits in Vulkan branch

	private:
		void AllocateStorage(); // Removed in more recent commits in Vulkan branch?
		void BindTextures(); // Removed in more recent commits in Vulkan branch?

		void OnShaderReloaded();

		Hazel::ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name);
		Hazel::ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);
		Hazel::Buffer& GetUniformBufferTarget(Hazel::ShaderUniformDeclaration* uniformDeclaration);

	protected:
		Hazel::Ref<ShaderHazelLegacy> m_Shader;
		std::string m_Name;
		Hazel::Buffer m_UniformStorageBuffer; // should it be located in MaterialHazelLegacy or VulkanMaterial?
		std::vector<Ref<TextureHazelLegacy>> m_Textures;
		std::vector<Hazel::Ref<ImageHazelLegacy>> m_Images;

	private:
		// std::unordered_set<MaterialHazelLegacy*> m_MaterialInstances;
		std::unordered_set<MaterialInstanceHazelLegacy*> m_MaterialInstances;

		Hazel::Buffer m_VSUniformStorageBuffer;
		Hazel::Buffer m_PSUniformStorageBuffer;

		uint32_t m_MaterialFlags = 0;
	};

	class MaterialInstanceHazelLegacy : public Hazel::RefCounted
	{
		friend class MaterialHazelLegacy;

	public:
		MaterialInstanceHazelLegacy(const Hazel::Ref<MaterialHazelLegacy>& material, const std::string& name = "");
		virtual ~MaterialInstanceHazelLegacy();

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			m_OverriddenValues.insert(name);
		}

		void Set(const std::string& name, TextureHazelLegacy* texture, uint32_t slot)
		{
			if (m_Textures.size() <= slot)
				m_Textures.resize((size_t)slot + 1);
			m_Textures[slot] = texture;
		}

		void Set(const std::string& name, Texture2DHazelLegacy* texture)
		{
			Set(name, (TextureHazelLegacy*)texture);
		}

		void Set(const std::string& name, TextureCubeHazelLegacy* texture)
		{
			Set(name, (TextureHazelLegacy*)texture);
		}

		void Bind(); // Removed in more recent commits in Vulkan branch
		void AllocateStorage(); // Removed in more recent commits in Vulkan branch?

		uint32_t GetFlags() const { return m_Material->GetFlags(); }
		bool GetFlag(HazelMaterialFlag flag) const { return (uint32_t)flag & m_Material->GetFlags(); }
		void SetFlag(HazelMaterialFlag flag, bool value = true);

		Hazel::Ref<ShaderHazelLegacy> GetShader() { return m_Material->GetShader(); }

		static MaterialInstanceHazelLegacy* Create(MaterialHazelLegacy* material);

#if 0
		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = FindUniformDeclaration(name);
			HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		Ref<T> GetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			HZ_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
			return m_Textures[slot];
		}
#endif

	public:
		static Hazel::Ref<MaterialInstanceHazelLegacy> Create(const Hazel::Ref<MaterialHazelLegacy>& material);

	private:
		void OnShaderReloaded();
		Hazel::Buffer& GetUniformBufferTarget(Hazel::ShaderUniformDeclaration* uniformDeclaration);
		void OnMaterialValueUpdated(Hazel::ShaderUniformDeclaration* decl);

	private:
		std::string m_Name;
		Hazel::Ref<MaterialHazelLegacy> m_Material;

		std::vector<Hazel::Ref<TextureHazelLegacy>> m_Textures;

		// Buffer m_UniformStorageBuffer; // The property should be in parent MaterialHazelLegacy
		std::vector<Hazel::Ref<ImageHazelLegacy>> m_Images;

		Hazel::Buffer m_VSUniformStorageBuffer;
		Hazel::Buffer m_PSUniformStorageBuffer;

		Hazel::Buffer m_UniformStorageBuffer; // could be obsolete in later versions of the vulkan branch

		// TODO: This is temporary; come up with a proper system to track overrides
		std::unordered_set<std::string> m_OverriddenValues;

	};

}
