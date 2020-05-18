#pragma once

#include "Shader.h"
#include "Mesh.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Block.h"
#include "Ring.h"
#include "Scene.h"


const int GIZMO_MODE_TRANSLATE = 0;
const int GIZMO_MODE_SCALE     = 1;
const int GIZMO_MODE_ROTATE    = 2;


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
};

class Gizmo
{
public:
	Gizmo();
	void ChangeMode(int mode);
	void ToggleMode();
	void SetSceneObject(SceneObject* sceneObject);
	void Update(glm::vec3 cameraPosition, Window& mainWindow);
	void Render(Shader* shader);
	~Gizmo();

private:
	int m_Mode;

	SceneObject* m_SceneObject;
	glm::vec3 m_Position;
	glm::vec3 m_Rotation;
	glm::vec3 m_Scale;

	Bool3 m_AxesEnabled;

	EventCooldown m_ToggleMode;

	std::vector<GizmoObject*> m_GizmoObjects;

	// Translate meshes
	GizmoObject* m_Axis_T_X;
	GizmoObject* m_Axis_T_Y;
	GizmoObject* m_Axis_T_Z;

	GizmoObject* m_Arrow_T_X;
	GizmoObject* m_Arrow_T_Y;
	GizmoObject* m_Arrow_T_Z;

	GizmoObject* m_Square_T_XY; // yellow
	GizmoObject* m_Square_T_YZ; // cyan
	GizmoObject* m_Square_T_ZX; // magenta

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

	float m_FactorTranslate = 0.04f;
	float m_FactorScale = 0.04f;
	float m_FactorRotate = 0.1f;
};
