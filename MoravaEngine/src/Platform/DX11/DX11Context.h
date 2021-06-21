#pragma once

#include "Hazel/Renderer/HazelRenderer.h"

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

#include "Core/Window.h"

#include <d3d11.h>


struct GLFWwindow;


enum class DX11CullMode
{
	None = 0,
	Front,
	Back,
};


class DX11Context : public Hazel::RendererContext
{
public:
	DX11Context(Window* window);
	virtual ~DX11Context();

	virtual void Create() override;
	virtual void SwapBuffers() override;

	virtual void OnResize(uint32_t width, uint32_t height) override;

	virtual void BeginFrame() override;

	Hazel::Ref<DX11Device> GetDevice() { return m_Device; }
	std::shared_ptr<DX11SwapChain> GetSwapChain() { return m_SwapChain; }

	// static Hazel::Ref<DX11Context> Get() { return Hazel::Ref<DX11Context>(Hazel::HazelRenderer::GetContext()); }
	Hazel::Ref<DX11Device> GetCurrentDevice() { return Get()->GetDevice(); }
	IDXGIFactory* GetIDXGIFactory() { return m_IDXGI_Factory; };
	ID3D11DeviceContext* GetDX11DeviceContext() { return m_DX11DeviceContext; };

	// ---------------------------------------------------------------

	static Hazel::Ref<DX11Context> Get() { return Hazel::Ref<DX11Context>(Hazel::HazelRenderer::GetContext()); }

	ID3D11Device* GetDX11Device() { return m_DX11Device; }

	std::shared_ptr<DX11SwapChain> CreateSwapChain(HWND hwnd, UINT width, UINT height);

	void SetRasterizerState(DX11CullMode cullMode);

	void SetViewportSize(uint32_t width, uint32_t height);

	// this should probably be DX11Framebuffer->Bind()
	void SetRenderTarget(Hazel::Ref<DX11Texture2D> renderTarget, Hazel::Ref<DX11Texture2D> depthStencil);

	// this should probably be DX11Texture[2D]->Bind()
	void SetTexture(Hazel::Ref<DX11Shader> shader, DX11Shader::Type shaderType, const std::vector<Hazel::Ref<DX11Texture2D>>& textures, uint32_t textureCount);

private:
	void InitRasterizerState();

public:

private:
	ID3D11Device* m_DX11Device;

	DX11Context* m_Instance;

	Window* m_Window;

	// Devices
	Hazel::Ref<DX11PhysicalDevice> m_PhysicalDevice;
	Hazel::Ref<DX11Device> m_Device;

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
