/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "AssetH2M.h"

#include <filesystem>


namespace H2M
{

	struct AssetMetadataH2M
	{
		AssetHandleH2M Handle = 0;
		AssetTypeH2M Type;

		std::filesystem::path FilePath;
		bool IsDataLoaded = false;

		bool IsValid() const { return Handle != 0; }
	};
}
