#pragma once

#include "Core/CommonValues.h"
#include "Mesh/Mesh.h"
#include "Shader/MoravaShader.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>


class Skybox
{

public:
	Skybox();
	Skybox(std::vector<std::string> faceLocations, bool flipVert = false);
	void Draw(glm::mat4 modelMatrix, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
	inline unsigned int GetID() const { return textureID; };
	~Skybox();

private:
	Mesh* skyMesh;
	Hazel::Ref<MoravaShader> m_SkyShader;
	GLuint textureID;

};
