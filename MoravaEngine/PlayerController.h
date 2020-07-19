#pragma once

#include "Player.h"
#include "Terrain3D.h"


class PlayerController
{
public:
	PlayerController();
	PlayerController(Player* player);
	void SetTerrain(Terrain3D* terrain);
	bool IsColliding(glm::vec3 position);
	~PlayerController();

	void KeyControl(bool* keys, float deltaTime);
	void MouseControl(bool* buttons, float xChange, float yChange);
	void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset);
	inline Player* GetPlayer() { return m_Player; };

	void Update();

private:
	Player* m_Player;
	Terrain3D* m_Terrain;

	float m_Gravity = 0.05f;
	float m_MoveSpeed = 0.1f;
	float m_JumpSpeed = 0.4f;

	glm::vec3 m_MoveDirection;
	glm::vec3 m_MoveDirectionLast;
	float m_DistanceAllowed = 1.0f;
	glm::vec3 m_CollisionDirection;

};
