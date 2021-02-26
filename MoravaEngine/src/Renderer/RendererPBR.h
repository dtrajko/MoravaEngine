#pragma once

#include "RendererBasic.h"

#include "../../Cube.h"
#include "../../Cubemap.h"
#include "../../RadianceHDR.h"


class RendererPBR : public RendererBasic
{

public:
	RendererPBR();
	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	virtual ~RendererPBR() override;

private:
	RadianceHDR* m_RadianceHDR;
	Cubemap* m_EnvironmentCubemap;
	Cube* m_Cube1x1;

};
