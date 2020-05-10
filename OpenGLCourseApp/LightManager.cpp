#include "LightManager.h"



DirectionalLight LightManager::directionalLight;
PointLight LightManager::pointLights[MAX_POINT_LIGHTS];
SpotLight LightManager::spotLights[MAX_SPOT_LIGHTS];

unsigned int LightManager::pointLightCount = 0;
unsigned int LightManager::spotLightCount = 0;


LightManager::LightManager(SceneSettings sceneSettings)
{
	LightManager::directionalLight = DirectionalLight(sceneSettings.shadowMapWidth, sceneSettings.shadowMapHeight, { 1.0f, 1.0f, 1.0f },
		sceneSettings.ambientIntensity, sceneSettings.diffuseIntensity, sceneSettings.lightDirection);
	LightManager::directionalLight.SetLightProjection(sceneSettings.lightProjectionMatrix);

	if (sceneSettings.enablePointLights)
	{
		LightManager::pointLights[0] = PointLight(1024, 1024, 0.01f, 100.0f,
			sceneSettings.pLight_0_color, 0.4f,
			sceneSettings.pLight_0_diffuseIntensity,
			sceneSettings.pLight_0_position, 0.3f, 0.2f, 0.1f);
		LightManager::pointLightCount++;
		LightManager::pointLights[1] = PointLight(1024, 1024, 0.01f, 100.0f,
			sceneSettings.pLight_1_color, 0.4f,
			sceneSettings.pLight_1_diffuseIntensity,
			sceneSettings.pLight_1_position, 0.3f, 0.2f, 0.1f);
		LightManager::pointLightCount++;
		LightManager::pointLights[2] = PointLight(1024, 1024, 0.01f, 100.0f,
			sceneSettings.pLight_2_color, 0.4f,
			sceneSettings.pLight_2_diffuseIntensity,
			sceneSettings.pLight_2_position, 0.3f, 0.2f, 0.1f);
		LightManager::pointLightCount++;
		LightManager::pointLights[3] = PointLight(1024, 1024, 0.01f, 100.0f,
			sceneSettings.pLight_3_color, 0.4f,
			sceneSettings.pLight_3_diffuseIntensity,
			sceneSettings.pLight_3_position, 0.3f, 0.2f, 0.1f);
		LightManager::pointLightCount++;
	}

	if (sceneSettings.enableSpotLights)
	{
		LightManager::spotLights[0] = SpotLight(1024, 1024, 0.01f, 100.0f, 
			sceneSettings.sLight_0_color, sceneSettings.sLight_0_ambientIntensity, sceneSettings.sLight_0_diffuseIntensity,
			sceneSettings.sLight_0_position, sceneSettings.sLight_0_direction,
			0.4f, 0.3f, 0.2f, 35.0f);
		LightManager::spotLightCount++;
		LightManager::spotLights[1] = SpotLight(1024, 1024, 0.01f, 100.0f,
			sceneSettings.sLight_1_color, sceneSettings.sLight_1_ambientIntensity, sceneSettings.sLight_1_diffuseIntensity,
			sceneSettings.sLight_1_position, sceneSettings.sLight_1_direction,
			0.4f, 0.3f, 0.2f, 35.0f);
		LightManager::spotLightCount++;
		LightManager::spotLights[2] = SpotLight(1024, 1024, 0.01f, 100.0f,
			sceneSettings.sLight_2_color, sceneSettings.sLight_2_ambientIntensity, sceneSettings.sLight_2_diffuseIntensity,
			sceneSettings.sLight_2_position, sceneSettings.sLight_2_direction,
			0.4f, 0.3f, 0.2f, 35.0f);
		LightManager::spotLightCount++;
		LightManager::spotLights[3] = SpotLight(1024, 1024, 0.01f, 100.0f,
			sceneSettings.sLight_3_color, sceneSettings.sLight_3_ambientIntensity, sceneSettings.sLight_3_diffuseIntensity,
			sceneSettings.sLight_3_position, sceneSettings.sLight_3_direction,
			0.4f, 0.3f, 0.2f, 35.0f);
		LightManager::spotLightCount++;
	}
}

LightManager::~LightManager()
{
}
