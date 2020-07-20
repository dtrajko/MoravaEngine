#include "Player.h"


Player::Player()
{
}

Player::Player(glm::vec3 position, Mesh* mesh, Camera* camera)
{
	m_Position = position;
	m_Rotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
	m_Mesh = mesh;
	m_Camera = camera;
}

Player::~Player()
{
}

void Player::Update()
{
}

void Player::Render()
{
	m_Mesh->Render();
}
