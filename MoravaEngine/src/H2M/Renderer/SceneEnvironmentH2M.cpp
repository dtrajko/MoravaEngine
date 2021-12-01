#include "SceneEnvironment.h"

#include "SceneRenderer.h"

namespace Hazel {

	Environment Environment::Load(const std::string& filepath)
	{
		// FilePath = filepath;
		auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(filepath);
		// return { filepath, radiance, irradiance };
		return { filepath, radiance, irradiance };
	}
}
