#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/Ref.h"

#include "DX11.h"
#include "DX11ConstantBuffer.h"
#include "DX11Texture2D.h"


class DX11VertexShader : public H2M::RefCounted
{

public:
	DX11VertexShader(const wchar_t* vertexShaderPath);
	virtual ~DX11VertexShader();

	inline ID3D11VertexShader* GetDX11Shader() { return m_DX11VertexShader; }

	inline void* GetBytecodePointer() { return m_BytecodePointer; }
	inline size_t GetBytecodeLength() { return m_BytecodeLength; }

	void Bind();
	void BindConstantBuffer(H2M::Ref<DX11ConstantBuffer> constantBuffer);

	// moved from DX11Context
	void SetTextures(const std::vector<H2M::Ref<H2M::HazelTexture>>& textures);

private:
	bool CompileDX11Shader(const wchar_t* fileName);
	void ReleaseCompiledDX11Shader();

private:
	ID3D11VertexShader* m_DX11VertexShader;

	ID3DBlob* m_Blob = nullptr;

	void* m_BytecodePointer = nullptr;
	size_t m_BytecodeLength = 0;

};
