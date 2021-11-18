#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Core/Events/Event.h"
#include "Hazel/Core/Events/MouseEvent.h"
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
	virtual glm::mat4& GetViewMatrix() override;

	void UpdateProjection();
	void UpdateView();

};
