#include "DX11VertexBuffer.h"

#include "DX11Context.h"

#include "Hazel/Renderer/HazelRenderer.h"


// DX11VertexBuffer::DX11VertexBuffer(uint32_t size, Hazel::VertexBufferUsage usage) : m_Size(size) {}
// DX11VertexBuffer::DX11VertexBuffer(void* data, uint32_t size, Hazel::VertexBufferUsage usage) : m_Size(size) {}

DX11VertexBuffer::DX11VertexBuffer(void* list_vertices, uint32_t size_vertex, uint32_t size_list, void* shader_byte_code, uint32_t size_byte_shader)
{
	// m_LocalData = Hazel::Buffer::Copy(data, size);
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	D3D11_BUFFER_DESC buff_desc = {};
	buff_desc.Usage = D3D11_USAGE_DEFAULT;
	buff_desc.ByteWidth = size_vertex * size_list;
	buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buff_desc.CPUAccessFlags = 0;
	buff_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA init_data = {};
	init_data.pSysMem = list_vertices;

	m_size_vertex = size_vertex;
	m_size_list = size_list;

	HRESULT hr = dx11Device->CreateBuffer(&buff_desc, &init_data, &m_buffer);
	if (FAILED(hr))
	{
		throw std::exception("DX11VertexBuffer initialization failed.");
	}

	/**** BEGIN InputLayout TODO: move to DX11Pipeline::Invalidate ****/

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		// SEMANTIC NAME - SEMANTIC INDEX - FORMAT - INPUT SLOT - ALIGNED BYTE OFFSET - INPUT SLOT CLASS - INSTANCE DATA STEP RATE
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	UINT size_layout = ARRAYSIZE(layout);

	hr = dx11Device->CreateInputLayout(layout, size_layout, shader_byte_code, size_byte_shader, &m_layout);
	if (FAILED(hr))
	{
		throw std::exception("DX11VertexBuffer: CreateInputLayout failed.");
	}

	/**** END InputLayout TODO: move to DX11Pipeline::Invalidate ****/
}

DX11VertexBuffer::DX11VertexBuffer(void* data, uint32_t size, Hazel::VertexBufferUsage usage)
{
	Log::GetLogger()->error("DX11VertexBuffer::DX11VertexBuffer not implemented yet!");
}

DX11VertexBuffer::DX11VertexBuffer(uint32_t size, Hazel::VertexBufferUsage usage)
{
	Log::GetLogger()->error("DX11VertexBuffer::DX11VertexBuffer not implemented yet!");
}

uint32_t DX11VertexBuffer::GetSizeVertexList()
{
	return this->m_size_list;
}

DX11VertexBuffer::~DX11VertexBuffer()
{
	m_buffer->Release();
	m_layout->Release();
}
