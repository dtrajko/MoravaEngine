#pragma once

#include "RendererBasic.h"

#include "LearnOpenGL/ModelJoey.h"
#include "AABB.h"


class RendererEditorFramebuffer : public RendererBasic
{

public:
	RendererEditorFramebuffer();
	~RendererEditorFramebuffer();

	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	void RenderPassShadow(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderWaterEffects(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassWaterReflection(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassWaterRefraction(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);

private:
	std::map<std::string, int> m_OmniShadowTxSlots;

};
