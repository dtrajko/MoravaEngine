#include "Player.h"


Player::Player()
	: Player(glm::vec3(0.0f, 0.0f, 0.0f), nullptr, nullptr)
{
}

Player::Player(glm::vec3 position, Mesh* mesh, Camera* camera)
{
	m_Position = position;
	m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	m_Front = glm::vec3(1.0f, 0.0f, 0.0f);

	m_Mesh = mesh;
	m_Camera = camera;

	m_Color = glm::vec4(1.0f, 0.4f, 0.0f, 1.0f);

	Update();
}

Player::~Player()
{
}

void Player::Update()
{
	CalculateFront();
	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Player::CalculateFront()
{
	m_Front.x = cos(glm::radians(-m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
	m_Front.y = sin(glm::radians(m_Rotation.x));
	m_Front.z = sin(glm::radians(-m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
	m_Front = glm::normalize(m_Front);
}

void Player::Render()
{
	m_Mesh->Render();
}
