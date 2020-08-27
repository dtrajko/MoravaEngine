#pragma once

#include <vector>
#include <map>
#include <string>

#include <glm/glm.hpp>

#include "CommonValues.h"
#include "Camera.h"
#include "CameraController.h"
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
#include "TextureLoader.h"
#include "SceneObject.h"


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
	bool enableParticles;

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
	virtual void UpdateImGui(float timestep, Window& mainWindow) = 0;
	virtual void ShowExampleAppDockSpace(bool* p_open, Window& mainWindow);
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) = 0;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) {};
	inline bool IsWireframeEnabled() { return m_WireframeEnabled; };
	virtual inline bool IsWaterOnScene() { return false; };
	virtual ~Scene();

	// Getters
	inline Camera* GetCamera() const { return m_Camera; };
	inline CameraController* GetCameraController() const { return m_CameraController; };
	static inline SceneSettings GetSettings() { return sceneSettings; };
	std::map<std::string, Texture*> GetTextures() const { return textures; };
	std::map<std::string, GLuint> GetTextureSlots() const { return textureSlots; };
	inline std::map<std::string, Material*> GetMaterials() const { return materials; };
	inline std::map<std::string, Model*> GetModels() const { return models; };
	inline std::map<std::string, Mesh*> GetMeshes() const { return meshes; };
	inline WaterManager* GetWaterManager() const { return m_WaterManager; };
	inline Skybox* GetSkybox() const { return m_Skybox; };
	inline float GetFOV() { return m_FOV; };
	inline std::map<std::string, float>* GetProfilerResults() { return &m_ProfilerResults; };

	// Setters
	virtual void SetCamera();
	virtual void SetLightManager();
	void SetWaterManager(int width, int height);
	inline void SetWireframeEnabled(bool wireframeEnabled) { m_WireframeEnabled = wireframeEnabled; };
	inline void SetFOV(float FOV) { m_FOV = FOV; };

private:
	virtual void SetupTextures();
	virtual void SetupTextureSlots();
	virtual void SetupMaterials();
	virtual void SetupMeshes();
	virtual void SetupParticles();
	virtual void SetupFramebuffers();
	virtual void SetupModels();
	virtual void SetSkybox();

protected:
	static SceneSettings sceneSettings;

	float m_FOV;
	float m_AspectRatio;

	Camera* m_Camera;
	CameraController* m_CameraController;

	Skybox* m_Skybox;
	std::vector<std::string> skyboxFaces;

	WaterManager* m_WaterManager;

	std::map<std::string, Texture*> textures;
	std::map<std::string, GLuint> textureSlots;
	std::map<std::string, Mesh*> meshes;
	std::map<std::string, Model*> models;
	std::map<std::string, Material*> materials;

	std::map<std::string, float> m_ProfilerResults;

private:
	DirectionalLight directionalLight;
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];

	std::map<std::string, Shader*> shaders;
	std::map<std::string, GLint> uniforms;

	unsigned int shadowMapWidth;
	unsigned int shadowMapHeight;

	bool m_WireframeEnabled;

};
