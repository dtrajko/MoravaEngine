#pragma once

#include "HazelTexture.h"

namespace Hazel {

	struct Environment : public Asset
	{
		std::string FilePath;
		Ref<HazelTextureCube> RadianceMap;
		Ref<HazelTextureCube> IrradianceMap;

		Environment() = default;
		Environment(const Ref<HazelTextureCube>& radianceMap, const Ref<HazelTextureCube>& irradianceMap)
			: RadianceMap(radianceMap), IrradianceMap(irradianceMap) {}
		Environment(const std::string& filepath, const Ref<HazelTextureCube>&radianceMap, const Ref<HazelTextureCube>&irradianceMap)
			: FilePath(filepath), RadianceMap(radianceMap), IrradianceMap(irradianceMap) {}

		static Environment Load(const std::string& filepath);

	};

}
