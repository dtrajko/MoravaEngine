#pragma once

#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Scene/HazelScene.h"

#include "Camera/Camera.h"
#include "Camera/CameraController.h"
#include "Core/AABB.h"
#include "Core/CommonStructs.h"
#include "Core/CommonValues.h"
#include "Core/ResourceManager.h"
#include "Core/Window.h"
#include "Editor/Pivot.h"
#include "Editor/SceneObject.h"
#include "ImGui/ImGuiWrapper.h"
#include "Light/DirectionalLight.h"
#include "Light/LightManager.h"
#include "Light/PointLight.h"
#include "Light/SpotLight.h"
#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "Mesh/Model.h"
#include "Renderer/RendererBasic.h"
#include "Skybox/Skybox.h"
#include "Texture/MoravaTexture.h"
#include "Texture/TextureLoader.h"
#include "Water/WaterManager.h"

#include <glm/glm.hpp>

#include <vector>
#include <map>
#include <string>


struct SceneSettings
{
	// features
	bool enableShadows;
	bool enableOmniShadows;
	bool enableCascadedShadowMaps;
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

	// omni shadows
	unsigned int omniShadowMapWidth;
	unsigned int omniShadowMapHeight;

	float shadowSpeed;

	// water
	float waterHeight;
	float waterWaveSpeed;
};

class LightManager;

class Scene : public Hazel::HazelScene
{
public:
	Scene();
	virtual ~Scene();

	virtual void Update(float timestep, Window* mainWindow);
	virtual void OnWindowResize(WindowResizeEvent& e);
	virtual void UpdateImGui(float timestep, Window* mainWindow) = 0;
	virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, MoravaShader*> shaders, std::map<std::string, int> uniforms) = 0;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, MoravaShader*> shaders, std::map<std::string, int> uniforms) {};
	inline bool IsWireframeEnabled() { return m_WireframeEnabled; };
	virtual inline bool IsWaterOnScene() { return false; };

	// Getters
	Camera* GetCamera();
	inline CameraController* GetCameraController() const { return m_CameraController; };
	static inline SceneSettings GetSettings() { return sceneSettings; };
	std::map<std::string, Hazel::Ref<MoravaTexture>> GetTextures() const { return textures; };
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
	virtual void SetWaterManager(int width, int height);
	inline void SetWireframeEnabled(bool wireframeEnabled) { m_WireframeEnabled = wireframeEnabled; };
	inline void SetFOV(float FOV) { m_FOV = FOV; };

private:
	virtual void SetupTextures();
	virtual void SetupTextureSlots();
	virtual void SetupMaterials();
	virtual void SetupMeshes();
	virtual void SetupParticles();
	virtual void SetupModels();
	virtual void SetupFramebuffers();
	virtual void SetupShaders();
	virtual void SetSkybox();

public:
	static glm::mat4* s_ImGuizmoTransform;
	static int s_ImGuizmoType;
	static SceneSettings sceneSettings;

protected:

	float m_FOV;
	float m_AspectRatio;

	Camera* m_Camera;
	CameraController* m_CameraController;

	Skybox* m_Skybox;
	std::vector<std::string> skyboxFaces;

	WaterManager* m_WaterManager;

	std::map<std::string, Hazel::Ref<MoravaTexture>> textures;
	std::map<std::string, GLuint> textureSlots;
	std::map<std::string, Mesh*> meshes;
	std::map<std::string, Model*> models;
	std::map<std::string, Material*> materials;

	std::map<std::string, float> m_ProfilerResults;

private:
	DirectionalLight directionalLight;
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];

	std::map<std::string, MoravaShader*> shaders;
	std::map<std::string, int> uniforms;

	unsigned int shadowMapWidth;
	unsigned int shadowMapHeight;

	unsigned int omniShadowMapWidth;
	unsigned int omniShadowMapHeight;

	bool m_WireframeEnabled;

	EventCooldown m_KeyPressCooldown;

};
