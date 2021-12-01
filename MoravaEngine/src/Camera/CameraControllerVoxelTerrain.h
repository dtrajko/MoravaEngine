#pragma once

#include "CameraController.h"
#include "Player/Player.h"


class CameraControllerVoxelTerrain : public CameraController
{

public:
	CameraControllerVoxelTerrain();
	CameraControllerVoxelTerrain(H2M::CameraH2M* camera, Player* player, float aspectRatio, float moveSpeed, float turnSpeed, float cameraPlayerDistance);
	~CameraControllerVoxelTerrain();

	virtual void KeyControl(bool* keys, float deltaTime) override;
	virtual void MouseControl(bool* buttons, float xChange, float yChange) override;
	virtual void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset) override;
	virtual void Update() override;
	void InvertPitch();

private:
	void UpdateDebugInfo();

	// ThinMatrix methods for 3rd person camera
	void CalculateZoom(float yOffset);
	void CalculatePitch(bool* buttons, float yChange);
	void CalculateAngleAroundPlayer(bool* keys, bool* buttons, float xChange);
	float CalculateHorizontalDistance();
	float CalculateVerticalDistance();
	void CalculateCameraPosition(float horizontalDistance, float verticalDistance);

private:
	Player* m_Player;

	float m_CameraPlayerDistance;
	float m_AngleAroundPlayer;

	float m_PitchChangeSpeed;
	float m_YawChangeSpeed;

	// KeyControl
	bool* m_Keys;
	// MouseControl(bool* buttons, float xChange, float yChange)
	bool* m_Buttons;
	float m_xChange;
	float m_yChange;
	// MouseScrollControl
	float m_xOffset;
	float m_yOffset;

	float m_Theta;

public:
	glm::vec3 m_DebugPlayerPosition;
	glm::vec3 m_DebugCameraPosition;
	glm::vec3 m_DebugPlayerFront;
	glm::vec3 m_DebugCameraFront;
	float m_DebugCameraPitch;
	float m_DebugCameraYaw;
	float m_DebugAngleAroundPlayer;
	float m_DebugTheta;

};
