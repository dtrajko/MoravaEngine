#pragma once

#include <glm/glm.hpp>


class Math
{
public:
	static glm::mat4 CreateTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
};

