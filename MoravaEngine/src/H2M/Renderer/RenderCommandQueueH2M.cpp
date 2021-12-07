/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "RenderCommandQueueH2M.h"

#include <memory>
#include <functional>
#include <cstring>


namespace H2M
{

    RenderCommandQueueH2M::RenderCommandQueueH2M()
    {
        m_CommandBuffer = new uint8_t[10 * 1024 * 1024]; // 10MB buffer
        m_CommandBufferPtr = m_CommandBuffer;
        memset(m_CommandBuffer, 0, 10 * 1024 * 1024);
    }

    RenderCommandQueueH2M::~RenderCommandQueueH2M()
    {
        delete[] m_CommandBuffer;
    }

    void* RenderCommandQueueH2M::Allocate(RenderCommandFn fn, uint32_t size)
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

    void RenderCommandQueueH2M::Execute()
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
