#pragma once

#include "Hazel/Core/Ref.h"

#include <d3d11.h>


class DX11VertexShader : public Hazel::RefCounted
{

public:
	DX11VertexShader(const wchar_t* vertexShaderPath);
	~DX11VertexShader();

	inline ID3D11VertexShader* GetDX11Shader() { return m_DX11VertexShader; }

	inline const void* GetBytecodeWithInputSignature() { return m_ShaderBytecodePointer; }
	inline size_t GetBytecodeLength() { return m_BytecodeLength; }

private:
	bool CompileDX11Shader(const wchar_t* fileName);
	void ReleaseCompiledDX11Shader();

private:
	ID3D11VertexShader* m_DX11VertexShader;

	ID3DBlob* m_Blob = nullptr;

	const void* m_ShaderBytecodePointer;
	size_t m_BytecodeLength;

};
