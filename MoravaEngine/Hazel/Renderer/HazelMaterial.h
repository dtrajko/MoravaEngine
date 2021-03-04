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

	class HazelMaterialInstance;

	class HazelMaterial : public RefCounted
	{
		friend class HazelMaterialInstance;

	public:
		HazelMaterial(Ref<HazelShader> shader);
		virtual ~HazelMaterial();

		void Bind() const;

		uint32_t GetFlags() const { return m_MaterialFlags; }
		void SetFlag(HazelMaterialFlag flag) { m_MaterialFlags |= (uint32_t)flag; }

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = FindUniformDeclaration(name);
			HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());

			for (auto mi : m_MaterialInstances)
				mi->OnMaterialValueUpdated(decl);
		}

		void Set(const std::string& name, const Ref<HazelTexture>& texture)
		{
			auto decl = FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			if (m_Textures.size() <= slot)
				m_Textures.resize((size_t)slot + 1);
			m_Textures[slot] = texture;
		}

		void Set(const std::string& name, const Ref<HazelTexture2D>& texture)
		{
			Set(name, (const Ref<HazelTexture>&)texture);
		}

		void Set(const std::string& name, const Ref<HazelTextureCube>& texture)
		{
			Set(name, (const Ref<HazelTexture>&)texture);
		}

		// Setters
		void SetMaterialFlags(uint32_t materialFlags) { m_MaterialFlags = materialFlags; }

		// Getters
		uint32_t GetMaterialFlags() { return m_MaterialFlags; }
		Ref<HazelShader> GetShader() { return m_Shader; }
		std::unordered_set<HazelMaterialInstance*>* GetMaterialInstances() { return &m_MaterialInstances; }

		void BindTextures() const;

		static Ref<HazelMaterial> Create(Ref<HazelShader> shader);

	private:
		void AllocateStorage();
		void OnShaderReloaded();
		void BindTextures();

		ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name);
		ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);

	private:
		Ref<HazelShader> m_Shader;
		std::unordered_set<HazelMaterialInstance*> m_MaterialInstances;

		Buffer m_VSUniformStorageBuffer;
		Buffer m_PSUniformStorageBuffer;
		std::vector<Ref<HazelTexture>> m_Textures;

		uint32_t m_MaterialFlags;
	};

	class HazelMaterialInstance : public RefCounted
	{
		friend class HazelMaterial;

	public:
		HazelMaterialInstance(const Ref<HazelMaterial > & material, const std::string& name = "");
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

		void Bind();

		uint32_t GetFlags() const { return m_Material->GetFlags(); }
		bool GetFlag(HazelMaterialFlag flag) const { return (uint32_t)flag & m_Material->GetFlags(); }
		void SetFlag(HazelMaterialFlag flag, bool value = true);

		Ref<HazelShader> GetShader() { return m_Material->GetShader(); }

		static HazelMaterialInstance* Create(HazelMaterial* material);

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

	public:
		static Ref <HazelMaterialInstance> Create(const Ref<HazelMaterial>& material);

	private:
		void AllocateStorage();
		void OnShaderReloaded();
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);
		void OnMaterialValueUpdated(ShaderUniformDeclaration* decl);

	private:
		Ref<HazelMaterial> m_Material;
		std::string m_Name;

		Buffer m_VSUniformStorageBuffer;
		Buffer m_PSUniformStorageBuffer;
		std::vector<Ref<HazelTexture>> m_Textures;

		// TODO: This is temporary; come up with a proper system to track overrides
		std::unordered_set<std::string> m_OverriddenValues;
	};

}
