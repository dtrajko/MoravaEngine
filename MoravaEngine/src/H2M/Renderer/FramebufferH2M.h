#pragma once

#include "Hazel-dev/Renderer/RendererAPI.h"

#include "H2M/Core/RefH2M.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>


namespace H2M {

	class FramebufferH2M;

	enum class FramebufferBlendMode
	{
		None = 0,
		OneZero,
		SrcAlphaOneMinusSrcAlpha,
		Additive,
		Zero_SrcColor
	};


	struct FramebufferTextureSpecificationH2M
	{
		FramebufferTextureSpecificationH2M() = default;
		FramebufferTextureSpecificationH2M(ImageFormatH2M format) : Format(format) {}

		ImageFormatH2M Format;
		bool Blend = true;
		FramebufferBlendMode BlendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpecificationH2M
	{
		FramebufferAttachmentSpecificationH2M() = default;
		FramebufferAttachmentSpecificationH2M(const std::initializer_list<FramebufferTextureSpecificationH2M>& attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecificationH2M> Attachments;
	};

	struct HazelFramebufferSpecification
	{
		float Scale = 1.0f;
		uint32_t Width = 0;
		uint32_t Height = 0;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		bool ClearOnLoad = true;
		FramebufferAttachmentSpecificationH2M Attachments;
		uint32_t Samples = 1; // multisampling

		// TODO: Temp, needs scale
		bool NoResize = false;

		// Master switch (individual attachments can be disabled in FramebufferTextureSpecification)
		bool Blend = true;
		// None means use BlendMode in FramebufferTextureSpecification
		FramebufferBlendMode BlendMode = FramebufferBlendMode::None;

		// SwapChainTarget = screen buffer (i.e. no framebuffer)
		bool SwapChainTarget = false;

		// Note: these are used to attach multi-layered depth images and color image arrays
		Ref<Image2DH2M> ExistingImage;
		std::vector<uint32_t> ExistingImageLayers;

		// Specify existing images to attach instead of creating
		// new images. attachment index -> image
		std::map<uint32_t, Ref<HazelImage2D>> ExistingImages;

		// At the moment this will just create a new render pass
		// with an existing framebuffer
		Ref<HazelFramebuffer> ExistingFramebuffer;

		std::string DebugName;
	};

	class FramebufferH2M : public RefCountedH2M
	{
	public:
		virtual ~FramebufferH2M() {}
		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) = 0;
		virtual void AddResizeCallback(const std::function<void(Ref<FramebufferH2M>)>& func) = 0;

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual RendererID GetRendererID() const = 0;

		virtual Ref<HazelImage2D> GetImage(uint32_t attachmentIndex = 0) const = 0;
		virtual Ref<HazelImage2D> GetDepthImage() const = 0;

		virtual const HazelFramebufferSpecification& GetSpecification() const = 0;

		static Ref<FramebufferH2M> Create(const HazelFramebufferSpecification& spec);
	};

	class HazelFramebufferPool final
	{
	public:
		HazelFramebufferPool(uint32_t maxFBs = 32);
		~HazelFramebufferPool();

		std::weak_ptr<FramebufferH2M> AllocateBuffer();
		void Add(const Ref<FramebufferH2M>& framebuffer);

		std::vector<Ref<FramebufferH2M>>& GetAll() { return m_Pool; }
		const std::vector<Ref<FramebufferH2M>>& GetAll() const { return m_Pool; }

		inline static HazelFramebufferPool* GetGlobal() { return s_Instance; }
	private:
		std::vector<Ref<FramebufferH2M>> m_Pool;

		static HazelFramebufferPool* s_Instance;
	};

}
