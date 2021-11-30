#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "H2M/Core/Buffer.h"
#include "H2M/Renderer/IndexBuffer.h"


class DX11IndexBuffer : public H2M::IndexBuffer
{
public:
	DX11IndexBuffer(void* data, uint32_t size);
	DX11IndexBuffer(uint32_t size);
	~DX11IndexBuffer();

	uint32_t GetIndexCount() { return m_Count; }
	inline ID3D11Buffer* GetBuffer() { return m_Buffer; }

	// DX11IndexBuffer(void* data, uint32_t size = 0);
	virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override {};
	virtual void Bind() const override;
	virtual uint32_t GetCount() const override { return m_Count; }
	virtual uint32_t GetSize() const override { return m_Size; }
	virtual H2M::RendererID GetRendererID() const override;

private:
	uint32_t m_Size;
	uint32_t m_Count;
	ID3D11Buffer* m_Buffer;

	// uint32_t m_Size = 0;
	// H2M::Buffer m_LocalData;

};
