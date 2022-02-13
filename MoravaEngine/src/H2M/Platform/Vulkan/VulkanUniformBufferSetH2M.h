/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#pragma once

#include "H2M/Core/AssertH2M.h"
#include "H2M/Renderer/UniformBufferSetH2M.h"

#include <map>


namespace H2M
{

	class VulkanUniformBufferSetH2M : public UniformBufferSetH2M
	{

	public:
		VulkanUniformBufferSetH2M(uint32_t frames) : m_Frames(frames) {}
		virtual ~VulkanUniformBufferSetH2M() {}

		virtual void Create(uint32_t size, uint32_t binding) override
		{
			for (uint32_t frame = 0; frame < m_Frames; frame++)
			{
				RefH2M<UniformBufferH2M> uniformBuffer = UniformBufferH2M::Create(size, binding);
				Set(uniformBuffer, 0, frame);
			}
		}

		virtual RefH2M<UniformBufferH2M> Get(uint32_t binding, uint32_t set = 0, uint32_t frame = 0) override
		{
			H2M_CORE_ASSERT(m_UniformBuffers.find(frame) != m_UniformBuffers.end());
			H2M_CORE_ASSERT(m_UniformBuffers.at(frame).find(set) != m_UniformBuffers.at(frame).end());
			H2M_CORE_ASSERT(m_UniformBuffers.at(frame).at(set).find(binding) != m_UniformBuffers.at(frame).at(set).end());

			return m_UniformBuffers.at(frame).at(set).at(binding);
		}

		virtual void Set(RefH2M<UniformBufferH2M> uniformBuffer, uint32_t set = 0, uint32_t frame = 0) override
		{
			m_UniformBuffers[frame][set][uniformBuffer->GetBinding()] = uniformBuffer;
		}

	private:
		uint32_t m_Frames;
		std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, RefH2M<UniformBufferH2M>>>> m_UniformBuffers; // frame->set->binding

	};

}
