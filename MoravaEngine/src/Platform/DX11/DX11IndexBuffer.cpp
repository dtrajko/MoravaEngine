#include "DX11IndexBuffer.h"

#include "DX11Context.h"

#include "H2M/Renderer/RendererH2M.h"


DX11IndexBuffer::DX11IndexBuffer(void* data, uint32_t size)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	D3D11_BUFFER_DESC buff_desc = {};
	buff_desc.Usage = D3D11_USAGE_DEFAULT;
	buff_desc.ByteWidth = size;
	buff_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	buff_desc.CPUAccessFlags = 0;
	buff_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA init_data = {};
	init_data.pSysMem = data;

	m_Size = size;
	m_Count = size / sizeof(uint32_t);

	HRESULT hr = dx11Device->CreateBuffer(&buff_desc, &init_data, &m_Buffer);
	if (FAILED(hr))
	{
		throw std::exception("DX11IndexBuffer initialization failed.");
	}

	Log::GetLogger()->info("DX11IndexBuffer successfully created!");
}

DX11IndexBuffer::DX11IndexBuffer(uint32_t size)
{
	Log::GetLogger()->error("DX11IndexBuffer(uint32_t size) not yet implemented!");
}

void DX11IndexBuffer::Bind() const
{
	DX11Context::Get()->GetDX11DeviceContext()->IASetIndexBuffer(m_Buffer, DXGI_FORMAT_R32_UINT, 0);
}

H2M::RendererID_H2M DX11IndexBuffer::GetRendererID() const
{
	Log::GetLogger()->error("GetRendererID() Method not implemented yet!");
	return H2M::RendererID();
}

DX11IndexBuffer::~DX11IndexBuffer()
{
	m_Buffer->Release();

	Log::GetLogger()->info("DX11IndexBuffer destroyed!");
}

// void DX11IndexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset) {}
// void DX11IndexBuffer::Bind() const {}
// H2M::RendererID_H2M DX11IndexBuffer::GetRendererID() const { return 0; }
