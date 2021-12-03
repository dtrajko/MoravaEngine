#pragma once

#include <string>


namespace H2M
{

	struct RendererCapabilitiesH2M
	{
		std::string Vendor;
		std::string Device;
		std::string Version;

		int MaxSamples = 0;
		float MaxAnisotropy = 0.0f;
		int MaxTextureUnits = 0;
	};

}
