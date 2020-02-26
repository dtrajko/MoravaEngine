#pragma once

#include "Scene.h"


class SceneEiffel : public Scene
{

public:
	SceneEiffel();
	virtual void Update(float timestep) override;
	virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, bool shadowPass,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms,
		std::map<std::string, Texture*> textures, std::map<std::string, GLuint> textureSlots,
		std::map<std::string, Mesh*> meshes, std::map<std::string, Material*> materials,
		std::map<std::string, Model*> models) override;
	~SceneEiffel();

private:

};
