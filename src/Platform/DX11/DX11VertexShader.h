#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/RefH2M.h"

#include "DX11.h"
#include "DX11ConstantBuffer.h"
#include "DX11Texture2D.h"


class DX11VertexShader : public H2M::RefCountedH2M
{

public:
	DX11VertexShader(const wchar_t* vertexShaderPath);
	virtual ~DX11VertexShader();

	inline ID3D11VertexShader* GetDX11Shader() { return m_DX11VertexShader; }

	inline void* GetBytecodePointer() { return m_BytecodePointer; }
	inline size_t GetBytecodeLength() { return m_BytecodeLength; }

	void Bind();
	void BindConstantBuffer(H2M::RefH2M<DX11ConstantBuffer> constantBuffer);

	// moved from DX11Context
	void SetTextures(const std::vector<H2M::RefH2M<H2M::TextureH2M>>& textures);

private:
	bool CompileDX11Shader(const wchar_t* fileName);
	void ReleaseCompiledDX11Shader();

private:
	ID3D11VertexShader* m_DX11VertexShader;

	ID3DBlob* m_Blob = nullptr;

	void* m_BytecodePointer = nullptr;
	size_t m_BytecodeLength = 0;

};
