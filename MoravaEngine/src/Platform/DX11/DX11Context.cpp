#include "DX11Context.h"
#include "DX11.h"

#include "Platform/Windows/WindowsWindow.h"

#include <glfw/glfw3.h>


DX11Context::DX11Context(Window* window)
	: m_Window(window)
{
}

void DX11Context::Create()
{
	MORAVA_CORE_INFO("DX11Context::Create");

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

	UINT Flags = NULL;
#if defined(DEBUG) || defined(_DEBUG)  
	Flags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT res = 0;
	for (UINT driver_type_index = 0; driver_type_index < num_driver_types;)
	{
		res = D3D11CreateDevice(NULL, driver_types[driver_type_index], NULL, Flags, feature_levels, num_feature_levels,
			D3D11_SDK_VERSION, &m_DX11Device, &m_FeatureLevel, &m_DX11DeviceContext);

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

	m_DX11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&m_DXGI_Device);
	m_DXGI_Device->GetParent(__uuidof(IDXGIAdapter), (void**)&m_DXGI_Adapter);
	m_DXGI_Adapter->GetParent(__uuidof(IDXGIFactory), (void**)&m_IDXGI_Factory);

	InitRasterizerState();

	m_Device = Hazel::Ref<DX11Device>::Create(m_PhysicalDevice);

	CreateSwapChain(m_Window->GetHWND(), m_Window->GetWidth(), m_Window->GetHeight());
}

DX11Context::~DX11Context()
{
	m_DX11DeviceContext->Release();
}

std::shared_ptr<DX11SwapChain> DX11Context::CreateSwapChain(HWND hwnd, UINT width, UINT height)
{
	try
	{
		m_SwapChain = std::make_shared<DX11SwapChain>(hwnd, width, height, this);
	}
	catch (const std::exception&)
	{
		throw std::exception("DX11SwapChain initialization failed.");
	}
	return m_SwapChain;
}

void DX11Context::SetRasterizerState(DX11CullMode cullMode)
{
	switch (cullMode)
	{
	case DX11CullMode::Front:
		m_DX11DeviceContext->RSSetState(m_CullStateFront);
		break;
	case DX11CullMode::Back:
		m_DX11DeviceContext->RSSetState(m_CullStateBack);
		break;
	case DX11CullMode::None:
		m_DX11DeviceContext->RSSetState(m_CullStateNone);
		break;
	default:
		m_DX11DeviceContext->RSSetState(m_CullStateNone);
		break;
	}
}

void DX11Context::InitRasterizerState()
{
	D3D11_RASTERIZER_DESC desc = {};
	desc.CullMode = D3D11_CULL_FRONT;
	desc.DepthClipEnable = true;
	desc.FillMode = D3D11_FILL_SOLID;
	m_DX11Device->CreateRasterizerState(&desc, &m_CullStateFront);

	desc.CullMode = D3D11_CULL_BACK;
	m_DX11Device->CreateRasterizerState(&desc, &m_CullStateBack);

	desc.CullMode = D3D11_CULL_NONE;
	m_DX11Device->CreateRasterizerState(&desc, &m_CullStateNone);
}

void DX11Context::OnResize(uint32_t width, uint32_t height)
{
	// m_SwapChain->OnResize(width, height); // done in WindowsWindow::OnSize();
}

void DX11Context::BeginFrame()
{
	m_SwapChain->BeginFrame();
}

void DX11Context::SwapBuffers()
{
	m_SwapChain->Present(m_VSync);
}

void DX11Context::SetViewportSize(uint32_t width, uint32_t height)
{
	D3D11_VIEWPORT vp = {};
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;

	m_DX11DeviceContext->RSSetViewports(1, &vp);
}
