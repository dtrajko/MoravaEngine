#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/Ref.h"

#include "DX11.h"
#include "DX11ConstantBuffer.h"
#include "DX11Texture2D.h"


class DX11PixelShader : public H2M::RefCounted
{

public:
	DX11PixelShader(const wchar_t* pixelShaderPath);
	virtual ~DX11PixelShader();

	inline ID3D11PixelShader* GetDX11Shader() { return m_DX11PixelShader; }

	inline void* GetBytecodePointer() { return m_BytecodePointer; }
	inline size_t GetBytecodeLength() { return m_BytecodeLength; }

	void Bind();
	void BindConstantBuffer(H2M::RefH2M<DX11ConstantBuffer> constantBuffer);

	// moved from DX11Context
	void SetTextures(const std::vector<H2M::RefH2M<H2M::HazelTexture>>& textures);

private:
	bool CompileDX11Shader(const wchar_t* fileName);
	void ReleaseCompiledDX11Shader();

private:
	ID3D11PixelShader* m_DX11PixelShader;

	ID3DBlob* m_Blob = nullptr;

	void* m_BytecodePointer = nullptr;
	size_t m_BytecodeLength = 0;

};
