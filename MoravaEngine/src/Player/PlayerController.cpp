#include "Player/PlayerController.h"

#include "Core/CommonValues.h"
#include "Core/Log.h"


PlayerController::PlayerController()
{
	m_Player = nullptr;
	m_Terrain = nullptr;
}

PlayerController::PlayerController(Player* player)
{
	m_Player = player;
	m_Terrain = nullptr;
}

void PlayerController::SetTerrain(TerrainBase* terrain)
{
	m_Terrain = (TerrainVoxel*)terrain;
}

PlayerController::~PlayerController()
{
}

void PlayerController::KeyControl(bool* keys, float deltaTime)
{
	glm::vec3 newPosition = m_Player->GetPosition();
	glm::vec3 oldPosition = newPosition;

	// Set gravity
	newPosition = oldPosition + glm::vec3(0.0f, -1.0f, 0.0f) * m_Gravity;
	m_MoveDirection = -m_Player->GetUp();

	float moveSpeed = m_MoveSpeed;
	if (keys[GLFW_KEY_LEFT_SHIFT]) {
		moveSpeed = m_MoveSpeed * m_MoveFastFactor;
	}

	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		newPosition = oldPosition + m_Player->GetFront() * moveSpeed;
		m_MoveDirection = m_Player->GetFront();
	}
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		newPosition = oldPosition - m_Player->GetFront() * moveSpeed;
		m_MoveDirection = -m_Player->GetFront();
	}
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		newPosition = oldPosition - m_Player->GetRight() * moveSpeed;
		m_MoveDirection = -m_Player->GetRight();
	}
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		newPosition = oldPosition + m_Player->GetRight() * moveSpeed;
		m_MoveDirection = m_Player->GetRight();
	}
	if (keys[GLFW_KEY_Q])
	{
		newPosition = oldPosition - m_Player->GetUp() * moveSpeed;
		m_MoveDirection = -m_Player->GetUp();
	}
	if (keys[GLFW_KEY_E] || keys[GLFW_KEY_SPACE])
	{
		newPosition = oldPosition + m_Player->GetUp() * m_JumpSpeed;
		m_MoveDirection = m_Player->GetUp();
	}

	bool bColliding = IsColliding(newPosition, m_DistanceAllowed);
	// printf("PlayerController newPosition [ %.2ff %.2ff %.2ff ] minDistance = %.2ff\n", newPosition.x, newPosition.y, newPosition.z, minDistance);

	// Check the collision
	if (bColliding) {
		// Collision detected
		if (m_MoveDirection != m_MoveDirectionLast) {
			newPosition = glm::vec3(std::round(oldPosition.x), std::round(oldPosition.y), std::round(oldPosition.z));
			m_Player->SetPosition(newPosition);
			m_MoveDirectionLast = m_MoveDirection;
		}

		// printf("PlayerController COLLISION DETECTED Position: [ %.2ff %.2ff %.2ff ] m_MoveDirectionLast [ %.2ff %.2ff %.2ff ]\n",
		// 	oldPosition.x, oldPosition.y, oldPosition.z, m_MoveDirectionLast.x, m_MoveDirectionLast.y, m_MoveDirectionLast.z);
	}
	else {
		// Collision not detected
		m_Player->SetPosition(newPosition);
		// printf("PlayerController COLLISION NOT DETECTED Position: [ %.2ff %.2ff %.2ff ]\n", newPosition.x, newPosition.y, newPosition.z);
	}
}

void PlayerController::MouseControl(bool* buttons, float xChange, float yChange)
{
	if (m_UnlockRotation || buttons[GLFW_MOUSE_BUTTON_RIGHT]) {
		glm::vec3 oldRotation = m_Player->GetRotation();
		glm::vec3 newRotation = glm::vec3(oldRotation.x, oldRotation.y - xChange * m_TurnSpeed, oldRotation.z);
		m_Player->SetRotation(newRotation);
		m_Player->Update();
	}
}

void PlayerController::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset)
{
}

bool PlayerController::IsColliding(glm::vec3 position, float distanceAllowed)
{
	bool isColliding = false;

	constexpr float maxFloatValue = std::numeric_limits<float>::max();
	float minDistance = maxFloatValue;

	if (m_Terrain == nullptr) {
		Log::GetLogger()->error("Missing reference to TerrainVoxel!");
		return isColliding;
	}

	float tempDistance;
	glm::vec3 collidingPosition;

	// printf("PlayerController::IsColliding m_Voxels.size = %zu\n", m_Terrain->m_Voxels.size());

	for (auto voxel : m_Terrain->m_Voxels)
	{
		tempDistance = glm::distance(position, (glm::vec3)voxel.second->position);

		//	printf("GetTerrainMinimumDistance position [ %.2ff %.2ff %.2ff ] terrainPosition [ %.2ff %.2ff %.2ff ] tempDistance = %.2ff\n",
		//		position.x, position.y, position.z, terrainPosition.x, terrainPosition.y, terrainPosition.z, tempDistance);

		if (tempDistance <= distanceAllowed) {
			minDistance = tempDistance;
			collidingPosition = (glm::vec3)voxel.second->position;
			break;
		}

		if (tempDistance < minDistance) {
			minDistance = tempDistance;
			collidingPosition = (glm::vec3)voxel.second->position;
		}
	}

	if (minDistance <= distanceAllowed) {
		isColliding = true;

		// printf("Player position [ %.2ff %.2ff %.2ff ] collidingPosition [ %.2ff %.2ff %.2ff ]\n",
		// 	position.x, position.y, position.z, collidingPosition.x, collidingPosition.y, collidingPosition.z);
	}

	return isColliding;
}

void PlayerController::Update()
{
	m_Player->Update();
}
