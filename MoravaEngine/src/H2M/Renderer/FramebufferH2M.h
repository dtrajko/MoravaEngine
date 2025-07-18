/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>


namespace H2M
{

	class FramebufferH2M;

	enum class FramebufferBlendModeH2M
	{
		None = 0,
		OneZero,
		SrcAlphaOneMinusSrcAlpha,
		Additive,
		Zero_SrcColor
	};

	enum class FramebufferFormatH2M
	{
		None    = 0,
		RGBA8   = 1,
		RGBA16F = 2
	};

	struct FramebufferTextureSpecificationH2M
	{
		FramebufferTextureSpecificationH2M() = default;
		FramebufferTextureSpecificationH2M(ImageFormatH2M format) : Format(format) {}

		ImageFormatH2M Format = ImageFormatH2M::None;
		bool Blend = true;
		FramebufferBlendModeH2M BlendMode = FramebufferBlendModeH2M::SrcAlphaOneMinusSrcAlpha;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpecificationH2M
	{
		FramebufferAttachmentSpecificationH2M() = default;
		FramebufferAttachmentSpecificationH2M(const std::initializer_list<FramebufferTextureSpecificationH2M>& attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecificationH2M> Attachments;
	};

	struct FramebufferSpecificationH2M
	{
		float Scale = 1.0f;
		uint32_t Width = 0;
		uint32_t Height = 0;
		FramebufferFormatH2M Format = FramebufferFormatH2M::None;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		bool ClearOnLoad = true;
		FramebufferAttachmentSpecificationH2M Attachments;
		uint32_t Samples = 1; // multisampling

		// TODO: Temp, needs scale
		bool NoResize = false;

		// Master switch (individual attachments can be disabled in FramebufferTextureSpecification)
		bool Blend = true;
		// None means use BlendMode in FramebufferTextureSpecification
		FramebufferBlendModeH2M BlendMode = FramebufferBlendModeH2M::None;

		// SwapChainTarget = screen buffer (i.e. no framebuffer)
		bool SwapChainTarget = false;

		// Note: these are used to attach multi-layered depth images and color image arrays
		RefH2M<Image2D_H2M> ExistingImage;
		std::vector<uint32_t> ExistingImageLayers;

		// Specify existing images to attach instead of creating
		// new images. attachment index -> image
		std::map<uint32_t, RefH2M<Image2D_H2M>> ExistingImages;

		// At the moment this will just create a new render pass
		// with an existing framebuffer
		RefH2M<FramebufferH2M> ExistingFramebuffer;

		std::string DebugName;
	};

	class FramebufferH2M : public RefCountedH2M
	{
	public:
		virtual ~FramebufferH2M() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;
		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;
		virtual void AddResizeCallback(const std::function<void(RefH2M<FramebufferH2M>)>& func) = 0;
		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;
		virtual RefH2M<Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const = 0;
		virtual RefH2M<Image2D_H2M> GetDepthImage() const = 0;
		virtual const FramebufferSpecificationH2M& GetSpecification() const = 0;

		// virtual methods from OpenGLFramebufferHazel2D
		virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;
		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;

		static RefH2M<FramebufferH2M> Create(const FramebufferSpecificationH2M& spec);

	};

	class FramebufferPoolH2M final
	{
	public:
		FramebufferPoolH2M(uint32_t maxFBs = 32);
		~FramebufferPoolH2M();

		std::weak_ptr<FramebufferH2M> AllocateBuffer();
		void Add(const RefH2M<FramebufferH2M>& framebuffer);

		std::vector<RefH2M<FramebufferH2M>>& GetAll() { return m_Pool; }
		const std::vector<RefH2M<FramebufferH2M>>& GetAll() const { return m_Pool; }

		inline static FramebufferPoolH2M* GetGlobal() { return s_Instance; }
	private:
		std::vector<RefH2M<FramebufferH2M>> m_Pool;

		static FramebufferPoolH2M* s_Instance;
	};

}
