#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11Device.h"


class DX11ConstantBuffer : public H2M::RefCountedH2M
{
public:
	DX11ConstantBuffer() = default;
	DX11ConstantBuffer(void* buffer, size_t bufferSize);
	virtual ~DX11ConstantBuffer();

	void Update(void* buffer);

	inline ID3D11Buffer* GetBuffer() { return m_Buffer; }
	inline size_t GetBufferSize() { return m_BufferSize; }

private:
	ID3D11Buffer* m_Buffer;
	size_t m_BufferSize;

	friend class DX11Context;
	friend class DX11Renderer;
	friend class DX11VertexShader;
	friend class DX11PixelShader;

};
