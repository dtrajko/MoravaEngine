#include "DX11SwapChain.h"
#include "DX11Context.h"

#include "Core/Log.h"

#include <GLFW/glfw3.h>


DX11SwapChain::DX11SwapChain(HWND hwnd, uint32_t width, uint32_t height)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();
	IDXGIFactory* dxgiFactory = DX11Context::GetIDXGIFactory();

	Init(hwnd, width, height);
}

DX11SwapChain::~DX11SwapChain()
{
	Cleanup();
}

void DX11SwapChain::Cleanup()
{
	m_dsv->Release();
	m_rtv->Release();
	m_swap_chain->Release();
}

void DX11SwapChain::Init(HWND hwnd, uint32_t width, uint32_t height)
{
	m_Device = DX11Context::GetCurrentDevice();

	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();
	IDXGIFactory* dxgiFactory = DX11Context::GetIDXGIFactory();

	m_Width = width;
	m_Height = height;

	// SwapChain::SwapChain
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BufferCount = 1;
	desc.BufferDesc.Width = (UINT)width;
	desc.BufferDesc.Height = (UINT)height;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = hwnd;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	desc.Windowed = TRUE;

	HRESULT hr = dxgiFactory->CreateSwapChain(dx11Device, &desc, &m_swap_chain);

	if (FAILED(hr))
	{
		throw std::exception("DX11SwapChain initialization failed.");
	}

	ReloadBuffers(width, height);
}

bool DX11SwapChain::Present(bool vsync)
{
	m_swap_chain->Present(vsync, NULL);
	return true;
}

void DX11SwapChain::OnResize(uint32_t width, uint32_t height)
{
	MORAVA_CORE_WARN("DX11SwapChain::OnResize");
	// auto device = m_Device->GetDX11Device();

	// Create(&width, &height);
	// CreateDepthStencil();
	// CreateFramebuffer();

	if (m_rtv) m_rtv->Release();
	if (m_dsv) m_dsv->Release();

	m_swap_chain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	ReloadBuffers(width, height);
}

void DX11SwapChain::SetFullScreen(bool fullscreen, unsigned int width, unsigned int height)
{
	OnResize(width, height);
	m_swap_chain->SetFullscreenState(fullscreen, nullptr);
}

void DX11SwapChain::ReloadBuffers(uint32_t width, uint32_t height)
{
	CreateRenderTargetView();
	CreateDepthStencilView(width, height);
}

void DX11SwapChain::CreateRenderTargetView()
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	ID3D11Texture2D* buffer = NULL;
	HRESULT hr = m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);

	if (FAILED(hr))
	{
		throw std::exception("SwapChain: GetBuffer failed.");
	}

	hr = dx11Device->CreateRenderTargetView(buffer, NULL, &m_rtv);
	buffer->Release();

	if (FAILED(hr))
	{
		throw std::exception("SwapChain: CreateRenderTargetView failed.");
	}
}

void DX11SwapChain::CreateDepthStencilView(uint32_t width, uint32_t height)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	ID3D11Texture2D* buffer = NULL;
	HRESULT hr = m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);

	D3D11_TEXTURE2D_DESC tex_desc = {};
	tex_desc.Width = width;
	tex_desc.Height = height;
	tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	tex_desc.MipLevels = 1;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.MiscFlags = 0;
	tex_desc.ArraySize = 1;
	tex_desc.CPUAccessFlags = 0;

	hr = dx11Device->CreateTexture2D(&tex_desc, nullptr, &buffer);

	if (FAILED(hr))
	{
		throw std::exception("SwapChain: CreateTexture2D failed.");
	}

	hr = dx11Device->CreateDepthStencilView(buffer, NULL, &m_dsv);
	buffer->Release();

	if (FAILED(hr))
	{
		throw std::exception("SwapChain: CreateDepthStencilView failed.");
	}
}

void DX11SwapChain::InitSurface(GLFWwindow* windowHandle)
{
	// DX11PhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetDX11PhysicalDevice();

	// glfwCreateWindowSurface(m_Instance, windowHandle, nullptr, &m_Surface);

	// TODO
}

void DX11SwapChain::BeginFrame()
{
	// TODO
}

void DX11SwapChain::Create(uint32_t* width, uint32_t* height, bool vsync)
{
	// DX11Device device = m_Device->GetDX11Device();
	// DX11PhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetDX11PhysicalDevice();

	m_Width = *width;
	m_Height = *height;

	// CreateDepthStencil();
	// CreateFramebuffer();
}

//	void DX11SwapChain::CreateDrawBuffers()
//	{
//		// TODO
//	}

//	void DX11SwapChain::FindImageFormatAndColorSpace()
//	{
//		// DX11PhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetDX11PhysicalDevice();
//	
//		// TODO	}
//	}
