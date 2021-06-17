#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11Device.h"

#include "Hazel/Renderer/VertexBuffer.h"

// #include "Hazel/Core/Buffer.h"
// #include "DX11Allocator.h"


class DX11VertexBuffer : public Hazel::VertexBuffer
{
public:
	DX11VertexBuffer(void* list_vertices, uint32_t size_vertex, uint32_t size_list, void* shader_byte_code, uint32_t size_byte_shader);
	DX11VertexBuffer(void* data, uint32_t size, Hazel::VertexBufferUsage usage = Hazel::VertexBufferUsage::Static);
	DX11VertexBuffer(uint32_t size, Hazel::VertexBufferUsage usage = Hazel::VertexBufferUsage::Dynamic);
	virtual ~DX11VertexBuffer();

	uint32_t GetSizeVertexList();

	virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override {}
	virtual void Bind() const override {}

	virtual const Hazel::VertexBufferLayout& GetLayout() const override { return {}; }
	virtual void SetLayout(const Hazel::VertexBufferLayout& layout) override {}

	virtual uint32_t GetSize() const override { return m_size_vertex; }
	virtual Hazel::RendererID GetRendererID() const override { return 0; }

	inline uint32_t GetBufferSize() { return m_size_vertex; }
	inline ID3D11Buffer* GetBuffer() { return m_buffer; }

private:
	// uint32_t m_Size = 0;
	// Hazel::Buffer m_LocalData;

	uint32_t m_size_vertex;
	uint32_t m_size_list;

	ID3D11Buffer* m_buffer;
	ID3D11InputLayout* m_layout;

	friend class DX11Context;

};
