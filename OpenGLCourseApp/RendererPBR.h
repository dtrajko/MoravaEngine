#pragma once

#include "RendererBasic.h"

#include "RadianceHDR.h"
#include "Cubemap.h"
#include "Cube.h"


class RendererPBR : public RendererBasic
{

public:

	RendererPBR();
	void Init();
	void SetUniforms();
	void SetShaders();
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass();
	~RendererPBR();

private:
	RadianceHDR* m_RadianceHDR;
	Cubemap* m_EnvironmentCubemap;
	Cube* m_Cube1x1;

};
