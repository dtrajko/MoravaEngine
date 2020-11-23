#pragma once

#include "Hazel/Scene/SceneCamera.h"

#include "glm/glm.hpp"


class Camera : public Hazel::SceneCamera
{

public:
	Camera();
	Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch);
	~Camera();

	void Update();

	// getters
	inline glm::vec3 GetDirection() const { return glm::normalize(m_Front); }
	inline float GetPitch() const { return m_Pitch; }
	inline float GetYaw() const { return m_Yaw; }
	inline glm::vec3 GetFront() const { return m_Front; }
	inline glm::vec3 GetUp() const { return m_Up; }
	inline glm::vec3 GetRight() const { return m_Right; }

	// setters
	inline void SetPosition(glm::vec3 position) { m_Position = position; };
	inline void SetFront(glm::vec3 front) { m_Front = front; };
	inline void SetYaw(float yaw) { m_Yaw = yaw; };
	inline void SetExposure(float exposure) { m_Exposure = exposure; }
	void SetPitch(float pitch);

private:
	glm::vec3 m_Front;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;

	float m_Roll; // not used

};
