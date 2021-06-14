#include "DX11Context.h"
#include "DX11.h"

#include <glfw/glfw3.h>


DX11Context* DX11Context::s_Instance = nullptr;

bool DX11Context::s_Validation = true;
IDXGIFactory* DX11Context::s_IDXGI_Factory;
ID3D11DeviceContext* DX11Context::s_DX11_DeviceContext;
ID3D11Device* DX11Context::s_DX11Device;
D3D_FEATURE_LEVEL DX11Context::s_FeatureLevel;
IDXGIDevice* DX11Context::s_DXGI_Device;
IDXGIAdapter* DX11Context::s_DXGI_Adapter;
ID3D11RasterizerState* DX11Context::s_CullFrontState;
ID3D11RasterizerState* DX11Context::s_CullBackState;

Hazel::Ref<DX11PhysicalDevice> DX11Context::s_PhysicalDevice;
Hazel::Ref<DX11Device> DX11Context::s_Device;


DX11Context::DX11Context()
{
}

DX11Context::DX11Context(GLFWwindow* windowHandle)
	: m_WindowHandle(windowHandle)
{
}

DX11Context::~DX11Context()
{
	s_DX11_DeviceContext->Release();
}

DX11Context* DX11Context::Get()
{
	if (s_Instance == nullptr)
	{
		s_Instance = new DX11Context();
	}

	return s_Instance;
}

void DX11Context::Create()
{
	MORAVA_CORE_INFO("DX11Context::Create");

	D3D_DRIVER_TYPE driver_types[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT num_driver_types = ARRAYSIZE(driver_types);

	D3D_FEATURE_LEVEL feature_levels[] =
	{
		D3D_FEATURE_LEVEL_11_0
	};
	UINT num_feature_levels = ARRAYSIZE(feature_levels);

	HRESULT res = 0;
	for (UINT driver_type_index = 0; driver_type_index < num_driver_types;)
	{
		res = D3D11CreateDevice(NULL, driver_types[driver_type_index], NULL, NULL, feature_levels, num_feature_levels,
			D3D11_SDK_VERSION, &s_DX11Device, &s_FeatureLevel, &s_DX11_DeviceContext);

		if (SUCCEEDED(res))
		{
			break;
			++driver_type_index;
		}
	}

	if (FAILED(res))
	{
		throw std::exception("DX11Context: D3D11CreateDevice failed.");
	}

	s_DX11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&s_DXGI_Device);
	s_DXGI_Device->GetParent(__uuidof(IDXGIAdapter), (void**)&s_DXGI_Adapter);
	s_DXGI_Adapter->GetParent(__uuidof(IDXGIFactory), (void**)&s_IDXGI_Factory);

	InitRasterizerState();

	s_Device = Hazel::Ref<DX11Device>::Create(s_PhysicalDevice);
}

std::shared_ptr<DX11SwapChain> DX11Context::CreateSwapChain(HWND hwnd, UINT width, UINT height)
{
	try
	{
		m_SwapChain = std::make_shared<DX11SwapChain>(hwnd, width, height);
	}
	catch (const std::exception&)
	{
		throw std::exception("DX11SwapChain initialization failed.");
	}
	return m_SwapChain;
}

void DX11Context::SetRasterizerState(bool cull_front)
{
	if (cull_front) {
		s_DX11_DeviceContext->RSSetState(s_CullFrontState);
	}
	else {
		s_DX11_DeviceContext->RSSetState(s_CullBackState);
	}
}

void DX11Context::InitRasterizerState()
{
	D3D11_RASTERIZER_DESC desc = {};
	desc.CullMode = D3D11_CULL_FRONT;
	desc.DepthClipEnable = true;
	desc.FillMode = D3D11_FILL_SOLID;
	s_DX11Device->CreateRasterizerState(&desc, &s_CullFrontState);

	desc.CullMode = D3D11_CULL_BACK;
	s_DX11Device->CreateRasterizerState(&desc, &s_CullBackState);
}

void DX11Context::OnResize(uint32_t width, uint32_t height)
{
	m_SwapChain->OnResize(width, height);
}

void DX11Context::BeginFrame()
{
	m_SwapChain->BeginFrame();
}

void DX11Context::SwapBuffers()
{
	m_SwapChain->Present(m_VSync);
}

void DX11Context::ClearRenderTargetColor(float red, float green, float blue, float alpha)
{
	FLOAT clear_color[] = { red, green, blue, alpha };

	s_DX11_DeviceContext->ClearRenderTargetView(m_SwapChain->m_rtv, clear_color);
	s_DX11_DeviceContext->ClearDepthStencilView(m_SwapChain->m_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	s_DX11_DeviceContext->OMSetRenderTargets(1, &m_SwapChain->m_rtv, m_SwapChain->m_dsv);
}

void DX11Context::ClearRenderTargetColor(Hazel::Ref<DX11Texture2D> renderTarget, float red, float green, float blue, float alpha)
{
	if (renderTarget->GetType() != DX11Texture2D::Type::RenderTarget) return;
	FLOAT clear_color[] = { red, green, blue, alpha };
	s_DX11_DeviceContext->ClearRenderTargetView(renderTarget->m_render_target_view, clear_color);
}

void DX11Context::ClearDepthStencil()
{
	s_DX11_DeviceContext->ClearDepthStencilView(m_SwapChain->m_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void DX11Context::ClearDepthStencil(Hazel::Ref<DX11Texture2D> depthStencil)
{
	if (depthStencil->GetType() != DX11Texture2D::Type::DepthStencil) return;
	s_DX11_DeviceContext->ClearDepthStencilView(depthStencil->m_depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void DX11Context::SetViewportSize(uint32_t width, uint32_t height)
{
	D3D11_VIEWPORT vp = {};
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	s_DX11_DeviceContext->RSSetViewports(1, &vp);
}

void DX11Context::SetVertexBuffer(Hazel::Ref<DX11VertexBuffer> vertexBuffer)
{
	uint32_t stride = vertexBuffer->GetBufferSize();
	uint32_t offset = 0;

	s_DX11_DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer->m_buffer, &stride, &offset);
	s_DX11_DeviceContext->IASetInputLayout(vertexBuffer->m_layout);
}

void DX11Context::SetIndexBuffer(Hazel::Ref<DX11IndexBuffer> indexBuffer)
{
	s_DX11_DeviceContext->IASetIndexBuffer(indexBuffer->GetBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void DX11Context::SetRenderTarget(Hazel::Ref<DX11Texture2D> renderTarget, Hazel::Ref<DX11Texture2D> depthStencil)
{
	if (renderTarget->GetType() != DX11Texture2D::Type::RenderTarget) return;
	if (depthStencil->GetType() != DX11Texture2D::Type::DepthStencil) return;

	s_DX11_DeviceContext->OMSetRenderTargets(1, &renderTarget->m_render_target_view, depthStencil->m_depth_stencil_view);
}

void DX11Context::SetVertexShader(Hazel::Ref<DX11Shader> vertexShader)
{
	s_DX11_DeviceContext->VSSetShader(vertexShader->GetVertexShaderDX11(), nullptr, 0);
}

void DX11Context::SetPixelShader(Hazel::Ref<DX11Shader> pixelShader)
{
	if (!pixelShader) return;
	s_DX11_DeviceContext->PSSetShader(pixelShader->GetPixelShaderDX11(), nullptr, 0);
}

void DX11Context::DrawTriangleList(uint32_t vertexCount, uint32_t startVertexIndex)
{
	s_DX11_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	s_DX11_DeviceContext->Draw((UINT)vertexCount, (UINT)startVertexIndex);
}

void DX11Context::DrawIndexedTriangleList(uint32_t indexCount, uint32_t startVertexIndex, uint32_t startIndexLocation)
{
	s_DX11_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	s_DX11_DeviceContext->DrawIndexed((UINT)indexCount, (UINT)startIndexLocation, (UINT)startVertexIndex);
}

void DX11Context::DrawTriangleStrip(uint32_t vertexCount, uint32_t startVertexIndex)
{
	s_DX11_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	s_DX11_DeviceContext->Draw((UINT)vertexCount, (UINT)startVertexIndex);
}

void DX11Context::SetTexture(Hazel::Ref<DX11Shader> shader, DX11Shader::Type shaderType, const std::vector<Hazel::Ref<DX11Texture2D>>& textures, uint32_t textureCount)
{
	ID3D11ShaderResourceView* list_res[32];
	ID3D11SamplerState* list_sampler[32];

	for (unsigned int i = 0; i < textureCount; i++)
	{
		list_res[i] = textures[i]->m_shader_res_view;
		list_sampler[i] = textures[i]->m_sampler_state;
	}

	if (shaderType == DX11Shader::Type::Vertex)
	{
		s_DX11_DeviceContext->VSSetShaderResources(0, textureCount, list_res);
		s_DX11_DeviceContext->VSSetSamplers(0, textureCount, list_sampler);
	}
	else if (shaderType == DX11Shader::Type::Pixel)
	{
		s_DX11_DeviceContext->PSSetShaderResources(0, textureCount, list_res);
		s_DX11_DeviceContext->PSSetSamplers(0, textureCount, list_sampler);
	}
}

void DX11Context::SetConstantBuffer(Hazel::Ref<DX11Shader> shader, DX11Shader::Type shaderType, Hazel::Ref<DX11ConstantBuffer> buffer)
{
	if (shaderType == DX11Shader::Type::Vertex)
	{
		s_DX11_DeviceContext->VSSetConstantBuffers(0, 1, &buffer->m_buffer);
	}
	else if (shaderType == DX11Shader::Type::Pixel)
	{
		s_DX11_DeviceContext->PSSetConstantBuffers(0, 1, &buffer->m_buffer);
	}
}
