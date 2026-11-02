#pragma once

#include "Framebuffer/MoravaFramebuffer.h"
#include "Shader/MoravaShader.h"
#include "Texture/MoravaTexture.h"


class BlurEffect
{
public:
	BlurEffect();
	~BlurEffect();

	void Init(int width, int height, int textureID);
	void Generate(int width, int height);
	void Render();
	H2M::RefH2M<FramebufferTexture> GetHorizontalOutputTexture();
	H2M::RefH2M<FramebufferTexture> GetVerticalOutputTexture();
	void Unbind(int width, int height);
	inline H2M::RefH2M<MoravaFramebuffer> GetHorizontalFBO() { return m_HorizontalFBO; };
	inline H2M::RefH2M<MoravaFramebuffer> GetVerticalFBO() { return m_VerticalFBO; };

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

	H2M::RefH2M<MoravaFramebuffer> m_HorizontalFBO;
	H2M::RefH2M<MoravaFramebuffer> m_VerticalFBO;

	int m_Width;
	int m_Height;
	int m_TextureID;

	H2M::RefH2M<MoravaShader> m_ShaderHorizontalBlur;
	H2M::RefH2M<MoravaShader> m_ShaderVerticalBlur;

	int m_OriginalTextureSlot;

};
