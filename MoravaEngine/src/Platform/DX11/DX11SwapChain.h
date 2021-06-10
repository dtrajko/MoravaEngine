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


class DX11SwapChain
{
public:
	DX11SwapChain() = default;
	DX11SwapChain(Hazel::Ref<DX11Device> device, IDXGIFactory* dxgiFactory, HWND hwnd, uint32_t width, uint32_t height);
	virtual ~DX11SwapChain();

	void Init(Hazel::Ref<DX11Device> device, IDXGIFactory* dxgiFactory, HWND hwnd, uint32_t width, uint32_t height);
	void InitSurface(GLFWwindow* windowHandle);
	void Create(uint32_t* width, uint32_t* height, bool vsync = false);
	
	void OnResize(Hazel::Ref<DX11Device>& device, uint32_t width, uint32_t height);
	void SetFullScreen(Hazel::Ref<DX11Device>& device, bool fullscreen, unsigned int width, unsigned int height);

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

	void BeginFrame();
	bool Present(bool vsync);

	void Cleanup();

private:
	void ReloadBuffers(Hazel::Ref<DX11Device>& device, uint32_t width, uint32_t height);

	void CreateRenderTargetView(Hazel::Ref<DX11Device>& device);
	void CreateDepthStencilView(Hazel::Ref<DX11Device>& device, uint32_t width, uint32_t height);

	// uint32_t GetImageCount() const { return m_ImageCount; }
	// uint32_t GetCurrentBufferIndex() const { return m_CurrentBufferIndex; }
	// void CreateDrawBuffers();
	// void FindImageFormatAndColorSpace();

private:
	Hazel::Ref<DX11Device> m_Device;

	uint32_t m_Width = 0;
	uint32_t m_Height = 0;

	IDXGISwapChain* m_swap_chain = nullptr;
	ID3D11RenderTargetView* m_rtv = nullptr;
	ID3D11DepthStencilView* m_dsv = nullptr;

	// uint32_t m_ImageCount = 0;
	// uint32_t m_CurrentBufferIndex = 0;
	// uint32_t m_QueueNodeIndex = UINT32_MAX;

};
