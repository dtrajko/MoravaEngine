#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "Hazel/Core/Buffer.h"
#include "Hazel/Renderer/IndexBuffer.h"


class DX11IndexBuffer : public Hazel::IndexBuffer
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
	virtual Hazel::RendererID GetRendererID() const override;

private:
	uint32_t m_Size;
	uint32_t m_Count;
	ID3D11Buffer* m_Buffer;

	// uint32_t m_Size = 0;
	// Hazel::Buffer m_LocalData;

};
