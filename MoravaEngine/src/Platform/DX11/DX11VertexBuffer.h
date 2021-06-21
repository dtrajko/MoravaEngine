#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11Device.h"
#include "DX11Shader.h"

#include "Hazel/Renderer/VertexBuffer.h"

// #include "Hazel/Core/Buffer.h"
// #include "DX11Allocator.h"


class DX11VertexBuffer : public Hazel::VertexBuffer
{
public:
	DX11VertexBuffer(void* data, uint32_t stride, uint32_t count, Hazel::Ref<DX11Shader> shader);
	DX11VertexBuffer(void* data, uint32_t size, Hazel::VertexBufferUsage usage = Hazel::VertexBufferUsage::Static);
	DX11VertexBuffer(uint32_t size, Hazel::VertexBufferUsage usage = Hazel::VertexBufferUsage::Dynamic);
	virtual ~DX11VertexBuffer();


	virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override {}
	virtual void Bind() const override {}

	virtual const Hazel::VertexBufferLayout& GetLayout() const override { return m_Layout; }
	virtual void SetLayout(const Hazel::VertexBufferLayout& layout) override { m_Layout = layout; }

	virtual uint32_t GetSize() const override { return m_Stride; }
	virtual Hazel::RendererID GetRendererID() const override { return 0; }

	inline uint32_t GetStride() { return m_Stride; }
	inline uint32_t GetVertexCount() { return m_Count; };
	inline ID3D11Buffer* GetDX11Buffer() { return m_Buffer; }

private:
	// uint32_t m_Size = 0;
	// Hazel::Buffer m_LocalData;
	Hazel::VertexBufferLayout m_Layout; // OpenGL-specific? Or it belongs to pipeline?

	uint32_t m_Stride;
	uint32_t m_Count;

	ID3D11Buffer* m_Buffer;

	friend class DX11Context;

};
