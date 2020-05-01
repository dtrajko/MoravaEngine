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

	m_NormalizedCoords = glm::vec2(0.0f);
	m_ClipCoords = glm::vec4(0.0f);
	m_EyeCoords = glm::vec4(0.0f);
	m_WorldRay = glm::vec3(0.0f);
	m_CameraPosition = glm::vec3(0.0f);
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
	m_NormalizedCoords = GetNormalizedDeviceCoords();
	m_ClipCoords = glm::vec4(m_NormalizedCoords.x, m_NormalizedCoords.y, -1.0f, 1.0f);
	m_EyeCoords = ToEyeCoords(m_ClipCoords);
	m_WorldRay = ToWorldCoords(m_EyeCoords);
	return m_WorldRay;
}

glm::vec2 MousePicker::GetNormalizedDeviceCoords()
{
	float x = (m_MouseX * 2.0f) / m_DisplayWidth - 1.0f;
	float y = (m_MouseY * 2.0f) / m_DisplayHeight - 1.0f;
	return glm::vec2(x, -y); // it may happen to be { x, -y }
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

glm::vec3 MousePicker::GetPointOnRay(glm::vec3 cameraPosition, float distance)
{
	m_CameraPosition = cameraPosition;
	glm::vec3 start = glm::vec3(m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z);
	glm::vec3 scaledRay = glm::vec3(m_CurrentRay.x * distance, m_CurrentRay.y * distance, m_CurrentRay.z * distance);
	return start + scaledRay;
}

MousePicker::~MousePicker()
{
}
