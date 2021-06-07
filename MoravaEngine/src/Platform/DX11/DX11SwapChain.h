#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Assert.h"

#include "DX11.h"
#include "DX11Device.h"
#include "DX11Allocator.h"

#include <vector>

struct GLFWwindow;


class DX11SwapChain
{
public:
	DX11SwapChain() = default;

	void Init(const Hazel::Ref<DX11Device>& device);
	void InitSurface(GLFWwindow* windowHandle);
	void Create(uint32_t* width, uint32_t* height, bool vsync = false);
		
	void OnResize(uint32_t width, uint32_t height);

	void BeginFrame();
	void Present();

	uint32_t GetImageCount() const { return m_ImageCount; }

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

	uint32_t GetCurrentBufferIndex() const { return m_CurrentBufferIndex; }

	void Cleanup();

private:
	void CreateFramebuffer();
	void CreateDepthStencil();
	void CreateDrawBuffers();
	void FindImageFormatAndColorSpace();

private:
	Hazel::Ref<DX11Device> m_Device;
	DX11Allocator m_Allocator;

	uint32_t m_ImageCount = 0;

	uint32_t m_CurrentBufferIndex = 0;

	uint32_t m_QueueNodeIndex = UINT32_MAX;
	uint32_t m_Width = 0, m_Height = 0;

	friend class DX11Context;
};
