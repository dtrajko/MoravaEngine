#include "HazelMaterial.h"

namespace Hazel {

	//////////////////////////////////////////////////////////////////////////////////
	// Material
	//////////////////////////////////////////////////////////////////////////////////

	HazelMaterial* HazelMaterial::Create(Shader* shader)
	{
		return new HazelMaterial(shader);
	}

	HazelMaterial::HazelMaterial(Shader* shader)
		: m_Shader(shader)
	{
		m_MaterialFlags |= (uint32_t)HazelMaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)HazelMaterialFlag::Blend;
	}

	HazelMaterial::~HazelMaterial()
	{
	}

	void HazelMaterial::Bind() const
	{
		m_Shader->Bind();
		// ...
		BindTextures();
	}

	void HazelMaterial::BindTextures() const
	{
		for (uint32_t i = 0; i < (uint32_t)m_Textures.size(); i++)
		{
			auto& texture = m_Textures[i];
			if (texture)
				texture->Bind(i);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// HazelMaterialInstance
	//////////////////////////////////////////////////////////////////////////////////

	HazelMaterialInstance* HazelMaterialInstance::Create(HazelMaterial* material)
	{
		return new HazelMaterialInstance(material);
	}

	HazelMaterialInstance::HazelMaterialInstance(HazelMaterial* material)
		: m_Material(material)
	{
		m_Material->m_MaterialInstances.insert(this);
	}

	HazelMaterialInstance::~HazelMaterialInstance()
	{
		m_Material->m_MaterialInstances.erase(this);
	}

	void HazelMaterialInstance::OnShaderReloaded()
	{
		m_OverriddenValues.clear();
	}

	void HazelMaterialInstance::SetFlag(HazelMaterialFlag flag, bool value)
	{
		if (value)
		{
			m_Material->m_MaterialFlags |= (uint32_t)flag;
		}
		else
		{
			m_Material->m_MaterialFlags &= ~(uint32_t)flag;
		}
	}

	void HazelMaterialInstance::Bind() const
	{
		m_Material->m_Shader->Bind();

		m_Material->BindTextures();
		for (uint32_t i = 0; i < (uint32_t)m_Textures.size(); i++)
		{
			auto& texture = m_Textures[i];
			if (texture)
				texture->Bind(i);
		}
	}

}
