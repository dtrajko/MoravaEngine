#pragma once

#include "H2M/Scene/SceneCameraH2M.h"

#include "glm/glm.hpp"


class Camera : public H2M::SceneCameraH2M
{

public:
	Camera();
	Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch);
	Camera(glm::vec3 position, float yaw, float pitch, float fovDegrees, float aspectRatio, float moveSpeed, float turnSpeed);
	~Camera();

	virtual void OnUpdate(H2M::TimestepH2M ts) override;
	virtual void OnEvent(H2M::EventH2M& e) override;

	virtual void SetViewportSize(float width, float height) override;
	virtual void SetPitch(float pitch) override;
	virtual glm::mat4& GetViewMatrix() override;

	void UpdateProjection();
	void UpdateView();

};
