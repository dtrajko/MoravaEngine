#pragma once

#include "Scene.h"

#include "LearnOpenGL/ModelJoey.h"


struct NanosuitMaterial
{
	int diffuse;   // sampler2D 0
	int specular;  // sampler2D 1
	int normalMap; // sampler2D 2
	float shininess;
};

struct NanosuitLight
{
	glm::vec3 position;
	glm::vec3 direction;
	float cutOff;
	float outerCutOff;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

struct NanosuitUniforms
{
	glm::vec3 viewPos;
	bool enableNormalMap;
	NanosuitMaterial material;
	NanosuitLight light;
};

class SceneNanosuit : public Scene
{

public:
	SceneNanosuit();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	inline std::map<std::string, ModelJoey*> GetModels() const { return models; };
	NanosuitUniforms* GetNanosuitUniforms() { return nanosuitUniforms; };
	void InitNanosuitUniforms();
	virtual ~SceneNanosuit() override;

public:
	bool m_LightOnCamera;
	bool m_IsRotating;
	float m_RotationSpeed;
	bool m_DefaultNanosuitUniforms;
	bool m_LightSourceVisible;
	glm::vec3 m_BgColor;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	std::map<std::string, ModelJoey*> models;
	NanosuitUniforms* nanosuitUniforms;

};
