#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Assert.h"

#include "DX11.h"
#include "DX11Device.h"
#include "DX11Allocator.h"

#include <d3d11.h>

#include <wrl/client.h>

#include <vector>

struct GLFWwindow;


class DX11Context;

class DX11SwapChain
{
public:
	DX11SwapChain() = default;
	DX11SwapChain(HWND hwnd, uint32_t width, uint32_t height, Hazel::Ref<DX11Context> dx11Context);
	virtual ~DX11SwapChain();

	void Invalidate();
	
	void OnResize(uint32_t width, uint32_t height);
	void SetFullScreen(bool fullscreenEnabled, uint32_t width, uint32_t height);

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

	void BeginFrame();
	bool Present(bool vsync);

	void Cleanup();

	// DirectX 11 specific methods
	ID3D11RenderTargetView* GetRenderTargetViewDX11() { return m_RenderTargetViewDX11.Get(); }
	ID3D11DepthStencilView* GetDepthStencilViewDX11() { return m_DepthStencilViewDX11.Get(); }

	void ClearRenderTargetColor(float red, float green, float blue, float alpha);

private:
	void ReloadBuffers();

	void CreateRenderTargetView(uint32_t width, uint32_t height);
	void CreateDepthStencilView(uint32_t width, uint32_t height);

private:
	Hazel::Ref<DX11Device> m_Device;
	Hazel::Ref<DX11Context> m_DX11Context;

	HWND m_HWND;
	uint32_t m_Width = 0;
	uint32_t m_Height = 0;

	Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChainDX11 = nullptr;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_RenderTargetBufferDX11 = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_RenderTargetViewDX11 = nullptr;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_DepthStencilBufferDX11 = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_DepthStencilViewDX11 = nullptr;

	bool m_FullscreenToggleMode = false;

	friend class DX11Context;
	friend class DX11Renderer;

};
