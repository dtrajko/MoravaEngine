#pragma once

#include "Camera.h"
#include "Mesh.h"

#include <glm/ext/quaternion_float.hpp>


class Player
{
public:
	Player();
	Player(glm::vec3 position, Mesh* mesh, Camera* camera);
	~Player();

	inline glm::vec3 GetPosition() { return m_Position; };
	void Update();
	void Render();

private:
	glm::vec3 m_Position;
	glm::quat m_Rotation;
	Mesh* m_Mesh;
	Camera* m_Camera;

};
