#pragma once

#include "RendererBasic.h"

#include "Deferred/GBuffer.h"
#include "Mesh/Block.h"


class RendererDeferredOGL : public RendererBasic
{

public:
	RendererDeferredOGL();
	~RendererDeferredOGL();

	virtual void Init(Scene* scene) override;
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void ForwardPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

	virtual void SetShaders() override;

	void GeometryPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void LightPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

private:
	void SetupTextures();
	void SetupTextureSlots();
	void SetupMeshes();

	void CreateGBuffer();

private:
	GBuffer m_gbuffer;

	uint32_t m_TextureSlot_Diffuse;

	Hazel::Ref<Shader> m_ShaderForwardBasic;
	Hazel::Ref<Shader> m_ShaderGeometryPass;

	Hazel::Ref<Mesh> m_MeshBlock;

	uint32_t m_WindowWidthOld = 0;
	uint32_t m_WindowHeightOld = 0;

};
