#pragma once

#include "RendererBasic.h"

#include "RadianceHDR.h"
#include "Cubemap.h"
#include "Cube.h"


class RendererIBL : public RendererBasic
{

public:

	static void Init();
	static void SetUniforms();
	static void SetShaders();
	static std::map<std::string, Shader*> GetShaders() { return shaders; };

	static void RenderPass(glm::mat4 projectionMatrix, Window& mainWindow, Scene* scene, Camera* camera, WaterManager* waterManager);
	static void RenderEnvironmentCubemap(Window& mainWindow, Scene* scene);
	static void RenderSimpleSkyboxJoey(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene);

private:

	static RadianceHDR* m_RadianceHDR;
	static Cubemap* m_EnvironmentCubemap;
	static Cube* m_Cube1x1;

};
