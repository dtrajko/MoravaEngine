#pragma once

#include "Player.h"


class PlayerController
{
public:
	PlayerController();
	PlayerController(Player* player);
	~PlayerController();

	void KeyControl(bool* keys, float deltaTime);
	void MouseControl(bool* buttons, float xChange, float yChange);
	void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset);
	inline Player* GetPlayer() { return m_Player; };

	void Update();

private:
	Player* m_Player;

	float m_Gravity = -0.01f;
	float m_MoveSpeed = 0.05f;

};
