#pragma once

#include <glm/gtc/type_ptr.hpp>


struct VertexTBN
{
	glm::vec3 Position;
	glm::vec2 TexCoord;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
};
