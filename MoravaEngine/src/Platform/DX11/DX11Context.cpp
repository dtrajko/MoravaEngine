#include "DX11Context.h"
#include "DX11.h"

#include <glfw/glfw3.h>


bool DX11Context::s_Validation = true;
IDXGIFactory* DX11Context::s_IDXGIFactory;
ID3D11DeviceContext* DX11Context::s_ImmediateContext;


DX11Context::DX11Context(GLFWwindow* windowHandle)
	: m_WindowHandle(windowHandle)
{
	D3D_DRIVER_TYPE driver_types[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT num_driver_types = ARRAYSIZE(driver_types);

	D3D_FEATURE_LEVEL feature_levels[] =
	{
		D3D_FEATURE_LEVEL_11_0
	};
	UINT num_feature_levels = ARRAYSIZE(feature_levels);

	HRESULT res = 0;
	for (UINT driver_type_index = 0; driver_type_index < num_driver_types;)
	{
		res = D3D11CreateDevice(NULL, driver_types[driver_type_index], NULL, NULL, feature_levels, num_feature_levels,
			D3D11_SDK_VERSION, &m_d3d_device, &m_feature_level, &s_ImmediateContext);

		if (SUCCEEDED(res))
		{
			break;
			++driver_type_index;
		}
	}

	if (FAILED(res))
	{
		throw std::exception("DX11Context: D3D11CreateDevice failed.");
	}

	m_d3d_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&m_dxgi_device);
	m_dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)&m_dxgi_adapter);
	m_dxgi_adapter->GetParent(__uuidof(IDXGIFactory), (void**)&s_IDXGIFactory);

	InitRasterizerState();
}

DX11Context::~DX11Context()
{
	m_device_context->Release();
}

DX11SwapChain* DX11Context::CreateSwapChain(HWND hwnd, UINT width, UINT height)
{
	try
	{
		m_SwapChain = new DX11SwapChain(hwnd, width, height);
	}
	catch (const std::exception&)
	{
		throw std::exception("SwapChain initialization failed.");
	}
	return m_SwapChain;
}

void DX11Context::SetRasterizerState(bool cull_front)
{
	if (cull_front) {
		s_ImmediateContext->RSSetState(m_cull_front_state);
	}
	else {
		s_ImmediateContext->RSSetState(m_cull_back_state);
	}
}

void DX11Context::InitRasterizerState()
{
	D3D11_RASTERIZER_DESC desc = {};
	desc.CullMode = D3D11_CULL_FRONT;
	desc.DepthClipEnable = true;
	desc.FillMode = D3D11_FILL_SOLID;
	m_d3d_device->CreateRasterizerState(&desc, &m_cull_front_state);

	desc.CullMode = D3D11_CULL_BACK;
	m_d3d_device->CreateRasterizerState(&desc, &m_cull_back_state);
}

void DX11Context::Create()
{
	MORAVA_CORE_INFO("DX11Context::Create");

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Application Info
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// TODO

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Instance and Surface Creation
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	m_PhysicalDevice = DX11PhysicalDevice::Select();

	m_Device = Hazel::Ref<DX11Device>::Create(m_PhysicalDevice);
		
	// m_SwapChain.Init(m_Device);
	// m_SwapChain.InitSurface(m_WindowHandle);

	uint32_t width = 1280, height = 720;
	// m_SwapChain.Create(&width, &height);
}

void DX11Context::OnResize(uint32_t width, uint32_t height)
{
	m_SwapChain->OnResize(width, height);
}

void DX11Context::BeginFrame()
{
	m_SwapChain->BeginFrame();
}

void DX11Context::SwapBuffers()
{
	m_SwapChain->Present(m_VSync);
}

void DX11Context::ClearRenderTargetColor(float red, float green, float blue, float alpha)
{
	FLOAT clear_color[] = { red, green, blue, alpha };

	m_device_context->ClearRenderTargetView(m_SwapChain->m_rtv, clear_color);
	m_device_context->ClearDepthStencilView(m_SwapChain->m_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	m_device_context->OMSetRenderTargets(1, &m_SwapChain->m_rtv, m_SwapChain->m_dsv);
}

void DX11Context::ClearDepthStencil()
{
	m_device_context->ClearDepthStencilView(m_SwapChain->m_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void DX11Context::SetViewportSize(uint32_t width, uint32_t height)
{
	D3D11_VIEWPORT vp = {};
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	m_device_context->RSSetViewports(1, &vp);
}

void DX11Context::SetVertexBuffer(Hazel::Ref<DX11VertexBuffer> vertexBuffer)
{
	uint32_t stride = vertexBuffer->GetBufferSize();
	uint32_t offset = 0;

	m_device_context->IASetVertexBuffers(0, 1, &vertexBuffer->m_buffer, &stride, &offset);
	m_device_context->IASetInputLayout(vertexBuffer->m_layout);
}

void DX11Context::SetIndexBuffer(Hazel::Ref<DX11IndexBuffer> indexBuffer)
{
	m_device_context->IASetIndexBuffer(indexBuffer->GetBuffer(), DXGI_FORMAT_R32_UINT, 0);
}