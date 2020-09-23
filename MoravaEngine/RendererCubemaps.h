#pragma once

#include "RendererBasic.h"

#include "SceneCubemaps.h"
#include "AABB.h"
#include "Pivot.h"


class RendererCubemaps : public RendererBasic
{

public:
	RendererCubemaps();
	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	~RendererCubemaps();

private:
	std::map<std::string, ModelJoey*> models;
	float m_CubeRenderLastTime = 0.0f;
	float m_CubeRenderCooldown = 1.0f;
	glm::mat4 m_ModelCube;
	AABB* m_CubeAABB;
	Pivot* m_PivotCube;
	Pivot* m_PivotScene;

};
