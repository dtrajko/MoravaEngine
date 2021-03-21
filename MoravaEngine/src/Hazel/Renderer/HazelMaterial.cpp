#include "HazelMaterial.h"

#include "../Core/Assert.h"


namespace Hazel {

	//////////////////////////////////////////////////////////////////////////////////
	// Material
	//////////////////////////////////////////////////////////////////////////////////

	Ref<HazelMaterial> HazelMaterial::Create(const Ref<HazelShader>& shader)
	{
		return Ref<HazelMaterial>::Create(shader);
	}

	void HazelMaterial::AllocateStorage()
	{
		if (m_Shader->HasVSMaterialUniformBuffer())
		{
			const auto& vsBuffer = m_Shader->GetVSMaterialUniformBuffer();
			m_VSUniformStorageBuffer.Allocate(vsBuffer.GetSize());
			m_VSUniformStorageBuffer.ZeroInitialize();
		}

		if (m_Shader->HasPSMaterialUniformBuffer())
		{
			const auto& psBuffer = m_Shader->GetPSMaterialUniformBuffer();
			m_PSUniformStorageBuffer.Allocate(psBuffer.GetSize());
			m_PSUniformStorageBuffer.ZeroInitialize();
		}
	}

	void HazelMaterial::OnShaderReloaded()
	{
		return;
		AllocateStorage();

		for (auto mi : m_MaterialInstances)
			mi->OnShaderReloaded();
	}

	ShaderUniformDeclaration* HazelMaterial::FindUniformDeclaration(const std::string& name)
	{
		return nullptr;
	}

	void HazelMaterial::BindTextures()
	{
	}

	ShaderResourceDeclaration* HazelMaterial::FindResourceDeclaration(const std::string& name)
	{
		// auto& resources = m_Shader->GetResources();


		return nullptr;
	}

	HazelMaterial::HazelMaterial(const Ref<HazelShader>& shader)
		: m_Shader(shader)
	{
		// m_Shader->AddShaderReloadedCallback(std::bind(&HazelMaterial::OnShaderReloaded, this));
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)HazelMaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)HazelMaterialFlag::Blend;
	}

	HazelMaterial::~HazelMaterial()
	{
	}

	void HazelMaterial::Bind() const
	{
		// m_Shader->Bind();
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

	HazelMaterialInstance::HazelMaterialInstance(const Ref<HazelMaterial>& material, const std::string& name)
		: m_Material(material), m_Name(name)
	{
		m_Material->m_MaterialInstances.insert(this);
		AllocateStorage();
	}

	HazelMaterialInstance::~HazelMaterialInstance()
	{
		m_Material->GetMaterialInstances()->erase(this);
	}

	Ref<HazelMaterialInstance> HazelMaterialInstance::Create(const Ref<HazelMaterial>& material)
	{
		return Ref<HazelMaterialInstance>::Create(material);
	}

	void HazelMaterialInstance::AllocateStorage()
	{
	}

	void HazelMaterialInstance::OnShaderReloaded()
	{
		m_OverriddenValues.clear();
	}

	Buffer& HazelMaterialInstance::GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration)
	{
		switch (uniformDeclaration->GetDomain())
		{
		case ShaderDomain::Vertex:    return m_VSUniformStorageBuffer;
		case ShaderDomain::Pixel:     return m_PSUniformStorageBuffer;
		}

		HZ_CORE_ASSERT(false, "Invalid uniform declaration domain! Material does not support this shader type.");
		return m_VSUniformStorageBuffer;
	}

	void HazelMaterialInstance::OnMaterialValueUpdated(ShaderUniformDeclaration* decl)
	{
		if (m_OverriddenValues.find(decl->GetName()) == m_OverriddenValues.end())
		{
			auto& buffer = GetUniformBufferTarget(decl);
			auto& materialBuffer = m_Material->GetUniformBufferTarget(decl);
			buffer.Write(materialBuffer.Data + decl->GetOffset(), decl->GetSize(), decl->GetOffset());
		}
	}

	void HazelMaterialInstance::Bind()
	{
		m_Material->GetShader()->Bind();

		if (m_VSUniformStorageBuffer)
			// m_Material->m_Shader->SetVSMaterialUniformBuffer(m_VSUniformStorageBuffer);

		if (m_PSUniformStorageBuffer)
			// m_Material->m_Shader->SetPSMaterialUniformBuffer(m_PSUniformStorageBuffer);

		m_Material->BindTextures();
		for (size_t i = 0; i < m_Textures.size(); i++)
		{
			auto& texture = m_Textures[i];
			if (texture)
				texture->Bind((uint32_t)i);
		}
	}

	Buffer& HazelMaterial::GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration)
	{
		switch (uniformDeclaration->GetDomain())
		{
		case ShaderDomain::Vertex:    return m_VSUniformStorageBuffer;
		case ShaderDomain::Pixel:     return m_PSUniformStorageBuffer;
		}

		HZ_CORE_ASSERT(false, "Invalid uniform declaration domain! Material does not support this shader type.");
		return m_VSUniformStorageBuffer;
	}

	void HazelMaterialInstance::SetFlag(HazelMaterialFlag flag, bool value)
	{
		if (value)
		{
			uint32_t materialFlags = m_Material->GetMaterialFlags();
			m_Material->SetMaterialFlags(materialFlags |= (uint32_t)flag);
		}
		else
		{
			uint32_t materialFlags = m_Material->GetMaterialFlags();
			m_Material->SetMaterialFlags(materialFlags &= ~(uint32_t)flag);
		}
	}

}
