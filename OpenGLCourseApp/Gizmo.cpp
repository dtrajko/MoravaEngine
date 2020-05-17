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

	m_ActiveAxis = -1;

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
	m_Axis_T_X = new SceneObject{};
	m_Axis_T_X->meshType = MESH_TYPE_CYLINDER;
	m_Axis_T_X->color = m_Color_Red;
	m_Axis_T_X->position = m_Position + glm::vec3(2.0f, 0.0f, 0.0f);
	m_Axis_T_X->rotation = glm::vec3(0.0f, 0.0f, -90.0f);
	m_Axis_T_X->scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_T_X->mesh = new Cylinder(m_Axis_T_X->scale);
	m_Axis_T_X->AABB = new AABB(m_Axis_T_X->position, m_Axis_T_X->rotation, m_Axis_T_X->scale);
	m_SceneObjects.push_back(m_Axis_T_X);

	m_Axis_T_Y = new SceneObject{};
	m_Axis_T_Y->meshType = MESH_TYPE_CYLINDER;
	m_Axis_T_Y->color = m_Color_Green;
	m_Axis_T_Y->position = m_Position + glm::vec3(0.0f, 2.0f, 0.0f);
	m_Axis_T_Y->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Axis_T_Y->scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_T_Y->mesh = new Cylinder(m_Axis_T_Y->scale);
	m_Axis_T_Y->AABB = new AABB(m_Axis_T_Y->position, m_Axis_T_Y->rotation, m_Axis_T_Y->scale);
	m_SceneObjects.push_back(m_Axis_T_Y);

	m_Axis_T_Z = new SceneObject{};
	m_Axis_T_Z->meshType = MESH_TYPE_CYLINDER;
	m_Axis_T_Z->color = m_Color_Blue;
	m_Axis_T_Z->position = m_Position + glm::vec3(0.0f, 0.0f, 2.0f);
	m_Axis_T_Z->rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Axis_T_Z->scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_T_Z->mesh = new Cylinder(m_Axis_T_Z->scale);
	m_Axis_T_Z->AABB = new AABB(m_Axis_T_Z->position, m_Axis_T_Z->rotation, m_Axis_T_Z->scale);
	m_SceneObjects.push_back(m_Axis_T_Z);

	m_Arrow_T_X = new SceneObject{};
	m_Arrow_T_X->meshType = MESH_TYPE_CONE;
	m_Arrow_T_X->color = m_Color_Red;
	m_Arrow_T_X->position = m_Position + glm::vec3(4.4f, 0.0f, 0.0f);
	m_Arrow_T_X->rotation = glm::vec3(0.0f, 0.0f, -90.0f);
	m_Arrow_T_X->scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Arrow_T_X->mesh = new Cone(m_Arrow_T_X->scale);
	m_Arrow_T_X->AABB = new AABB(m_Arrow_T_X->position, m_Arrow_T_X->rotation, m_Arrow_T_X->scale);
	m_SceneObjects.push_back(m_Arrow_T_X);

	m_Arrow_T_Y = new SceneObject{};
	m_Arrow_T_Y->meshType = MESH_TYPE_CONE;
	m_Arrow_T_Y->color = m_Color_Green;
	m_Arrow_T_Y->position = m_Position + glm::vec3(0.0f, 4.4f, 0.0f);
	m_Arrow_T_Y->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Arrow_T_Y->scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Arrow_T_Y->mesh = new Cone(m_Arrow_T_Y->scale);
	m_Arrow_T_Y->AABB = new AABB(m_Arrow_T_Y->position, m_Arrow_T_Y->rotation, m_Arrow_T_Y->scale);
	m_SceneObjects.push_back(m_Arrow_T_Y);

	m_Arrow_T_Z = new SceneObject{};
	m_Arrow_T_Z->meshType = MESH_TYPE_CONE;
	m_Arrow_T_Z->color = m_Color_Blue;
	m_Arrow_T_Z->position = m_Position + glm::vec3(0.0f, 0.0f, 4.4f);
	m_Arrow_T_Z->rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Arrow_T_Z->scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Arrow_T_Z->mesh = new Cone(m_Arrow_T_Z->scale);
	m_Arrow_T_Z->AABB = new AABB(m_Arrow_T_Z->position, m_Arrow_T_Z->rotation, m_Arrow_T_Z->scale);
	m_SceneObjects.push_back(m_Arrow_T_Z);

	m_Square_T_XY = new SceneObject{};
	m_Square_T_XY->meshType = MESH_TYPE_CUBE;
	m_Square_T_XY->color = m_Color_Yellow;
	m_Square_T_XY->position = m_Position + glm::vec3(2.0f, 2.0f, 0.0f);
	m_Square_T_XY->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Square_T_XY->scale = glm::vec3(1.5f, 1.5f, 0.15f);
	m_Square_T_XY->mesh = new Block(m_Square_T_XY->scale);
	m_Square_T_XY->AABB = new AABB(m_Square_T_XY->position, m_Square_T_XY->rotation, m_Square_T_XY->scale);
	m_SceneObjects.push_back(m_Square_T_XY);

	m_Square_T_YZ = new SceneObject{};
	m_Square_T_YZ->meshType = MESH_TYPE_CUBE;
	m_Square_T_YZ->color = m_Color_Cyan;
	m_Square_T_YZ->position = m_Position + glm::vec3(0.0f, 2.0f, 2.0f);
	m_Square_T_YZ->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Square_T_YZ->scale = glm::vec3(0.15f, 1.5f, 1.5f);
	m_Square_T_YZ->mesh = new Block(m_Square_T_YZ->scale);
	m_Square_T_YZ->AABB = new AABB(m_Square_T_YZ->position, m_Square_T_YZ->rotation, m_Square_T_YZ->scale);
	m_SceneObjects.push_back(m_Square_T_YZ);

	m_Square_T_ZX = new SceneObject{};
	m_Square_T_ZX->meshType = MESH_TYPE_CUBE;
	m_Square_T_ZX->color = m_Color_Magenta;
	m_Square_T_ZX->position = m_Position + glm::vec3(2.0f, 0.0f, 2.0f);
	m_Square_T_ZX->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Square_T_ZX->scale = glm::vec3(1.5f, 0.15f, 1.5f);
	m_Square_T_ZX->mesh = new Block(m_Square_T_ZX->scale);
	m_Square_T_ZX->AABB = new AABB(m_Square_T_ZX->position, m_Square_T_ZX->rotation, m_Square_T_ZX->scale);
	m_SceneObjects.push_back(m_Square_T_ZX);

	// Initialize Scale meshes
	m_Axis_S_X = new SceneObject{};
	m_Axis_S_X->meshType = MESH_TYPE_CYLINDER;
	m_Axis_S_X->color = m_Color_Red;
	m_Axis_S_X->position = m_Position + glm::vec3(2.0f, 0.0f, 0.0f);
	m_Axis_S_X->rotation = glm::vec3(0.0f, 0.0f, -90.0f);
	m_Axis_S_X->scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_S_X->mesh = new Cylinder(m_Axis_S_X->scale);
	m_Axis_S_X->AABB = new AABB(m_Axis_S_X->position, m_Axis_S_X->rotation, m_Axis_S_X->scale);
	m_SceneObjects.push_back(m_Axis_S_X);

	m_Axis_S_Y = new SceneObject{};
	m_Axis_S_Y->meshType = MESH_TYPE_CYLINDER;
	m_Axis_S_Y->color = m_Color_Green;
	m_Axis_S_Y->position = m_Position + glm::vec3(0.0f, 2.0f, 0.0f);
	m_Axis_S_Y->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Axis_S_Y->scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_S_Y->mesh = new Cylinder(m_Axis_S_Y->scale);
	m_Axis_S_Y->AABB = new AABB(m_Axis_S_Y->position, m_Axis_S_Y->rotation, m_Axis_S_Y->scale);
	m_SceneObjects.push_back(m_Axis_S_Y);

	m_Axis_S_Z = new SceneObject{};
	m_Axis_S_Z->meshType = MESH_TYPE_CYLINDER;
	m_Axis_S_Z->color = m_Color_Blue;
	m_Axis_S_Z->position = m_Position + glm::vec3(0.0f, 0.0f, 2.0f);
	m_Axis_S_Z->rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Axis_S_Z->scale = glm::vec3(0.2f, 4.0f, 0.2f);
	m_Axis_S_Z->mesh = new Cylinder(m_Axis_S_Z->scale);
	m_Axis_S_Z->AABB = new AABB(m_Axis_S_Z->position, m_Axis_S_Z->rotation, m_Axis_S_Z->scale);
	m_SceneObjects.push_back(m_Axis_S_Z);

	// Initialize Scale Handles
	m_Handle_S_X = new SceneObject{};
	m_Handle_S_X->meshType = MESH_TYPE_CYLINDER;
	m_Handle_S_X->color = m_Color_Red;
	m_Handle_S_X->position = m_Position + glm::vec3(4.4f, 0.0f, 0.0f);
	m_Handle_S_X->rotation = glm::vec3(0.0f, 0.0f, -90.0f);
	m_Handle_S_X->scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Handle_S_X->mesh = new Cylinder(m_Handle_S_X->scale);
	m_Handle_S_X->AABB = new AABB(m_Handle_S_X->position, m_Handle_S_X->rotation, m_Handle_S_X->scale);
	m_SceneObjects.push_back(m_Handle_S_X);

	m_Handle_S_Y = new SceneObject{};
	m_Handle_S_Y->meshType = MESH_TYPE_CYLINDER;
	m_Handle_S_Y->color = m_Color_Green;
	m_Handle_S_Y->position = m_Position + glm::vec3(0.0f, 4.4f, 0.0f);
	m_Handle_S_Y->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Handle_S_Y->scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Handle_S_Y->mesh = new Cylinder(m_Handle_S_Y->scale);
	m_Handle_S_Y->AABB = new AABB(m_Handle_S_Y->position, m_Handle_S_Y->rotation, m_Handle_S_Y->scale);
	m_SceneObjects.push_back(m_Handle_S_Y);

	m_Handle_S_Z = new SceneObject{};
	m_Handle_S_Z->meshType = MESH_TYPE_CYLINDER;
	m_Handle_S_Z->color = m_Color_Blue;
	m_Handle_S_Z->position = m_Position + glm::vec3(0.0f, 0.0f, 4.4f);
	m_Handle_S_Z->rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Handle_S_Z->scale = glm::vec3(0.6f, 0.8f, 0.6f);
	m_Handle_S_Z->mesh = new Cylinder(m_Handle_S_Z->scale);
	m_Handle_S_Z->AABB = new AABB(m_Handle_S_Z->position, m_Handle_S_Z->rotation, m_Handle_S_Z->scale);
	m_SceneObjects.push_back(m_Handle_S_Z);

	// Initialize Rotation meshes
	m_Ring_R_X = new SceneObject{};
	m_Ring_R_X->meshType = MESH_TYPE_RING;
	m_Ring_R_X->color = m_Color_Red;
	m_Ring_R_X->position = m_Position + glm::vec3(0.0f, 0.0f, 0.0f);
	m_Ring_R_X->rotation = glm::vec3(0.0f, 0.0f, 90.0f);
	m_Ring_R_X->scale = glm::vec3(6.0f, 0.2f, 6.0f);
	m_Ring_R_X->mesh = new Ring(m_Ring_R_X->scale);
	m_Ring_R_X->AABB = new AABB(m_Ring_R_X->position, m_Ring_R_X->rotation, m_Ring_R_X->scale);
	m_SceneObjects.push_back(m_Ring_R_X);

	m_Ring_R_Y = new SceneObject{};
	m_Ring_R_Y->meshType = MESH_TYPE_RING;
	m_Ring_R_Y->color = m_Color_Green;
	m_Ring_R_Y->position = m_Position + glm::vec3(0.0f, 0.0f, 0.0f);
	m_Ring_R_Y->rotation = glm::vec3(0.0f, 1.0f, 0.0f);
	m_Ring_R_Y->scale = glm::vec3(6.0f, 0.2f, 6.0f);
	m_Ring_R_Y->mesh = new Ring(m_Ring_R_Y->scale);
	m_Ring_R_Y->AABB = new AABB(m_Ring_R_Y->position, m_Ring_R_Y->rotation, m_Ring_R_Y->scale);
	m_SceneObjects.push_back(m_Ring_R_Y);

	m_Ring_R_Z = new SceneObject{};
	m_Ring_R_Z->meshType = MESH_TYPE_RING;
	m_Ring_R_Z->color = m_Color_Blue;
	m_Ring_R_Z->position = m_Position + glm::vec3(0.0f, 0.0f, 0.0f);
	m_Ring_R_Z->rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	m_Ring_R_Z->scale = glm::vec3(6.0f, 0.2f, 6.0f);
	m_Ring_R_Z->mesh = new Ring(m_Ring_R_Z->scale);
	m_Ring_R_Z->AABB = new AABB(m_Ring_R_Z->position, m_Ring_R_Z->rotation, m_Ring_R_Z->scale);
	m_SceneObjects.push_back(m_Ring_R_Z);
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

void Gizmo::Update(glm::vec3 cameraPosition)
{
	m_Position = m_SceneObject->position;
	m_Rotation = m_SceneObject->rotation;
	m_Scale    = m_SceneObject->scale;

	MousePicker::Get()->GetPointOnRay(cameraPosition, MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

	glm::vec3 currentRay = MousePicker::Get()->GetCurrentRay();
	bool isIntersectionFound = false;
	bool isIntersecting = false;
	for (int i = 0; i < m_SceneObjects.size(); i++) {

		m_SceneObjects[i]->AABB->Update(m_Position + m_SceneObjects[i]->position, m_SceneObjects[i]->rotation, m_SceneObjects[i]->scale);

		isIntersecting = AABB::IntersectRayAab(cameraPosition, currentRay,
			m_SceneObjects[i]->AABB->GetMin(), m_SceneObjects[i]->AABB->GetMax(), glm::vec2(0.0f));

		if (isIntersecting && !isIntersectionFound) {
			// printf("Gizmo::Update CurrentRay [ %.2ff %.2ff %.2ff ]\n", currentRay.x, currentRay.y, currentRay.z);
			m_SceneObjects[i]->AABB->m_Color = m_Color_Red;
			m_SceneObjects[i]->AABB->m_IsColliding = true;
			// isIntersectionFound = true;
		} else {
			m_SceneObjects[i]->AABB->m_Color = m_Color_Green;
			m_SceneObjects[i]->AABB->m_IsColliding = false;
		}
	}
}

void Gizmo::Render(Shader* shader)
{
	// Render Translation Gizmo
	if (m_Mode == GIZMO_MODE_TRANSLATE)
	{
		// Translation Gizmo - Axes
		m_Axis_T_X->transform = Math::CreateTransform(m_Position + m_Axis_T_X->position, m_Axis_T_X->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_X->transform);
		shader->setVec4("tintColor", m_Axis_T_X->AABB->m_IsColliding || m_Arrow_T_X->AABB->m_IsColliding ? m_Color_Selected : m_Axis_T_X->color);
		m_Axis_T_X->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_X->AABB->m_Color);
		// m_Axis_T_X->AABB->Draw();

		m_Axis_T_Y->transform = Math::CreateTransform(m_Position + m_Axis_T_Y->position, m_Axis_T_Y->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_Y->transform);
		shader->setVec4("tintColor", m_Axis_T_Y->AABB->m_IsColliding || m_Arrow_T_Y->AABB->m_IsColliding ? m_Color_Selected : m_Axis_T_Y->color);
		m_Axis_T_Y->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_Y->AABB->m_Color);
		// m_Axis_T_Y->AABB->Draw();

		m_Axis_T_Z->transform = Math::CreateTransform(m_Position + m_Axis_T_Z->position, m_Axis_T_Z->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_Z->transform);
		shader->setVec4("tintColor", m_Axis_T_Z->AABB->m_IsColliding || m_Arrow_T_Z->AABB->m_IsColliding ? m_Color_Selected : m_Axis_T_Z->color);
		m_Axis_T_Z->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_Z->AABB->m_Color);
		// m_Axis_T_Z->AABB->Draw();

		// Translation Gizmo - Arrows (Cones)
		m_Arrow_T_X->transform = Math::CreateTransform(m_Position + m_Arrow_T_X->position, m_Arrow_T_X->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_X->transform);
		shader->setVec4("tintColor", m_Axis_T_X->AABB->m_IsColliding || m_Arrow_T_X->AABB->m_IsColliding ? m_Color_Selected : m_Arrow_T_X->color);
		m_Arrow_T_X->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_X->AABB->m_Color);
		// m_Arrow_T_X->AABB->Draw();

		m_Arrow_T_Y->transform = Math::CreateTransform(m_Position + m_Arrow_T_Y->position, m_Arrow_T_Y->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_Y->transform);
		shader->setVec4("tintColor", m_Axis_T_Y->AABB->m_IsColliding || m_Arrow_T_Y->AABB->m_IsColliding ? m_Color_Selected : m_Arrow_T_Y->color);
		m_Arrow_T_Y->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_Y->AABB->m_Color);
		// m_Arrow_T_Y->AABB->Draw();

		m_Arrow_T_Z->transform = Math::CreateTransform(m_Position + m_Arrow_T_Z->position, m_Arrow_T_Z->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Arrow_T_Z->transform);
		shader->setVec4("tintColor", m_Axis_T_Z->AABB->m_IsColliding || m_Arrow_T_Z->AABB->m_IsColliding ? m_Color_Selected : m_Arrow_T_Z->color);
		m_Arrow_T_Z->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Arrow_T_Z->AABB->m_Color);
		// m_Arrow_T_Z->AABB->Draw();

		// Translation Gizmo - 2D squares
		m_Square_T_XY->transform = Math::CreateTransform(m_Position + m_Square_T_XY->position, m_Square_T_XY->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_XY->transform);
		shader->setVec4("tintColor", m_Square_T_XY->AABB->m_IsColliding ? m_Color_Selected : m_Square_T_XY->color);
		m_Square_T_XY->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_XY->AABB->m_Color);
		// m_Square_T_XY->AABB->Draw();

		m_Square_T_YZ->transform = Math::CreateTransform(m_Position + m_Square_T_YZ->position, m_Square_T_YZ->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_YZ->transform);
		shader->setVec4("tintColor", m_Square_T_YZ->AABB->m_IsColliding ? m_Color_Selected : m_Square_T_YZ->color);
		m_Square_T_YZ->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_YZ->AABB->m_Color);
		// m_Square_T_YZ->AABB->Draw();

		m_Square_T_ZX->transform = Math::CreateTransform(m_Position + m_Square_T_ZX->position, m_Square_T_ZX->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Square_T_ZX->transform);
		shader->setVec4("tintColor", m_Square_T_ZX->AABB->m_IsColliding ? m_Color_Selected : m_Square_T_ZX->color);
		m_Square_T_ZX->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Square_T_ZX->AABB->m_Color);
		// m_Square_T_ZX->AABB->Draw();
	}

	// Render Scale Gizmo
	if (m_Mode == GIZMO_MODE_SCALE)
	{
		// Scale Gizmo - Axes
		m_Axis_S_X->transform = Math::CreateTransform(m_Position + m_Axis_S_X->position, m_Axis_S_X->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_S_X->transform);
		shader->setVec4("tintColor", m_Axis_S_X->AABB->m_IsColliding || m_Handle_S_X->AABB->m_IsColliding ? m_Color_Selected : m_Axis_S_X->color);
		m_Axis_S_X->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_S_X->AABB->m_Color);
		// m_Axis_S_X->AABB->Draw();

		m_Axis_S_Y->transform = Math::CreateTransform(m_Position + m_Axis_S_Y->position, m_Axis_S_Y->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_S_Y->transform);
		shader->setVec4("tintColor", m_Axis_S_Y->AABB->m_IsColliding || m_Handle_S_Y->AABB->m_IsColliding ? m_Color_Selected : m_Axis_S_Y->color);
		m_Axis_S_Y->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_S_Y->AABB->m_Color);
		// m_Axis_S_Y->AABB->Draw();

		m_Axis_S_Z->transform = Math::CreateTransform(m_Position + m_Axis_S_Z->position, m_Axis_S_Z->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_S_Z->transform);
		shader->setVec4("tintColor", m_Axis_S_Z->AABB->m_IsColliding || m_Handle_S_Z->AABB->m_IsColliding ? m_Color_Selected : m_Axis_S_Z->color);
		m_Axis_S_Z->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_S_Z->AABB->m_Color);
		// m_Axis_S_Z->AABB->Draw();

		// Scale Gizmo - Handles
		m_Handle_S_X->transform = Math::CreateTransform(m_Position + m_Handle_S_X->position, m_Handle_S_X->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Handle_S_X->transform);
		shader->setVec4("tintColor", m_Axis_S_X->AABB->m_IsColliding || m_Handle_S_X->AABB->m_IsColliding ? m_Color_Selected : m_Handle_S_X->color);
		m_Handle_S_X->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Handle_S_X->AABB->m_Color);
		// m_Handle_S_X->AABB->Draw();

		m_Handle_S_Y->transform = Math::CreateTransform(m_Position + m_Handle_S_Y->position, m_Handle_S_Y->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Handle_S_Y->transform);
		shader->setVec4("tintColor", m_Axis_S_Y->AABB->m_IsColliding || m_Handle_S_Y->AABB->m_IsColliding ? m_Color_Selected : m_Handle_S_Y->color);
		m_Handle_S_Y->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Handle_S_Y->AABB->m_Color);
		// m_Handle_S_Y->AABB->Draw();

		m_Handle_S_Z->transform = Math::CreateTransform(m_Position + m_Handle_S_Z->position, m_Handle_S_Z->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Handle_S_Z->transform);
		shader->setVec4("tintColor", m_Axis_S_Z->AABB->m_IsColliding || m_Handle_S_Z->AABB->m_IsColliding ? m_Color_Selected : m_Handle_S_Z->color);
		m_Handle_S_Z->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Handle_S_Z->AABB->m_Color);
		// m_Handle_S_Z->AABB->Draw();
	}

	// Render Rotate Gizmo
	if (m_Mode == GIZMO_MODE_ROTATE)
	{
		m_Ring_R_X->transform = Math::CreateTransform(m_Position + m_Ring_R_X->position, m_Ring_R_X->rotation, m_Ring_R_X->scale);
		shader->setMat4("model", m_Ring_R_X->transform);
		shader->setVec4("tintColor", m_Ring_R_X->AABB->m_IsColliding ? m_Color_Selected : m_Ring_R_X->color);
		m_Ring_R_X->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Ring_R_X->AABB->m_Color);
		// m_Ring_R_X->AABB->Draw();

		m_Ring_R_Y->transform = Math::CreateTransform(m_Position + m_Ring_R_Y->position, m_Ring_R_Y->rotation, m_Ring_R_Y->scale);
		shader->setMat4("model", m_Ring_R_Y->transform);
		shader->setVec4("tintColor", m_Ring_R_Y->AABB->m_IsColliding ? m_Color_Selected : m_Ring_R_Y->color);
		m_Ring_R_Y->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Ring_R_Y->AABB->m_Color);
		// m_Ring_R_Y->AABB->Draw();

		m_Ring_R_Z->transform = Math::CreateTransform(m_Position + m_Ring_R_Z->position, m_Ring_R_Z->rotation, m_Ring_R_Z->scale);
		shader->setMat4("model", m_Ring_R_Z->transform);
		shader->setVec4("tintColor", m_Ring_R_Z->AABB->m_IsColliding ? m_Color_Selected : m_Ring_R_Z->color);
		m_Ring_R_Z->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Ring_R_Z->AABB->m_Color);
		// m_Ring_R_Z->AABB->Draw();
	}
}

Gizmo::~Gizmo()
{
	// if (m_SceneObject != nullptr)
	delete m_SceneObject;

	for (auto& object : m_SceneObjects)
	{
		delete object->AABB;
		delete object->pivot;
		delete object->mesh;
	}

	m_SceneObjects.clear();
}
