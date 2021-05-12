#pragma once

#include "RendererBasic.h"

#include "Deferred/GBuffer.h"
#include "Deferred/FramebufferSSAO.h"
#include "Mesh/Block.h"


class RendererDeferredOGL : public RendererBasic
{

public:
	RendererDeferredOGL();
	~RendererDeferredOGL();

	virtual void Init(Scene* scene) override;

	virtual void BeginFrame() override;
	virtual void WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;

	void ForwardPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

	virtual void SetShaders() override;

	void GeometryPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void LightPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

private:
	void SetupTextures();
	void SetupTextureSlots();
	void SetupMeshes();

	void CreateBuffers();

private:
	GBuffer m_gbuffer;
	FramebufferSSAO m_FramebufferSSAO;

	uint32_t m_TextureSlot_Diffuse;

	Hazel::Ref<Shader> m_ShaderForwardBasic;
	Hazel::Ref<Shader> m_ShaderGeometryPass;

	Hazel::Ref<Mesh> m_MeshBlock;

	uint32_t m_WindowWidthOld = 0;
	uint32_t m_WindowHeightOld = 0;

};
