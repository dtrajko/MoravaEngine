#include "OpenGLFramebuffer.h"

#include "../../Renderer/HazelRenderer.h"

#include <GL/glew.h>


namespace Hazel {

	OpenGLFramebuffer::OpenGLFramebuffer(const HazelFramebufferSpecification& spec)
		: m_Specification(spec)
	{
		if (!spec.SwapChainTarget)
			Resize(spec.Width, spec.Height, true);
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		GLuint rendererID = m_RendererID;
		HazelRenderer::Submit([rendererID]() {
		});

		glDeleteFramebuffers(1, &rendererID);
	}

	void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		if (!forceRecreate && (m_Specification.Width == width && m_Specification.Height == height))
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;
		Ref<OpenGLFramebuffer> instance = this;
		HazelRenderer::Submit([instance]() mutable
		{
		});

		if (instance->m_RendererID)
		{
			glDeleteFramebuffers(1, &instance->m_RendererID);
			glDeleteTextures(1, &instance->m_ColorAttachment);
			glDeleteTextures(1, &instance->m_DepthAttachment);
		}

		glGenFramebuffers(1, &instance->m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, instance->m_RendererID);

		bool multisample = instance->m_Specification.Samples > 1;
		if (multisample)
		{
			glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &instance->m_ColorAttachment);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, instance->m_ColorAttachment);

			// TODO: Create Hazel texture object based on format here
			if (instance->m_Specification.Format == FramebufferFormat::RGBA16F)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, instance->m_Specification.Samples, GL_RGBA16F, instance->m_Specification.Width, instance->m_Specification.Height, GL_FALSE);
			}
			else if (instance->m_Specification.Format == FramebufferFormat::RGBA8)
			{
				glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, instance->m_Specification.Samples, GL_RGBA8, instance->m_Specification.Width, instance->m_Specification.Height, GL_FALSE);
			}
			// glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			// glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		}
		else
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &instance->m_ColorAttachment);
			glBindTexture(GL_TEXTURE_2D, instance->m_ColorAttachment);

			// TODO: Create Hazel texture object based on format here
			if (instance->m_Specification.Format == FramebufferFormat::RGBA16F)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, instance->m_Specification.Width, instance->m_Specification.Height, 0, GL_RGBA, GL_FLOAT, nullptr);
			}
			else if (instance->m_Specification.Format == FramebufferFormat::RGBA8)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, instance->m_Specification.Width, instance->m_Specification.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, instance->m_ColorAttachment, 0);
		}

		if (multisample)
		{
			glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &instance->m_DepthAttachment);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, instance->m_DepthAttachment);
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, instance->m_Specification.Samples, GL_DEPTH24_STENCIL8, instance->m_Specification.Width, instance->m_Specification.Height, GL_FALSE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		}
		else
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &instance->m_DepthAttachment);
			glBindTexture(GL_TEXTURE_2D, instance->m_DepthAttachment);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, instance->m_Specification.Width, instance->m_Specification.Height, 0,
				GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
			);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, instance->m_DepthAttachment, 0);
		}

		if (multisample)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, instance->m_ColorAttachment, 0);
		else
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, instance->m_ColorAttachment, 0);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, instance->m_DepthAttachment, 0);

		HZ_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::Bind() const
	{
		if (m_Specification.SwapChainTarget)
		{
			HazelRenderer::Submit([=]() {
			});

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, m_Specification.Width, m_Specification.Height);
		}
		else
		{
			HazelRenderer::Submit([=]() {
			});

			glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
			glViewport(0, 0, m_Specification.Width, m_Specification.Height);
		}
	}

	void OpenGLFramebuffer::Unbind() const
	{
		HazelRenderer::Submit([=]() {
		});

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::BindTexture(uint32_t slot) const
	{
		HZ_CORE_ASSERT(!m_Specification.SwapChainTarget);
		if (m_Specification.SwapChainTarget)
			return;

		HazelRenderer::Submit([=]() {
		});

		glBindTextureUnit(slot, m_ColorAttachment);
	}
}
