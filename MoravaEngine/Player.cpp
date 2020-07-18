#include "Player.h"


Player::Player()
{
}

Player::Player(glm::vec3 position, Mesh* mesh, Camera* camera)
{
	m_Position = position;
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
