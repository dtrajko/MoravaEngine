#pragma once

#include "Scene.h"


class SceneSponza : public Scene
{

public:
	SceneSponza();
	virtual void Update(float timestep) override;
	virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, bool shadowPass = false) override;
	~SceneSponza();

private:

};
