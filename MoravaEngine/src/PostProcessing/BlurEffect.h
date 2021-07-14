#pragma once

#include "Framebuffer/MoravaFramebuffer.h"
#include "Shader/MoravaShader.h"
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
	inline MoravaFramebuffer* GetHorizontalFBO() { return m_HorizontalFBO; };
	inline MoravaFramebuffer* GetVerticalFBO() { return m_VerticalFBO; };

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

	MoravaFramebuffer* m_HorizontalFBO;
	MoravaFramebuffer* m_VerticalFBO;

	int m_Width;
	int m_Height;
	int m_TextureID;

	MoravaShader* m_ShaderHorizontalBlur;
	MoravaShader* m_ShaderVerticalBlur;

	int m_OriginalTextureSlot;

};
