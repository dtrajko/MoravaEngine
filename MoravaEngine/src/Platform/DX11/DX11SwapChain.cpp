#include "DX11SwapChain.h"
#include "DX11Context.h"
#include "DX11RendererBasic.h"

#include "Core/Log.h"

#include <GLFW/glfw3.h>


DX11SwapChain::DX11SwapChain(HWND hwnd, uint32_t width, uint32_t height, Hazel::Ref<DX11Context> dx11Context)
	: m_DX11Context(dx11Context)
{
	Init(hwnd, width, height);
}

DX11SwapChain::~DX11SwapChain()
{
	Cleanup();
}

void DX11SwapChain::Cleanup()
{
	// m_DX11RenderTargetBuffer->Release();
	// m_DX11DepthStencilBuffer->Release();
	m_DX11RenderTargetView->Release();
	m_DX11DepthStencilView->Release();
	m_DX11SwapChain->Release();
}

void DX11SwapChain::Init(HWND hwnd, uint32_t width, uint32_t height)
{
	ID3D11Device* dx11Device = m_DX11Context->GetDX11Device();
	IDXGIFactory* dxgiFactory = m_DX11Context->GetIDXGIFactory();

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

	HRESULT hr = dxgiFactory->CreateSwapChain(dx11Device, &desc, &m_DX11SwapChain);
	if (FAILED(hr))
	{
		throw std::exception("DX11SwapChain initialization failed.");
	}

	ReloadBuffers(width, height);
}

bool DX11SwapChain::Present(bool vsync)
{
	m_DX11SwapChain->Present(vsync, NULL);
	return true;
}

void DX11SwapChain::OnResize(uint32_t width, uint32_t height)
{
	MORAVA_CORE_WARN("DX11SwapChain::OnResize({0}, {1})", width, height);

	/* 1. Clear render targets from device context */
	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullRenderTargetViews[] = { nullptr };
	ID3D11DepthStencilView* nullDepthStencilView = nullptr;
	m_DX11Context->GetDX11DeviceContext()->OMSetRenderTargets(_countof(nullRenderTargetViews), nullRenderTargetViews, nullDepthStencilView);
	// m_DX11Context->GetDX11DeviceContext()->OMSetRenderTargets(0, 0, 0);

	// if (m_DX11RenderTargetBuffer) m_DX11RenderTargetBuffer->Release();
	// if (m_DX11DepthStencilBuffer) m_DX11DepthStencilBuffer->Release();
	// m_DX11RenderTargetView.Reset();
	// m_DX11DepthStencilView.Reset();
	m_DX11RenderTargetView->Release();
	m_DX11DepthStencilView->Release();

	m_DX11Context->GetDX11DeviceContext()->Flush();

	HRESULT hr = m_DX11SwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	if (FAILED(hr))
	{
		// Log::GetLogger()->error("DX11SwapChain::OnResize({0}, {1}) failed to resize buffers!", width, height);
		// throw std::exception("DX11SwapChain::OnResize() failed to resize buffers!");
	}

	ReloadBuffers(width, height);

	// m_DX11Context->GetDX11DeviceContext()->OMSetRenderTargets(1, &m_DX11RenderTargetView, m_DX11DepthStencilView);

	DX11RendererBasic::SetViewportSize(width, height);
}

void DX11SwapChain::ReloadBuffers(uint32_t width, uint32_t height)
{
	CreateRenderTargetView(width, height);
	CreateDepthStencilView(width, height);
}

void DX11SwapChain::CreateRenderTargetView(uint32_t width, uint32_t height)
{
	ID3D11Device* dx11Device = m_DX11Context->GetDX11Device();

	// Microsoft::WRL::ComPtr<ID3D11Texture2D> dx11RenderTargetBuffer;
	ID3D11Texture2D* dx11RenderTargetBuffer;
	HRESULT hr = m_DX11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&dx11RenderTargetBuffer));
	if (FAILED(hr))
	{
		throw std::exception("SwapChain: GetBuffer failed.");
	}

	hr = dx11Device->CreateRenderTargetView(dx11RenderTargetBuffer, 0, &m_DX11RenderTargetView);
	dx11RenderTargetBuffer->Release();

	if (FAILED(hr))
	{
		throw std::exception("SwapChain: CreateRenderTargetView failed.");
	}
	else if (SUCCEEDED(hr))
	{
		Log::GetLogger()->info("DX11SwapChain::CreateRenderTargetView({0}, {1}) successful!", width, height);
	}
}

void DX11SwapChain::CreateDepthStencilView(uint32_t width, uint32_t height)
{
	ID3D11Device* dx11Device = m_DX11Context->GetDX11Device();

	// Microsoft::WRL::ComPtr<ID3D11Texture2D> dx11DepthStencilBuffer;
	ID3D11Texture2D* dx11DepthStencilBuffer;
	HRESULT hr = m_DX11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&dx11DepthStencilBuffer));
	if (FAILED(hr))
	{
		throw std::exception("SwapChain: GetBuffer failed.");
	}

	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.MiscFlags = 0;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.CPUAccessFlags = 0;

	hr = dx11Device->CreateTexture2D(&depthStencilDesc, nullptr, &dx11DepthStencilBuffer);
	if (FAILED(hr))
	{
		throw std::exception("SwapChain: CreateTexture2D failed.");
	}

	hr = dx11Device->CreateDepthStencilView(dx11DepthStencilBuffer, NULL, &m_DX11DepthStencilView);
	dx11DepthStencilBuffer->Release();

	if (FAILED(hr))
	{
		throw std::exception("SwapChain: CreateDepthStencilView failed.");
	}
	else if (SUCCEEDED(hr))
	{
		Log::GetLogger()->info("DX11SwapChain::CreateDepthStencilView({0}, {1}) successful!", width, height);
	}
}

void DX11SwapChain::SetFullScreen(bool fullscreen, uint32_t width, uint32_t height)
{
	OnResize(width, height);
	m_DX11SwapChain->SetFullscreenState(fullscreen, nullptr);
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

void DX11SwapChain::ClearRenderTargetColor(float red, float green, float blue, float alpha)
{
	FLOAT clear_color[] = { red, green, blue, alpha };

	m_DX11Context->GetDX11DeviceContext()->ClearRenderTargetView(m_DX11RenderTargetView, clear_color);
	m_DX11Context->GetDX11DeviceContext()->ClearDepthStencilView(m_DX11DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	m_DX11Context->GetDX11DeviceContext()->OMSetRenderTargets(1, &m_DX11RenderTargetView, m_DX11DepthStencilView);
}
