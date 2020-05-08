#include "Math.h"

#include <glm/gtc/matrix_transform.hpp>


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
