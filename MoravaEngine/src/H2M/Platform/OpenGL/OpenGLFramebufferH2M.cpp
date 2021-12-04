/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLFramebufferH2M.h"

#include "H2M/Renderer/ImageH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "OpenGLImageH2M.h"

#include <GL/glew.h>


namespace H2M
{

	namespace Utils
	{
		static GLenum TextureTarget(bool multisampled)
		{
			return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		}

		static void CreateTextures(bool multisampled, RendererID_H2M* outID, uint32_t count)
		{
			glCreateTextures(TextureTarget(multisampled), 1, outID);
		}

		static void BindTexture(bool multisampled, RendererID_H2M id)
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

			H2M_CORE_ASSERT(false, "Unknown format");
			return 0;
		}

		static GLenum DepthAttachmentType(ImageFormatH2M format)
		{
			switch (format)
			{
			case ImageFormatH2M::DEPTH32F:        return GL_DEPTH_ATTACHMENT;
			case ImageFormatH2M::DEPTH24STENCIL8: return GL_DEPTH_STENCIL_ATTACHMENT;
			}
			H2M_CORE_ASSERT(false, "Unknown format");
			return 0;
		}

		static RefH2M<Image2D_H2M> CreateAndAttachColorAttachment(int samples, ImageFormatH2M format, uint32_t width, uint32_t height, int index)
		{
			bool multisampled = samples > 1;
			RefH2M<Image2D_H2M> image;
			if (multisampled)
			{
				//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				image = Image2D_H2M::Create(format, width, height);
				image->Invalidate();
			}

			RefH2M<OpenGLImage2D_H2M> glImage = image.As<OpenGLImage2D_H2M>();
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), glImage->GetRendererID(), 0);
			return image;
		}

		static RefH2M<Image2D_H2M> AttachDepthTexture(int samples, ImageFormatH2M format, uint32_t width, uint32_t height)
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
			RefH2M<Image2D_H2M> image;
			if (multisampled)
			{
				//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				image = Image2D_H2M::Create(format, width, height);
				image->Invalidate();
			}

			RefH2M<OpenGLImage2D_H2M> glImage = image.As<OpenGLImage2D_H2M>();
			glFramebufferTexture2D(GL_FRAMEBUFFER, Utils::DepthAttachmentType(format), TextureTarget(multisampled), glImage->GetRendererID(), 0);
			return image;

		}

	}

	OpenGLFramebufferH2M::OpenGLFramebufferH2M(const FramebufferSpecificationH2M& spec)
		: m_Specification(spec)
	{
		if (!spec.SwapChainTarget || spec.Width == 0 || spec.Height == 0)
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

	OpenGLFramebufferH2M::~OpenGLFramebufferH2M()
	{
		GLuint rendererID = m_RendererID;
		// HazelRenderer::Submit([rendererID]() {});

		glDeleteFramebuffers(1, &rendererID);
	}

	void OpenGLFramebufferH2M::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		if (!forceRecreate && (m_Width == width && m_Height == height)) return;

		m_Width = width;
		m_Height = height;

		// RefH2M<OpenGLFramebufferH2M> instance = this;
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

			if (m_DepthAttachmentFormat != ImageFormatH2M::None)
			{
				m_DepthAttachment = Utils::AttachDepthTexture(m_Specification.Samples, m_DepthAttachmentFormat, m_Width, m_Height);
			}

			if (m_ColorAttachments.size() > 1)
			{
				H2M_CORE_ASSERT(m_ColorAttachments.size() <= 4);
				GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
				glDrawBuffers((GLsizei)m_ColorAttachments.size(), buffers);
			}
			else if (m_ColorAttachments.empty())
			{
				// Only depth-pass
				glDrawBuffer(GL_NONE);
			}

			H2M_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	void OpenGLFramebufferH2M::Bind() const
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

	void OpenGLFramebufferH2M::Unbind() const
	{
		// HazelRenderer::Submit([=]() {});

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebufferH2M::BindTexture(uint32_t attachmentIndex, uint32_t slot) const
	{
		// RefH2M<const OpenGLFramebufferH2M> instance = this;
		// HazelRenderer::Submit([instance, attachmentIndex, slot]() {});

		glBindTextureUnit(slot, m_ColorAttachments[attachmentIndex]);
	}
}
