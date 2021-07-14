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
	Hazel::Ref<FramebufferTexture> GetHorizontalOutputTexture();
	Hazel::Ref<FramebufferTexture> GetVerticalOutputTexture();
	void Unbind(int width, int height);
	inline Hazel::Ref<MoravaFramebuffer> GetHorizontalFBO() { return m_HorizontalFBO; };
	inline Hazel::Ref<MoravaFramebuffer> GetVerticalFBO() { return m_VerticalFBO; };

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

	Hazel::Ref<MoravaFramebuffer> m_HorizontalFBO;
	Hazel::Ref<MoravaFramebuffer> m_VerticalFBO;

	int m_Width;
	int m_Height;
	int m_TextureID;

	Hazel::Ref<MoravaShader> m_ShaderHorizontalBlur;
	Hazel::Ref<MoravaShader> m_ShaderVerticalBlur;

	int m_OriginalTextureSlot;

};
