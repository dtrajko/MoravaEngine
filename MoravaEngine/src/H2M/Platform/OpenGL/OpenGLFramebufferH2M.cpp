/**
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

	static const uint32_t s_MaxFramebufferSize = 8192;

	namespace Utils
	{

		static GLenum TextureTarget(bool multisampled)
		{
			return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		}

		static void CreateTextures(bool multisampled, uint32_t* outID, uint32_t count)
		{
			glCreateTextures(TextureTarget(multisampled), count, outID);
		}

		static void BindTexture(bool multisampled, uint32_t id)
		{
			glBindTexture(TextureTarget(multisampled), id);
		}

		static GLenum DataType(GLenum format)
		{
			switch (format)
			{
				case GL_RGBA8:            return GL_UNSIGNED_BYTE;
				case GL_RG16F:
				case GL_RG32F:
				case GL_RGBA16F:
				case GL_RGBA32F:          return GL_FLOAT;
				case GL_DEPTH24_STENCIL8: return GL_UNSIGNED_INT_24_8;
				case GL_RED_INTEGER:      return GL_UNSIGNED_BYTE;
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

		static void AttachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
		{
			bool multisampled = samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);

			const char* internalFormatName = Util::FormatToString(internalFormat);
			const char* formatName = Util::FormatToString(format);
			Log::GetLogger()->debug("OpenGLFramebufferH2M::AttachColorTexture - internalFormat: {0} format: {1} [{2}x{3}]", internalFormatName, formatName, width, height);
		}

		static RefH2M<Image2D_H2M> AttachDepthTexture(int samples, ImageFormatH2M format, uint32_t width, uint32_t height)
		{
			bool multisampled = samples > 1;
			RefH2M<Image2D_H2M> image;
			if (multisampled)
			{
				// glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
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

		static void AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height)
		{
			bool multisampled = samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);

			const char* formatName = Util::FormatToString(format);
			const char* attachmentTypeName = Util::FormatToString(attachmentType);
			Log::GetLogger()->debug("OpenGLFramebufferH2M::AttachDepthTexture - format: {0} attachmentType: {1} [{2}x{3}]", formatName, attachmentTypeName, width, height);
		}

		static bool IsDepthFormatHazel2D(ImageFormatH2M format)
		{
			switch (format)
			{
				case ImageFormatH2M::DEPTH24STENCIL8:  return true;
			}

			return false;
		}

		static GLenum HazelFBTextureFormatToGL(ImageFormatH2M format)
		{
			switch (format)
			{
				case ImageFormatH2M::RGBA8:       return GL_RGBA8;
				case ImageFormatH2M::RED_INTEGER: return GL_RED_INTEGER;
			}

			H2M_CORE_ASSERT(false);
			return 0;
		}

	}

	OpenGLFramebufferH2M::OpenGLFramebufferH2M(const FramebufferSpecificationH2M& spec)
		: m_Specification(spec)
	{
		for (auto spec : m_Specification.Attachments.Attachments)
		{
			if (!Utils::IsDepthFormat(spec.Format))
			{
				m_ColorAttachmentSpecifications.emplace_back(spec);
			}
			else
			{
				m_DepthAttachmentSpecification = spec;
			}
		}

		Invalidate();
	}

	OpenGLFramebufferH2M::~OpenGLFramebufferH2M()
	{
		// GLuint rendererID = m_RendererID;
		// HazelRenderer::Submit([rendererID]() {});

		glDeleteFramebuffers(1, &m_RendererID);
		glDeleteTextures((GLsizei)m_ColorAttachmentIDs.size(), m_ColorAttachmentIDs.data());
		glDeleteTextures(1, &m_DepthAttachmentID);
	}

	void OpenGLFramebufferH2M::Invalidate()
	{
		if (m_RendererID)
		{
			glDeleteFramebuffers(1, &m_RendererID);
			glDeleteTextures((GLsizei)m_ColorAttachmentIDs.size(), m_ColorAttachmentIDs.data());
			glDeleteTextures(1, &m_DepthAttachmentID);

			m_ColorAttachmentIDs.clear();
			m_DepthAttachmentID = 0;
		}

		glCreateFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

		bool multisample = m_Specification.Samples > 1;

		// Attachments
		if (m_ColorAttachmentSpecifications.size())
		{
			m_ColorAttachmentIDs.resize(m_ColorAttachmentSpecifications.size());
			Utils::CreateTextures(multisample, m_ColorAttachmentIDs.data(), (uint32_t)m_ColorAttachmentIDs.size());

			for (size_t i = 0; i < m_ColorAttachmentIDs.size(); i++)
			{
				Utils::BindTexture(multisample, m_ColorAttachmentIDs[i]);
				switch (m_ColorAttachmentSpecifications[i].Format)
				{
					case ImageFormatH2M::RGBA8:
						Utils::AttachColorTexture(m_ColorAttachmentIDs[i], m_Specification.Samples, GL_RGBA8, GL_RGBA, m_Specification.Width, m_Specification.Height, (int)i);
						break;
					case ImageFormatH2M::RED_INTEGER:
						Utils::AttachColorTexture(m_ColorAttachmentIDs[i], m_Specification.Samples, GL_R16I/*GL_R32I*/, GL_RED_INTEGER, m_Specification.Width, m_Specification.Height, (int)i);
						break;
				}
			}
		}

		if (m_DepthAttachmentSpecification.Format != ImageFormatH2M::None)
		{
			Utils::CreateTextures(multisample, &m_DepthAttachmentID, 1);
			Utils::BindTexture(multisample, m_DepthAttachmentID);
			switch (m_DepthAttachmentSpecification.Format)
			{
				case ImageFormatH2M::DEPTH24STENCIL8:
					Utils::AttachDepthTexture(m_DepthAttachmentID, m_Specification.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, m_Specification.Width, m_Specification.Height);
					break;
			}
		}

		if (m_ColorAttachmentIDs.size() > 1)
		{
			H2M_CORE_ASSERT(m_ColorAttachmentIDs.size() <= 4);
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers((GLsizei)m_ColorAttachmentIDs.size(), buffers);
		}
		else if (m_ColorAttachmentIDs.empty())
		{
			// Only depth-pass
			glDrawBuffer(GL_NONE);
		}

		// H2M_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			Log::GetLogger()->error("OpenGLFramebufferH2M::Invalidate: Framebuffer is incomplete!");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

		glBindTextureUnit(slot, m_ColorAttachmentIDs[attachmentIndex]);
	}

	void OpenGLFramebufferH2M::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
		{
			H2M_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
			return;
		}

		m_Specification.Width = width;
		m_Specification.Height = height;

		Invalidate();
	}

	int OpenGLFramebufferH2M::ReadPixel(uint32_t attachmentIndex, int x, int y)
	{
		H2M_CORE_ASSERT(attachmentIndex < m_ColorAttachmentIDs.size());

		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
		int pixelData;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
		return pixelData;
	}

	void OpenGLFramebufferH2M::ClearAttachment(uint32_t attachmentIndex, int value)
	{
		H2M_CORE_ASSERT(attachmentIndex < m_ColorAttachmentIDs.size());

		auto& spec = m_ColorAttachmentSpecifications[attachmentIndex];
		glClearTexImage(m_ColorAttachmentIDs[attachmentIndex], 0,
			Utils::HazelFBTextureFormatToGL(spec.Format), GL_INT, &value);
	}

}
