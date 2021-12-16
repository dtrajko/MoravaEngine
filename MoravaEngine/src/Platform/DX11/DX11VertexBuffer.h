#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11Device.h"

#include "H2M/Renderer/VertexBufferH2M.h"

// #include "H2M/Core/Buffer.h"
// #include "DX11Allocator.h"


class DX11VertexBuffer : public H2M::VertexBufferH2M
{
public:
	DX11VertexBuffer(void* data, uint32_t stride, uint32_t count);
	DX11VertexBuffer(void* data, uint32_t size, H2M::VertexBufferUsageH2M usage = H2M::VertexBufferUsageH2M::Static);
	DX11VertexBuffer(uint32_t size, H2M::VertexBufferUsageH2M usage = H2M::VertexBufferUsageH2M::Dynamic);
	virtual ~DX11VertexBuffer();


	virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override {}
	virtual void RT_SetData(void* buffer, uint32_t size, uint32_t offset = 0) override {}
	virtual void Bind() const override;

	virtual const H2M::VertexBufferLayoutH2M& GetLayout() const override { return m_Layout; }
	virtual void SetLayout(const H2M::VertexBufferLayoutH2M& layout) override { m_Layout = layout; }

	virtual uint32_t GetSize() const override { return m_Stride; }
	virtual uint32_t GetRendererID() const override { return 0; }

	inline uint32_t GetStride() { return m_Stride; }
	inline uint32_t GetVertexCount() { return m_Count; };
	inline ID3D11Buffer* GetDX11Buffer() { return m_Buffer; }

private:
	// uint32_t m_Size = 0;
	// H2M::BufferH2M m_LocalData;
	H2M::VertexBufferLayoutH2M m_Layout; // OpenGL-specific? Or it belongs to pipeline?

	uint32_t m_Stride;
	uint32_t m_Count;

	ID3D11Buffer* m_Buffer;

	friend class DX11Context;

};
