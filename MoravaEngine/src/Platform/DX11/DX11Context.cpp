#include "DX11Context.h"
#include "DX11.h"

#include "Platform/Windows/WindowsWindow.h"

#include <glfw/glfw3.h>


DX11Context::DX11Context(Window* window)
	: m_Window(window)
{
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
			D3D11_SDK_VERSION, &m_DX11Device, &m_FeatureLevel, &m_DX11DeviceContext);

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

	m_DX11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&m_DXGI_Device);
	m_DXGI_Device->GetParent(__uuidof(IDXGIAdapter), (void**)&m_DXGI_Adapter);
	m_DXGI_Adapter->GetParent(__uuidof(IDXGIFactory), (void**)&m_IDXGI_Factory);

	InitRasterizerState();

	m_Device = Hazel::Ref<DX11Device>::Create(m_PhysicalDevice);

	CreateSwapChain(m_Window->GetHWND(), m_Window->GetWidth(), m_Window->GetHeight());
}

DX11Context::~DX11Context()
{
	m_DX11DeviceContext->Release();
}

std::shared_ptr<DX11SwapChain> DX11Context::CreateSwapChain(HWND hwnd, UINT width, UINT height)
{
	try
	{
		m_SwapChain = std::make_shared<DX11SwapChain>(hwnd, width, height, this);
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
		m_DX11DeviceContext->RSSetState(m_CullFrontState);
	}
	else {
		m_DX11DeviceContext->RSSetState(m_CullBackState);
	}
}

void DX11Context::InitRasterizerState()
{
	D3D11_RASTERIZER_DESC desc = {};
	desc.CullMode = D3D11_CULL_FRONT;
	desc.DepthClipEnable = true;
	desc.FillMode = D3D11_FILL_SOLID;
	m_DX11Device->CreateRasterizerState(&desc, &m_CullFrontState);

	desc.CullMode = D3D11_CULL_BACK;
	m_DX11Device->CreateRasterizerState(&desc, &m_CullBackState);
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

void DX11Context::SetViewportSize(uint32_t width, uint32_t height)
{
	D3D11_VIEWPORT vp = {};
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	m_DX11DeviceContext->RSSetViewports(1, &vp);
}

void DX11Context::SetVertexBuffer(Hazel::Ref<DX11VertexBuffer> vertexBuffer, Hazel::Ref<DX11Pipeline> pipeline)
{
	uint32_t stride = vertexBuffer->GetStride();
	uint32_t offset = 0;

	m_DX11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer->m_Buffer, &stride, &offset);
	m_DX11DeviceContext->IASetInputLayout(pipeline->GetInputLayout());
}

void DX11Context::SetIndexBuffer(Hazel::Ref<DX11IndexBuffer> indexBuffer)
{
	m_DX11DeviceContext->IASetIndexBuffer(indexBuffer->GetBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void DX11Context::SetRenderTarget(Hazel::Ref<DX11Texture2D> renderTarget, Hazel::Ref<DX11Texture2D> depthStencil)
{
	if (renderTarget->GetType() != DX11Texture2D::Type::RenderTarget) return;
	if (depthStencil->GetType() != DX11Texture2D::Type::DepthStencil) return;

	m_DX11DeviceContext->OMSetRenderTargets(1, &renderTarget->m_RenderTargetView, depthStencil->GetDepthStencilView());
}

void DX11Context::SetVertexShader(Hazel::Ref<DX11Shader> vertexShader)
{
	m_DX11DeviceContext->VSSetShader(vertexShader->GetVertexShader()->GetDX11Shader(), nullptr, 0);
}

void DX11Context::SetPixelShader(Hazel::Ref<DX11Shader> pixelShader)
{
	if (!pixelShader) return;
	m_DX11DeviceContext->PSSetShader(pixelShader->GetPixelShader()->GetDX11Shader(), nullptr, 0);
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
		m_DX11DeviceContext->VSSetShaderResources(0, textureCount, list_res);
		m_DX11DeviceContext->VSSetSamplers(0, textureCount, list_sampler);
	}
	else if (shaderType == DX11Shader::Type::Pixel)
	{
		m_DX11DeviceContext->PSSetShaderResources(0, textureCount, list_res);
		m_DX11DeviceContext->PSSetSamplers(0, textureCount, list_sampler);
	}
}

void DX11Context::SetConstantBuffer(Hazel::Ref<DX11Shader> shader, DX11Shader::Type shaderType, Hazel::Ref<DX11ConstantBuffer> buffer)
{
	if (shaderType == DX11Shader::Type::Vertex)
	{
		m_DX11DeviceContext->VSSetConstantBuffers(0, 1, &buffer->m_buffer);
	}
	else if (shaderType == DX11Shader::Type::Pixel)
	{
		m_DX11DeviceContext->PSSetConstantBuffers(0, 1, &buffer->m_buffer);
	}
}
