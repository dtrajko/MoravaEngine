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

	LightManager::spotLights[0] = SpotLight(1024, 1024, 0.01f, 100.0f, { 1.0f, 1.0f, 1.0f }, 1.0f, 10.0f, { 0.0f, 20.0f, -28.0f }, { 0.0f, 0.0f, -1.0f }, 0.3f, 0.2f, 0.1f, 160.0f);
	LightManager::spotLightCount++;
	LightManager::spotLights[1] = SpotLight(1024, 1024, 0.01f, 100.0f, { 0.8f, 0.8f, 1.0f }, 0.3f, 6.0f, { 8.0f, 2.0f, 0.0f }, { -0.25f, 0.5f, -0.5f }, 0.3f, 0.2f, 0.1f, 45.0f);
	LightManager::spotLightCount++;
	LightManager::spotLights[2] = SpotLight(1024, 1024, 0.01f, 100.0f, { 1.0f, 1.0f, 1.0f }, 0.4f, 1.6f, glm::vec3(), glm::vec3(), 0.4f, 0.3f, 0.2f, 35.0f);
	LightManager::spotLightCount++;
}

LightManager::~LightManager()
{
}
