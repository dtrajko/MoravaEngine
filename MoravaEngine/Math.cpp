#include "Math.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <time.h>


bool Math::s_IsRandomInit = false;

glm::mat4 Math::CreateTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	glm::mat4 transform = glm::mat4(1.0f);
	transform *= glm::translate(glm::mat4(1.0f), position);
	transform *= glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	transform *= glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	transform *= glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	transform *= glm::scale(glm::mat4(1.0f), scale);
	return transform;
}

glm::mat4 Math::CreateTransform(glm::vec3 position, glm::quat rotation, glm::vec3 scale)
{
	glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), position);
	glm::mat4 rotateMatrix    = glm::toMat4(rotation);
	glm::mat4 scaleMatrix     = glm::scale(glm::mat4(1.0f), scale);
	return translateMatrix * rotateMatrix * scaleMatrix;
}

glm::mat4 Math::aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
    glm::mat4 to;

    to[0][0] = (float)from->a1; to[0][1] = (float)from->b1;  to[0][2] = (float)from->c1; to[0][3] = (float)from->d1;
    to[1][0] = (float)from->a2; to[1][1] = (float)from->b2;  to[1][2] = (float)from->c2; to[1][3] = (float)from->d2;
    to[2][0] = (float)from->a3; to[2][1] = (float)from->b3;  to[2][2] = (float)from->c3; to[2][3] = (float)from->d3;
    to[3][0] = (float)from->a4; to[3][1] = (float)from->b4;  to[3][2] = (float)from->c4; to[3][3] = (float)from->d4;

    return to;
}

/**
 * Convert an arbitrary range to [0-1] range
 */
float Math::InverseLerp(float xx, float yy, float value)
{
	return (value - xx) / (yy - xx);
}

float Math::ConvertRangeFloat(float value, float oldMin, float oldMax, float newMin, float newMax)
{
	return (((value - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
}
