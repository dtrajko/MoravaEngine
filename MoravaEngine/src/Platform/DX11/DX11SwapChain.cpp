#include "DX11SwapChain.h"
#include "DX11Context.h"
#include "DX11RendererBasic.h"

#include "Core/Log.h"

#include <GLFW/glfw3.h>


DX11SwapChain::DX11SwapChain(HWND hwnd, uint32_t width, uint32_t height, H2M::RefH2M<DX11Context> dx11Context)
	: m_HWND(hwnd), m_Width(width), m_Height(height), m_DX11Context(dx11Context)
{
	Invalidate();
}

void DX11SwapChain::OnResize(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;

	Invalidate();
}

DX11SwapChain::~DX11SwapChain()
{
	Cleanup();
}

void DX11SwapChain::Cleanup()
{
	// m_RenderTargetBufferDX11->Release();
	// m_DepthStencilBufferDX11->Release();
	// m_RenderTargetViewDX11->Release();
	// m_DepthStencilViewDX11->Release();

	m_RenderTargetBufferDX11->Release();
	m_DepthStencilBufferDX11->Release();
	m_RenderTargetViewDX11->Release();
	m_DepthStencilViewDX11->Release();
	m_SwapChainDX11->Release();
}

void DX11SwapChain::Invalidate()
{
	MORAVA_CORE_WARN("DX11SwapChain::OnResize({0}, {1})", m_Width, m_Height);

	ID3D11Device* dx11Device = m_DX11Context->GetDX11Device();
	IDXGIFactory* dxgiFactory = m_DX11Context->GetIDXGIFactory();

	/* 1. Clear render targets from device context */
	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullRenderTargetViews[] = { nullptr };
	ID3D11DepthStencilView* nullDepthStencilView = nullptr;
	m_DX11Context->GetDX11DeviceContext()->OMSetRenderTargets(_countof(nullRenderTargetViews), nullRenderTargetViews, nullDepthStencilView);
	// m_DX11Context->GetDX11DeviceContext()->OMSetRenderTargets(0, 0, 0);

	if (m_RenderTargetBufferDX11) m_RenderTargetBufferDX11.Reset();
	if (m_DepthStencilBufferDX11) m_DepthStencilBufferDX11.Reset();
	if (m_RenderTargetViewDX11) m_RenderTargetViewDX11.Reset();
	if (m_DepthStencilViewDX11) m_DepthStencilViewDX11.Reset();

	m_DX11Context->GetDX11DeviceContext()->ClearState();
	m_DX11Context->GetDX11DeviceContext()->Flush();

	HRESULT hr;

	m_SwapChainDX11.Reset();

	// SwapChain::SwapChain
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BufferCount = 1;
	desc.BufferDesc.Width = (UINT)m_Width;
	desc.BufferDesc.Height = (UINT)m_Height;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = m_HWND;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	desc.Windowed = TRUE;

	hr = dxgiFactory->CreateSwapChain(dx11Device, &desc, m_SwapChainDX11.GetAddressOf());
	if (FAILED(hr))
	{
		throw std::exception("DX11SwapChain initialization failed.");
	}
	else if (SUCCEEDED(hr))
	{
		Log::GetLogger()->info("IDXGISwapChain (buffer size: {0}x{1}) successfully created!", m_Width, m_Height);
	}

	hr = m_SwapChainDX11->ResizeBuffers(1, m_Width, m_Height, DXGI_FORMAT_UNKNOWN, 0); // DXGI_FORMAT_R8G8B8A8_UNORM
	if (FAILED(hr))
	{
		Log::GetLogger()->error("DX11SwapChain::OnResize({0}, {1}) failed to resize buffers!", m_Width, m_Height);
		throw std::exception("DX11SwapChain::OnResize() failed to resize buffers!");
	}

	ReloadBuffers();

	// m_DX11Context->GetDX11DeviceContext()->OMSetRenderTargets(1, &m_DX11RenderTargetView, m_DX11DepthStencilView);
	// DX11RendererBasic::SetViewportSize(m_Width, m_Height);

	D3D11_VIEWPORT vp = {};
	vp.Width = (FLOAT)m_Width;
	vp.Height = (FLOAT)m_Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;

	m_DX11Context->GetDX11DeviceContext()->RSSetViewports(1, &vp);

	m_FullscreenToggleMode = false;
}

bool DX11SwapChain::Present(bool vsync)
{
	m_SwapChainDX11->Present(vsync, NULL);
	return true;
}

void DX11SwapChain::ReloadBuffers()
{
	CreateRenderTargetView(m_Width, m_Height);
	CreateDepthStencilView(m_Width, m_Height);
}

void DX11SwapChain::CreateRenderTargetView(uint32_t width, uint32_t height)
{
	ID3D11Device* dx11Device = m_DX11Context->GetDX11Device();

	// Microsoft::WRL::ComPtr<ID3D11Texture2D> dx11RenderTargetBuffer;
	// ID3D11Texture2D* dx11RenderTargetBuffer;
	HRESULT hr = m_SwapChainDX11->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(m_RenderTargetBufferDX11.GetAddressOf()));
	if (FAILED(hr))
	{
		throw std::exception("SwapChain: GetBuffer failed.");
	}

	hr = dx11Device->CreateRenderTargetView(m_RenderTargetBufferDX11.Get(), 0, m_RenderTargetViewDX11.GetAddressOf());
	m_RenderTargetBufferDX11.Reset();

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
	// ID3D11Texture2D* dx11DepthStencilBuffer;
	HRESULT hr = m_SwapChainDX11->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(m_DepthStencilBufferDX11.GetAddressOf()));
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

	hr = dx11Device->CreateTexture2D(&depthStencilDesc, nullptr, m_DepthStencilBufferDX11.GetAddressOf());
	if (FAILED(hr))
	{
		throw std::exception("SwapChain: CreateTexture2D failed.");
	}

	hr = dx11Device->CreateDepthStencilView(m_DepthStencilBufferDX11.Get(), NULL, &m_DepthStencilViewDX11);
	// dx11DepthStencilBuffer->Release();
	m_DepthStencilBufferDX11.Reset();

	if (FAILED(hr))
	{
		throw std::exception("SwapChain: CreateDepthStencilView failed.");
	}
	else if (SUCCEEDED(hr))
	{
		Log::GetLogger()->info("DX11SwapChain::CreateDepthStencilView({0}, {1}) successful!", width, height);
	}
}

void DX11SwapChain::SetFullScreen(bool fullscreenEnabled, uint32_t width, uint32_t height)
{
	m_FullscreenToggleMode = true;

	// OnResize(width, height);
	// m_SwapChainDX11->SetFullscreenState(fullscreenEnabled, nullptr);
}

void DX11SwapChain::BeginFrame()
{
	// TODO
}

void DX11SwapChain::ClearRenderTargetColor(float red, float green, float blue, float alpha)
{
	FLOAT clear_color[] = { red, green, blue, alpha };

	m_DX11Context->GetDX11DeviceContext()->ClearRenderTargetView(m_RenderTargetViewDX11.Get(), clear_color);
	m_DX11Context->GetDX11DeviceContext()->ClearDepthStencilView(m_DepthStencilViewDX11.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	m_DX11Context->GetDX11DeviceContext()->OMSetRenderTargets(1, m_RenderTargetViewDX11.GetAddressOf(), m_DepthStencilViewDX11.Get());
}
