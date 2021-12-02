#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/ShaderH2M.h"

#include "Core/Log.h"

#include <unordered_set>


namespace H2M {

	enum class MaterialFlagH2M
	{
		None       = BIT(0),
		DepthTest  = BIT(1),
		Blend      = BIT(2),
		TwoSided   = BIT(3),
	};

	class MaterialH2M : public RefCountedH2M
	{
		friend class MaterialInstanceH2M; // Removed in more recent commits in Vulkan branch?

	public:
		MaterialH2M();
		MaterialH2M(const RefH2M<ShaderH2M>& shader, const std::string& name = "");
		static RefH2M<MaterialH2M> Create(const RefH2M<ShaderH2M>& shader, const std::string& name = "");
		virtual ~MaterialH2M();

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

		virtual void Set(const std::string& name, const RefH2M<Texture2D_H2M>& texture) = 0;
		virtual void Set(const std::string& name, const RefH2M<Texture2D_H2M>& texture, uint32_t arrayIndex) = 0;
		virtual void Set(const std::string& name, const RefH2M<TextureCubeH2M>& texture) = 0;
		virtual void Set(const std::string& name, const RefH2M<Image2D_H2M>& image) = 0;

		virtual float& GetFloat(const std::string& name) = 0;
		virtual int32_t& GetInt(const std::string& name) = 0;
		virtual uint32_t& GetUInt(const std::string& name) = 0;
		virtual bool& GetBool(const std::string& name) = 0;
		virtual glm::vec2& GetVector2(const std::string& name) = 0;
		virtual glm::vec3& GetVector3(const std::string& name) = 0;
		virtual glm::vec4& GetVector4(const std::string& name) = 0;
		virtual glm::mat3& GetMatrix3(const std::string& name) = 0;
		virtual glm::mat4& GetMatrix4(const std::string& name) = 0;

		virtual RefH2M<Texture2D_H2M> GetTexture2D(const std::string& name) = 0;
		virtual RefH2M<TextureCubeH2M> GetTextureCube(const std::string& name) = 0;

		virtual RefH2M<Texture2D_H2M> TryGetTexture2D(const std::string& name) = 0;
		virtual RefH2M<TextureCubeH2M> TryGetTextureCube(const std::string& name) = 0;

#if 0
		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = FindUniformDeclaration(name);
			H2M_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		RefH2M<T> GetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			H2M_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
			return m_Textures[slot];
		}
#endif

		virtual uint32_t GetFlags() const = 0;
		virtual bool GetFlag(MaterialFlagH2M flag) const = 0;
		virtual void SetFlag(MaterialFlagH2M flag, bool value = true) = 0;

		virtual RefH2M<ShaderH2M> GetShader() = 0;
		virtual const std::string& GetName() const = 0;

		BufferH2M GetUniformStorageBuffer() { return m_UniformStorageBuffer; }; // should it be located in HazelMaterial or VulkanMaterial?

		// TODO: obsolete?
		void Bind(); // Removed in more recent commits in Vulkan branch

	private:
		void AllocateStorage(); // Removed in more recent commits in Vulkan branch?
		void BindTextures(); // Removed in more recent commits in Vulkan branch?

		void OnShaderReloaded();

		ShaderUniformDeclarationH2M* FindUniformDeclaration(const std::string& name);
		ShaderResourceDeclarationH2M* FindResourceDeclaration(const std::string& name);
		BufferH2M& GetUniformBufferTarget(ShaderUniformDeclarationH2M* uniformDeclaration);

	protected:
		RefH2M<ShaderH2M> m_Shader;
		std::string m_Name;
		BufferH2M m_UniformStorageBuffer; // should it be located in MaterialH2M or VulkanMaterial?
		std::vector<RefH2M<TextureH2M>> m_Textures;
		std::vector<RefH2M<ImageH2M>> m_Images;

	private:
		// std::unordered_set<MaterialH2M*> m_MaterialInstances;
		std::unordered_set<MaterialInstanceH2M*> m_MaterialInstances;

		BufferH2M m_VSUniformStorageBuffer;
		BufferH2M m_PSUniformStorageBuffer;

		uint32_t m_MaterialFlags = 0;
	};

	class MaterialInstanceH2M : public RefCountedH2M
	{
		friend class MaterialH2M;

	public:
		MaterialInstanceH2M(const RefH2M<MaterialH2M>& material, const std::string& name = "");
		virtual ~MaterialInstanceH2M();

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			m_OverriddenValues.insert(name);
		}

		void Set(const std::string& name, TextureH2M* texture, uint32_t slot)
		{
			if (m_Textures.size() <= slot)
				m_Textures.resize((size_t)slot + 1);
			m_Textures[slot] = texture;
		}

		void Set(const std::string& name, Texture2D_H2M* texture)
		{
			Set(name, (TextureH2M*)texture);
		}

		void Set(const std::string& name, TextureCubeH2M* texture)
		{
			Set(name, (TextureH2M*)texture);
		}

		void Bind(); // Removed in more recent commits in Vulkan branch
		void AllocateStorage(); // Removed in more recent commits in Vulkan branch?

		uint32_t GetFlags() const { return m_Material->GetFlags(); }
		bool GetFlag(MaterialFlagH2M flag) const { return (uint32_t)flag & m_Material->GetFlags(); }
		void SetFlag(MaterialFlagH2M flag, bool value = true);

		RefH2M<ShaderH2M> GetShader() { return m_Material->GetShader(); }

		static MaterialInstanceH2M* Create(MaterialH2M* material);

#if 0
		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = FindUniformDeclaration(name);
			H2M_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		RefH2M<T> GetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			H2M_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
			return m_Textures[slot];
		}
#endif

	public:
		static RefH2M<MaterialInstanceH2M> Create(const RefH2M<MaterialH2M>& material);

	private:
		void OnShaderReloaded();
		BufferH2M& GetUniformBufferTarget(ShaderUniformDeclarationH2M* uniformDeclaration);
		void OnMaterialValueUpdated(ShaderUniformDeclarationH2M* decl);

	private:
		std::string m_Name;
		RefH2M<MaterialH2M> m_Material;

		std::vector<RefH2M<TextureH2M>> m_Textures;

		// Buffer m_UniformStorageBuffer; // The property should be in parent MaterialH2M
		std::vector<RefH2M<ImageH2M>> m_Images;

		BufferH2M m_VSUniformStorageBuffer;
		BufferH2M m_PSUniformStorageBuffer;

		BufferH2M m_UniformStorageBuffer; // could be obsolete in later versions of the vulkan branch

		// TODO: This is temporary; come up with a proper system to track overrides
		std::unordered_set<std::string> m_OverriddenValues;

	};

}
