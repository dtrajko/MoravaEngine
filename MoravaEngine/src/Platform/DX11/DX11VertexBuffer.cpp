#include "DX11VertexBuffer.h"

#include "DX11Context.h"

#include "Hazel/Renderer/HazelRenderer.h"


DX11VertexBuffer::DX11VertexBuffer(uint32_t size, Hazel::VertexBufferUsage usage)
	: m_Size(size)
{
}

DX11VertexBuffer::DX11VertexBuffer(void* data, uint32_t size, Hazel::VertexBufferUsage usage)
	: m_Size(size)
{
	m_LocalData = Hazel::Buffer::Copy(data, size);

	// TODO: Use staging
	// auto device = DX11Context::GetCurrentDevice()->GetDX11Device();

	// TODO
}
