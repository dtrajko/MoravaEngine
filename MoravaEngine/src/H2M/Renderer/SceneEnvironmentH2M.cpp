/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "SceneEnvironmentH2M.h"

#include "SceneRendererH2M.h"

namespace H2M
{

	EnvironmentH2M EnvironmentH2M::Load(const std::string& filepath)
	{
		// FilePath = filepath;
		auto [radiance, irradiance] = SceneRendererH2M::CreateEnvironmentMap(filepath);
		// return { filepath, radiance, irradiance };
		return { filepath, radiance, irradiance };
	}
}
