#ifndef _AABB_H
#define _AABB_H

#include <iostream>
#include <glm/glm.hpp>

#include "Shader.h"


/**
 * Taken from the strifeEngine project
 */
class AABB
{
public:
	glm::vec3 m_Position = glm::vec3(0.0f);
	glm::vec3 m_Scale = glm::vec3(1.0f);

private:
	std::vector<float> m_Vertices;
	std::vector<unsigned int> m_Indices;

public:
	AABB(glm::vec3 position, glm::vec3 scale);
	bool Contains(glm::vec3 position, glm::vec3 scale);
	bool TestAABBOverlap(AABB* a, AABB* b);
	void UpdatePosition(glm::vec3 position);
	void Update(glm::vec3 position, glm::vec3 scale);
	bool IntersectRayAab(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, glm::vec2 result);
	void Draw(Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	virtual ~AABB();

};
	
#endif
