#pragma once

#include <vector>
#include <string>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CommonValues.h"
#include "Mesh.h"
#include "Shader.h"


class Skybox
{

public:
	Skybox();
	Skybox(std::vector<std::string> faceLocations, bool flipVert = false);
	void Draw(glm::mat4 modelMatrix, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
	~Skybox();

private:
	Mesh* skyMesh;
	Shader* skyShader;
	GLuint textureID;
	GLint uniformModel;
	GLint uniformView;
	GLint uniformProjection;

};
