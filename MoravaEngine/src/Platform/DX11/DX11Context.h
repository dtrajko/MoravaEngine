#pragma once

#include "Hazel/Renderer/HazelRenderer.h"

#include "DX11.h"
#include "DX11Device.h"
#include "DX11Allocator.h"
#include "DX11SwapChain.h"


struct GLFWwindow;


class DX11Context : public Hazel::RendererContext
{
public:
	DX11Context(GLFWwindow* windowHandle);
	virtual ~DX11Context();

	virtual void Create() override;
	virtual void SwapBuffers() override;

	virtual void OnResize(uint32_t width, uint32_t height) override;

	virtual void BeginFrame() override;

	Hazel::Ref<DX11Device> GetDevice() { return m_Device; }
	// DX11SwapChain& GetSwapChain() { return m_SwapChain; }

	static Hazel::Ref<DX11Context> Get() { return Hazel::Ref<DX11Context>(Hazel::HazelRenderer::GetContext()); }
	static Hazel::Ref<DX11Device> GetCurrentDevice() { return Get()->GetDevice(); }

private:
	GLFWwindow* m_WindowHandle;

	// Devices
	Hazel::Ref<DX11PhysicalDevice> m_PhysicalDevice;
	Hazel::Ref<DX11Device> m_Device;

	DX11Allocator m_Allocator;
	// DX11SwapChain m_SwapChain;
};
