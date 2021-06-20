#pragma once

#include "Hazel/Core/Ref.h"

#include <d3d11.h>


class DX11PixelShader : public Hazel::RefCounted
{

public:
	DX11PixelShader(const wchar_t* pixelShaderPath);
	~DX11PixelShader();

	inline ID3D11PixelShader* GetDX11Shader() { return m_DX11PixelShader; }

	inline void* GetBytecodePointer() { return m_BytecodePointer; }
	inline size_t GetBytecodeLength() { return m_BytecodeLength; }

private:
	bool CompileDX11Shader(const wchar_t* fileName);
	void ReleaseCompiledDX11Shader();

private:
	ID3D11PixelShader* m_DX11PixelShader;

	ID3DBlob* m_Blob = nullptr;

	void* m_BytecodePointer = nullptr;
	size_t m_BytecodeLength = 0;

};
