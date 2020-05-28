#pragma once

#include "Scene.h"
#include "Shader.h"
#include "Mesh.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Block.h"
#include "Ring.h"


const int GIZMO_MODE_NONE      = 0;
const int GIZMO_MODE_TRANSLATE = 1;
const int GIZMO_MODE_SCALE     = 2;
const int GIZMO_MODE_ROTATE    = 3;


struct Bool3
{
	bool x;
	bool y;
	bool z;
};

struct GizmoObject
{
	SceneObject so;
	Bool3 axes;
	std::string name;
};

class Gizmo
{
public:
	Gizmo();
	void Update(glm::vec3 cameraPosition, Window& mainWindow);
	void UpdateActive(glm::vec3 cameraPosition, Window& mainWindow);
	void Render(Shader* shader);
	void CreateObjects();
	void ChangeMode(int mode);
	void ToggleMode();
	void SetSceneObject(SceneObject* sceneObject);
	void UpdateTransformFromObject(SceneObject* sceneObject);
	void SetActive(bool active);
	inline bool GetActive() { return m_Active; };
	inline Bool3 GetAxesEnabled() { return m_AxesEnabled; };
	std::string GetModeDescriptive(int modeID = -1);
	void PrintObjects();
	void OnMousePress(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	void OnMouseRelease(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	~Gizmo();

private:
	int m_Mode;
	Bool3 m_AxesEnabled;
	bool m_Active;

	SceneObject* m_SceneObject;
	glm::vec3 m_Position;
	glm::vec3 m_Rotation;
	glm::vec3 m_Scale;

	const glm::vec3 m_ScaleGlobal = glm::vec3(0.5f);

	EventCooldown m_MousePress;
	EventCooldown m_MouseRelease;
	EventCooldown m_PrintObjects;
	EventCooldown m_ChangeActive;
	EventCooldown m_ChangeMode;

	std::vector<GizmoObject*> m_GizmoObjects;

	// Translate meshes
	GizmoObject* m_Axis_T_X;
	GizmoObject* m_Axis_T_Y;
	GizmoObject* m_Axis_T_Z;

	GizmoObject* m_Arrow_T_X;
	GizmoObject* m_Arrow_T_Y;
	GizmoObject* m_Arrow_T_Z;

	GizmoObject* m_Square_T_YZ; // cyan
	GizmoObject* m_Square_T_ZX; // magenta
	GizmoObject* m_Square_T_XY; // yellow

	// Scale meshes
	GizmoObject* m_Axis_S_X;
	GizmoObject* m_Axis_S_Y;
	GizmoObject* m_Axis_S_Z;

	GizmoObject* m_Handle_S_X;
	GizmoObject* m_Handle_S_Y;
	GizmoObject* m_Handle_S_Z;

	// Rotate meshes
	GizmoObject* m_Ring_R_X;
	GizmoObject* m_Ring_R_Y;
	GizmoObject* m_Ring_R_Z;

	// Colors
	glm::vec4 m_Color_Red   = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 m_Color_Green = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	glm::vec4 m_Color_Blue  = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	glm::vec4 m_Color_Yellow  = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	glm::vec4 m_Color_Cyan    = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
	glm::vec4 m_Color_Magenta = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);

	glm::vec4 m_Color_Selected = m_Color_Yellow;

	float m_FactorTranslate = 0.001f;
	float m_FactorScale = 0.001f;
	float m_FactorRotate = 0.02f;
	float m_ShiftSpeed = 10.0f;
};
