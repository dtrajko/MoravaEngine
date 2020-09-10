#pragma once

#include "RendererBasic.h"

#include "LearnOpenGL/ModelJoey.h"


class RendererAsteroids : public RendererBasic
{

public:
	RendererAsteroids();
	virtual void Init(Scene* scene) override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	~RendererAsteroids();

private:
	unsigned int quadVAO;
	unsigned int quadVBO;

	// Asteroid field
	unsigned int amount = 50000;
	glm::mat4* modelMatrices;

	std::map<std::string, ModelJoey*> models;

	float planetRotationY = 0.0f;
	float m_FOV = 60.0f;

};
