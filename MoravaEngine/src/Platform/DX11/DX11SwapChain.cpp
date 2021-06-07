#include "DX11SwapChain.h"

#include "Core/Log.h"

#include <GLFW/glfw3.h>


void DX11SwapChain::Init(const Hazel::Ref<DX11Device>& device)
{
	m_Device = device;
	m_Allocator = DX11Allocator(m_Device, std::string("SwapChain"));
}
	
void DX11SwapChain::InitSurface(GLFWwindow* windowHandle)
{
	// DX11PhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetDX11PhysicalDevice();

	// glfwCreateWindowSurface(m_Instance, windowHandle, nullptr, &m_Surface);

	// TODO
}

void DX11SwapChain::Create(uint32_t* width, uint32_t* height, bool vsync)
{
	// DX11Device device = m_Device->GetDX11Device();
	// DX11PhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetDX11PhysicalDevice();

	m_Width = *width;
	m_Height = *height;

	// TODO

	CreateDepthStencil();

	CreateFramebuffer();
}

void DX11SwapChain::CreateDepthStencil()
{
	// TODO
}

void DX11SwapChain::CreateFramebuffer()
{
	// TODO: Maybe move into DX11SwapChain
	// Setup Framebuffer
}

void DX11SwapChain::CreateDrawBuffers()
{
	// TODO
}

void DX11SwapChain::OnResize(uint32_t width, uint32_t height)
{
	MORAVA_CORE_WARN("DX11SwapChain::OnResize");
	// auto device = m_Device->GetDX11Device();

	Create(&width, &height);

	CreateDepthStencil();

	CreateFramebuffer();
}

void DX11SwapChain::BeginFrame()
{
	// TODO
}

void DX11SwapChain::Present()
{
	// TODO
}

void DX11SwapChain::Cleanup()
{
	// VkDevice device = m_Device->GetDX11Device();

	// TODO
}

void DX11SwapChain::FindImageFormatAndColorSpace()
{
	// DX11PhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetDX11PhysicalDevice();

	// TODO	}
}
