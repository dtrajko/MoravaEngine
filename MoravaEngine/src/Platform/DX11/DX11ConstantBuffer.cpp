#include "DX11ConstantBuffer.h"

#include "DX11Context.h"


DX11ConstantBuffer::DX11ConstantBuffer(void* buffer, uint32_t bufferSize)
	: m_BufferSize(bufferSize)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetCurrentDevice()->GetDX11Device();

	D3D11_BUFFER_DESC buff_desc = {};
	buff_desc.Usage = D3D11_USAGE_DEFAULT;
	buff_desc.ByteWidth = bufferSize;
	buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buff_desc.CPUAccessFlags = 0;
	buff_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA init_data = {};
	init_data.pSysMem = buffer;

	HRESULT hr = dx11Device->CreateBuffer(&buff_desc, &init_data, &m_buffer);
	if (FAILED(hr))
	{
		throw std::exception("DX11ConstantBuffer initialization failed.");
	}
}

void DX11ConstantBuffer::Update(void* buffer)
{
	ID3D11DeviceContext* immediateContext = DX11Context::Get()->GetImmediateContext();

	immediateContext->UpdateSubresource(m_buffer, NULL, NULL, buffer, NULL, NULL);
}

DX11ConstantBuffer::~DX11ConstantBuffer()
{
	m_buffer->Release();
}
