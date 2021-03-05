#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Scene/SceneCamera.h"

#include "glm/glm.hpp"


class Camera : public Hazel::SceneCamera
{

public:
	Camera();
	Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch);
	Camera(glm::vec3 position, float yaw, float pitch, float fovDegrees, float aspectRatio, float moveSpeed, float turnSpeed);
	~Camera();

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

};
