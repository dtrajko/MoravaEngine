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

	void Update();

	void OnUpdate(Hazel::Timestep ts);
	void OnEvent(Event& e);

	bool OnMouseScroll(MouseScrolledEvent& e);

	void SetPitch(float pitch);

};
