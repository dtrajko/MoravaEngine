#pragma once

#include "CameraController.h"
#include "Player.h"


class CameraControllerVoxelTerrain : public CameraController
{

public:
	CameraControllerVoxelTerrain();
	CameraControllerVoxelTerrain(Camera* camera, Player* player, float moveSpeed, float turnSpeed);
	~CameraControllerVoxelTerrain();

	virtual void KeyControl(bool* keys, float deltaTime) override;
	virtual void MouseControl(bool* buttons, float xChange, float yChange) override;
	virtual void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset) override;
	virtual void Update() override;

private:
	Player* m_Player;

};
