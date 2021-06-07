#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/VertexBuffer.h"

#include "Hazel/Core/Buffer.h"

#include "DX11Allocator.h"


class DX11VertexBuffer : public Hazel::VertexBuffer
{
public:
	DX11VertexBuffer(void* data, uint32_t size, Hazel::VertexBufferUsage usage = Hazel::VertexBufferUsage::Static);
	DX11VertexBuffer(uint32_t size, Hazel::VertexBufferUsage usage = Hazel::VertexBufferUsage::Dynamic);

	virtual ~DX11VertexBuffer() {}

	virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override {}
	virtual void Bind() const override {}

	virtual const Hazel::VertexBufferLayout& GetLayout() const override { return {}; }
	virtual void SetLayout(const Hazel::VertexBufferLayout& layout) override {}

	virtual unsigned int GetSize() const override { return m_Size; }
	virtual Hazel::RendererID GetRendererID() const override { return 0; }

private:
	uint32_t m_Size = 0;
	Hazel::Buffer m_LocalData;

};
