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

#include <d3d11.h>


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
	static IDXGIFactory* GetIDXGIFactory() { return s_IDXGIFactory; };
	static ID3D11DeviceContext* GetImmediateContext() { return s_ImmediateContext; };

	// ---------------------------------------------------------------

	DX11SwapChain* CreateSwapChain(HWND hwnd, UINT width, UINT height);
	void SetRasterizerState(bool cull_front);

	void ClearRenderTargetColor(float red, float green, float blue, float alpha);
	void ClearRenderTargetColor(Hazel::Ref<DX11Texture2D> renderTarget, float red, float green, float blue, float alpha);
	void ClearDepthStencil();
	void ClearDepthStencil(Hazel::Ref<DX11Texture2D> depthStencil);

	void SetViewportSize(uint32_t width, uint32_t height);

	void SetVertexBuffer(Hazel::Ref<DX11VertexBuffer> vertexBuffer);
	void SetIndexBuffer(Hazel::Ref<DX11IndexBuffer> indexBuffer);

	void SetRenderTarget(Hazel::Ref<DX11Texture2D> renderTarget, Hazel::Ref<DX11Texture2D> depthStencil);

	void SetVertexShader(Hazel::Ref<DX11Shader> vertexShader);
	void SetPixelShader(Hazel::Ref<DX11Shader> pixelShader);

	void DrawTriangleList(uint32_t vertexCount, uint32_t startVertexIndex);
	void DrawIndexedTriangleList(uint32_t indexCount, uint32_t startVertexIndex, uint32_t startIndexLocation);
	void DrawTriangleStrip(uint32_t vertexCount, uint32_t startVertexIndex);

private:
	void InitRasterizerState();

private:
	GLFWwindow* m_WindowHandle;

	// Devices
	Hazel::Ref<DX11PhysicalDevice> m_PhysicalDevice;
	Hazel::Ref<DX11Device> m_Device;

public:
	ID3D11Device* m_d3d_device;

private:
	DX11SwapChain* m_SwapChain;

	ID3D11DeviceContext* m_device_context;

	D3D_FEATURE_LEVEL m_feature_level;
	IDXGIDevice* m_dxgi_device;
	IDXGIAdapter* m_dxgi_adapter;
	static IDXGIFactory* s_IDXGIFactory;
	static ID3D11DeviceContext* s_ImmediateContext;
	ID3DBlob* m_blob = nullptr;

	ID3D11RasterizerState* m_cull_front_state = nullptr;
	ID3D11RasterizerState* m_cull_back_state = nullptr;

	static bool s_Validation;

	bool m_VSync = true;

};
