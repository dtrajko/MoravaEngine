#include "RenderCommandQueue.h"

#include <memory>
#include <functional>


namespace Hazel {

    RenderCommandQueue::RenderCommandQueue()
    {
        m_CommandBuffer = new uint8_t[10 * 1024 * 1024]; // 10MB buffer
        m_CommandBufferPtr = m_CommandBuffer;
        memset(m_CommandBuffer, 0, 10 * 1024 * 1024);
    }

    RenderCommandQueue::~RenderCommandQueue()
    {
        delete[] m_CommandBuffer;
    }

    void* RenderCommandQueue::Allocate(RenderCommandFn fn, uint32_t size)
    {
        // TODO: alignment
        *(RenderCommandFn*)m_CommandBufferPtr = fn;
        m_CommandBufferPtr += sizeof(RenderCommandFn);

        *(uint32_t*)m_CommandBufferPtr = size;
        m_CommandBufferPtr += sizeof(uint32_t);

        void* memory = m_CommandBufferPtr;
        m_CommandBufferPtr += size;

        m_CommandCount++;
        return memory;
    }

    void RenderCommandQueue::Execute()
    {
        uint8_t* buffer = m_CommandBuffer;

        for (uint32_t i = 0; i < m_CommandCount; i++)
        {
            RenderCommandFn function = *(RenderCommandFn*)buffer;
            buffer += sizeof(RenderCommandFn);

            uint32_t size = *(uint32_t*)buffer;
            buffer += sizeof(uint32_t);
            function(buffer);
            buffer += size;
        }

        m_CommandBufferPtr = m_CommandBuffer;
        m_CommandCount = 0;
    }

}
