#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/UniformBuffer.h"


namespace Hazel
{

	class UniformBufferSet : public RefCounted
	{
	public:
		virtual ~UniformBufferSet() {};

		virtual void Create(uint32_t size, uint32_t binding) = 0;

		virtual Ref<UniformBuffer> Get(uint32_t binding, uint32_t set = 0, uint32_t frame = 0) = 0;
		virtual void Set(Ref<UniformBuffer> uniformBuffer, uint32_t set = 0, uint32_t frame = 0) = 0;

		static Ref<UniformBufferSet> Create(uint32_t frames);

	};

}
