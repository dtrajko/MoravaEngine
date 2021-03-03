#pragma once

#include "Scene/Scene.h"

#include "../../LearnOpenGL/ModelJoey.h"

#include "Material/Material.h"
#include "Shader/Shader.h"


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
	Material material;
	NanosuitLight light;
};

class SceneNanosuit : public Scene
{

public:
	SceneNanosuit();
	virtual ~SceneNanosuit() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;
	inline std::map<std::string, ModelJoey*> GetModels() const { return models; };
	inline std::map<std::string, MeshJoey*> GetMeshesJoey() const { return meshesJoey; };
	NanosuitUniforms* GetNanosuitUniforms() { return nanosuitUniforms; };

private:
	virtual void SetSkybox() override {};
	virtual void SetupMeshes() override {};
	virtual void SetupTextures() override;
	virtual void SetupModels() override;

	void SetupShaders();
	void InitNanosuitUniforms();

private:
	bool m_LightOnCamera;
	bool m_IsRotating;
	float m_RotationSpeed;
	bool m_LightSourceVisible;
	glm::vec4 m_BgColor;

	Shader* m_ShaderNanosuit;

	std::map<std::string, ModelJoey*> models;
	std::map<std::string, MeshJoey*> meshesJoey;
	NanosuitUniforms* nanosuitUniforms;
	float m_ModelRotationY = 0.0f;

};
