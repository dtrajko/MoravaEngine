#pragma once

#include "Scene.h"


class SceneJoey : public Scene
{

public:
	SceneJoey();
	virtual void Update(float timestep) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;

	std::map<std::string, unsigned int> GetTextureIDs() const { return m_TextureIDs; };

	~SceneJoey();

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;

public:
	glm::vec3 m_LightPositions[4];
	glm::vec3 m_LightColors[4];

private:
	std::map<std::string, unsigned int> m_TextureIDs;

};
