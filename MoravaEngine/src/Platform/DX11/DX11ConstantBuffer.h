#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11Device.h"


class DX11ConstantBuffer
{
public:
	DX11ConstantBuffer() = default;
	DX11ConstantBuffer(void* buffer, uint32_t size_buffer);
	virtual ~DX11ConstantBuffer();

	void Update(void* buffer);

private:
	ID3D11Buffer* m_buffer;

};
