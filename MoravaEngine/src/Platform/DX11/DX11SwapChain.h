#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Assert.h"

#include "DX11.h"
#include "DX11Device.h"
#include "DX11Allocator.h"

#include <d3d11.h>

#include <vector>

struct GLFWwindow;


class DX11Context;

class DX11SwapChain
{
public:
	DX11SwapChain() = default;
	DX11SwapChain(HWND hwnd, uint32_t width, uint32_t height, Hazel::Ref<DX11Context> dx11Context);
	virtual ~DX11SwapChain();

	void Init(HWND hwnd, uint32_t width, uint32_t height);
	void Create(uint32_t* width, uint32_t* height, bool vsync = false);
	
	void OnResize(uint32_t width, uint32_t height);
	void SetFullScreen(bool fullscreen, uint32_t width, uint32_t height);

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

	void BeginFrame();
	bool Present(bool vsync);

	void Cleanup();

	// DirectX 11 specific methods
	ID3D11RenderTargetView* GetRenderTargetView() { return m_DX11RenderTargetView; }
	ID3D11DepthStencilView* GetDepthStencilView() { return m_DX11DepthStencilView; }

private:
	void ReloadBuffers(uint32_t width, uint32_t height);

	void CreateRenderTargetView(uint32_t width, uint32_t height);
	void CreateDepthStencilView(uint32_t width, uint32_t height);

	// uint32_t GetImageCount() const { return m_ImageCount; }
	// uint32_t GetCurrentBufferIndex() const { return m_CurrentBufferIndex; }
	// void CreateDrawBuffers();
	// void FindImageFormatAndColorSpace();

private:
	Hazel::Ref<DX11Device> m_Device;
	Hazel::Ref<DX11Context> m_DX11Context;

	uint32_t m_Width = 0;
	uint32_t m_Height = 0;

	IDXGISwapChain* m_DX11SwapChain = nullptr;

	ID3D11Texture2D* m_DX11RenderTargetBuffer = nullptr;
	ID3D11RenderTargetView* m_DX11RenderTargetView = nullptr;

	ID3D11Texture2D* m_DX11DepthStencilBuffer = nullptr;
	ID3D11DepthStencilView* m_DX11DepthStencilView = nullptr;

	// uint32_t m_ImageCount = 0;
	// uint32_t m_CurrentBufferIndex = 0;
	// uint32_t m_QueueNodeIndex = UINT32_MAX;

	friend class DX11Context;
	friend class DX11Renderer;

};
