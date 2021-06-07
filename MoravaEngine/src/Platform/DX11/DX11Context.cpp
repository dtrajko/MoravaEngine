#include "DX11Context.h"
#include "DX11.h"
#include "DX11Allocator.h"

#include <glfw/glfw3.h>


static bool s_Validation = true;


DX11Context::DX11Context(GLFWwindow* windowHandle)
	: m_WindowHandle(windowHandle)
{
}

DX11Context::~DX11Context()
{
}

void DX11Context::Create()
{
	MORAVA_CORE_INFO("DX11Context::Create");

	HZ_CORE_ASSERT(glfwVulkanSupported(), "GLFW must support DirectX 11!");

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Application Info
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// TODO

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Instance and Surface Creation
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	m_PhysicalDevice = DX11PhysicalDevice::Select();

	m_Device = Hazel::Ref<DX11Device>::Create(m_PhysicalDevice);

	// Why is this here?
	m_Allocator = DX11Allocator(m_Device, std::string("Default"));
		
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
