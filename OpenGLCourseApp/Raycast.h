#pragma once

#include <glm/glm.hpp>
#include "Shader.h"


/****
 *
 * Unity documentation:
 * "A raycast is conceptually like a laser beam that is fired from a point in space
 * along a particular direction.Any object making contact with the beam can be detected and reported"
 *
 */
class Raycast
{
public:
	Raycast();
	void Draw(glm::vec3 start, glm::vec3 end, glm::vec4 color, Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	~Raycast();

public:
	// ray casting line
	glm::vec3 m_LineStart = glm::vec3(0.0f);
	glm::vec3 m_LineEnd = glm::vec3(0.0f);
	glm::vec4 m_Color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

};
