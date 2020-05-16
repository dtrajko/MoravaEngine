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
	m_Axis_T_X->rotation = glm::vec3(0.0f, 0.0f, 90.0f);
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

	/*
	m_Arrow_T_X = new Cone();
	m_Meshes.push_back(m_Arrow_T_X);
	m_Arrow_T_Y = new Cone();
	m_Meshes.push_back(m_Arrow_T_Y);
	m_Arrow_T_Z = new Cone();
	m_Meshes.push_back(m_Arrow_T_Z);

	m_Arrow_T_X_AABB = new AABB();
	m_AABBs.push_back(m_Arrow_T_X_AABB);
	m_Arrow_T_Y_AABB = new AABB();
	m_AABBs.push_back(m_Arrow_T_Y_AABB);
	m_Arrow_T_Z_AABB = new AABB();
	m_AABBs.push_back(m_Arrow_T_Z_AABB);

	m_Square_T_XY = new Block(); // yellow
	m_Meshes.push_back(m_Square_T_XY);
	m_Square_T_YZ = new Block(); // cyan
	m_Meshes.push_back(m_Square_T_YZ);
	m_Square_T_ZX = new Block(); // magenta
	m_Meshes.push_back(m_Square_T_ZX);

	m_Square_T_XY_AABB = new AABB();
	m_AABBs.push_back(m_Square_T_XY_AABB);
	m_Square_T_YZ_AABB = new AABB();
	m_AABBs.push_back(m_Square_T_YZ_AABB);
	m_Square_T_ZX_AABB = new AABB();
	m_AABBs.push_back(m_Square_T_ZX_AABB);

	// Initialize Scale meshes
	m_Axis_S_X = new Cylinder();
	m_Meshes.push_back(m_Axis_S_X);
	m_Axis_S_Y = new Cylinder();
	m_Meshes.push_back(m_Axis_S_Y);
	m_Axis_S_Z = new Cylinder();
	m_Meshes.push_back(m_Axis_S_Z);

	m_Axis_S_X_AABB = new AABB();
	m_AABBs.push_back(m_Axis_S_X_AABB);
	m_Axis_S_Y_AABB = new AABB();
	m_AABBs.push_back(m_Axis_S_Y_AABB);
	m_Axis_S_Z_AABB = new AABB();
	m_AABBs.push_back(m_Axis_S_Z_AABB);

	m_Handle_S_X = new Cylinder();
	m_Meshes.push_back(m_Handle_S_X);
	m_Handle_S_Y = new Cylinder();
	m_Meshes.push_back(m_Handle_S_Y);
	m_Handle_S_Z = new Cylinder();
	m_Meshes.push_back(m_Handle_S_Z);

	m_Handle_S_X_AABB = new AABB();
	m_AABBs.push_back(m_Handle_S_X_AABB);
	m_Handle_S_Y_AABB = new AABB();
	m_AABBs.push_back(m_Handle_S_Y_AABB);
	m_Handle_S_Z_AABB = new AABB();
	m_AABBs.push_back(m_Handle_S_Z_AABB);

	// Initialize Rotation meshes
	m_Ring_S_X = new Ring();
	m_Meshes.push_back(m_Ring_S_X);
	m_Ring_S_Y = new Ring();
	m_Meshes.push_back(m_Ring_S_Y);
	m_Ring_S_Z = new Ring();
	m_Meshes.push_back(m_Ring_S_Z);

	m_Ring_S_X_AABB = new AABB();
	m_AABBs.push_back(m_Ring_S_X_AABB);
	m_Ring_S_Y_AABB = new AABB();
	m_AABBs.push_back(m_Ring_S_Y_AABB);
	m_Ring_S_Z_AABB = new AABB();
	m_AABBs.push_back(m_Ring_S_Z_AABB);
	*/
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
	bool result = false;
	for (int i = 0; i < m_SceneObjects.size(); i++) {

		m_SceneObjects[i]->AABB->Update(m_Position + m_SceneObjects[i]->position, m_SceneObjects[i]->rotation, m_SceneObjects[i]->scale);

		result = AABB::IntersectRayAab(cameraPosition, currentRay,
			m_SceneObjects[i]->AABB->GetMin(), m_SceneObjects[i]->AABB->GetMax(), glm::vec2(0.0f));

		if (result) {
			// printf("Gizmo::Update CurrentRay [ %.2ff %.2ff %.2ff ]\n", currentRay.x, currentRay.y, currentRay.z);
			m_SceneObjects[i]->AABB->m_Color = m_Color_Red;
			m_SceneObjects[i]->AABB->m_IsColliding = true;
		} else {
			m_SceneObjects[i]->AABB->m_Color = m_Color_Green;
			m_SceneObjects[i]->AABB->m_IsColliding = false;
		}
	}
}

void Gizmo::Render(Shader* shader)
{
	glm::mat4 model;

	// Render Translation Gizmo
	if (m_Mode == GIZMO_MODE_TRANSLATE)
	{
		// Translation Gizmo - Axes
		m_Axis_T_X->transform = Math::CreateTransform(m_Position + m_Axis_T_X->position, m_Axis_T_X->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_X->transform);
		shader->setVec4("tintColor", m_Axis_T_X->AABB->m_IsColliding ? m_Color_Selected : m_Color_Red);
		m_Axis_T_X->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_X->AABB->m_Color);
		m_Axis_T_X->AABB->Draw();

		m_Axis_T_Y->transform = Math::CreateTransform(m_Position + m_Axis_T_Y->position, m_Axis_T_Y->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_Y->transform);
		shader->setVec4("tintColor", m_Axis_T_Y->AABB->m_IsColliding ? m_Color_Selected : m_Color_Green);
		m_Axis_T_Y->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_Y->AABB->m_Color);
		m_Axis_T_Y->AABB->Draw();

		m_Axis_T_Z->transform = Math::CreateTransform(m_Position + m_Axis_T_Z->position, m_Axis_T_Z->rotation, glm::vec3(1.0f));
		shader->setMat4("model", m_Axis_T_Z->transform);
		shader->setVec4("tintColor", m_Axis_T_Z->AABB->m_IsColliding ? m_Color_Selected : m_Color_Blue);
		m_Axis_T_Z->mesh->Render();

		shader->setMat4("model", glm::mat4(1.0f));
		shader->setVec4("tintColor", m_Axis_T_Z->AABB->m_Color);
		m_Axis_T_Z->AABB->Draw();

		// Translation Gizmo - Arrows (Cones)
		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(4.4f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		// m_Arrow_T_X->Render();

		// shader->setVec4("tintColor", m_Arrow_T_X_AABB->m_Color);
		// m_Arrow_T_X_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 4.4f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		// m_Arrow_T_Y->Render();

		// shader->setVec4("tintColor", m_Arrow_T_Y_AABB->m_Color);
		// m_Arrow_T_Y_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 0.0f, 4.4f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		// m_Arrow_T_Z->Render();

		// shader->setVec4("tintColor", m_Arrow_T_Z_AABB->m_Color);
		// m_Arrow_T_Z_AABB->Draw();

		// Translation Gizmo - 2D squares
		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(2.0f, 2.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 0.15f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Yellow);
		// m_Square_T_XY->Render();

		// shader->setVec4("tintColor", m_Square_T_XY_AABB->m_Color);
		// m_Square_T_XY_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 2.0f, 2.0f));
		// model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.15f, 1.5f, 1.5f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Cyan);
		// m_Square_T_YZ->Render();

		// shader->setVec4("tintColor", m_Square_T_YZ_AABB->m_Color);
		// m_Square_T_YZ_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(2.0f, 0.0f, 2.0f));
		// model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 0.15f, 1.5f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Magenta);
		// m_Square_T_ZX->Render();

		// shader->setVec4("tintColor", m_Square_T_ZX_AABB->m_Color);
		// m_Square_T_ZX_AABB->Draw();
	}

	// Render Scale Gizmo
	if (m_Mode == GIZMO_MODE_SCALE)
	{
		// Scale Gizmo - Axes
		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(2.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		// m_Axis_S_X->Render();

		// shader->setVec4("tintColor", m_Axis_S_X_AABB->m_Color);
		// m_Axis_S_X_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 2.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		// m_Axis_S_Y->Render();

		// shader->setVec4("tintColor", m_Axis_S_Y_AABB->m_Color);
		// m_Axis_S_Y_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 0.0f, 2.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		// m_Axis_S_Z->Render();

		// shader->setVec4("tintColor", m_Axis_S_Z_AABB->m_Color);
		// m_Axis_S_Z_AABB->Draw();

		// Scale Gizmo - Handles
		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(4.4f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		// m_Handle_S_X->Render();

		// shader->setVec4("tintColor", m_Handle_S_X_AABB->m_Color);
		// m_Handle_S_X_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 4.4f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		// m_Handle_S_Y->Render();

		// shader->setVec4("tintColor", m_Handle_S_Y_AABB->m_Color);
		// m_Handle_S_Y_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 0.0f, 4.4f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		// m_Handle_S_Z->Render();

		// shader->setVec4("tintColor", m_Handle_S_Z_AABB->m_Color);
		// m_Handle_S_Z_AABB->Draw();
	}

	// Render Rotate Gizmo
	if (m_Mode == GIZMO_MODE_ROTATE)
	{
		// Rotation gizmo - Rings
		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(8.0f, 0.6f, 8.0f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		// m_Ring_S_X->Render();

		// shader->setVec4("tintColor", m_Ring_S_X_AABB->m_Color);
		// m_Ring_S_X_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(8.0f, 0.6f, 8.0f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		// m_Ring_S_Y->Render();

		// shader->setVec4("tintColor", m_Ring_S_Y_AABB->m_Color);
		// m_Ring_S_Y_AABB->Draw();

		model = glm::mat4(1.0f);
		model = glm::translate(model, m_Position + glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(8.0f, 0.6f, 8.0f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		// m_Ring_S_Z->Render();

		// shader->setVec4("tintColor", m_Ring_S_Z_AABB->m_Color);
		// m_Ring_S_Z_AABB->Draw();
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
