#include "DX11IndexBuffer.h"

#include "DX11Context.h"

#include "Hazel/Renderer/HazelRenderer.h"


DX11IndexBuffer::DX11IndexBuffer(void* list_indices, uint32_t size_list)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	D3D11_BUFFER_DESC buff_desc = {};
	buff_desc.Usage = D3D11_USAGE_DEFAULT;
	buff_desc.ByteWidth = size_list * 4;
	buff_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	buff_desc.CPUAccessFlags = 0;
	buff_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA init_data = {};
	init_data.pSysMem = list_indices;

	m_size_list = size_list;

	HRESULT hr = dx11Device->CreateBuffer(&buff_desc, &init_data, &m_buffer);
	if (FAILED(hr))
	{
		throw std::exception("DX11IndexBuffer initialization failed.");
	}
}

DX11IndexBuffer::DX11IndexBuffer(uint32_t size)
{
	Log::GetLogger()->error("DX11IndexBuffer(uint32_t size) not yet implemented!");
}

uint32_t DX11IndexBuffer::GetSizeIndexList()
{
	return m_size_list;
}

Hazel::RendererID DX11IndexBuffer::GetRendererID() const
{
	Log::GetLogger()->error("GetRendererID() Method not implemented yet!");
	return Hazel::RendererID();
}

DX11IndexBuffer::~DX11IndexBuffer()
{
	m_buffer->Release();
}

// void DX11IndexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset) {}
// void DX11IndexBuffer::Bind() const {}
// Hazel::RendererID DX11IndexBuffer::GetRendererID() const { return 0; }
