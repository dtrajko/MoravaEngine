#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11Device.h"


class DX11ConstantBuffer : public Hazel::RefCounted
{
public:
	DX11ConstantBuffer() = default;
	DX11ConstantBuffer(void* buffer, uint32_t bufferSize);
	virtual ~DX11ConstantBuffer();

	void Update(void* buffer);

	inline ID3D11Buffer* GetBuffer() { return m_Buffer; }
	inline uint32_t GetBufferSize() { return m_BufferSize; }

private:
	ID3D11Buffer* m_Buffer;
	uint32_t m_BufferSize;

	friend class DX11Context;

};
