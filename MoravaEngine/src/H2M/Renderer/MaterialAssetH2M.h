#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Asset/AssetH2M.h"
#include "H2M/Renderer/MaterialH2M.h"


namespace H2M
{

	class MaterialAssetH2M : public AssetH2M
	{
	public:
		MaterialAssetH2M();
		MaterialAssetH2M(RefH2M<MaterialH2M> material);
		~MaterialAssetH2M();

		glm::vec3& GetAlbedoColor();
		void SetAlbedoColor(const glm::vec3& color);

		float& GetMetalness();
		void SetMetalness(float value);

		float& GetRoughness();
		void SetRoughness(float value);

		float& GetEmission();
		void SetEmission(float value);

		// Textures
		RefH2M<Texture2D_H2M> GetAlbedoMap();
		void SetAlbedoMap(RefH2M<Texture2D_H2M> texture);
		void ClearAlbedoMap();

		RefH2M<Texture2D_H2M> GetNormalMap();
		void SetNormalMap(RefH2M<Texture2D_H2M> texture);
		bool IsUsingNormalMap();
		void SetUseNormalMap(bool value);
		void ClearNormalMap();

		RefH2M<Texture2D_H2M> GetMetalnessMap();
		void SetMetalnessMap(RefH2M<Texture2D_H2M> texture);
		void ClearMetalnessMap();

		RefH2M<Texture2D_H2M> GetRoughnessMap();
		void SetRoughnessMap(RefH2M<Texture2D_H2M> texture);
		void ClearRoughnessMap();

		static AssetTypeH2M GetStaticType() { return AssetTypeH2M::Material; }
		virtual AssetTypeH2M GetAssetType() const override { return GetStaticType(); }

		RefH2M<MaterialH2M> GetMaterial() const { return m_Material; }
	private:
		RefH2M<MaterialH2M> m_Material;
	};

	class MaterialTableH2M : public RefCountedH2M
	{
	public:
		MaterialTableH2M(uint32_t materialCount = 1);
		MaterialTableH2M(RefH2M<MaterialTableH2M> other);
		~MaterialTableH2M() = default;

		bool HasMaterial(uint32_t materialIndex) const { return m_Materials.find(materialIndex) != m_Materials.end(); }
		void SetMaterial(uint32_t index, RefH2M<MaterialAssetH2M> material);
		void ClearMaterial(uint32_t index);

		RefH2M<MaterialAssetH2M> GetMaterial(uint32_t materialIndex) const
		{
			H2M_CORE_ASSERT(HasMaterial(materialIndex));
			return m_Materials.at(materialIndex);
		}
		std::map<uint32_t, RefH2M<MaterialAssetH2M>>& GetMaterials() { return m_Materials; }
		const std::map<uint32_t, RefH2M<MaterialAssetH2M>>& GetMaterials() const { return m_Materials; }

		uint32_t GetMaterialCount() const { return m_MaterialCount; }
		void SetMaterialCount(uint32_t materialCount) { m_MaterialCount = materialCount; }

		void Clear();

	private:
		std::map<uint32_t, RefH2M<MaterialAssetH2M>> m_Materials;
		uint32_t m_MaterialCount;
	};

}
