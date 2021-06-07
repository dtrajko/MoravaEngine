#include "DX11IndexBuffer.h"

#include "DX11Context.h"

#include "Hazel/Renderer/HazelRenderer.h"


DX11IndexBuffer::DX11IndexBuffer(uint32_t size)
	: m_Size(size)
{
}

DX11IndexBuffer::DX11IndexBuffer(void* data, uint32_t size)
	: m_Size(size)
{
	m_LocalData = Hazel::Buffer::Copy(data, size);

	// auto device = DX11Context::GetCurrentDevice()->GetDX11Device();

	// TODO
}

void DX11IndexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
{
}

void DX11IndexBuffer::Bind() const
{
}

Hazel::RendererID DX11IndexBuffer::GetRendererID() const
{
	return 0;
}
