#include "Gizmo.h"

Gizmo::Gizmo()
{
	m_Mode = GIZMO_MODE_TRANSLATE;

	// Initialize Translation meshes
	m_Axis_T_X = new Cylinder();
	m_Axis_T_Y = new Cylinder();
	m_Axis_T_Z = new Cylinder();

	m_Arrow_T_X = new Cone();
	m_Arrow_T_Y = new Cone();
	m_Arrow_T_Z = new Cone();

	m_Square_T_XY = new Block(); // yellow
	m_Square_T_YZ = new Block(); // cyan
	m_Square_T_ZX = new Block(); // magenta

	// Initialize Scale meshes
	m_Axis_S_X = new Cylinder();
	m_Axis_S_Y = new Cylinder();
	m_Axis_S_Z = new Cylinder();

	m_Handle_S_X = new Cylinder();
	m_Handle_S_Y = new Cylinder();
	m_Handle_S_Z = new Cylinder();

	// Initialize Rotation meshes
	m_Ring_S_X = new Ring();
	m_Ring_S_Y = new Ring();
	m_Ring_S_Z = new Ring();

}

void Gizmo::ChangeMode(int mode)
{
	m_Mode = mode;
}

void Gizmo::ToggleMode()
{
	if (m_Mode == GIZMO_MODE_TRANSLATE) m_Mode = GIZMO_MODE_SCALE;
	if (m_Mode == GIZMO_MODE_SCALE)     m_Mode = GIZMO_MODE_ROTATE;
	if (m_Mode == GIZMO_MODE_ROTATE)    m_Mode = GIZMO_MODE_TRANSLATE;
}

void Gizmo::Update()
{
}

void Gizmo::Render(Shader* shader)
{
	glm::mat4 model;

	// Render Translation Gizmo
	if (m_Mode == GIZMO_MODE_TRANSLATE)
	{
		// Translation Gizmo - Axes
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		m_Axis_T_X->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		m_Axis_T_Y->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 2.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		m_Axis_T_Z->Render();

		// Translation Gizmo - Arrows (Cones)
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(4.4f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		m_Arrow_T_X->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 4.4f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		m_Arrow_T_Y->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 4.4f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		m_Arrow_T_Z->Render();

		// Translation Gizmo - 2D squares
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 2.0f, 0.0f));
		// model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 0.15f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Yellow);
		m_Square_T_XY->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 2.0f, 2.0f));
		// model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.15f, 1.5f, 1.5f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Cyan);
		m_Square_T_YZ->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 2.0f));
		// model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 0.15f, 1.5f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Magenta);
		m_Square_T_ZX->Render();
	}

	// Render Scale Gizmo
	if (m_Mode == GIZMO_MODE_SCALE)
	{
		// Scale Gizmo - Axes
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		m_Axis_S_X->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		m_Axis_S_Y->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 2.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 4.0f, 0.2f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		m_Axis_S_Z->Render();

		// Scale Gizmo - Handles
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(4.4f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		m_Handle_S_X->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 4.4f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		m_Handle_S_Y->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 4.4f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.6f, 0.8f, 0.6f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		m_Handle_S_Z->Render();
	}

	// Render Rotate Gizmo
	if (m_Mode == GIZMO_MODE_ROTATE)
	{
		// Rotation gizmo - Rings
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(8.0f, 0.6f, 8.0f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Red);
		m_Ring_S_X->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(8.0f, 0.6f, 8.0f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Green);
		m_Ring_S_Y->Render();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(8.0f, 0.6f, 8.0f));
		shader->setMat4("model", model);
		shader->setVec4("tintColor", m_Color_Blue);
		m_Ring_S_Z->Render();
	}
}

Gizmo::~Gizmo()
{
	// Translation
	delete m_Axis_T_X;
	delete m_Axis_T_Y;
	delete m_Axis_T_Z;

	delete m_Arrow_T_X;
	delete m_Arrow_T_Y;
	delete m_Arrow_T_Z;

	delete m_Square_T_XY;
	delete m_Square_T_YZ;
	delete m_Square_T_ZX;

	// Scale
	delete m_Axis_S_X;
	delete m_Axis_S_Y;
	delete m_Axis_S_Z;

	delete m_Handle_S_X;
	delete m_Handle_S_Y;
	delete m_Handle_S_Z;

	// Rotate
	delete m_Ring_S_X;
	delete m_Ring_S_Y;
	delete m_Ring_S_Z;
}
