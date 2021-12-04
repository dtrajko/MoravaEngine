#include "DX11Framebuffer.h"

#include "DX11Context.h"
#include "DX11Texture2D.h"
#include "DX11SwapChain.h"

#include "Core/Application.h"


namespace Utils {

	static bool IsDepthFormat(H2M::ImageFormatH2M format)
	{
		switch (format)
		{
		case H2M::ImageFormatH2M::DEPTH24STENCIL8:
		case H2M::ImageFormatH2M::DEPTH32F:
			return true;
		}
		return false;
	}

}

DX11Framebuffer::DX11Framebuffer(const H2M::FramebufferSpecificationH2M& spec)
	: m_Specification(spec)
{
	if (spec.Width == 0 || spec.Height == 0)
	{
		auto width = Application::Get()->GetWindow()->GetWidth();
		auto height = Application::Get()->GetWindow()->GetHeight();
		Resize((uint32_t)(width * spec.Scale), (uint32_t)(height * spec.Scale), true);
	}
	else
	{
		Resize(spec.Width, spec.Height, true);
	}
}

DX11Framebuffer::~DX11Framebuffer()
{
}

void DX11Framebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
{
	m_Width = width;
	m_Height = height;

	if (!m_Specification.SwapChainTarget)
	{
		// Ref<DX11Framebuffer> instance = this;
		// HazelRenderer::Submit([instance, width, height]() mutable
		// {
		// });
		{
			// auto device = DX11Context::GetCurrentDevice()->GetDX11Device();

			// COLOR ATTACHMENT
			{
				// TODO
			}

			// DEPTH ATTACHMENT
			{
				// TODO
			}
		}
	}
	else
	{
		// DX11SwapChain& swapChain = DX11Context::Get()->GetSwapChain();
		// m_RenderPass = swapChain.GetRenderPass();
	}

	for (auto& callback : m_ResizeCallbacks)
	{
		callback(this);
	}
}

void DX11Framebuffer::AddResizeCallback(const std::function<void(H2M::RefH2M<H2M::FramebufferH2M>)>& func)
{
	m_ResizeCallbacks.push_back(func);
}
