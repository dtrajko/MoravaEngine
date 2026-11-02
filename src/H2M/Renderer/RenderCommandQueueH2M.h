/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <cstdint>


namespace H2M
{

	class RenderCommandQueueH2M
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		RenderCommandQueueH2M();
		~RenderCommandQueueH2M();

		void* Allocate(RenderCommandFn fn, uint32_t size);
		void Execute();

	private:
		uint8_t* m_CommandBuffer = nullptr;
		uint8_t* m_CommandBufferPtr = nullptr;
		uint32_t m_CommandCount = 0;

	};

}
