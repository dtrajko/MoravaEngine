#include "Gizmo.h"

#include "MousePicker.h"
#include "Math.h"


Gizmo::Gizmo()
{
	m_Mode = GIZMO_MODE_NONE;

	m_Active = false;

	m_MousePress   = { 0.0f, 1.0f };
	m_MouseRelease = { 0.0f, 1.0f };
	m_PrintObjects = { 0.0f, 1.0f };
	m_ChangeActive = { 0.0f, 1.0f };

	m_AxesEnabled = { false, false, false };

	m_Position = glm::vec3(0.0f);
	m_Rotation = glm::vec3(0.0f);
	m_Scale = glm::vec3(1.0f);

	// the scene object controlled by the Gizmo
	m_SceneObject = new SceneObject{};
	m_SceneObject->transform = glm::mat4(1.0f);
	m_SceneObject->position = m_Position;
	m_SceneObject->rotation = m_Rotation;
	m_SceneObject->scale = m_Scale;
	m_SceneObject->AABB = new AABB(m_Position, m_Rotation, m_Scale);
	m_SceneObject->pivot = new Pivot(m_Position, m_Scale);
	m_SceneObject->color = glm::vec4(1.0f);

	CreateObjects();
}

void Gizmo::SetSceneObject(SceneObject* sceneObject)
{
	if (m_SceneObject != nullptr && sceneObject->id != m_SceneObject->id)
		m_Mode = GIZMO_MODE_NONE; // reset mode to NONE when swiching to a different object

	m_SceneObject = sceneObject;
	UpdateTransformFromObject(sceneObject);

	if (m_GizmoObjects.empty())
		CreateObjects();
}

void Gizmo::UpdateTransformFromObject(SceneObject* sceneObject)
{
	if (sceneObject != nullptr)
	{
		m_Position = sceneObject->position;
		m_Rotation = sceneObject->rotation;
		m_Scale    = sceneObject->scale;
	}
}

void Gizmo::SetActive(bool active)
{
	float currentTimestamp = (float)glfwGetTime();
	if (currentTimestamp - m_ChangeActive.lastTime < m_ChangeActive.cooldown) return;
	m_ChangeActive.lastTime = currentTimestamp;

	if (!active)
	{
		m_SceneObject = nullptr;
		ChangeMode(GIZMO_MODE_NONE);
	}

	if (active && !m_Active)
		ChangeMode(GIZMO_MODE_NONE);

	m_Active = active;

	printf("Gizmo::SetActive m_Active = %d m_Mode %s\n", m_Active, GetModeDescriptive().c_str());
}

std::string Gizmo::GetModeDescriptive(int modeID)
{
	if (modeID == -1) modeID = m_Mode;
	if (modeID == GIZMO_MODE_NONE)      return "GIZMO_MODE_NONE";
	if (modeID == GIZMO_MODE_TRANSLATE) return "GIZMO_MODE_TRANSLATE";
	if (modeID == GIZMO_MODE_SCALE)     return "GIZMO_MODE_SCALE";
	if (modeID == GIZMO_MODE_ROTATE)    return "GIZMO_MODE_ROTATE";
	return "Undefined";
}

void Gizmo::OnMousePress(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex)
{
	float currentTimestamp = (float)glfwGetTime();
	if (currentTimestamp - m_MousePress.lastTime < m_MousePress.cooldown) return;
	m_MousePress.lastTime = currentTimestamp;

	// printf("Gizmo::OnMouseClick\n");

	if (sceneObjects->size() > 0 && sceneObjects->at(selectedIndex)->isSelected)
	{
	}
}

void Gizmo::OnMouseRelease(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex)
{
	float currentTimestamp = (float)glfwGetTime();
	if (currentTimestamp - m_MouseRelease.lastTime < m_MouseRelease.cooldown) return;
	m_MouseRelease.lastTime = currentTimestamp;

	// printf("Gizmo::OnMouseRelease\n");

	if (sceneObjects->size() > 0 && sceneObjects->at(selectedIndex)->isSelected)
	{
		SceneObject* sceneObject = nullptr;
		if (selectedIndex < (unsigned int)sceneObjects->size())
			sceneObject = sceneObjects->at(selectedIndex);

		if (sceneObject == nullptr) return;

		bool canToggleGizmo = false;

		// start rules for switching / toggling the Gizmo
		if (m_SceneObject == nullptr)
			canToggleGizmo = true;

		if (sceneObjects->size() == 1)
			canToggleGizmo = true;

		if (m_SceneObject != nullptr && mainWindow.getKeys()[GLFW_KEY_TAB])
			canToggleGizmo = true;

		if (canToggleGizmo)
		{
			SetActive(true);
			SetSceneObject(sceneObject);
			ToggleMode();
		}
	}
}

void Gizmo::ChangeMode(int mode)
{
	m_Mode = mode;
	UpdateTransformFromObject(m_SceneObject);
	CreateObjects();
}

void Gizmo::ToggleMode()
{
	int oldMode = m_Mode;

	if (!m_Active)
	{
		m_Mode = GIZMO_MODE_NONE;
		return;
	}

	if (m_Mode == GIZMO_MODE_NONE) {
		m_Mode = GIZMO_MODE_TRANSLATE;
	}
	else if (m_Mode == GIZMO_MODE_TRANSLATE) {
		m_Mode = GIZMO_MODE_SCALE;
	}
	else if (m_Mode == GIZMO_MODE_SCALE) {
		m_Mode = GIZMO_MODE_ROTATE;
	}
	else if (m_Mode == GIZMO_MODE_ROTATE) {
		m_Mode = GIZMO_MODE_NONE;
	}

	printf("Gizmo::ToggleMode m_Active = %d oldMode = %s m_Mode = %s m_GizmoObjects.size = %zu\n",
		m_Active, GetModeDescriptive(oldMode).c_str(), GetModeDescriptive().c_str(), m_GizmoObjects.size());

	UpdateTransformFromObject(m_SceneObject);
	CreateObjects();
}

void Gizmo::PrintObjects()
{
	float currentTimestamp = (float)glfwGetTime();
	if (currentTimestamp - m_PrintObjects.lastTime < m_PrintObjects.cooldown) return;
	m_PrintObjects.lastTime = currentTimestamp;

	for (size_t i = 0; i < m_GizmoObjects.size(); i++)
	{
		printf("Gizmo Object\ti = %zu\tSO.id = %i\tName = '%s'\t\tAxes [ %d %d %d ]\n",
			i, m_GizmoObjects[i]->so.id, m_GizmoObjects[i]->name.c_str(), m_GizmoObjects[i]->axes.x, m_GizmoObjects[i]->axes.y, m_GizmoObjects[i]->axes.z);
	}
}

void Gizmo::Update(glm::vec3 cameraPosition, Window& mainWindow)
{
	// Update Gizmo if in Active status (visible)
	if (m_Active)
		UpdateActive(cameraPosition, mainWindow);

	// Update Gizmo if not in Active status (invisible)

	if (mainWindow.getKeys()[GLFW_KEY_P])
		PrintObjects();
}

void Gizmo::UpdateActive(glm::vec3 cameraPosition, Window& mainWindow)
{
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
			m_AxesEnabled = m_GizmoObjects[i]->axes;

			// printf("m_AxesEnabled [ %d %d %d ]\n", m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z);
			// isIntersectionFound = true;
		}
		else {
			m_GizmoObjects[i]->so.AABB->m_Color = m_Color_Green;
			m_GizmoObjects[i]->so.AABB->m_IsColliding = false;
		}
	}

	// printf("m_AxesEnabled [ %d %d %d ] intersectingObjectIndex = %i m_GizmoObjects.size = %zu\n",
	// 	m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z, intersectingObjectIndex, m_GizmoObjects.size());

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

		float shiftSpeed = 1.0f;
		if (mainWindow.getKeys()[GLFW_KEY_LEFT_SHIFT])
			shiftSpeed = m_ShiftSpeed;

		if (m_Mode == GIZMO_MODE_TRANSLATE)
		{
			if (m_AxesEnabled.x && !m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Position.x += (mouseDeltaX + mouseDeltaY) * m_FactorTranslate * shiftSpeed;
				// printf("Gizmo::UpdateActive m_AxesEnabled [ %d %d %d mouseDeltaX [ %.2ff %.2ff ]\n",
				// 	m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z, mouseDeltaX, mouseDeltaY);
			}
			else if (!m_AxesEnabled.x && m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Position.y += (mouseDeltaX + mouseDeltaY) * m_FactorTranslate * shiftSpeed;
				// printf("Gizmo::UpdateActive m_AxesEnabled [ %d %d %d mouseDeltaX [ %.2ff %.2ff ]\n",
				// 	m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z, mouseDeltaX, mouseDeltaY);
			}
			else if (!m_AxesEnabled.x && !m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Position.z -= (mouseDeltaX + mouseDeltaY) * m_FactorTranslate * shiftSpeed;
				// printf("Gizmo::UpdateActive m_AxesEnabled [ %d %d %d mouseDeltaX [ %.2ff %.2ff ]\n",
				// 	m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z, mouseDeltaX, mouseDeltaY);
			}
			else if (m_AxesEnabled.x && m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Position.x += mouseDeltaX * m_FactorTranslate * shiftSpeed;
				m_Position.y += mouseDeltaY * m_FactorTranslate * shiftSpeed;
				// printf("Gizmo::UpdateActive m_AxesEnabled [ %d %d %d mouseDeltaX [ %.2ff %.2ff ]\n",
				// 	m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z, mouseDeltaX, mouseDeltaY);
			}
			else if (!m_AxesEnabled.x && m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Position.y += mouseDeltaY * m_FactorTranslate * shiftSpeed;
				m_Position.z -= mouseDeltaX * m_FactorTranslate * shiftSpeed;
				// printf("Gizmo::UpdateActive m_AxesEnabled [ %d %d %d mouseDeltaX [ %.2ff %.2ff ]\n",
				// 	m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z, mouseDeltaX, mouseDeltaY);
			}
			else if (m_AxesEnabled.x && !m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Position.z -= mouseDeltaY * m_FactorTranslate * shiftSpeed;
				m_Position.x += mouseDeltaX * m_FactorTranslate * shiftSpeed;
				// printf("Gizmo::UpdateActive m_AxesEnabled [ %d %d %d mouseDeltaX [ %.2ff %.2ff ]\n",
				// 	m_AxesEnabled.x, m_AxesEnabled.y, m_AxesEnabled.z, mouseDeltaX, mouseDeltaY);
			}
		}

		if (m_Mode == GIZMO_MODE_SCALE)
		{
			if (m_AxesEnabled.x && !m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Scale.x += (mouseDeltaX + mouseDeltaY) * m_FactorScale * shiftSpeed;
			}
			else if (!m_AxesEnabled.x && m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Scale.y += (mouseDeltaX + mouseDeltaY) * m_FactorScale * shiftSpeed;
			}
			else if (!m_AxesEnabled.x && !m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Scale.z -= (mouseDeltaX + mouseDeltaY) * m_FactorScale * shiftSpeed;
			}
		}

		if (m_Mode == GIZMO_MODE_ROTATE)
		{
			if (m_AxesEnabled.x && !m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Rotation.x += (mouseDeltaX + mouseDeltaY) * m_FactorRotate * shiftSpeed;
			}
			else if (!m_AxesEnabled.x && m_AxesEnabled.y && !m_AxesEnabled.z) {
				m_Rotation.y += (mouseDeltaX + mouseDeltaY) * m_FactorRotate * shiftSpeed;
			}
			else if (!m_AxesEnabled.x && !m_AxesEnabled.y && m_AxesEnabled.z) {
				m_Rotation.z += (mouseDeltaX + mouseDeltaY) * m_FactorRotate * shiftSpeed;
			}
		}

		m_SceneObject->position = m_Position;
		m_SceneObject->rotation = m_Rotation;
		m_SceneObject->scale = m_Scale;
	}
}

void Gizmo::Render(Shader* shader)
{
	if (!m_Active || m_Mode == GIZMO_MODE_NONE) return;

	if (m_GizmoObjects.empty())
		CreateObjects();

	shader->Bind();
	// glDepthMask(GL_FALSE);

	// Render Translation Gizmo
	if (m_Mode == GIZMO_MODE_TRANSLATE)
	{
		// Translation Gizmo - Axes
		m_Axis_T_X->so.transform = Math::CreateTransform(m_Position + m_Axis_T_X->so.position, m_Axis_T_X->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_X->so.transform);
		shader->setVec4("tintColor", m_AxesEnabled.x ? m_Color_Selected : m_Axis_T_X->so.color);
		m_Axis_T_X->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_X->so.AABB->m_Color);
		// m_Axis_T_X->so.AABB->Draw();

		m_Axis_T_Y->so.transform = Math::CreateTransform(m_Position + m_Axis_T_Y->so.position, m_Axis_T_Y->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_Y->so.transform);
		shader->setVec4("tintColor", m_AxesEnabled.y ? m_Color_Selected : m_Axis_T_Y->so.color);
		m_Axis_T_Y->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_Y->so.AABB->m_Color);
		// m_Axis_T_Y->so.AABB->Draw();

		m_Axis_T_Z->so.transform = Math::CreateTransform(m_Position + m_Axis_T_Z->so.position, m_Axis_T_Z->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_Z->so.transform);
		shader->setVec4("tintColor", m_AxesEnabled.z ? m_Color_Selected : m_Axis_T_Z->so.color);
		m_Axis_T_Z->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_Z->so.AABB->m_Color);
		// m_Axis_T_Z->so.AABB->Draw();

		// Translation Gizmo - Arrows (Cones)
		m_Arrow_T_X->so.transform = Math::CreateTransform(m_Position + m_Arrow_T_X->so.position, m_Arrow_T_X->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_X->so.transform);
		shader->setVec4("tintColor", m_AxesEnabled.x ? m_Color_Selected : m_Arrow_T_X->so.color);
		m_Arrow_T_X->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_X->so.AABB->m_Color);
		// m_Arrow_T_X->so.AABB->Draw();

		m_Arrow_T_Y->so.transform = Math::CreateTransform(m_Position + m_Arrow_T_Y->so.position, m_Arrow_T_Y->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_Y->so.transform);
		shader->setVec4("tintColor", m_AxesEnabled.y ? m_Color_Selected : m_Arrow_T_Y->so.color);
		m_Arrow_T_Y->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_Y->so.AABB->m_Color);
		// m_Arrow_T_Y->so.AABB->Draw();

		m_Arrow_T_Z->so.transform = Math::CreateTransform(m_Position + m_Arrow_T_Z->so.position, m_Arrow_T_Z->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_Z->so.transform);
		shader->setVec4("tintColor", m_AxesEnabled.z ? m_Color_Selected : m_Arrow_T_Z->so.color);
		m_Arrow_T_Z->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_Z->so.AABB->m_Color);
		// m_Arrow_T_Z->so.AABB->Draw();

		// Translation Gizmo - 2D squares
		m_Square_T_YZ->so.transform = Math::CreateTransform(m_Position + m_Square_T_YZ->so.position, m_Square_T_YZ->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_YZ->so.transform);
		shader->setVec4("tintColor", (m_AxesEnabled.y && m_AxesEnabled.z) ? m_Color_Selected : m_Square_T_YZ->so.color);
		m_Square_T_YZ->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_YZ->so.AABB->m_Color);
		// m_Square_T_YZ->so.AABB->Draw();

		m_Square_T_ZX->so.transform = Math::CreateTransform(m_Position + m_Square_T_ZX->so.position, m_Square_T_ZX->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_ZX->so.transform);
		shader->setVec4("tintColor", (m_AxesEnabled.z && m_AxesEnabled.x) ? m_Color_Selected : m_Square_T_ZX->so.color);
		m_Square_T_ZX->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_ZX->so.AABB->m_Color);
		// m_Square_T_ZX->so.AABB->Draw();

		m_Square_T_XY->so.transform = Math::CreateTransform(m_Position + m_Square_T_XY->so.position, m_Square_T_XY->so.rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_XY->so.transform);
		shader->setVec4("tintColor", (m_AxesEnabled.x && m_AxesEnabled.y) ? m_Color_Selected : m_Square_T_XY->so.color);
		m_Square_T_XY->so.mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_XY->so.AABB->m_Color);
		// m_Square_T_XY->so.AABB->Draw();
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

	// glDepthMask(GL_TRUE);
}

void Gizmo::CreateObjects()
{
	if (!m_Active) return;

	m_GizmoObjects.clear();

	// Initialize Translation meshes
	if (m_Mode == GIZMO_MODE_TRANSLATE)
	{
		m_Axis_T_X = new GizmoObject{};
		m_Axis_T_X->so.id = (int)m_GizmoObjects.size();
		m_Axis_T_X->so.meshType = MESH_TYPE_CYLINDER;
		m_Axis_T_X->so.color = m_Color_Red;
		m_Axis_T_X->so.position = glm::vec3(2.0f, 0.0f, 0.0f);
		m_Axis_T_X->so.rotation = glm::vec3(0.0f, 0.0f, -90.0f);
		m_Axis_T_X->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
		m_Axis_T_X->so.mesh = new Cylinder(m_Axis_T_X->so.scale);
		m_Axis_T_X->so.AABB = new AABB(m_Axis_T_X->so.position, m_Axis_T_X->so.rotation, m_Axis_T_X->so.scale);
		m_Axis_T_X->axes = { true, false, false };
		m_Axis_T_X->name = "m_Axis_T_X";
		m_GizmoObjects.push_back(m_Axis_T_X);

		m_Axis_T_Y = new GizmoObject{};
		m_Axis_T_Y->so.id = (int)m_GizmoObjects.size();
		m_Axis_T_Y->so.meshType = MESH_TYPE_CYLINDER;
		m_Axis_T_Y->so.color = m_Color_Green;
		m_Axis_T_Y->so.position = glm::vec3(0.0f, 2.0f, 0.0f);
		m_Axis_T_Y->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Axis_T_Y->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
		m_Axis_T_Y->so.mesh = new Cylinder(m_Axis_T_Y->so.scale);
		m_Axis_T_Y->so.AABB = new AABB(m_Axis_T_Y->so.position, m_Axis_T_Y->so.rotation, m_Axis_T_Y->so.scale);
		m_Axis_T_Y->axes = { false, true, false };
		m_Axis_T_Y->name = "m_Axis_T_Y";
		m_GizmoObjects.push_back(m_Axis_T_Y);

		m_Axis_T_Z = new GizmoObject{};
		m_Axis_T_Z->so.id = (int)m_GizmoObjects.size();
		m_Axis_T_Z->so.meshType = MESH_TYPE_CYLINDER;
		m_Axis_T_Z->so.color = m_Color_Blue;
		m_Axis_T_Z->so.position = glm::vec3(0.0f, 0.0f, 2.0f);
		m_Axis_T_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
		m_Axis_T_Z->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
		m_Axis_T_Z->so.mesh = new Cylinder(m_Axis_T_Z->so.scale);
		m_Axis_T_Z->so.AABB = new AABB(m_Axis_T_Z->so.position, m_Axis_T_Z->so.rotation, m_Axis_T_Z->so.scale);
		m_Axis_T_Z->axes = { false, false, true };
		m_Axis_T_Z->name = "m_Axis_T_Z";
		m_GizmoObjects.push_back(m_Axis_T_Z);

		m_Arrow_T_X = new GizmoObject{};
		m_Arrow_T_X->so.id = (int)m_GizmoObjects.size();
		m_Arrow_T_X->so.meshType = MESH_TYPE_CONE;
		m_Arrow_T_X->so.color = m_Color_Red;
		m_Arrow_T_X->so.position = glm::vec3(4.4f, 0.0f, 0.0f);
		m_Arrow_T_X->so.rotation = glm::vec3(0.0f, 0.0f, -90.0f);
		m_Arrow_T_X->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
		m_Arrow_T_X->so.mesh = new Cone(m_Arrow_T_X->so.scale);
		m_Arrow_T_X->so.AABB = new AABB(m_Arrow_T_X->so.position, m_Arrow_T_X->so.rotation, m_Arrow_T_X->so.scale);
		m_Arrow_T_X->axes = { true, false, false };
		m_Arrow_T_X->name = "m_Arrow_T_X";
		m_GizmoObjects.push_back(m_Arrow_T_X);

		m_Arrow_T_Y = new GizmoObject{};
		m_Arrow_T_Y->so.id = (int)m_GizmoObjects.size();
		m_Arrow_T_Y->so.meshType = MESH_TYPE_CONE;
		m_Arrow_T_Y->so.color = m_Color_Green;
		m_Arrow_T_Y->so.position = glm::vec3(0.0f, 4.4f, 0.0f);
		m_Arrow_T_Y->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Arrow_T_Y->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
		m_Arrow_T_Y->so.mesh = new Cone(m_Arrow_T_Y->so.scale);
		m_Arrow_T_Y->so.AABB = new AABB(m_Arrow_T_Y->so.position, m_Arrow_T_Y->so.rotation, m_Arrow_T_Y->so.scale);
		m_Arrow_T_Y->axes = { false, true, false };
		m_Arrow_T_Y->name = "m_Arrow_T_Y";
		m_GizmoObjects.push_back(m_Arrow_T_Y);

		m_Arrow_T_Z = new GizmoObject{};
		m_Arrow_T_Z->so.id = (int)m_GizmoObjects.size();
		m_Arrow_T_Z->so.meshType = MESH_TYPE_CONE;
		m_Arrow_T_Z->so.color = m_Color_Blue;
		m_Arrow_T_Z->so.position = glm::vec3(0.0f, 0.0f, 4.4f);
		m_Arrow_T_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
		m_Arrow_T_Z->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
		m_Arrow_T_Z->so.mesh = new Cone(m_Arrow_T_Z->so.scale);
		m_Arrow_T_Z->so.AABB = new AABB(m_Arrow_T_Z->so.position, m_Arrow_T_Z->so.rotation, m_Arrow_T_Z->so.scale);
		m_Arrow_T_Z->axes = { false, false, true };
		m_Arrow_T_Z->name = "m_Arrow_T_Z";
		m_GizmoObjects.push_back(m_Arrow_T_Z);

		m_Square_T_YZ = new GizmoObject{};
		m_Square_T_YZ->so.id = (int)m_GizmoObjects.size();
		m_Square_T_YZ->so.meshType = MESH_TYPE_CUBE;
		m_Square_T_YZ->so.color = m_Color_Cyan;
		m_Square_T_YZ->so.position = glm::vec3(0.0f, 2.0f, 2.0f);
		m_Square_T_YZ->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Square_T_YZ->so.scale = glm::vec3(0.15f, 1.5f, 1.5f);
		m_Square_T_YZ->so.mesh = new Block(m_Square_T_YZ->so.scale);
		m_Square_T_YZ->so.AABB = new AABB(m_Square_T_YZ->so.position, m_Square_T_YZ->so.rotation, m_Square_T_YZ->so.scale);
		m_Square_T_YZ->axes = { false, true, true };
		m_Square_T_YZ->name = "m_Square_T_YZ";
		m_GizmoObjects.push_back(m_Square_T_YZ);

		m_Square_T_ZX = new GizmoObject{};
		m_Square_T_ZX->so.id = (int)m_GizmoObjects.size();
		m_Square_T_ZX->so.meshType = MESH_TYPE_CUBE;
		m_Square_T_ZX->so.color = m_Color_Magenta;
		m_Square_T_ZX->so.position = glm::vec3(2.0f, 0.0f, 2.0f);
		m_Square_T_ZX->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Square_T_ZX->so.scale = glm::vec3(1.5f, 0.15f, 1.5f);
		m_Square_T_ZX->so.mesh = new Block(m_Square_T_ZX->so.scale);
		m_Square_T_ZX->so.AABB = new AABB(m_Square_T_ZX->so.position, m_Square_T_ZX->so.rotation, m_Square_T_ZX->so.scale);
		m_Square_T_ZX->axes = { true, false, true };
		m_Square_T_ZX->name = "m_Square_T_ZX";
		m_GizmoObjects.push_back(m_Square_T_ZX);

		m_Square_T_XY = new GizmoObject{};
		m_Square_T_XY->so.id = (int)m_GizmoObjects.size();
		m_Square_T_XY->so.meshType = MESH_TYPE_CUBE;
		m_Square_T_XY->so.color = m_Color_Yellow;
		m_Square_T_XY->so.position = glm::vec3(2.0f, 2.0f, 0.0f);
		m_Square_T_XY->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Square_T_XY->so.scale = glm::vec3(1.5f, 1.5f, 0.15f);
		m_Square_T_XY->so.mesh = new Block(m_Square_T_XY->so.scale);
		m_Square_T_XY->so.AABB = new AABB(m_Square_T_XY->so.position, m_Square_T_XY->so.rotation, m_Square_T_XY->so.scale);
		m_Square_T_XY->axes = { true, true, false };
		m_Square_T_XY->name = "m_Square_T_XY";
		m_GizmoObjects.push_back(m_Square_T_XY);
	}

	// Initialize Scale meshes
	if (m_Mode == GIZMO_MODE_SCALE)
	{
		m_Axis_S_X = new GizmoObject{};
		m_Axis_S_X->so.id = (int)m_GizmoObjects.size();
		m_Axis_S_X->so.meshType = MESH_TYPE_CYLINDER;
		m_Axis_S_X->so.color = m_Color_Red;
		m_Axis_S_X->so.position = glm::vec3(2.0f, 0.0f, 0.0f);
		m_Axis_S_X->so.rotation = glm::vec3(0.0f, 0.0f, -90.0f);
		m_Axis_S_X->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
		m_Axis_S_X->so.mesh = new Cylinder(m_Axis_S_X->so.scale);
		m_Axis_S_X->so.AABB = new AABB(m_Axis_S_X->so.position, m_Axis_S_X->so.rotation, m_Axis_S_X->so.scale);
		m_Axis_S_X->axes = { true, false, false };
		m_Axis_S_X->name = "m_Axis_S_X";
		m_GizmoObjects.push_back(m_Axis_S_X);

		m_Axis_S_Y = new GizmoObject{};
		m_Axis_S_Y->so.id = (int)m_GizmoObjects.size();
		m_Axis_S_Y->so.meshType = MESH_TYPE_CYLINDER;
		m_Axis_S_Y->so.color = m_Color_Green;
		m_Axis_S_Y->so.position = glm::vec3(0.0f, 2.0f, 0.0f);
		m_Axis_S_Y->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Axis_S_Y->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
		m_Axis_S_Y->so.mesh = new Cylinder(m_Axis_S_Y->so.scale);
		m_Axis_S_Y->so.AABB = new AABB(m_Axis_S_Y->so.position, m_Axis_S_Y->so.rotation, m_Axis_S_Y->so.scale);
		m_Axis_S_Y->axes = { false, true, false };
		m_Axis_S_Y->name = "m_Axis_S_Y";
		m_GizmoObjects.push_back(m_Axis_S_Y);

		m_Axis_S_Z = new GizmoObject{};
		m_Axis_S_Z->so.id = (int)m_GizmoObjects.size();
		m_Axis_S_Z->so.meshType = MESH_TYPE_CYLINDER;
		m_Axis_S_Z->so.color = m_Color_Blue;
		m_Axis_S_Z->so.position = glm::vec3(0.0f, 0.0f, 2.0f);
		m_Axis_S_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
		m_Axis_S_Z->so.scale = glm::vec3(0.2f, 4.0f, 0.2f);
		m_Axis_S_Z->so.mesh = new Cylinder(m_Axis_S_Z->so.scale);
		m_Axis_S_Z->so.AABB = new AABB(m_Axis_S_Z->so.position, m_Axis_S_Z->so.rotation, m_Axis_S_Z->so.scale);
		m_Axis_S_Z->axes = { false, false, true };
		m_Axis_S_Z->name = "m_Axis_S_Z";
		m_GizmoObjects.push_back(m_Axis_S_Z);

		// Initialize Scale Handles
		m_Handle_S_X = new GizmoObject{};
		m_Handle_S_X->so.id = (int)m_GizmoObjects.size();
		m_Handle_S_X->so.meshType = MESH_TYPE_CYLINDER;
		m_Handle_S_X->so.color = m_Color_Red;
		m_Handle_S_X->so.position = glm::vec3(4.4f, 0.0f, 0.0f);
		m_Handle_S_X->so.rotation = glm::vec3(0.0f, 0.0f, -90.0f);
		m_Handle_S_X->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
		m_Handle_S_X->so.mesh = new Cylinder(m_Handle_S_X->so.scale);
		m_Handle_S_X->so.AABB = new AABB(m_Handle_S_X->so.position, m_Handle_S_X->so.rotation, m_Handle_S_X->so.scale);
		m_Handle_S_X->axes = { true, false, false };
		m_Handle_S_X->name = "m_Handle_S_X";
		m_GizmoObjects.push_back(m_Handle_S_X);

		m_Handle_S_Y = new GizmoObject{};
		m_Handle_S_Y->so.id = (int)m_GizmoObjects.size();
		m_Handle_S_Y->so.meshType = MESH_TYPE_CYLINDER;
		m_Handle_S_Y->so.color = m_Color_Green;
		m_Handle_S_Y->so.position = glm::vec3(0.0f, 4.4f, 0.0f);
		m_Handle_S_Y->so.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Handle_S_Y->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
		m_Handle_S_Y->so.mesh = new Cylinder(m_Handle_S_Y->so.scale);
		m_Handle_S_Y->so.AABB = new AABB(m_Handle_S_Y->so.position, m_Handle_S_Y->so.rotation, m_Handle_S_Y->so.scale);
		m_Handle_S_Y->axes = { false, true, false };
		m_Handle_S_Y->name = "m_Handle_S_Y";
		m_GizmoObjects.push_back(m_Handle_S_Y);

		m_Handle_S_Z = new GizmoObject{};
		m_Handle_S_Z->so.id = (int)m_GizmoObjects.size();
		m_Handle_S_Z->so.meshType = MESH_TYPE_CYLINDER;
		m_Handle_S_Z->so.color = m_Color_Blue;
		m_Handle_S_Z->so.position = glm::vec3(0.0f, 0.0f, 4.4f);
		m_Handle_S_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
		m_Handle_S_Z->so.scale = glm::vec3(0.6f, 0.8f, 0.6f);
		m_Handle_S_Z->so.mesh = new Cylinder(m_Handle_S_Z->so.scale);
		m_Handle_S_Z->so.AABB = new AABB(m_Handle_S_Z->so.position, m_Handle_S_Z->so.rotation, m_Handle_S_Z->so.scale);
		m_Handle_S_Z->axes = { false, false, true };
		m_Handle_S_Z->name = "m_Handle_S_Z";
		m_GizmoObjects.push_back(m_Handle_S_Z);
	}

	// Initialize Rotation meshes
	if (m_Mode == GIZMO_MODE_ROTATE)
	{
		m_Ring_R_X = new GizmoObject{};
		m_Ring_R_X->so.id = (int)m_GizmoObjects.size();
		m_Ring_R_X->so.meshType = MESH_TYPE_RING;
		m_Ring_R_X->so.color = m_Color_Red;
		m_Ring_R_X->so.position = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Ring_R_X->so.rotation = glm::vec3(0.0f, 0.0f, 90.0f);
		m_Ring_R_X->so.scale = glm::vec3(6.0f, 0.2f, 6.0f);
		m_Ring_R_X->so.mesh = new Ring(m_Ring_R_X->so.scale);
		m_Ring_R_X->so.AABB = new AABB(m_Ring_R_X->so.position, m_Ring_R_X->so.rotation, m_Ring_R_X->so.scale);
		m_Ring_R_X->axes = { true, false, false };
		m_Ring_R_X->name = "m_Ring_R_X";
		m_GizmoObjects.push_back(m_Ring_R_X);

		m_Ring_R_Y = new GizmoObject{};
		m_Ring_R_Y->so.id = (int)m_GizmoObjects.size();
		m_Ring_R_Y->so.meshType = MESH_TYPE_RING;
		m_Ring_R_Y->so.color = m_Color_Green;
		m_Ring_R_Y->so.position = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Ring_R_Y->so.rotation = glm::vec3(0.0f, 1.0f, 0.0f);
		m_Ring_R_Y->so.scale = glm::vec3(6.0f, 0.2f, 6.0f);
		m_Ring_R_Y->so.mesh = new Ring(m_Ring_R_Y->so.scale);
		m_Ring_R_Y->so.AABB = new AABB(m_Ring_R_Y->so.position, m_Ring_R_Y->so.rotation, m_Ring_R_Y->so.scale);
		m_Ring_R_Y->axes = { false, true, false };
		m_Ring_R_Y->name = "m_Ring_R_Y";
		m_GizmoObjects.push_back(m_Ring_R_Y);

		m_Ring_R_Z = new GizmoObject{};
		m_Ring_R_Z->so.id = (int)m_GizmoObjects.size();
		m_Ring_R_Z->so.meshType = MESH_TYPE_RING;
		m_Ring_R_Z->so.color = m_Color_Blue;
		m_Ring_R_Z->so.position = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Ring_R_Z->so.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
		m_Ring_R_Z->so.scale = glm::vec3(6.0f, 0.2f, 6.0f);
		m_Ring_R_Z->so.mesh = new Ring(m_Ring_R_Z->so.scale);
		m_Ring_R_Z->so.AABB = new AABB(m_Ring_R_Z->so.position, m_Ring_R_Z->so.rotation, m_Ring_R_Z->so.scale);
		m_Ring_R_Z->axes = { false, false, true };
		m_Ring_R_Z->name = "m_Ring_R_Z";
		m_GizmoObjects.push_back(m_Ring_R_Z);
	}

	printf("Gizmo::CreateObjects m_Active: %d Mode: %s Object Count: %zu\n", m_Active, GetModeDescriptive().c_str(), m_GizmoObjects.size());
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
