/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/AssertH2M.h"

#include <string>


namespace H2M {

	enum class AssetFlagH2M : uint16_t
	{
		None = 0,
		Missing = BIT(0),
		Invalid = BIT(1)
	};

	enum class AssetTypeH2M : uint16_t
	{
		None = 0,
		Scene,
		Mesh,
		StaticMesh,
		MeshSource,
		Material,
		Texture,
		EnvMap,
	};

	namespace Utils {

		inline AssetTypeH2M AssetTypeFromString(const std::string& assetType)
		{
			if (assetType == "None")       return AssetTypeH2M::None;
			if (assetType == "Scene")      return AssetTypeH2M::Scene;
			if (assetType == "Mesh")       return AssetTypeH2M::Mesh;
			if (assetType == "StaticMesh") return AssetTypeH2M::StaticMesh;
			if (assetType == "MeshAsset")  return AssetTypeH2M::MeshSource; // Deprecated
			if (assetType == "MeshSource") return AssetTypeH2M::MeshSource;
			if (assetType == "Material")   return AssetTypeH2M::Material;
			if (assetType == "Texture")    return AssetTypeH2M::Texture;
			if (assetType == "EnvMap")     return AssetTypeH2M::EnvMap;

			H2M_CORE_ASSERT(false, "Unknown Asset Type");
			return AssetTypeH2M::None;
		}

		inline const char* AssetTypeToString(AssetTypeH2M assetType)
		{
			switch (assetType)
			{
				case AssetTypeH2M::None:        return "None";
				case AssetTypeH2M::Scene:       return "Scene";
				case AssetTypeH2M::Mesh:        return "Mesh";
				case AssetTypeH2M::Material:    return "Material";
				case AssetTypeH2M::Texture:     return "Texture";
				case AssetTypeH2M::EnvMap:     return "EnvMap";
			}
		
			H2M_CORE_ASSERT(false, "Unknown Asset Type");
			return "None";
		}

	}
}
