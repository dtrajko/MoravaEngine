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
#include "Window.h"


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

	// directional light
	glm::vec3 lightDirection;
	float ambientIntensity;
	float diffuseIntensity;
	glm::mat4 lightProjectionMatrix;
	// point light
	glm::vec3 pLight_0_color;
	glm::vec3 pLight_0_position;
	float pLight_0_diffuseIntensity;
	glm::vec3 pLight_1_color;
	glm::vec3 pLight_1_position;
	float pLight_1_diffuseIntensity;
	glm::vec3 pLight_2_color;
	glm::vec3 pLight_2_position;
	float pLight_2_diffuseIntensity;
	// spot light
	glm::vec3 sLight_2_color;
	glm::vec3 sLight_2_position;
	glm::vec3 sLight_2_direction;
	float sLight_2_ambientIntensity;
	float sLight_2_diffuseIntensity;
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
	virtual void Update(float timestep, Window& mainWindow) = 0;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) = 0;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) = 0;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) {};
	inline Skybox* GetSkybox() const { return m_Skybox; };
	static inline SceneSettings GetSettings() { return sceneSettings; };
	std::map<std::string, Texture*> GetTextures() const { return textures; };
	std::map<std::string, GLuint> GetTextureSlots() const { return textureSlots; };
	void SetCamera();
	inline Camera* GetCamera() const { return m_Camera; };
	virtual void SetLightManager();
	inline LightManager* GetLightManager() const { return m_LightManager; };
	void SetWaterManager(int width, int height);
	inline WaterManager* GetWaterManager() const { return m_WaterManager; };
	inline std::map<std::string, Model*> GetModels() const { return models; };
	inline std::map<std::string, Mesh*> GetMeshes() const { return meshes; };
	virtual ~Scene();

private:
	virtual void SetSkybox() = 0;
	virtual void SetupModels() = 0;
	virtual void SetupMeshes();
	virtual void SetTextures();
	virtual void SetTextureSlots();
	void SetupMaterials();

protected:
	static SceneSettings sceneSettings;

	Camera* m_Camera;
	Skybox* m_Skybox;
	std::vector<std::string> skyboxFaces;

	LightManager* m_LightManager;
	WaterManager* m_WaterManager;

	std::map<std::string, Texture*> textures;
	std::map<std::string, GLuint> textureSlots;
	std::map<std::string, Mesh*> meshes;
	std::map<std::string, Material*> materials;
	std::map<std::string, Model*> models;

private:
	DirectionalLight directionalLight;
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];

	std::map<std::string, Shader*> shaders;
	std::map<std::string, GLint> uniforms;

	unsigned int shadowMapWidth;
	unsigned int shadowMapHeight;
};
