#include "RendererBasic.h"
#include "CommonValues.h"
#include "Application.h"


glm::mat4 RendererBasic::s_ProjectionMatrix;

RendererBasic::RendererBasic()
{
}

void RendererBasic::Init(Scene* scene)
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

void RendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
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

void RendererBasic::EnableTransparency()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RendererBasic::DisableTransparency()
{
	glDisable(GL_BLEND);
}

void RendererBasic::ClearDepthBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void RendererBasic::Cleanup()
{
	for (auto& shader : shaders)
		delete shader.second;

	shaders.clear();
	uniforms.clear();
}

void RendererBasic::UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene)
{
	uint32_t width = Application::Get()->GetWindow()->GetBufferWidth();
	uint32_t height = Application::Get()->GetWindow()->GetBufferHeight();
	scene->GetCameraController()->OnResize((float)width, (float)height);

	// Override the Projection matrix
	float aspectRatio = scene->GetCameraController()->GetAspectRatio();
	*projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio,
		scene->GetSettings().nearPlane, scene->GetSettings().farPlane);

	RendererBasic::SetProjectionMatrix(*projectionMatrix);
}

RendererBasic::~RendererBasic()
{
}
