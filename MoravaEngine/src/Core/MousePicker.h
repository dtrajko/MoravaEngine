#pragma once

#include "Terrain/TerrainBase.h"

#include <glm/glm.hpp>


/**
 * A singleton class
 *
 */
class MousePicker
{
public:
	MousePicker();
	static MousePicker* Get();
	glm::vec3 GetCurrentRay();
	void Update(glm::mat4 viewMatrix);
	void Update(int screenMouseX, int screenMouseY, int viewportX, int viewportY, int viewportWidth, int viewportHeight, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	inline void SetTerrain(TerrainBase* terrain) { m_Terrain = terrain; };
	glm::vec3 CalculateMouseRay();
	glm::vec2 GetNormalizedDeviceCoords();
	glm::vec4 ToEyeCoords(glm::vec4 clipCoords);
	glm::vec3 ToWorldCoords(glm::vec4 eyeCoords);
	glm::vec3 GetPointOnRay(glm::vec3 rayStartPoint, glm::vec3 ray, float distance);
	glm::vec3 BinarySearch(glm::vec3 rayStartPoint, glm::vec3 ray, float startDistance, float finishDistance, int count);
	bool IntersectionInRange(glm::vec3 rayStartPoint, glm::vec3 ray, float startDistance, float finishDistance);
	bool IsOutsideVolume(glm::vec3 testPoint);
	void SetViewport(int viewportX, int viewportY, int viewportWidth, int viewportHeight);
	~MousePicker();

public:
	struct Viewport
	{
		int X;
		int Y;
		int Width;
		int Height;
		int MouseX;
		int MouseY;
	} m_Viewport;

	static MousePicker* s_Instance;
	glm::vec3 m_CurrentRay;
	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewMatrix;
	int m_ScreenMouseX;
	int m_ScreenMouseY;

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

	TerrainBase* m_Terrain = nullptr;

	glm::vec3 m_TestPoint = glm::vec3(0.0f);
	int m_TerrainHeight = 0;

};
