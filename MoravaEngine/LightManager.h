#pragma once

#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

#include "Core/CommonValues.h"
#include "Scene/Scene.h"


struct SceneSettings;

class LightManager
{

public:
	static void Init(SceneSettings sceneSettings);

public:
	static DirectionalLight directionalLight;
	static PointLight pointLights[MAX_POINT_LIGHTS];
	static SpotLight spotLights[MAX_SPOT_LIGHTS];

	static unsigned int pointLightCount;
	static unsigned int spotLightCount;

};
