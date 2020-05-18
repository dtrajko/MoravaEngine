#include "Gizmo.h"

#include "MousePicker.h"
#include "Math.h"


Gizmo::Gizmo()
{
	m_Mode = GIZMO_MODE_TRANSLATE;

	m_Position = glm::vec3(0.0f);
	m_Rotation = glm::vec3(0.0f);
	m_Scale = glm::vec3(1.0f);

	m_ToggleMode = { 0.0f, 0.5f };

	m_AxesEnabled = { false, false, false };

	// the scene object controlled by the Gizmo
	m_SceneObject = new SceneObject{};
	m_SceneObject->transform = glm::mat4(1.0f);
	m_SceneObject->position = m_Position;
	m_SceneObject->rotation = m_Rotation;
	m_SceneObject->scale = m_Scale;
	m_SceneObject->AABB = new AABB(m_Position, m_Rotation, m_Scale);
	m_SceneObject->pivot = new Pivot(m_Position, m_Scale);
	m_SceneObject->color = glm::vec4(1.0f);

	// Initialize Translation meshes
	m_Axis_T_X = new GizmoObject{};
	m_Axis_T_X->so.meshType = MESH_TYPE_CYLINDER;
	m_Axis_T_X->so.color = m_Color_Red;
	m_Axis_T_X->so.position = m_Position + glm::vec3(2.0f, 0.0f, 0.0f);
	m_Axis_T_X->so.rotation = glm::vec3(0.0f, 0.0f, -90.0f);
	m_Axis_T_X->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_T_X->so.mesh = new Cylinder(m_Axis_T_X->so.scale);
	m_Axis_T_X->so.AABB = new AABB(m_Axis_T_X->so.position, m_Axis_T_X->so.rotation, m_Axis_T_X->so.scale);
	m_Axis_T_X->axes = { true, false, false };
	m_GizmoObjects.push_back(m_Axis_T_X);

	m_Axis_T_Y = new GizmoObject{};
	m_Axis_T_Y->so.meshType = MESH_TYPE_CYLINDER;
	m_Axis_T_Y->so.color = m_Color_Green;
	m_Axis_T_Y->so.position = m_Position + glm::vec3(0.0f, 2.0f, 0.0f);
	m_Axis_T_Y->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Axis_T_Y->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_T_Y->so.mesh = new Cylinder(m_Axis_T_Y->so.scale);
	m_Axis_T_Y->so.AABB = new AABB(m_Axis_T_Y->so.position, m_Axis_T_Y->so.rotation, m_Axis_T_Y->so.scale);
	m_Axis_T_Y->axes = { false, true, false };
	m_GizmoObjects.push_back(m_Axis_T_Y);

	m_Axis_T_Z = new GizmoObject{};
	m_Axis_T_Z->so.meshType = MESH_TYPE_CYLINDER;
	m_Axis_T_Z->so.color = m_Color_Blue;
	m_Axis_T_Z->so.position = m_Position + glm::vec3(0.0f, 0.0f, 2.0f);
	m_Axis_T_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Axis_T_Z->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_T_Z->so.mesh = new Cylinder(m_Axis_T_Z->so.scale);
	m_Axis_T_Z->so.AABB = new AABB(m_Axis_T_Z->so.position, m_Axis_T_Z->so.rotation, m_Axis_T_Z->so.scale);
	m_Axis_T_Z->axes = { false, false, true };
	m_GizmoObjects.push_back(m_Axis_T_Z);

	m_Arrow_T_X = new GizmoObject{};
	m_Arrow_T_X->so.meshType = MESH_TYPE_CONE;
	m_Arrow_T_X->so.color = m_Color_Red;
	m_Arrow_T_X->so.position = m_Position + glm::vec3(4.4f, 0.0f, 0.0f);
	m_Arrow_T_X->so.rotation = glm::vec3(0.0f, 0.0f, -90.0f);
	m_Arrow_T_X->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Arrow_T_X->so.mesh = new Cone(m_Arrow_T_X->so.scale);
	m_Arrow_T_X->so.AABB = new AABB(m_Arrow_T_X->so.position, m_Arrow_T_X->so.rotation, m_Arrow_T_X->so.scale);
	m_Arrow_T_X->axes = { true, false, false };
	m_GizmoObjects.push_back(m_Arrow_T_X);

	m_Arrow_T_Y = new GizmoObject{};
	m_Arrow_T_Y->so.meshType = MESH_TYPE_CONE;
	m_Arrow_T_Y->so.color = m_Color_Green;
	m_Arrow_T_Y->so.position = m_Position + glm::vec3(0.0f, 4.4f, 0.0f);
	m_Arrow_T_Y->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Arrow_T_Y->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Arrow_T_Y->so.mesh = new Cone(m_Arrow_T_Y->so.scale);
	m_Arrow_T_Y->so.AABB = new AABB(m_Arrow_T_Y->so.position, m_Arrow_T_Y->so.rotation, m_Arrow_T_Y->so.scale);
	m_Arrow_T_Y->axes = { false, true, false };
	m_GizmoObjects.push_back(m_Arrow_T_Y);

	m_Arrow_T_Z = new GizmoObject{};
	m_Arrow_T_Z->so.meshType = MESH_TYPE_CONE;
	m_Arrow_T_Z->so.color = m_Color_Blue;
	m_Arrow_T_Z->so.position = m_Position + glm::vec3(0.0f, 0.0f, 4.4f);
	m_Arrow_T_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Arrow_T_Z->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Arrow_T_Z->so.mesh = new Cone(m_Arrow_T_Z->so.scale);
	m_Arrow_T_Z->so.AABB = new AABB(m_Arrow_T_Z->so.position, m_Arrow_T_Z->so.rotation, m_Arrow_T_Z->so.scale);
	m_Arrow_T_Z->axes = { false, false, true };
	m_GizmoObjects.push_back(m_Arrow_T_Z);

	m_Square_T_XY = new GizmoObject{};
	m_Square_T_XY->so.meshType = MESH_TYPE_CUBE;
	m_Square_T_XY->so.color = m_Color_Yellow;
	m_Square_T_XY->so.position = m_Position + glm::vec3(2.0f, 2.0f, 0.0f);
	m_Square_T_XY->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Square_T_XY->so.scale = glm::vec3(1.5f, 1.5f, 0.15f);
	m_Square_T_XY->so.mesh = new Block(m_Square_T_XY->so.scale);
	m_Square_T_XY->so.AABB = new AABB(m_Square_T_XY->so.position, m_Square_T_XY->so.rotation, m_Square_T_XY->so.scale);
	m_Square_T_XY->axes = { false, false, false };
	// m_Square_T_XY->axes = { true, true, false };
	m_GizmoObjects.push_back(m_Square_T_XY);

	m_Square_T_YZ = new GizmoObject{};
	m_Square_T_YZ->so.meshType = MESH_TYPE_CUBE;
	m_Square_T_YZ->so.color = m_Color_Cyan;
	m_Square_T_YZ->so.position = m_Position + glm::vec3(0.0f, 2.0f, 2.0f);
	m_Square_T_YZ->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Square_T_YZ->so.scale = glm::vec3(0.15f, 1.5f, 1.5f);
	m_Square_T_YZ->so.mesh = new Block(m_Square_T_YZ->so.scale);
	m_Square_T_YZ->so.AABB = new AABB(m_Square_T_YZ->so.position, m_Square_T_YZ->so.rotation, m_Square_T_YZ->so.scale);
	m_Square_T_YZ->axes = { false, false, false };
	// m_Square_T_YZ->axes = { false, true, true };
	m_GizmoObjects.push_back(m_Square_T_YZ);

	m_Square_T_ZX = new GizmoObject{};
	m_Square_T_ZX->so.meshType = MESH_TYPE_CUBE;
	m_Square_T_ZX->so.color = m_Color_Magenta;
	m_Square_T_ZX->so.position = m_Position + glm::vec3(2.0f, 0.0f, 2.0f);
	m_Square_T_ZX->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Square_T_ZX->so.scale = glm::vec3(1.5f, 0.15f, 1.5f);
	m_Square_T_ZX->so.mesh = new Block(m_Square_T_ZX->so.scale);
	m_Square_T_ZX->so.AABB = new AABB(m_Square_T_ZX->so.position, m_Square_T_ZX->so.rotation, m_Square_T_ZX->so.scale);
	m_Square_T_ZX->axes = { false, false, false };
	// m_Square_T_ZX->axes = { true, false, true };
	m_GizmoObjects.push_back(m_Square_T_ZX);

	// Initialize Scale meshes
	m_Axis_S_X = new GizmoObject{};
	m_Axis_S_X->so.meshType = MESH_TYPE_CYLINDER;
	m_Axis_S_X->so.color = m_Color_Red;
	m_Axis_S_X->so.position = m_Position + glm::vec3(2.0f, 0.0f, 0.0f);
	m_Axis_S_X->so.rotation = glm::vec3(0.0f, 0.0f, -90.0f);
	m_Axis_S_X->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_S_X->so.mesh = new Cylinder(m_Axis_S_X->so.scale);
	m_Axis_S_X->so.AABB = new AABB(m_Axis_S_X->so.position, m_Axis_S_X->so.rotation, m_Axis_S_X->so.scale);
	m_Axis_S_X->axes = { true, false, false };
	m_GizmoObjects.push_back(m_Axis_S_X);

	m_Axis_S_Y = new GizmoObject{};
	m_Axis_S_Y->so.meshType = MESH_TYPE_CYLINDER;
	m_Axis_S_Y->so.color = m_Color_Green;
	m_Axis_S_Y->so.position = m_Position + glm::vec3(0.0f, 2.0f, 0.0f);
	m_Axis_S_Y->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Axis_S_Y->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_S_Y->so.mesh = new Cylinder(m_Axis_S_Y->so.scale);
	m_Axis_S_Y->so.AABB = new AABB(m_Axis_S_Y->so.position, m_Axis_S_Y->so.rotation, m_Axis_S_Y->so.scale);
	m_Axis_S_Y->axes = { false, true, false };
	m_GizmoObjects.push_back(m_Axis_S_Y);

	m_Axis_S_Z = new GizmoObject{};
	m_Axis_S_Z->so.meshType = MESH_TYPE_CYLINDER;
	m_Axis_S_Z->so.color = m_Color_Blue;
	m_Axis_S_Z->so.position = m_Position + glm::vec3(0.0f, 0.0f, 2.0f);
	m_Axis_S_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Axis_S_Z->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_S_Z->so.mesh = new Cylinder(m_Axis_S_Z->so.scale);
	m_Axis_S_Z->so.AABB = new AABB(m_Axis_S_Z->so.position, m_Axis_S_Z->so.rotation, m_Axis_S_Z->so.scale);
	m_Axis_S_Z->axes = { false, false, true };
	m_GizmoObjects.push_back(m_Axis_S_Z);

	// Initialize Scale Handles
	m_Handle_S_X = new GizmoObject{};
	m_Handle_S_X->so.meshType = MESH_TYPE_CYLINDER;
	m_Handle_S_X->so.color = m_Color_Red;
	m_Handle_S_X->so.position = m_Position + glm::vec3(4.4f, 0.0f, 0.0f);
	m_Handle_S_X->so.rotation = glm::vec3(0.0f, 0.0f, -90.0f);
	m_Handle_S_X->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Handle_S_X->so.mesh = new Cylinder(m_Handle_S_X->so.scale);
	m_Handle_S_X->so.AABB = new AABB(m_Handle_S_X->so.position, m_Handle_S_X->so.rotation, m_Handle_S_X->so.scale);
	m_Handle_S_X->axes = { true, false, false };
	m_GizmoObjects.push_back(m_Handle_S_X);

	m_Handle_S_Y = new GizmoObject{};
	m_Handle_S_Y->so.meshType = MESH_TYPE_CYLINDER;
	m_Handle_S_Y->so.color = m_Color_Green;
	m_Handle_S_Y->so.position = m_Position + glm::vec3(0.0f, 4.4f, 0.0f);
	m_Handle_S_Y->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Handle_S_Y->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Handle_S_Y->so.mesh = new Cylinder(m_Handle_S_Y->so.scale);
	m_Handle_S_Y->so.AABB = new AABB(m_Handle_S_Y->so.position, m_Handle_S_Y->so.rotation, m_Handle_S_Y->so.scale);
	m_Handle_S_Y->axes = { false, true, false };
	m_GizmoObjects.push_back(m_Handle_S_Y);

	m_Handle_S_Z = new GizmoObject{};
	m_Handle_S_Z->so.meshType = MESH_TYPE_CYLINDER;
	m_Handle_S_Z->so.color = m_Color_Blue;
	m_Handle_S_Z->so.position = m_Position + glm::vec3(0.0f, 0.0f, 4.4f);
	m_Handle_S_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Handle_S_Z->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Handle_S_Z->so.mesh = new Cylinder(m_Handle_S_Z->so.scale);
	m_Handle_S_Z->so.AABB = new AABB(m_Handle_S_Z->so.position, m_Handle_S_Z->so.rotation, m_Handle_S_Z->so.scale);
	m_Handle_S_Z->axes = { false, false, true };
	m_GizmoObjects.push_back(m_Handle_S_Z);

	// Initialize Rotation meshes
	m_Ring_R_X = new GizmoObject{};
	m_Ring_R_X->so.meshType = MESH_TYPE_RING;
	m_Ring_R_X->so.color = m_Color_Red;
	m_Ring_R_X->so.position = m_Position + glm::vec3(0.0f, 0.0f, 0.0f);
	m_Ring_R_X->so.rotation = glm::vec3(0.0f, 0.0f, 90.0f);
	m_Ring_R_X->so.scale = glm::vec3(6.0f, 0.2f, 6.0f);
	m_Ring_R_X->so.mesh = new Ring(m_Ring_R_X->so.scale);
	m_Ring_R_X->so.AABB = new AABB(m_Ring_R_X->so.position, m_Ring_R_X->so.rotation, m_Ring_R_X->so.scale);
	m_Ring_R_X->axes = { true, false, false };
	m_GizmoObjects.push_back(m_Ring_R_X);

	m_Ring_R_Y = new GizmoObject{};
	m_Ring_R_Y->so.meshType = MESH_TYPE_RING;
	m_Ring_R_Y->so.color = m_Color_Green;
	m_Ring_R_Y->so.position = m_Position + glm::vec3(0.0f, 0.0f, 0.0f);
	m_Ring_R_Y->so.rotation = glm::vec3(0.0f, 1.0f, 0.0f);
	m_Ring_R_Y->so.scale = glm::vec3(6.0f, 0.2f, 6.0f);
	m_Ring_R_Y->so.mesh = new Ring(m_Ring_R_Y->so.scale);
	m_Ring_R_Y->so.AABB = new AABB(m_Ring_R_Y->so.position, m_Ring_R_Y->so.rotation, m_Ring_R_Y->so.scale);
	m_Ring_R_Y->axes = { false, true, false };
	m_GizmoObjects.push_back(m_Ring_R_Y);

	m_Ring_R_Z = new GizmoObject{};
	m_Ring_R_Z->so.meshType = MESH_TYPE_RING;
	m_Ring_R_Z->so.color = m_Color_Blue;
	m_Ring_R_Z->so.position = m_Position + glm::vec3(0.0f, 0.0f, 0.0f);
	m_Ring_R_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Ring_R_Z->so.scale = glm::vec3(6.0f, 0.2f, 6.0f);
	m_Ring_R_Z->so.mesh = new Ring(m_Ring_R_Z->so.scale);
	m_Ring_R_Z->so.AABB = new AABB(m_Ring_R_Z->so.position, m_Ring_R_Z->so.rotation, m_Ring_R_Z->so.scale);
	m_Ring_R_Z->axes = { false, false, true };
	m_GizmoObjects.push_back(m_Ring_R_Z);
}

void Gizmo::ChangeMode(int mode)
{
	m_Mode = mode;
}

void Gizmo::ToggleMode()
{
	float currentTimestamp = (float)glfwGetTime();
	if (currentTimestamp - m_ToggleMode.lastTime < m_ToggleMode.cooldown) return;
	m_ToggleMode.lastTime = currentTimestamp;

	if (m_Mode == GIZMO_MODE_TRANSLATE) {
		m_Mode = GIZMO_MODE_SCALE;
		// printf("Gizmo::ToggleMode - toggle from %i to %i\n", GIZMO_MODE_TRANSLATE, m_Mode);
		return;
	}
	if (m_Mode == GIZMO_MODE_SCALE) {
		m_Mode = GIZMO_MODE_ROTATE;
		// printf("Gizmo::ToggleMode - toggle from %i to %i\n", GIZMO_MODE_SCALE, m_Mode);
		return;
	}
	if (m_Mode == GIZMO_MODE_ROTATE) {
		m_Mode = GIZMO_MODE_TRANSLATE;
		// printf("Gizmo::ToggleMode - toggle from %i to %i\n", GIZMO_MODE_ROTATE, m_Mode);
		return;
	}
}

void Gizmo::SetSceneObject(SceneObject* sceneObject)
{
	if (m_SceneObject != nullptr && sceneObject->id != m_SceneObject->id)
		m_Mode = GIZMO_MODE_TRANSLATE; // reset mode to translate when swiching to a different object

	m_SceneObject = sceneObject;

	m_Position = sceneObject->position;
	m_Rotation = sceneObject->rotation;
	m_Scale    = sceneObject->scale;
}

void Gizmo::Update(glm::vec3 cameraPosition, Window& mainWindow)
{
	// m_Position = m_SceneObject->position;
	// m_Rotation = m_SceneObject->rotation;
	// m_Scale    = m_SceneObject->scale;

	MousePicker::Get()->GetPointOnRay(cameraPosition, MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

	glm::vec3 currentRay = MousePicker::Get()->GetCurrentRay();
	bool isIntersectionFound = false;
	bool isIntersecting = false;
	int intersectingObjectIndex = -1;
	for (int i = 0; i < m_GizmoObjects.size(); i++) {

		m_GizmoObjects[i]->so.AABB->Update(m_Position + m_GizmoObjects[i]->so.position, m_GizmoObjects[i]->so.rotation, m_GizmoObjects[i]->so.scale);

		isIntersecting = AABB::IntersectRayAab(cameraPosition, currentRay,
			m_GizmoObjects[i]->so.AABB->GetMin(), m_GizmoObjects[i]->so.AABB->GetMax(), glm::vec2(0.0f));

		if (isIntersecting && !isIntersectionFound) {
			// printf("Gizmo::Update CurrentRay [ %.2ff %.2ff %.2ff ]\n", currentRay.x, currentRay.y, currentRay.z);
			m_GizmoObjects[i]->so.AABB->m_Color = m_Color_Red;
			m_GizmoObjects[i]->so.AABB->m_IsColliding = true;
			intersectingObjectIndex = i;
			// printf("m_AxesEnabled [ %d %d %d ]\n", m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z);
			// isIntersectionFound = true;
		} else {
			m_GizmoObjects[i]->so.AABB->m_Color = m_Color_Green;
			m_GizmoObjects[i]->so.AABB->m_IsColliding = false;
		}
	}

	if (intersectingObjectIndex > -1 && intersectingObjectIndex < m_GizmoObjects.size())
		m_AxesEnabled = m_GizmoObjects[intersectingObjectIndex]->axes;
	else
		m_AxesEnabled = { false, false, false };

	if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1]) //  && isIntersectionFound
	{
		// Move objects based on mouse input
		float mouseDeltaX = 0.0f;
		float mouseDeltaY = 0.0f;

		if (std::abs(mainWindow.getXChange()) > 0.1f) {
			mouseDeltaX = mainWindow.getXChange();
		}
		if (std::abs(mainWindow.getYChange()) > 0.1f) {
			mouseDeltaY = mainWindow.getYChange();
		}

		if (m_Mode == GIZMO_MODE_TRANSLATE)
		{
			if (m_AxesEnabled.x && !m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Position.x += (mouseDeltaX + mouseDeltaY) * m_FactorTranslate;
			}
			else if (!m_AxesEnabled.x && m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Position.y += (mouseDeltaX + mouseDeltaY) * m_FactorTranslate;
			}
			else if (!m_AxesEnabled.x && !m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Position.z -= (mouseDeltaX + mouseDeltaY) * m_FactorTranslate;
			}
			else if (m_AxesEnabled.x && m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Position.x += mouseDeltaX * m_FactorTranslate;
				m_Position.y += mouseDeltaY * m_FactorTranslate;
			}
			else if (!m_AxesEnabled.x && m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Position.y += mouseDeltaX * m_FactorTranslate;
				m_Position.z += mouseDeltaY * m_FactorTranslate;
			}
			else if (m_AxesEnabled.x && !m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Position.z += mouseDeltaX * m_FactorTranslate;
				m_Position.x += mouseDeltaY * m_FactorTranslate;
			}
		}

		if (m_Mode == GIZMO_MODE_SCALE)
		{
			if (m_AxesEnabled.x && !m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Scale.x += (mouseDeltaX + mouseDeltaY) * m_FactorScale;
			}
			else if (!m_AxesEnabled.x && m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Scale.y += (mouseDeltaX + mouseDeltaY) * m_FactorScale;
			}
			else if (!m_AxesEnabled.x && !m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Scale.z += (mouseDeltaX + mouseDeltaY) * m_FactorScale;
			}
		}

		if (m_Mode == GIZMO_MODE_ROTATE)
		{
			if (m_AxesEnabled.x && !m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Rotation.x += (mouseDeltaX + mouseDeltaY) * m_FactorRotate;
			}
			else if (!m_AxesEnabled.x && m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Rotation.y += (mouseDeltaX + mouseDeltaY) * m_FactorRotate;
			}
			else if (!m_AxesEnabled.x && !m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Rotation.z += (mouseDeltaX + mouseDeltaY) * m_FactorRotate;
			}
		}

		m_SceneObject->position = m_Position;
		m_SceneObject->rotation = m_Rotation;
		m_SceneObject->scale = m_Scale;

		// printf("m_AxesEnabled [ %d %d %d ]\n", m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z);
		// printf("getXChange [ %.2ff ] getYChange [ %.2ff ]\n", mouseDeltaX, mouseDeltaY);
		// printf("m_Position [ %.2ff %.2ff %.2ff ]\n", m_Position.x, m_Position.y, m_Position.z);
		// printf("m_SceneObject->position [ %.2ff %.2ff %.2ff ]\n", m_SceneObject->position.x, m_SceneObject->position.y, m_SceneObject->position.z);
	}

}

void Gizmo::Render(Shader* shader)
{
	// Render Translation Gizmo
	if (m_Mode == GIZMO_MODE_TRANSLATE)
	{
		// Translation Gizmo - Axes
		m_Axis_T_X->so.transform = Math::CreateTransform(m_Position + m_Axis_T_X->so.position, m_Axis_T_X->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_X->so.transform);
		shader->setVec4("tintColor", m_Axis_T_X->so.AABB->m_IsColliding || m_Arrow_T_X->so.AABB->m_IsColliding ? m_Color_Selected : m_Axis_T_X->so.color);
		m_Axis_T_X->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_X->so.AABB->m_Color);
		// m_Axis_T_X->so.AABB->Draw();

		m_Axis_T_Y->so.transform = Math::CreateTransform(m_Position + m_Axis_T_Y->so.position, m_Axis_T_Y->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_Y->so.transform);
		shader->setVec4("tintColor", m_Axis_T_Y->so.AABB->m_IsColliding || m_Arrow_T_Y->so.AABB->m_IsColliding ? m_Color_Selected : m_Axis_T_Y->so.color);
		m_Axis_T_Y->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_Y->so.AABB->m_Color);
		// m_Axis_T_Y->so.AABB->Draw();

		m_Axis_T_Z->so.transform = Math::CreateTransform(m_Position + m_Axis_T_Z->so.position, m_Axis_T_Z->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_Z->so.transform);
		shader->setVec4("tintColor", m_Axis_T_Z->so.AABB->m_IsColliding || m_Arrow_T_Z->so.AABB->m_IsColliding ? m_Color_Selected : m_Axis_T_Z->so.color);
		m_Axis_T_Z->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_Z->so.AABB->m_Color);
		// m_Axis_T_Z->so.AABB->Draw();

		// Translation Gizmo - Arrows (Cones)
		m_Arrow_T_X->so.transform = Math::CreateTransform(m_Position + m_Arrow_T_X->so.position, m_Arrow_T_X->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_X->so.transform);
		shader->setVec4("tintColor", m_Axis_T_X->so.AABB->m_IsColliding || m_Arrow_T_X->so.AABB->m_IsColliding ? m_Color_Selected : m_Arrow_T_X->so.color);
		m_Arrow_T_X->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_X->so.AABB->m_Color);
		// m_Arrow_T_X->so.AABB->Draw();

		m_Arrow_T_Y->so.transform = Math::CreateTransform(m_Position + m_Arrow_T_Y->so.position, m_Arrow_T_Y->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_Y->so.transform);
		shader->setVec4("tintColor", m_Axis_T_Y->so.AABB->m_IsColliding || m_Arrow_T_Y->so.AABB->m_IsColliding ? m_Color_Selected : m_Arrow_T_Y->so.color);
		m_Arrow_T_Y->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_Y->so.AABB->m_Color);
		// m_Arrow_T_Y->so.AABB->Draw();

		m_Arrow_T_Z->so.transform = Math::CreateTransform(m_Position + m_Arrow_T_Z->so.position, m_Arrow_T_Z->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_Z->so.transform);
		shader->setVec4("tintColor", m_Axis_T_Z->so.AABB->m_IsColliding || m_Arrow_T_Z->so.AABB->m_IsColliding ? m_Color_Selected : m_Arrow_T_Z->so.color);
		m_Arrow_T_Z->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_Z->so.AABB->m_Color);
		// m_Arrow_T_Z->so.AABB->Draw();

		// Translation Gizmo - 2D squares
		m_Square_T_XY->so.transform = Math::CreateTransform(m_Position + m_Square_T_XY->so.position, m_Square_T_XY->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_XY->so.transform);
		shader->setVec4("tintColor", m_Square_T_XY->so.AABB->m_IsColliding ? m_Color_Selected : m_Square_T_XY->so.color);
		m_Square_T_XY->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_XY->so.AABB->m_Color);
		// m_Square_T_XY->so.AABB->Draw();

		m_Square_T_YZ->so.transform = Math::CreateTransform(m_Position + m_Square_T_YZ->so.position, m_Square_T_YZ->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_YZ->so.transform);
		shader->setVec4("tintColor", m_Square_T_YZ->so.AABB->m_IsColliding ? m_Color_Selected : m_Square_T_YZ->so.color);
		m_Square_T_YZ->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_YZ->so.AABB->m_Color);
		// m_Square_T_YZ->so.AABB->Draw();

		m_Square_T_ZX->so.transform = Math::CreateTransform(m_Position + m_Square_T_ZX->so.position, m_Square_T_ZX->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_ZX->so.transform);
		shader->setVec4("tintColor", m_Square_T_ZX->so.AABB->m_IsColliding ? m_Color_Selected : m_Square_T_ZX->so.color);
		m_Square_T_ZX->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_ZX->so.AABB->m_Color);
		// m_Square_T_ZX->so.AABB->Draw();
	}

	// Render Scale Gizmo
	if (m_Mode == GIZMO_MODE_SCALE)
	{
		// Scale Gizmo - Axes
		m_Axis_S_X->so.transform = Math::CreateTransform(m_Position + m_Axis_S_X->so.position, m_Axis_S_X->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_S_X->so.transform);
		shader->setVec4("tintColor", m_Axis_S_X->so.AABB->m_IsColliding || m_Handle_S_X->so.AABB->m_IsColliding ? m_Color_Selected : m_Axis_S_X->so.color);
		m_Axis_S_X->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_S_X->so.AABB->m_Color);
		// m_Axis_S_X->so.AABB->Draw();

		m_Axis_S_Y->so.transform = Math::CreateTransform(m_Position + m_Axis_S_Y->so.position, m_Axis_S_Y->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_S_Y->so.transform);
		shader->setVec4("tintColor", m_Axis_S_Y->so.AABB->m_IsColliding || m_Handle_S_Y->so.AABB->m_IsColliding ? m_Color_Selected : m_Axis_S_Y->so.color);
		m_Axis_S_Y->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_S_Y->so.AABB->m_Color);
		// m_Axis_S_Y->so.AABB->Draw();

		m_Axis_S_Z->so.transform = Math::CreateTransform(m_Position + m_Axis_S_Z->so.position, m_Axis_S_Z->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_S_Z->so.transform);
		shader->setVec4("tintColor", m_Axis_S_Z->so.AABB->m_IsColliding || m_Handle_S_Z->so.AABB->m_IsColliding ? m_Color_Selected : m_Axis_S_Z->so.color);
		m_Axis_S_Z->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_S_Z->so.AABB->m_Color);
		// m_Axis_S_Z->so.AABB->Draw();

		// Scale Gizmo - Handles
		m_Handle_S_X->so.transform = Math::CreateTransform(m_Position + m_Handle_S_X->so.position, m_Handle_S_X->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Handle_S_X->so.transform);
		shader->setVec4("tintColor", m_Axis_S_X->so.AABB->m_IsColliding || m_Handle_S_X->so.AABB->m_IsColliding ? m_Color_Selected : m_Handle_S_X->so.color);
		m_Handle_S_X->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Handle_S_X->so.AABB->m_Color);
		// m_Handle_S_X->so.AABB->Draw();

		m_Handle_S_Y->so.transform = Math::CreateTransform(m_Position + m_Handle_S_Y->so.position, m_Handle_S_Y->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Handle_S_Y->so.transform);
		shader->setVec4("tintColor", m_Axis_S_Y->so.AABB->m_IsColliding || m_Handle_S_Y->so.AABB->m_IsColliding ? m_Color_Selected : m_Handle_S_Y->so.color);
		m_Handle_S_Y->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Handle_S_Y->so.AABB->m_Color);
		// m_Handle_S_Y->so.AABB->Draw();

		m_Handle_S_Z->so.transform = Math::CreateTransform(m_Position + m_Handle_S_Z->so.position, m_Handle_S_Z->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Handle_S_Z->so.transform);
		shader->setVec4("tintColor", m_Axis_S_Z->so.AABB->m_IsColliding || m_Handle_S_Z->so.AABB->m_IsColliding ? m_Color_Selected : m_Handle_S_Z->so.color);
		m_Handle_S_Z->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Handle_S_Z->so.AABB->m_Color);
		// m_Handle_S_Z->so.AABB->Draw();
	}

	// Render Rotate Gizmo
	if (m_Mode == GIZMO_MODE_ROTATE)
	{
		m_Ring_R_X->so.transform = Math::CreateTransform(m_Position + m_Ring_R_X->so.position, m_Ring_R_X->so.rotation, m_Ring_R_X->so.scale);
		shader->setMat4("model", m_Ring_R_X->so.transform);
		shader->setVec4("tintColor", m_Ring_R_X->so.AABB->m_IsColliding ? m_Color_Selected : m_Ring_R_X->so.color);
		m_Ring_R_X->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Ring_R_X->so.AABB->m_Color);
		// m_Ring_R_X->so.AABB->Draw();

		m_Ring_R_Y->so.transform = Math::CreateTransform(m_Position + m_Ring_R_Y->so.position, m_Ring_R_Y->so.rotation, m_Ring_R_Y->so.scale);
		shader->setMat4("model", m_Ring_R_Y->so.transform);
		shader->setVec4("tintColor", m_Ring_R_Y->so.AABB->m_IsColliding ? m_Color_Selected : m_Ring_R_Y->so.color);
		m_Ring_R_Y->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Ring_R_Y->so.AABB->m_Color);
		// m_Ring_R_Y->so.AABB->Draw();

		m_Ring_R_Z->so.transform = Math::CreateTransform(m_Position + m_Ring_R_Z->so.position, m_Ring_R_Z->so.rotation, m_Ring_R_Z->so.scale);
		shader->setMat4("model", m_Ring_R_Z->so.transform);
		shader->setVec4("tintColor", m_Ring_R_Z->so.AABB->m_IsColliding ? m_Color_Selected : m_Ring_R_Z->so.color);
		m_Ring_R_Z->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Ring_R_Z->so.AABB->m_Color);
		// m_Ring_R_Z->so.AABB->Draw();
	}
}

Gizmo::~Gizmo()
{
	// if (m_SceneObject != nullptr)
	delete m_SceneObject;

	for (auto& object : m_GizmoObjects)
	{
		delete object->so.AABB;
		delete object->so.pivot;
		delete object->so.mesh;
	}

	m_GizmoObjects.clear();
}
