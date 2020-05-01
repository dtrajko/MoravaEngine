#include "MousePicker.h"


MousePicker* MousePicker::s_Instance = nullptr;

MousePicker* MousePicker::Get()
{
	if (s_Instance == nullptr)
	{
		s_Instance = new MousePicker();
	}

	return s_Instance;
}

MousePicker::MousePicker()
{
	m_CurrentRay = glm::vec3(1.0f);
	m_ProjectionMatrix = glm::mat4(1.0f);
	m_ViewMatrix = glm::mat4(1.0f);
	m_MouseX = 0.0f;
	m_MouseY = 0.0f;
	m_DisplayWidth = 0.0f;
	m_DisplayHeight = 0.0f;
}

glm::vec3 MousePicker::GetCurrentRay()
{
	return m_CurrentRay;
}

void MousePicker::Update(float mouseX, float mouseY, float displayWidth, float displayHeight, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	m_MouseX = mouseX;
	m_MouseY = mouseY;
	m_DisplayWidth = displayWidth;
	m_DisplayHeight = displayHeight;
	m_ProjectionMatrix = projectionMatrix;
	m_ViewMatrix = viewMatrix;
	m_CurrentRay = CalculateMouseRay();
}

glm::vec3 MousePicker::CalculateMouseRay()
{
	glm::vec2 normalizedCoords = GetNormalizedDeviceCoords();
	glm::vec4 clipCoords = glm::vec4(normalizedCoords.x, normalizedCoords.y, -1.0f, -1.0f);
	glm::vec4 eyeCoords = ToEyeCoords(clipCoords);
	glm::vec3 worldRay = ToWorldCoords(eyeCoords);
	return worldRay;
}

glm::vec2 MousePicker::GetNormalizedDeviceCoords()
{
	float x = (m_MouseX * 2.0f) / m_DisplayWidth - 1.0f;
	float y = (m_MouseY * 2.0f) / m_DisplayHeight - 1.0f;
	return glm::vec2(x, y); // it may happen to be { x, -y }
}

glm::vec4 MousePicker::ToEyeCoords(glm::vec4 clipCoords)
{
	glm::mat4 invertedProjection = glm::inverse(m_ProjectionMatrix);
	glm::vec4 eyeCoords = invertedProjection * clipCoords;
	return glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);
}

glm::vec3 MousePicker::ToWorldCoords(glm::vec4 eyeCoords)
{
	glm::mat4 invertedView = glm::inverse(m_ViewMatrix);
	glm::vec4 rayWorld = invertedView * eyeCoords;
	glm::vec3 mouseRay = glm::vec3(rayWorld.x, rayWorld.y, rayWorld.z);
	mouseRay = glm::normalize(mouseRay);
	return mouseRay;
}

glm::vec3 MousePicker::GetPointOnRay(glm::vec3 ray, glm::vec3 cameraPosition, float distance)
{
	glm::vec3 start = glm::vec3(cameraPosition.x, cameraPosition.y, cameraPosition.z);
	glm::vec3 scaledRay = glm::vec3(ray.x * distance, ray.y * distance, ray.z * distance);
	return start + scaledRay;
}

MousePicker::~MousePicker()
{
}
