#pragma once

#include <glm/glm.hpp>


class MousePicker
{
public:
	MousePicker();
	static MousePicker* Get();
	glm::vec3 GetCurrentRay();
	void Update(float mouseX, float mouseY, float displayWidth, float displayHeight, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	glm::vec3 CalculateMouseRay();
	glm::vec2 GetNormalizedDeviceCoords();
	glm::vec4 ToEyeCoords(glm::vec4 clipCoords);
	glm::vec3 ToWorldCoords(glm::vec4 eyeCoords);
	~MousePicker();

private:
	static MousePicker* s_Instance;
	glm::vec3 m_CurrentRay;
	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewMatrix;
	float m_MouseX;
	float m_MouseY;
	float m_DisplayWidth;
	float m_DisplayHeight;
};
