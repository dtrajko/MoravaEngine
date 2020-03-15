#include "RendererPBR.h"

#include "ShaderPBR.h"
#include "ShaderCubemap.h"
#include "ShaderSkyboxJoey.h"



RendererPBR::RendererPBR()
{
}

void RendererPBR::Init()
{
	SetUniforms();
	SetShaders();
}

void RendererPBR::SetUniforms()
{
}

void RendererPBR::SetShaders()
{
}

void RendererPBR::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererPBR::RenderPass()
{
}

RendererPBR::~RendererPBR()
{
}
