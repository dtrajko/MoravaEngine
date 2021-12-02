#include "MaterialAsset.h"

#include "Hazel/Renderer/HazelRenderer.h"


namespace Hazel
{

	static const std::string s_AlbedoColorUniform = "u_MaterialUniforms.AlbedoColor";
	static const std::string s_UseNormalMapUniform = "u_MaterialUniforms.UseNormalMap";
	static const std::string s_MetalnessUniform = "u_MaterialUniforms.Metalness";
	static const std::string s_RoughnessUniform = "u_MaterialUniforms.Roughness";
	static const std::string s_EmissionUniform = "u_MaterialUniforms.Emission";

	static const std::string s_AlbedoMapUniform = "u_AlbedoTexture";
	static const std::string s_NormalMapUniform = "u_NormalTexture";
	static const std::string s_MetalnessMapUniform = "u_MetalnessTexture";
	static const std::string s_RoughnessMapUniform = "u_RoughnessTexture";

	MaterialAsset::MaterialAsset()
	{
		m_Material = HazelMaterial::Create(HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static"));

		// Set defaults
		SetAlbedoColor(glm::vec3(0.8f));
		SetEmission(0.0f);
		SetUseNormalMap(false);
		SetMetalness(0.0f);
		SetRoughness(0.4f);

		// Maps
		SetAlbedoMap(HazelRenderer::GetWhiteTexture());
		SetNormalMap(HazelRenderer::GetWhiteTexture());
		SetMetalnessMap(HazelRenderer::GetWhiteTexture());
		SetRoughnessMap(HazelRenderer::GetWhiteTexture());
	}

	MaterialAsset::MaterialAsset(Ref<HazelMaterial> material)
	{
		// m_Material = HazelMaterial::Copy(material);
	}

	MaterialAsset::~MaterialAsset()
	{
	}

	glm::vec3& MaterialAsset::GetAlbedoColor()
	{
		// TODO: insert return statement here
		return m_Material->GetVector3(s_AlbedoColorUniform);
	}

	void MaterialAsset::SetAlbedoColor(const glm::vec3& color)
	{
		m_Material->Set(s_AlbedoColorUniform, color);
	}

	float& MaterialAsset::GetMetalness()
	{
		return m_Material->GetFloat(s_MetalnessUniform);
	}

	void MaterialAsset::SetMetalness(float value)
	{
		m_Material->Set(s_MetalnessUniform, value);
	}

	float& MaterialAsset::GetRoughness()
	{
		return m_Material->GetFloat(s_RoughnessUniform);
	}

	void MaterialAsset::SetRoughness(float value)
	{
		m_Material->Set(s_RoughnessUniform, value);
	}

	float& MaterialAsset::GetEmission()
	{
		return m_Material->GetFloat(s_EmissionUniform);
	}

	void MaterialAsset::SetEmission(float value)
	{
		m_Material->Set(s_EmissionUniform, value);
	}

	Ref<HazelTexture2D> MaterialAsset::GetAlbedoMap()
	{
		return m_Material->TryGetTexture2D(s_AlbedoMapUniform);
	}

	void MaterialAsset::SetAlbedoMap(Ref<HazelTexture2D> texture)
	{
		m_Material->Set(s_AlbedoMapUniform, texture);
	}

	void MaterialAsset::ClearAlbedoMap()
	{
		m_Material->Set(s_AlbedoMapUniform, HazelRenderer::GetWhiteTexture());
	}

	Ref<HazelTexture2D> MaterialAsset::GetNormalMap()
	{
		return m_Material->TryGetTexture2D(s_NormalMapUniform);
	}

	void MaterialAsset::SetNormalMap(Ref<HazelTexture2D> texture)
	{
		m_Material->Set(s_NormalMapUniform, texture);
	}

	bool MaterialAsset::IsUsingNormalMap()
	{
		return m_Material->GetBool(s_UseNormalMapUniform);
	}

	void MaterialAsset::SetUseNormalMap(bool value)
	{
		m_Material->Set(s_UseNormalMapUniform, value);
	}

	void MaterialAsset::ClearNormalMap()
	{
		m_Material->Set(s_NormalMapUniform, HazelRenderer::GetWhiteTexture());
	}

	Ref<HazelTexture2D> MaterialAsset::GetMetalnessMap()
	{
		return m_Material->TryGetTexture2D(s_MetalnessMapUniform);
	}

	void MaterialAsset::SetMetalnessMap(Ref<HazelTexture2D> texture)
	{
		m_Material->Set(s_MetalnessMapUniform, texture);
	}

	void MaterialAsset::ClearMetalnessMap()
	{
		m_Material->Set(s_MetalnessMapUniform, HazelRenderer::GetWhiteTexture());
	}

	Ref<HazelTexture2D> MaterialAsset::GetRoughnessMap()
	{
		return m_Material->TryGetTexture2D(s_RoughnessMapUniform);
	}

	void MaterialAsset::SetRoughnessMap(Ref<HazelTexture2D> texture)
	{
		m_Material->Set(s_RoughnessMapUniform, texture);
	}

	void MaterialAsset::ClearRoughnessMap()
	{
		m_Material->Set(s_RoughnessMapUniform, HazelRenderer::GetWhiteTexture());
	}

	MaterialTable::MaterialTable(uint32_t materialCount)
		: m_MaterialCount(materialCount)
	{
	}

	MaterialTable::MaterialTable(Ref<MaterialTable> other)
		: m_MaterialCount(other->m_MaterialCount)
	{
		const auto& meshMaterials = other->GetMaterials();
		for (auto [index, materialAsset] : meshMaterials)
			//SetMaterial(index, Ref<MaterialAsset>::Create(materialAsset->GetMaterial()));
			SetMaterial(index, materialAsset);
	}

	void MaterialTable::SetMaterial(uint32_t index, Ref<MaterialAsset> material)
	{
		m_Materials[index] = material;
		if (index >= m_MaterialCount)
			m_MaterialCount = index + 1;
	}

	void MaterialTable::ClearMaterial(uint32_t index)
	{
		H2M_CORE_ASSERT(HasMaterial(index));
		m_Materials.erase(index);
		if (index >= m_MaterialCount)
			m_MaterialCount = index + 1;
	}

	void MaterialTable::Clear()
	{
		m_Materials.clear();
	}

}
