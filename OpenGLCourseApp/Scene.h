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
#include "AABB.h"
#include "Pivot.h"


struct SceneObject
{
	int id;
	glm::mat4 transform;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::vec4 color;
	bool useTexture;
	std::string textureName;
	float tilingFactor;
	bool isSelected;
	AABB* AABB;
	Pivot* pivot;
	Mesh* mesh;
	int meshType;
};

struct EventCooldown
{
	float lastTime;
	float cooldown;
};

struct SLight
{
	bool enabled;
	glm::vec3 color;
	float ambientIntensity;
	float diffuseIntensity;
};

struct SDirectionalLight
{
	SLight base;
	glm::vec3 direction;
};

struct SPointLight
{
	SLight base;
	glm::vec3 position;
	float constant;
	float linear;
	float exponent;
};

struct SSpotLight
{
	SPointLight base;
	glm::vec3 direction;
	float edge;
};

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
	bool enableCulling;

	// camera
	glm::vec3 cameraPosition;
	float cameraStartYaw;
	float cameraStartPitch;
	float cameraMoveSpeed;
	float nearPlane;
	float farPlane;

	// directional light
	SDirectionalLight directionalLight;
	glm::mat4 lightProjectionMatrix;

	// point lights
	SPointLight pointLights[MAX_POINT_LIGHTS];

	// spot lights
	SSpotLight spotLights[MAX_POINT_LIGHTS];

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
	inline void SetWireframeEnabled(bool wireframeEnabled) { m_WireframeEnabled = wireframeEnabled; };
	inline bool IsWireframeEnabled() { return m_WireframeEnabled; };
	inline float GetFOV() { return m_FOV; };
	inline void SetFOV(float FOV) { m_FOV = FOV; };
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

	bool m_WireframeEnabled;

	float m_FOV = 60.0f;
};
