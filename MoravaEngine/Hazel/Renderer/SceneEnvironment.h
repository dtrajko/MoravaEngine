#pragma once

#include "HazelTexture.h"

namespace Hazel {

	struct Environment
	{
		std::string FilePath;
		Ref<HazelTextureCube> RadianceMap;
		Ref<HazelTextureCube> IrradianceMap;

		static Environment Load(const std::string& filepath);
	};

}
