#include "DX11ConstantBuffer.h"

#include "DX11Context.h"


DX11ConstantBuffer::DX11ConstantBuffer(void* buffer, size_t bufferSize)
	: m_BufferSize(bufferSize)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	D3D11_BUFFER_DESC buff_desc = {};
	buff_desc.Usage = D3D11_USAGE_DEFAULT;
	buff_desc.ByteWidth = (UINT)bufferSize;
	buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buff_desc.CPUAccessFlags = 0;
	buff_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA init_data = {};
	init_data.pSysMem = buffer;

	HRESULT hr = dx11Device->CreateBuffer(&buff_desc, &init_data, &m_Buffer);
	if (FAILED(hr))
	{
		throw std::exception("DX11ConstantBuffer initialization failed.");
	}

	Log::GetLogger()->info("DX11ConstantBuffer successfully created!");
}

void DX11ConstantBuffer::Update(void* buffer)
{
	ID3D11DeviceContext* immediateContext = DX11Context::Get()->GetImmediateContext();

	immediateContext->UpdateSubresource(m_Buffer, NULL, NULL, buffer, NULL, NULL);
}

DX11ConstantBuffer::~DX11ConstantBuffer()
{
	if (m_Buffer) m_Buffer->Release();
}
 