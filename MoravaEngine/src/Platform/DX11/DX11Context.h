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
	DX11Context();
	DX11Context(GLFWwindow* windowHandle);
	virtual ~DX11Context();

	virtual void Create() override;
	virtual void SwapBuffers() override;

	virtual void OnResize(uint32_t width, uint32_t height) override;

	virtual void BeginFrame() override;

	static Hazel::Ref<DX11Device> GetDevice() { return s_Device; }
	// DX11SwapChain& GetSwapChain() { return m_SwapChain; }

	// static Hazel::Ref<DX11Context> Get() { return Hazel::Ref<DX11Context>(Hazel::HazelRenderer::GetContext()); }
	static Hazel::Ref<DX11Device> GetCurrentDevice() { return Get()->GetDevice(); }
	static IDXGIFactory* GetIDXGIFactory() { return s_IDXGI_Factory; };
	static ID3D11DeviceContext* GetImmediateContext() { return s_DX11_DeviceContext; };

	// ---------------------------------------------------------------

	static DX11Context* Get();

	static ID3D11Device* GetDX11Device() { return s_DX11Device; }

	std::shared_ptr<DX11SwapChain> CreateSwapChain(HWND hwnd, UINT width, UINT height);
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

	void SetTexture(Hazel::Ref<DX11Shader> shader, DX11Shader::Type shaderType, const std::vector<Hazel::Ref<DX11Texture2D>>& textures, uint32_t textureCount);
	void SetConstantBuffer(Hazel::Ref<DX11Shader> shader, DX11Shader::Type shaderType, Hazel::Ref<DX11ConstantBuffer> buffer);

	void DrawTriangleList(uint32_t vertexCount, uint32_t startVertexIndex);
	void DrawIndexedTriangleList(uint32_t indexCount, uint32_t startVertexIndex, uint32_t startIndexLocation);
	void DrawTriangleStrip(uint32_t vertexCount, uint32_t startVertexIndex);

private:
	static void InitRasterizerState();

public:
	static ID3D11Device* s_DX11Device;

private:
	static DX11Context* s_Instance;

	GLFWwindow* m_WindowHandle;

	// Devices
	static Hazel::Ref<DX11PhysicalDevice> s_PhysicalDevice;
	static Hazel::Ref<DX11Device> s_Device;

	std::shared_ptr<DX11SwapChain> m_SwapChain;

	static D3D_FEATURE_LEVEL s_FeatureLevel;
	static IDXGIDevice* s_DXGI_Device;
	static IDXGIAdapter* s_DXGI_Adapter;
	static IDXGIFactory* s_IDXGI_Factory;
	static ID3D11DeviceContext* s_DX11_DeviceContext;
	static ID3D11RasterizerState* s_CullFrontState;
	static ID3D11RasterizerState* s_CullBackState;
	static bool s_Validation;

	ID3DBlob* m_blob = nullptr;

	bool m_VSync = true;

};
