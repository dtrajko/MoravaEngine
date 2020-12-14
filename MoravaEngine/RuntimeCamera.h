#pragma once

#include "Hazel/Renderer/HazelCamera.h"
#include "Hazel/Core/Timestep.h"

#include "Camera.h"
#include "CameraController.h"


/**
 * RuntimeCamera bundles Camera and CameraController and shares the interface with EditorCamera  
 */
class RuntimeCamera : public Hazel::HazelCamera
{

public:
	RuntimeCamera();
	RuntimeCamera(float fov, float aspectRatio, float nearClip, float farClip);

	void OnUpdate(Hazel::Timestep ts);
	void OnEvent(Event& e);

	virtual void SetViewportSize(float width, float height) override;

	const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
	virtual glm::mat4 GetViewProjection() override { return m_ProjectionMatrix * m_ViewMatrix; } // m_CameraController.CalculateViewMatrix()

	inline float& GetExposure() { return m_Exposure; }

public:
	CameraController m_CameraController;

};
