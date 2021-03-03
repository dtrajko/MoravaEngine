#pragma once

#include "Player/Player.h"
#include "Terrain/TerrainVoxel.h"


class PlayerController
{
public:
	PlayerController();
	PlayerController(Player* player);
	void SetTerrain(TerrainBase* terrain);
	bool IsColliding(glm::vec3 position, float distanceAllowed);
	~PlayerController();

	void KeyControl(bool* keys, float deltaTime);
	void MouseControl(bool* buttons, float xChange, float yChange);
	void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset);
	inline Player* GetPlayer() { return m_Player; };
	inline void SetUnlockRotation(bool unlockRotation) { m_UnlockRotation = unlockRotation; };
	inline void SetGravity(float gravity) { m_Gravity = gravity; };
	inline void SetMoveSpeed(float moveSpeed) { m_MoveSpeed = moveSpeed; };
	inline void SetMoveFastFactor(float moveFastFactor) { m_MoveFastFactor = moveFastFactor; };

	void Update();

private:
	Player* m_Player;
	TerrainVoxel* m_Terrain;

	float m_Gravity = 0.1f;
	float m_MoveSpeed = 0.1f;
	float m_JumpSpeed = 0.4f;
	float m_TurnSpeed = 0.1f;
	float m_MoveFastFactor = 4.0f;
	bool m_UnlockRotation;

	glm::vec3 m_MoveDirection;
	glm::vec3 m_MoveDirectionLast;
	float m_DistanceAllowed = 1.0f;
	glm::vec3 m_CollisionDirection;

};
