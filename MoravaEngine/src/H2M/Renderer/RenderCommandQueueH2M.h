#pragma once

#include <cstdint>


namespace Hazel {

	class RenderCommandQueue
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		RenderCommandQueue();
		~RenderCommandQueue();

		void* Allocate(RenderCommandFn fn, uint32_t size);
		void Execute();

	private:
		uint8_t* m_CommandBuffer = nullptr;
		uint8_t* m_CommandBufferPtr = nullptr;
		uint32_t m_CommandCount = 0;

	};

}
