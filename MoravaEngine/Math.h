#pragma once

#include "Random.h"

#include <glm/glm.hpp>
#include <assimp/scene.h>


class Math
{
public:
	static glm::mat4 CreateTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
	static glm::mat4 CreateTransform(glm::vec3 position, glm::quat rotation, glm::vec3 scale);
    static glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from);
	static float InverseLerp(float xx, float yy, float value);
	static float ConvertRangeFloat(float value, float oldMin, float oldMax, float newMin, float newMax);

private:
	static bool s_IsRandomInit;

};
