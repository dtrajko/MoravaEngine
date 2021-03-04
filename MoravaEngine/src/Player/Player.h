#pragma once

#include "Camera/Camera.h"
#include "Mesh/Mesh.h"

#include <glm/ext/quaternion_float.hpp>


class Player
{
public:
	Player();
	Player(glm::vec3 position, Mesh* mesh, Camera* camera);
	~Player();

	// getters
	inline glm::vec3 GetPosition() { return m_Position; };
	inline glm::vec3 GetRotation() { return m_Rotation; };
	inline glm::vec3 GetFront() const { return m_Front; };
	inline glm::vec3 GetUp() const { return m_Up; };
	inline glm::vec3 GetRight() const { return m_Right; };
	inline glm::vec4 GetColor() const { return m_Color; };

	// setters
	inline void SetPosition(glm::vec3 position) { m_Position = position; };
	inline void SetRotation(glm::vec3 rotation) { m_Rotation = rotation; };

	void Update();
	void Render();

private:
	void CalculateFront();

private:
	glm::vec3 m_Position;
	glm::vec3 m_Rotation;

	glm::vec3 m_Front;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;

	Mesh* m_Mesh;
	Camera* m_Camera;

	glm::vec4 m_Color;

};
