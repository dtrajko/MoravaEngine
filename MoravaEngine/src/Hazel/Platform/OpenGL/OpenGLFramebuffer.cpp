#include "OpenGLFramebuffer.h"

#include "Hazel/Renderer/HazelRenderer.h"
#include "OpenGLImage.h"

#include <GL/glew.h>


namespace Hazel {

	namespace Utils
	{
		static GLenum TextureTarget(bool multisampled)
		{
			return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		}

		static void CreateTextures(bool multisampled, RendererID* outID, uint32_t count)
		{
			glCreateTextures(TextureTarget(multisampled), 1, outID);
		}

		static void BindTexture(bool multisampled, RendererID id)
		{
			glBindTexture(TextureTarget(multisampled), id);
		}

		static GLenum DataType(GLenum format)
		{
			switch (format)
			{
			case GL_RGBA8:             return GL_UNSIGNED_BYTE;
			case GL_RG16F:
			case GL_RG32F:
			case GL_RGBA16F:
			case GL_RGBA32F:           return GL_FLOAT;
			case GL_DEPTH24_STENCIL8:  return GL_UNSIGNED_INT_24_8;
			}

			HZ_CORE_ASSERT(false, "Unknown format");
			return 0;
		}

		static GLenum DepthAttachmentType(HazelImageFormat format)
		{
			switch (format)
			{
			case HazelImageFormat::DEPTH32F:        return GL_DEPTH_ATTACHMENT;
			case HazelImageFormat::DEPTH24STENCIL8: return GL_DEPTH_STENCIL_ATTACHMENT;
			}
			HZ_CORE_ASSERT(false, "Unknown format");
			return 0;
		}

		static Ref<HazelImage2D> CreateAndAttachColorAttachment(int samples, HazelImageFormat format, uint32_t width, uint32_t height, int index)
		{
			bool multisampled = samples > 1;
			Ref<HazelImage2D> image;
			if (multisampled)
			{
				//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				image = HazelImage2D::Create(format, width, height);
				image->Invalidate();
			}

			Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), glImage->GetRendererID(), 0);
			return image;
		}

		static Ref<HazelImage2D> AttachDepthTexture(int samples, HazelImageFormat format, uint32_t width, uint32_t height)
		{
#if 0
			bool multisampled = samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

				glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
#endif
			bool multisampled = samples > 1;
			Ref<HazelImage2D> image;
			if (multisampled)
			{
				//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				image = HazelImage2D::Create(format, width, height);
				image->Invalidate();
			}

			Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
			glFramebufferTexture2D(GL_FRAMEBUFFER, Utils::DepthAttachmentType(format), TextureTarget(multisampled), glImage->GetRendererID(), 0);
			return image;

		}

	}

	OpenGLFramebuffer::OpenGLFramebuffer(const HazelFramebufferSpecification& spec)
		: m_Specification(spec)
	{
		if (!spec.SwapChainTarget)
		{
			Resize(spec.Width, spec.Height, true);
		}
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		GLuint rendererID = m_RendererID;
		// HazelRenderer::Submit([rendererID]() {});

		glDeleteFramebuffers(1, &rendererID);
	}

	void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		if (!forceRecreate && (m_Width == width && m_Height == height)) return;

		m_Width = width;
		m_Height = height;

		// Ref<OpenGLFramebuffer> instance = this;
		// HazelRenderer::Submit([instance]() mutable {});

		{
			if (m_RendererID)
			{
				glDeleteFramebuffers(1, &m_RendererID);

				m_ColorAttachments.clear();
			}

			glGenFramebuffers(1, &m_RendererID);
			glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

			if (m_ColorAttachmentFormats.size())
			{
				m_ColorAttachments.resize(m_ColorAttachmentFormats.size());

				// Create color attachments
				for (size_t i = 0; i < m_ColorAttachments.size(); i++)
				{
					m_ColorAttachments[i] = Utils::CreateAndAttachColorAttachment(m_Specification.Samples, m_ColorAttachmentFormats[i], m_Width, m_Height, (int)i);
				}
			}

			if (m_DepthAttachmentFormat != HazelImageFormat::None)
			{
				m_DepthAttachment = Utils::AttachDepthTexture(m_Specification.Samples, m_DepthAttachmentFormat, m_Width, m_Height);
			}

			if (m_ColorAttachments.size() > 1)
			{
				HZ_CORE_ASSERT(m_ColorAttachments.size() <= 4);
				GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
				glDrawBuffers((GLsizei)m_ColorAttachments.size(), buffers);
			}
			else if (m_ColorAttachments.empty())
			{
				// Only depth-pass
				glDrawBuffer(GL_NONE);
			}

			HZ_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	void OpenGLFramebuffer::Bind() const
	{
		if (m_Specification.SwapChainTarget)
		{
			// HazelRenderer::Submit([=]() {});

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, m_Specification.Width, m_Specification.Height);
		}
		else
		{
			// HazelRenderer::Submit([=]() {});

			glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
			glViewport(0, 0, m_Specification.Width, m_Specification.Height);
		}
	}

	void OpenGLFramebuffer::Unbind() const
	{
		// HazelRenderer::Submit([=]() {});

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::BindTexture(uint32_t attachmentIndex, uint32_t slot) const
	{
		// Ref<const OpenGLFramebuffer> instance = this;
		// HazelRenderer::Submit([instance, attachmentIndex, slot]() {});

		glBindTextureUnit(slot, m_ColorAttachments[attachmentIndex]);
	}
}
