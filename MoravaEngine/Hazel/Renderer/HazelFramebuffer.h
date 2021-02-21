#pragma once

#include "../Core/Ref.h"
#include "RendererAPI.h"

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

	struct FramebufferSpecification
	{
		uint32_t Width = 1280;
		uint32_t Height = 720;
		glm::vec4 ClearColor;
		FramebufferFormat Format = FramebufferFormat::RGBA8;
		uint32_t Samples = 1; // multisampling
		
		// SwapChainTarget = screen buffer (i.e. no framebuffer)
		bool SwapChainTarget = false;
	};

	class HazelFramebuffer : public RefCounted
	{
	public:
		virtual ~HazelFramebuffer() {}
		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) = 0;

		virtual void BindTexture(uint32_t slot = 0) const = 0;

		virtual RendererID GetRendererID() const = 0;
		virtual RendererID GetColorAttachmentRendererID() const = 0;
		virtual RendererID GetDepthAttachmentRendererID() const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<HazelFramebuffer> Create(const FramebufferSpecification& spec);
	};

	class FramebufferPool final
	{
	public:
		FramebufferPool(uint32_t maxFBs = 32);
		~FramebufferPool();

		std::weak_ptr<HazelFramebuffer> AllocateBuffer();
		void Add(const Ref<HazelFramebuffer>& framebuffer);

		std::vector<Ref<HazelFramebuffer>>& GetAll() { return m_Pool; }
		const std::vector<Ref<HazelFramebuffer>>& GetAll() const { return m_Pool; }

		inline static FramebufferPool* GetGlobal() { return s_Instance; }
	private:
		std::vector<Ref<HazelFramebuffer>> m_Pool;

		static FramebufferPool* s_Instance;
	};

}
