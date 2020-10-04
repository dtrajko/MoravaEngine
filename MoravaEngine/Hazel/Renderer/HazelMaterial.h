#pragma once

#include "../Core/Base.h"
#include "../Renderer/HazelTexture.h"
#include "../../Shader.h"
#include "../../Log.h"

#include <unordered_set>

namespace Hazel {

	enum class HazelMaterialFlag
	{
		None       = BIT(0),
		DepthTest  = BIT(1),
		Blend      = BIT(2)
	};

	class HazelMaterial
	{
		friend class HazelMaterialInstance;

	public:
		HazelMaterial(Shader* shader);
		virtual ~HazelMaterial();

		void Bind() const;

		uint32_t GetFlags() const { return m_MaterialFlags; }
		void SetFlag(HazelMaterialFlag flag) { m_MaterialFlags |= (uint32_t)flag; }

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			Log::GetLogger()->error("Method not implemented: Set(name {0}, value {1})", name, value);
		}

		void Set(const std::string& name, HazelTexture* texture, uint32_t slot)
		{
			if (m_Textures.size() <= slot)
				m_Textures.resize((size_t)slot + 1);
			m_Textures[slot] = texture;
		}

		void Set(const std::string& name, HazelTexture2D* texture, uint32_t slot)
		{
			Set(name, (HazelTexture*)texture, slot);
		}

		void Set(const std::string& name, HazelTextureCube* texture, uint32_t slot)
		{
			Set(name, (HazelTexture*)texture, slot);
		}

	public:
		static HazelMaterial* Create(Shader* shader);

	private:
		void BindTextures() const;

	private:
		Shader* m_Shader;
		std::unordered_set<HazelMaterialInstance*> m_MaterialInstances;

		Buffer m_VSUniformStorageBuffer;
		Buffer m_PSUniformStorageBuffer;
		std::vector<HazelTexture*> m_Textures;

		uint32_t m_MaterialFlags;
	};

	class HazelMaterialInstance
	{
		friend class HazelMaterial;

	public:
		HazelMaterialInstance(HazelMaterial* material);
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

		void Bind() const;

		uint32_t GetFlags() const { return m_Material->m_MaterialFlags; }
		bool GetFlag(HazelMaterialFlag flag) const { return (uint32_t)flag & m_Material->m_MaterialFlags; }
		void SetFlag(HazelMaterialFlag flag, bool value = true);

		Shader* GetShader() { return m_Material->m_Shader; }

		static HazelMaterialInstance* Create(HazelMaterial* material);

	private:
		void OnShaderReloaded();

	private:
		HazelMaterial* m_Material;

		Buffer m_VSUniformStorageBuffer;
		Buffer m_PSUniformStorageBuffer;
		std::vector<HazelTexture*> m_Textures;

		// TODO: This is temporary; come up with a proper system to track overrides
		std::unordered_set<std::string> m_OverriddenValues;
	};

}
