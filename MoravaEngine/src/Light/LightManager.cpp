#include "Light/LightManager.h"


DirectionalLight LightManager::directionalLight;
PointLight LightManager::pointLights[MAX_POINT_LIGHTS];
SpotLight LightManager::spotLights[MAX_SPOT_LIGHTS];

unsigned int LightManager::pointLightCount = 0;
unsigned int LightManager::spotLightCount = 0;


void LightManager::Init(SceneSettings sceneSettings)
{
	LightManager::directionalLight = DirectionalLight(sceneSettings.shadowMapWidth, sceneSettings.shadowMapHeight,
		sceneSettings.directionalLight.base.enabled, sceneSettings.directionalLight.base.color,
		sceneSettings.directionalLight.base.ambientIntensity, sceneSettings.directionalLight.base.diffuseIntensity,
		sceneSettings.directionalLight.direction, true);
	LightManager::directionalLight.SetLightProjection(sceneSettings.lightProjectionMatrix);

	if (sceneSettings.enablePointLights)
	{
		for (size_t i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			LightManager::pointLights[i] = PointLight(1024, 1024, 0.01f, 100.0f,
				sceneSettings.pointLights[i].base.enabled,
				sceneSettings.pointLights[i].base.color,
				sceneSettings.pointLights[i].base.ambientIntensity,
				sceneSettings.pointLights[i].base.diffuseIntensity,
				sceneSettings.pointLights[i].position,
				sceneSettings.pointLights[i].constant,
				sceneSettings.pointLights[i].linear,
				sceneSettings.pointLights[i].exponent);
			LightManager::pointLightCount++;
		}
	}

	if (sceneSettings.enableSpotLights)
	{
		for (size_t i = 0; i < MAX_SPOT_LIGHTS; i++)
		{
			LightManager::spotLights[i] = SpotLight(1024, 1024, 0.01f, 100.0f,
				sceneSettings.spotLights[i].base.base.enabled,
				sceneSettings.spotLights[i].base.base.color,
				sceneSettings.spotLights[i].base.base.ambientIntensity,
				sceneSettings.spotLights[i].base.base.diffuseIntensity,
				sceneSettings.spotLights[i].base.position,
				sceneSettings.spotLights[i].direction,
				sceneSettings.spotLights[i].base.constant,
				sceneSettings.spotLights[i].base.linear,
				sceneSettings.spotLights[i].base.exponent,
				sceneSettings.spotLights[i].edge);
			LightManager::spotLightCount++;
		}
	}
}
