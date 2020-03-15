#include "RendererBasic.h"

#include "GL/glew.h"
#include "glm/gtc/matrix_transform.hpp"
#include <GLFW/glfw3.h>



RendererBasic::RendererBasic()
{
}

void RendererBasic::Init()
{
	SetUniforms();
	SetShaders();

	bgColor = glm::vec4(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 1.0f);
}

void RendererBasic::SetUniforms()
{
	// common
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
	uniforms.insert(std::make_pair("projection", 0));
	uniforms.insert(std::make_pair("nearPlane", 0));
	uniforms.insert(std::make_pair("farPlane", 0));
	uniforms.insert(std::make_pair("normalMap", 0));
	uniforms.insert(std::make_pair("lightPosition", 0));

}

void RendererBasic::SetShaders()
{
}

void RendererBasic::RenderPass(Scene* scene, glm::mat4 projectionMatrix, Window& mainWindow)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Rendering here
}

void RendererBasic::EnableCulling()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

void RendererBasic::DisableCulling()
{
	glDisable(GL_CULL_FACE);
}

void RendererBasic::Cleanup()
{
	for (auto& shader : shaders)
		delete shader.second;

	shaders.clear();
	uniforms.clear();
}

RendererBasic::~RendererBasic()
{
}
