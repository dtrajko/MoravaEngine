#pragma once

#include "Hazel/Scene/SceneCamera.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/MouseEvent.h"

#include "glm/glm.hpp"


class Camera : public Hazel::SceneCamera
{

public:
	Camera();
	Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch);
	~Camera();

	virtual void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnEvent(Event& e) override;
	virtual void SetPitch(float pitch) override;
	virtual bool OnMouseScroll(MouseScrolledEvent& e) override;
	virtual glm::mat4& GetViewMatrix() override;

};
