/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "TextureH2M.h"

namespace H2M {

	struct EnvironmentH2M : public AssetH2M
	{
		std::string FilePath;
		RefH2M<TextureCubeH2M> RadianceMap;
		RefH2M<TextureCubeH2M> IrradianceMap;

		EnvironmentH2M() = default;
		EnvironmentH2M(const RefH2M<TextureCubeH2M>& radianceMap, const RefH2M<TextureCubeH2M>& irradianceMap)
			: RadianceMap(radianceMap), IrradianceMap(irradianceMap) {}
		EnvironmentH2M(std::string filePath, const RefH2M<TextureCubeH2M>& radianceMap, const RefH2M<TextureCubeH2M>& irradianceMap)
			: FilePath(filePath), RadianceMap(radianceMap), IrradianceMap(irradianceMap) {}

		static EnvironmentH2M Load(const std::string& filepath);
		static AssetTypeH2M GetStaticType() { return AssetTypeH2M::EnvMap; }
		virtual AssetTypeH2M GetAssetType() const override { return GetStaticType(); }

	};

}
