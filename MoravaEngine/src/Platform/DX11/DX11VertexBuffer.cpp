#include "DX11VertexBuffer.h"

#include "DX11Context.h"

#include "Hazel/Renderer/HazelRenderer.h"


DX11VertexBuffer::DX11VertexBuffer(void* data, uint32_t stride, uint32_t count, Hazel::Ref<DX11Shader> shader)
{
	// m_LocalData = Hazel::Buffer::Copy(data, size);
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	HRESULT hr{};

	D3D11_BUFFER_DESC buff_desc = {};
	buff_desc.Usage = D3D11_USAGE_DEFAULT;
	buff_desc.ByteWidth = stride * count;
	buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buff_desc.CPUAccessFlags = 0;
	buff_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA init_data = {};
	init_data.pSysMem = data;

	m_Stride = stride;
	m_Count = count;

	hr = dx11Device->CreateBuffer(&buff_desc, &init_data, &m_Buffer);
	if (FAILED(hr))
	{
		throw std::exception("DX11VertexBuffer initialization failed.");
	}


	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		// SEMANTIC NAME - SEMANTIC INDEX - FORMAT - INPUT SLOT - ALIGNED BYTE OFFSET - INPUT SLOT CLASS - INSTANCE DATA STEP RATE
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	UINT numElements = ARRAYSIZE(layout);

	const void* shaderBytecodePointer = shader->GetVertexShader()->GetBytecodePointer();
	size_t shaderBytecodeLength = shader->GetVertexShader()->GetBytecodeLength();

	// ::memcpy(&m_InputLayoutBuffer, shaderBytecodePointer, shaderBytecodeLength);

	hr = dx11Device->CreateInputLayout(layout, numElements, shaderBytecodePointer, shaderBytecodeLength, &m_InputLayout);
	if (FAILED(hr))
	{
		throw std::exception("DX11Pipeline: CreateInputLayout failed.");
	}

	Log::GetLogger()->info("DX11Pipeline: InputLayout successfully created!");
}

DX11VertexBuffer::DX11VertexBuffer(void* data, uint32_t size, Hazel::VertexBufferUsage usage)
{
	Log::GetLogger()->error("DX11VertexBuffer::DX11VertexBuffer not implemented yet!");
}

DX11VertexBuffer::DX11VertexBuffer(uint32_t size, Hazel::VertexBufferUsage usage)
{
	Log::GetLogger()->error("DX11VertexBuffer::DX11VertexBuffer not implemented yet!");
}

DX11VertexBuffer::~DX11VertexBuffer()
{
	m_Buffer->Release();
	m_InputLayout->Release();
}
