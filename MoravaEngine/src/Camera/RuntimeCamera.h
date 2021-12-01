#pragma once

#include "CameraController.h"


/**
 * RuntimeCamera bundles Camera and CameraController and shares the interface with EditorCamera  
 */
class RuntimeCamera : public H2M::CameraH2M
{
public:
	RuntimeCamera();
	RuntimeCamera(glm::vec3 position, float yaw, float pitch, float fovDegrees, float aspectRatio, float moveSpeed, float turnSpeed);
	~RuntimeCamera();

	virtual void OnUpdate(H2M::TimestepH2M ts) override;
	virtual void OnEvent(H2M::EventH2M& e) override;
	virtual void SetViewportSize(float width, float height) override;

	virtual void SetPitch(float pitch) override;
	virtual glm::mat4& GetViewMatrix() override;

private:
	void UpdateProjection();
	void UpdateView();

private:
	CameraController m_CameraController;

};
