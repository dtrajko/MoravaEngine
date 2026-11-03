#include "VulkanBufferReleaseH2M.h"

#include "VulkanContextH2M.h"

#include "Core/Log.h"

#include <vector>


namespace H2M
{
	namespace VulkanBufferReleaseH2M
	{
		static constexpr int FramesToWait = 4; // more than the frames in flight

		struct Pending
		{
			VkBuffer Buffer;
			VkDeviceMemory Memory;
			int FramesLeft;
		};

		// Never destroyed: a buffer may go after other statics are gone
		static std::vector<Pending>& GetPending()
		{
			static std::vector<Pending>* s_Pending = new std::vector<Pending>();
			return *s_Pending;
		}

		static bool s_ShutDown = false;

		static void Free(VkDevice device, const Pending& pending)
		{
			vkDestroyBuffer(device, pending.Buffer, nullptr);
			vkFreeMemory(device, pending.Memory, nullptr);
		}

		void Queue(VkBuffer buffer, VkDeviceMemory memory)
		{
			if (buffer && !s_ShutDown)
			{
				GetPending().push_back({ buffer, memory, FramesToWait });
			}
		}

		void OnFrame()
		{
			std::vector<Pending>& pending = GetPending();
			if (pending.empty())
			{
				return;
			}
			const VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
			size_t kept = 0;
			for (Pending& item : pending)
			{
				if (--item.FramesLeft <= 0)
				{
					Free(device, item);
				}
				else
				{
					pending[kept++] = item;
				}
			}
			if (kept < pending.size())
			{
				Log::GetLogger()->trace("Vertex and index buffers: {0} freed", pending.size() - kept);
			}
			pending.resize(kept);
		}

		void Shutdown()
		{
			const VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
			vkDeviceWaitIdle(device);
			for (const Pending& item : GetPending())
			{
				Free(device, item);
			}
			GetPending().clear();
			s_ShutDown = true;
		}
	}
}
