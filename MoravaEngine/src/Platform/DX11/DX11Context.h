#pragma once

#include "H2M/Renderer/RendererH2M.h"

#include "DX11.h"
#include "DX11Device.h"
#include "DX11Allocator.h"
#include "DX11SwapChain.h"
#include "DX11VertexBuffer.h"
#include "DX11IndexBuffer.h"
#include "DX11ConstantBuffer.h"
#include "DX11Texture2D.h"
#include "DX11Shader.h"
#include "DX11Pipeline.h"
#include "DX11Material.h"

#include "Core/Window.h"

#include <d3d11.h>


struct GLFWwindow;

class DX11Context : public H2M::RendererContextH2M
{
public:
	DX11Context(Window* window);
	virtual ~DX11Context();

	virtual void Init() override;

	// virtual void Create() override;                               // TODO: remove the method (replace with Init)
	virtual void SwapBuffers() override;                             // TODO: remove the method
	virtual void OnResize(uint32_t width, uint32_t height) override; // TODO: remove the method
	virtual void BeginFrame() override;                              // TODO: remove the method

	H2M::RefH2M<DX11Device> GetDevice() { return m_Device; }
	std::shared_ptr<DX11SwapChain> GetSwapChain() { return m_SwapChain; }

	// static H2M::RefH2M<DX11Context> Get() { return H2M::RefH2M<DX11Context>(H2M::HazelRenderer::GetContext()); }
	H2M::RefH2M<DX11Device> GetCurrentDevice() { return Get()->GetDevice(); }
	IDXGIFactory* GetIDXGIFactory() { return m_IDXGI_Factory; };
	ID3D11DeviceContext* GetDX11DeviceContext() { return m_DX11DeviceContext; };

	// ---------------------------------------------------------------

	static H2M::RefH2M<DX11Context> Get() { return H2M::RefH2M<DX11Context>(H2M::RendererH2M::GetContext()); }

	ID3D11Device* GetDX11Device() { return m_DX11Device; }

	std::shared_ptr<DX11SwapChain> CreateSwapChain(HWND hwnd, UINT width, UINT height);

	void SetRasterizerState(DX11CullMode cullMode);

	void SetViewportSize(uint32_t width, uint32_t height);

private:
	void InitRasterizerState();

public:

private:
	ID3D11Device* m_DX11Device;

	DX11Context* m_Instance;

	Window* m_Window;

	// Devices
	H2M::RefH2M<DX11PhysicalDevice> m_PhysicalDevice;
	H2M::RefH2M<DX11Device> m_Device;

	std::shared_ptr<DX11SwapChain> m_SwapChain;

	D3D_FEATURE_LEVEL m_FeatureLevel;
	IDXGIDevice* m_DXGI_Device;
	IDXGIAdapter* m_DXGI_Adapter;
	IDXGIFactory* m_IDXGI_Factory;
	ID3D11DeviceContext* m_DX11DeviceContext;

	ID3D11RasterizerState* m_CullStateFront;
	ID3D11RasterizerState* m_CullStateBack;
	ID3D11RasterizerState* m_CullStateNone;

	bool m_Validation;

	ID3DBlob* m_blob = nullptr;

	bool m_VSync = true;

};
