#pragma once

#include "AssetH2M.h"

#include <filesystem>


namespace H2M
{

	struct AssetMetadataH2M
	{
		AssetHandle Handle = 0;
		AssetType Type;

		std::filesystem::path FilePath;
		bool IsDataLoaded = false;

		bool IsValid() const { return Handle != 0; }
	};
}
