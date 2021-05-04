#include "SceneEnvironment.h"

#include "SceneRenderer.h"

namespace Hazel {

	Environment Environment::Load(const std::string& filepath)
	{
		auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(filepath);
		return { radiance, irradiance };
	}
}
