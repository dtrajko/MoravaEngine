#pragma once

#include "Scene.h"


class SceneCottage : public Scene
{

public:
	SceneCottage();
	virtual void Update(float timestep) override;
	virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, bool shadowPass = false) override;
	~SceneCottage();

private:

};
