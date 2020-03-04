#pragma once

#include <vector>
#include <map>
#include <string>

#include <glm/glm.hpp>

#include "CommonValues.h"
#include "Camera.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Model.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Skybox.h"
#include "LightManager.h"
#include "WaterManager.h"


struct SceneSettings
{
	// features
	bool enableShadows;
	bool enableOmniShadows;
	bool enablePointLights;
	bool enableSpotLights;
	bool enableWaterEffects;
	bool enableSkybox;
	bool enableNormalMaps;

	// camera
	glm::vec3 cameraPosition;
	float cameraStartYaw;
	float cameraMoveSpeed;
	float nearPlane;
	float farPlane;

	// light
	glm::vec3 lightDirection;
	float ambientIntensity;
	float diffuseIntensity;
	glm::mat4 lightProjectionMatrix;
	glm::vec3 pLight_0_color;
	glm::vec3 pLight_0_position;
	float pLight_0_diffuseIntensity;
	glm::vec3 pLight_1_color;
	glm::vec3 pLight_1_position;
	float pLight_1_diffuseIntensity;
	glm::vec3 pLight_2_color;
	glm::vec3 pLight_2_position;
	float pLight_2_diffuseIntensity;
	// shadow
	unsigned int shadowMapWidth;
	unsigned int shadowMapHeight;
	float shadowSpeed;
	// water
	float waterHeight;
	float waterWaveSpeed;
};

class LightManager;

class Scene
{

public:
	Scene();
	virtual void Update(float timestep, LightManager& lightManager) = 0;
	virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager) = 0;
	virtual void RenderWater(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager) {};
	inline Skybox* GetSkybox() const { return skybox; };
	static inline SceneSettings GetSettings() { return sceneSettings; };
	std::map<std::string, Texture*> GetTextures() const { return textures; };
	std::map<std::string, GLuint> GetTextureSlots() const { return textureSlots; };
	~Scene();

private:
	virtual void SetSkybox() = 0;
	virtual void SetupModels() = 0;
	virtual void SetupMeshes();
	virtual void SetTextures();
	virtual void SetTextureSlots();
	void SetupMaterials();

protected:
	static SceneSettings sceneSettings;

	Skybox* skybox;
	std::vector<std::string> skyboxFaces;

	std::map<std::string, Texture*> textures;
	std::map<std::string, GLuint> textureSlots;
	std::map<std::string, Mesh*> meshes;
	std::map<std::string, Material*> materials;
	std::map<std::string, Model*> models;

private:
	Camera camera;

	DirectionalLight directionalLight;
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];

	std::map<std::string, Shader*> shaders;
	std::map<std::string, GLint> uniforms;

	unsigned int shadowMapWidth;
	unsigned int shadowMapHeight;
};
