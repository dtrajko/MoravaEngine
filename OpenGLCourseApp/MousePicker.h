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
	glm::vec3 GetPointOnRay(glm::vec3 rayStartPoint, glm::vec3 ray, float distance);
	glm::vec3 BinarySearch(glm::vec3 rayStartPoint, glm::vec3 ray, float startDistance, float finishDistance, int count);
	bool IntersectionInRange(glm::vec3 rayStartPoint, glm::vec3 ray, float startDistance, float finishDistance);
	bool IsAboveGround(glm::vec3 testPoint);
	~MousePicker();

public:
	static MousePicker* s_Instance;
	glm::vec3 m_CurrentRay;
	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewMatrix;
	float m_MouseX;
	float m_MouseY;
	float m_DisplayWidth;
	float m_DisplayHeight;

	glm::vec2 m_NormalizedCoords;
	glm::vec4 m_ClipCoords;
	glm::vec4 m_EyeCoords;
	glm::vec3 m_WorldRay;
	glm::vec3 m_CameraPosition;

	glm::vec3 m_IntersectionPoint = glm::vec3(0.0f);
	bool m_Hit = false;

	glm::vec3 m_RayStartPoint = glm::vec3(0.0f); // it should be camera position or somewhere near

	int m_RecursionCount = 20;
	float m_RayRange = 600.0f;

};
