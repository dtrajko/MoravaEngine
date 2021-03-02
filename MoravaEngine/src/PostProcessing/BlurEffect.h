#pragma once

#include "Framebuffer/Framebuffer.h"
#include "Shader/Shader.h"
#include "Texture/Texture.h"


class BlurEffect
{
public:
	BlurEffect();
	~BlurEffect();

	void Init(int width, int height, int textureID);
	void Generate(int width, int height);
	void Render();
	FramebufferTexture* GetHorizontalOutputTexture();
	FramebufferTexture* GetVerticalOutputTexture();
	void Unbind(int width, int height);
	inline Framebuffer* GetHorizontalFBO() { return m_HorizontalFBO; };
	inline Framebuffer* GetVerticalFBO() { return m_VerticalFBO; };

private:
	void SetupShaders();
	void SetupGeometry();
	void HorizontalBlurSetup(int width, int height);
	void VerticalBlurSetup(int width, int height);
	void RenderHorizontal(int textureHorizontal);
	void RenderVertical(int textureVertical);
	void RenderQuadHorizontal();
	void RenderQuadVertical();
	void Release();

private:
	unsigned int m_QuadVAO;
	unsigned int m_QuadVBO;

	Framebuffer* m_HorizontalFBO;
	Framebuffer* m_VerticalFBO;

	int m_Width;
	int m_Height;
	int m_TextureID;

	Shader* m_ShaderHorizontalBlur;
	Shader* m_ShaderVerticalBlur;

	int m_OriginalTextureSlot;

};
