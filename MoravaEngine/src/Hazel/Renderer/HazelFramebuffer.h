#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/RendererAPI.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>


namespace Hazel {

	enum class FramebufferFormat
	{
		None    = 0,
		RGBA8   = 1,
		RGBA16F = 2
	};

	struct HazelFramebufferTextureSpecification
	{
		HazelFramebufferTextureSpecification() = default;
		HazelFramebufferTextureSpecification(HazelImageFormat format) : Format(format) {}

		HazelImageFormat Format;
		// TODO: filtering/wrap
	};

	struct HazelFramebufferAttachmentSpecification
	{
		HazelFramebufferAttachmentSpecification() = default;
		HazelFramebufferAttachmentSpecification(const std::initializer_list<HazelFramebufferTextureSpecification>& attachments)
			: Attachments(attachments) {}

		std::vector<HazelFramebufferTextureSpecification> Attachments;
	};

	struct HazelFramebufferSpecification
	{
		float Scale = 1.0f;
		uint32_t Width = 0;
		uint32_t Height = 0;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		HazelFramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1; // multisampling

		// TODO: Temp, needs scale
		bool NoResize = false;

		// SwapChainTarget = screen buffer (i.e. no framebuffer)
		bool SwapChainTarget = false;

		std::string DebugName;
	};

	class HazelFramebuffer : public RefCounted
	{
	public:
		virtual ~HazelFramebuffer() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;
		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) = 0;
		virtual void AddResizeCallback(const std::function<void(Ref<HazelFramebuffer>)>& func) = 0;
		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual RendererID GetRendererID() const = 0;
		virtual Ref<HazelImage2D> GetImage(uint32_t attachmentIndex = 0) const = 0;
		virtual Ref<HazelImage2D> GetDepthImage() const = 0;
		virtual const HazelFramebufferSpecification& GetSpecification() const = 0;

		static Ref<HazelFramebuffer> Create(const HazelFramebufferSpecification& spec);

	};

	class HazelFramebufferPool final
	{
	public:
		HazelFramebufferPool(uint32_t maxFBs = 32);
		~HazelFramebufferPool();

		std::weak_ptr<HazelFramebuffer> AllocateBuffer();
		void Add(const Ref<HazelFramebuffer>& framebuffer);

		std::vector<Ref<HazelFramebuffer>>& GetAll() { return m_Pool; }
		const std::vector<Ref<HazelFramebuffer>>& GetAll() const { return m_Pool; }

		inline static HazelFramebufferPool* GetGlobal() { return s_Instance; }
	private:
		std::vector<Ref<HazelFramebuffer>> m_Pool;

		static HazelFramebufferPool* s_Instance;
	};

}
