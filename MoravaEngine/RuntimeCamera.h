#pragma once

#include "Hazel/Renderer/HazelCamera.h"
#include "Hazel/Core/Timestep.h"
#include "CameraController.h"


/**
 * RuntimeCamera bundles Camera and CameraController and shares the interface with EditorCamera  
 */
class RuntimeCamera : public Hazel::HazelCamera
{
public:
	RuntimeCamera();
	RuntimeCamera(glm::vec3 position, float yaw, float pitch, float fovDegrees, float aspectRatio, float moveSpeed, float turnSpeed);
	~RuntimeCamera();

	virtual void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnEvent(Event& e) override;
	virtual void SetViewportSize(float width, float height) override;
	virtual void SetPitch(float pitch) override;
	virtual bool OnMouseScroll(MouseScrolledEvent& e) override;
	virtual glm::mat4& GetViewMatrix() override;
	virtual glm::mat4 GetViewProjection() override { return m_ProjectionMatrix * m_ViewMatrix; }
	inline float& GetExposure() { return m_Exposure; }

private:
	void UpdateProjection();
	void UpdateView();

private:
	CameraController* m_CameraController;

};
