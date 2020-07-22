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

	if (IntersectionInRange(m_RayStartPoint, m_CurrentRay, 0, m_RayRange))
		m_IntersectionPoint = BinarySearch(m_RayStartPoint, m_CurrentRay, 0, m_RayRange, 0);
	else
		m_IntersectionPoint = glm::vec3(0.0f);
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
	float y = 1.0f - (m_MouseY * 2.0f) / m_DisplayHeight;
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

glm::vec3 MousePicker::GetPointOnRay(glm::vec3 rayStartPoint, glm::vec3 ray, float distance)
{
	m_RayStartPoint = rayStartPoint;
	glm::vec3 scaledRay = glm::vec3(ray.x * distance, ray.y * distance, ray.z * distance);
	return m_RayStartPoint + scaledRay;
}

glm::vec3 MousePicker::BinarySearch(glm::vec3 rayStartPoint, glm::vec3 ray, float startDistance, float finishDistance, int count)
{
	float halfDistance = startDistance + ((finishDistance - startDistance) / 2.0f);
	if (count >= m_RecursionCount)
	{
		glm::vec3 endPoint = GetPointOnRay(rayStartPoint, ray, halfDistance);
		if (m_Terrain != nullptr)
			return endPoint;
		else
			return glm::vec3(0.0f);
	}
	if (IntersectionInRange(rayStartPoint, ray, startDistance, halfDistance))
		return BinarySearch(rayStartPoint, ray, startDistance, halfDistance, count + 1);
	else
		return BinarySearch(rayStartPoint, ray, halfDistance, finishDistance, count + 1);
}

bool MousePicker::IntersectionInRange(glm::vec3 rayStartPoint, glm::vec3 ray, float startDistance, float finishDistance)
{
	glm::vec3 startPoint = GetPointOnRay(rayStartPoint, ray, startDistance);
	glm::vec3 endPoint = GetPointOnRay(rayStartPoint, ray, finishDistance);
	if (IsOutsideVolume(startPoint) && !IsOutsideVolume(endPoint))
	{
		m_Hit = true;
		return true;
	}
	else
	{
		m_Hit = false;
		return false;
	}
}

bool MousePicker::IsOutsideVolume(glm::vec3 testPoint)
{
	m_TestPoint = glm::vec3((int)testPoint.x, (int)testPoint.y, (int)testPoint.z);

	m_TerrainHeight = 0;
	if (m_Terrain != nullptr)
	{
		m_TerrainHeight = (int)m_Terrain->GetMaxY((int)m_TestPoint.x, (int)m_TestPoint.y);
	}

	if ((int)testPoint.y > m_TerrainHeight)
		return true;
	else
		return false;
}

MousePicker::~MousePicker()
{
}
