#pragma once

#include "../Core/Base.h"
#include "../Core/Ref.h"
#include "../Renderer/HazelTexture.h"
#include "../Renderer/HazelShader.h"

#include "Core/Log.h"

#include <unordered_set>

namespace Hazel {

	enum class HazelMaterialFlag
	{
		None       = BIT(0),
		DepthTest  = BIT(1),
		Blend      = BIT(2),
		TwoSided   = BIT(3),
	};

	class HazelMaterial : public RefCounted
	{
		friend class HazelMaterialInstance; // Removed in more recent commits in Vulkan branch?

	public:
		HazelMaterial();
		HazelMaterial(const Ref<HazelShader>& shader, const std::string& name = "");
		static Ref<HazelMaterial> Create(const Ref<HazelShader>& shader, const std::string& name = "");
		virtual ~HazelMaterial() {};

		virtual void Set(const std::string& name, float value) = 0;
		virtual void Set(const std::string& name, int value) = 0;
		virtual void Set(const std::string& name, uint32_t value) = 0;
		virtual void Set(const std::string& name, bool value) = 0;
		virtual void Set(const std::string& name, const glm::vec2& value) = 0;
		virtual void Set(const std::string& name, const glm::vec3& value) = 0;
		virtual void Set(const std::string& name, const glm::vec4& value) = 0;
		virtual void Set(const std::string& name, const glm::mat3& value) = 0;
		virtual void Set(const std::string& name, const glm::mat4& value) = 0;

		virtual void Set(const std::string& name, const Ref<HazelTexture2D>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<HazelTextureCube>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<HazelImage2D>& image) = 0;

		virtual float& GetFloat(const std::string& name) = 0;
		virtual int32_t& GetInt(const std::string& name) = 0;
		virtual uint32_t& GetUInt(const std::string& name) = 0;
		virtual bool& GetBool(const std::string& name) = 0;
		virtual glm::vec2& GetVector2(const std::string& name) = 0;
		virtual glm::vec3& GetVector3(const std::string& name) = 0;
		virtual glm::vec4& GetVector4(const std::string& name) = 0;
		virtual glm::mat3& GetMatrix3(const std::string& name) = 0;
		virtual glm::mat4& GetMatrix4(const std::string& name) = 0;

		virtual Ref<HazelTexture2D> GetTexture2D(const std::string& name) = 0;
		virtual Ref<HazelTextureCube> GetTextureCube(const std::string& name) = 0;

		virtual Ref<HazelTexture2D> TryGetTexture2D(const std::string& name) = 0;
		virtual Ref<HazelTextureCube> TryGetTextureCube(const std::string& name) = 0;

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

		virtual Ref<HazelShader> GetShader() = 0;
		virtual const std::string& GetName() const = 0;

		// TODO: obsolete?
		void Bind(); // Removed in more recent commits in Vulkan branch

	private:
		void AllocateStorage(); // Removed in more recent commits in Vulkan branch?
		void BindTextures(); // Removed in more recent commits in Vulkan branch?

		void OnShaderReloaded();

		ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name);
		ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);

	private:
		Ref<HazelShader> m_Shader;
		std::unordered_set<HazelMaterial*> m_MaterialInstances;

		Buffer m_VSUniformStorageBuffer;
		Buffer m_PSUniformStorageBuffer;

		Buffer m_UniformStorageBuffer; // could be obsolete in later versions of vulkan branch

		std::vector<Ref<HazelTexture>> m_Textures;

		uint32_t m_MaterialFlags;
	};

	class HazelMaterialInstance : public RefCounted
	{
		friend class HazelMaterial;

	public:
		HazelMaterialInstance(const Ref<HazelMaterial>& material, const std::string& name = "");
		virtual ~HazelMaterialInstance();

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			m_OverriddenValues.insert(name);
		}

		void Set(const std::string& name, HazelTexture* texture, uint32_t slot)
		{
			if (m_Textures.size() <= slot)
				m_Textures.resize((size_t)slot + 1);
			m_Textures[slot] = texture;
		}

		void Set(const std::string& name, HazelTexture2D* texture)
		{
			Set(name, (HazelTexture*)texture);
		}

		void Set(const std::string& name, HazelTextureCube* texture)
		{
			Set(name, (HazelTexture*)texture);
		}

		void Bind(); // Removed in more recent commits in Vulkan branch
		void AllocateStorage(); // Removed in more recent commits in Vulkan branch?

		uint32_t GetFlags() const { return m_Material->GetFlags(); }
		bool GetFlag(HazelMaterialFlag flag) const { return (uint32_t)flag & m_Material->GetFlags(); }
		void SetFlag(HazelMaterialFlag flag, bool value = true);

		Ref<HazelShader> GetShader() { return m_Material->GetShader(); }

		static HazelMaterialInstance* Create(HazelMaterial* material);

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
		static Ref <HazelMaterialInstance> Create(const Ref<HazelMaterial>& material);

	private:
		void OnShaderReloaded();
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);
		void OnMaterialValueUpdated(ShaderUniformDeclaration* decl);

	private:
		Ref<HazelMaterial> m_Material;
		std::string m_Name;

		Buffer m_VSUniformStorageBuffer;
		Buffer m_PSUniformStorageBuffer;

		Buffer m_UniformStorageBuffer; // could be obsolete in later versions of vulkan branch

		std::vector<Ref<HazelTexture>> m_Textures;

		// TODO: This is temporary; come up with a proper system to track overrides
		std::unordered_set<std::string> m_OverriddenValues;
	};

}
