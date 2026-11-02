#pragma once

#include "Renderer/RendererBasic.h"

#include "Framebuffer/Cubemap.h"
#include "Mesh/Cube.h"
#include "Texture/RadianceHDR.h"


class RendererPBR : public RendererBasic
{

public:
	RendererPBR();
	virtual ~RendererPBR() override;

	virtual void Init(Scene* scene) override;

	virtual void BeginFrame() override;
	virtual void WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;

	virtual void SetUniforms() override;
	virtual void SetShaders() override;

	void RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

private:
	RadianceHDR* m_RadianceHDR;
	Cubemap* m_EnvironmentCubemap;
	Cube* m_Cube1x1;

};
