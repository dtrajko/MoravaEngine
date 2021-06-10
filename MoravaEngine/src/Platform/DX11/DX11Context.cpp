#include "DX11Context.h"
#include "DX11.h"

#include <glfw/glfw3.h>


static bool s_Validation = true;


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
			D3D11_SDK_VERSION, &m_d3d_device, &m_feature_level, &m_imm_context);

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
	m_dxgi_adapter->GetParent(__uuidof(IDXGIFactory), (void**)&m_dxgi_factory);

	InitRasterizerState();
}

DX11Context::~DX11Context()
{
}

DX11SwapChain* DX11Context::CreateSwapChain(HWND hwnd, UINT width, UINT height)
{
	try
	{
		m_SwapChain = new DX11SwapChain(m_Device, m_dxgi_factory, hwnd, width, height);
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
		m_imm_context->RSSetState(m_cull_front_state);
	}
	else {
		m_imm_context->RSSetState(m_cull_back_state);
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
	// m_SwapChain.OnResize(width, height);
}

void DX11Context::BeginFrame()
{
	// m_SwapChain.BeginFrame();
}

void DX11Context::SwapBuffers()
{
	// m_SwapChain.Present();
}
