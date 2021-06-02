#ifndef _AABB_H
#define _AABB_H

#include "Shader/MoravaShader.h"

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

#include <iostream>


/**
 * Taken from the strifeEngine project
 */
class AABB
{
public:
	AABB();
	AABB(glm::vec3 positionOrigin, glm::quat rotationOrigin, glm::vec3 scaleOrigin);
	bool Contains(glm::vec3 position, glm::vec3 scale);
	bool TestAABBOverlap(AABB* a, AABB* b);
	void UpdatePosition(glm::vec3 positionOrigin);
	void Update(glm::vec3 positionObject, glm::quat rotationObject, glm::vec3 scaleObject);
	void TransformBounds(glm::vec3 positionObject, glm::quat rotationObject, glm::vec3 scaleObject);
	void Draw();
	glm::vec3 GetMin() const;
	glm::vec3 GetMax() const;
	virtual ~AABB();

	static bool IntersectRayAab(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, glm::vec2 result);

public:
	glm::vec3 m_Position = glm::vec3(0.0f);
	glm::quat m_Rotation = glm::vec3(0.0f);
	glm::vec3 m_Scale = glm::vec3(1.0f);
	glm::vec4 m_Color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

	glm::vec3 m_ScaleOrigin = glm::vec3(1.0f);
	glm::vec3 m_PositionOrigin = glm::vec3(0.0f);

	glm::vec3 m_BoundMin;
	glm::vec3 m_BoundMax;

	bool m_IsColliding;

private:
	std::vector<float> m_Vertices;
	std::vector<float> m_VerticesInitial;
	std::vector<unsigned int> m_Indices;
	float m_UnitSize = 0.5f;
	float m_Offset = 0.02f;
	size_t m_VertexStride = 3 + 4;

};

#endif
