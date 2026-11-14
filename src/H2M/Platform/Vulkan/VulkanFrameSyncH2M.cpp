#include "VulkanFrameSyncH2M.h"

// This file calls the Vulkan functions themselves, not the shim's waiting forms
#include "VulkanSyncShimUndefH2M.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>


namespace H2M::VulkanFrameSync
{
	namespace
	{
		std::atomic<uint32_t> s_FramesInFlight{ MaxFramesInFlight };
		VkDevice s_Device = VK_NULL_HANDLE;

		// The frame on the GPU that hasn't been waited for (VK_NULL_HANDLE: none)
		std::mutex s_FenceMutex;
		VkFence s_InFlight = VK_NULL_HANDLE;
		std::atomic<bool> s_HasInFlight{ false };

		// The kept writes, in the order they were made
		std::mutex s_WritesMutex;
		struct UniformCopy
		{
			std::vector<uint8_t> Bytes;
			bool Pending = false;
		};
		std::unordered_map<VkDeviceMemory, UniformCopy> s_Uniforms;
		struct Write
		{
			VkDeviceMemory Memory = VK_NULL_HANDLE;
			VkDeviceSize Offset = 0;
			void* Destination = nullptr; // WriteMapped: where; WriteMemory: nullptr (mapped at Present)
			std::vector<uint8_t> Bytes;
		};
		std::vector<Write> s_Writes;

		std::atomic<uint64_t> s_DescriptorUpdates{ 0 };

		std::mutex s_StatsMutex;
		Stats s_Stats;

		// Waits for the fence in flight; true when it had to (the GPU was still busy), and how long
		bool Wait(double& blockedMs)
		{
			blockedMs = 0.0;
			if (!s_HasInFlight.load())
			{
				return false;
			}
			std::lock_guard<std::mutex> lock(s_FenceMutex);
			if (s_InFlight == VK_NULL_HANDLE)
			{
				return false;
			}
			bool blocked = false;
			if (vkGetFenceStatus(s_Device, s_InFlight) == VK_NOT_READY)
			{
				const auto start = std::chrono::steady_clock::now();
				vkWaitForFences(s_Device, 1, &s_InFlight, VK_TRUE, UINT64_MAX);
				blockedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
				blocked = true;
			}
			s_InFlight = VK_NULL_HANDLE;
			s_HasInFlight.store(false);
			return blocked;
		}

		void WriteNow(VkDeviceMemory memory, VkDeviceSize offset, const void* data, size_t size)
		{
			void* mapped = nullptr;
			if (size > 0 && vkMapMemory(s_Device, memory, offset, (VkDeviceSize)size, 0, &mapped) == VK_SUCCESS)
			{
				std::memcpy(mapped, data, size);
				vkUnmapMemory(s_Device, memory);
			}
		}
	}

	void SetFramesInFlight(uint32_t count)
	{
		s_FramesInFlight.store(std::clamp(count, 1u, MaxFramesInFlight));
	}

	uint32_t GetFramesInFlight()
	{
		return s_FramesInFlight.load();
	}

	bool IsDeferring()
	{
		return s_FramesInFlight.load() > 1 && s_Device != VK_NULL_HANDLE;
	}

	void SetDevice(VkDevice device)
	{
		s_Device = device;
	}

	void SetInFlight(VkFence fence)
	{
		std::lock_guard<std::mutex> lock(s_FenceMutex);
		s_InFlight = fence;
		s_HasInFlight.store(fence != VK_NULL_HANDLE);
	}

	void WaitForGpu()
	{
		double blockedMs = 0.0;
		if (Wait(blockedMs))
		{
			std::lock_guard<std::mutex> lock(s_StatsMutex);
			s_Stats.BlockingWaits++;
			s_Stats.BlockedMs += blockedMs;
		}
	}

	void WaitForLastFrame()
	{
		double blockedMs = 0.0;
		Wait(blockedMs);
	}

	void* BeginUniformWrite(VkDeviceMemory memory, VkDeviceSize size)
	{
		std::lock_guard<std::mutex> lock(s_WritesMutex);
		UniformCopy& copy = s_Uniforms[memory];
		if (copy.Bytes.size() != (size_t)size)
		{
			copy.Bytes.resize((size_t)size, 0);
		}
		return copy.Bytes.data();
	}

	void EndUniformWrite(VkDeviceMemory memory)
	{
		std::lock_guard<std::mutex> lock(s_WritesMutex);
		auto it = s_Uniforms.find(memory);
		if (it != s_Uniforms.end())
		{
			it->second.Pending = true;
		}
	}

	void WriteMemory(VkDeviceMemory memory, VkDeviceSize offset, const void* data, size_t size)
	{
		if (size == 0)
		{
			return;
		}
		if (!IsDeferring())
		{
			WriteNow(memory, offset, data, size);
			return;
		}
		Write write;
		write.Memory = memory;
		write.Offset = offset;
		write.Bytes.assign((const uint8_t*)data, (const uint8_t*)data + size);
		std::lock_guard<std::mutex> lock(s_WritesMutex);
		s_Writes.push_back(std::move(write));
	}

	void WriteMapped(VkDeviceMemory memory, void* destination, const void* data, size_t size)
	{
		if (size == 0)
		{
			return;
		}
		if (!IsDeferring())
		{
			std::memcpy(destination, data, size);
			return;
		}
		Write write;
		write.Memory = memory;
		write.Destination = destination;
		write.Bytes.assign((const uint8_t*)data, (const uint8_t*)data + size);
		std::lock_guard<std::mutex> lock(s_WritesMutex);
		s_Writes.push_back(std::move(write));
	}

	void ForgetMemory(VkDeviceMemory memory)
	{
		std::lock_guard<std::mutex> lock(s_WritesMutex);
		s_Uniforms.erase(memory);
		s_Writes.erase(std::remove_if(s_Writes.begin(), s_Writes.end(), [memory](const Write& write) { return write.Memory == memory; }), s_Writes.end());
	}

	void FlushWrites()
	{
		WaitForGpu();
		std::lock_guard<std::mutex> lock(s_WritesMutex);
		uint64_t bytes = 0;
		for (auto& [memory, copy] : s_Uniforms)
		{
			if (copy.Pending)
			{
				WriteNow(memory, 0, copy.Bytes.data(), copy.Bytes.size());
				copy.Pending = false;
				bytes += copy.Bytes.size();
			}
		}
		for (const Write& write : s_Writes)
		{
			if (write.Destination)
			{
				std::memcpy(write.Destination, write.Bytes.data(), write.Bytes.size());
			}
			else
			{
				WriteNow(write.Memory, write.Offset, write.Bytes.data(), write.Bytes.size());
			}
			bytes += write.Bytes.size();
		}
		s_Writes.clear();
		std::lock_guard<std::mutex> statsLock(s_StatsMutex);
		s_Stats.DeferredBytes += bytes;
	}

	void CountDescriptorUpdate()
	{
		s_DescriptorUpdates.fetch_add(1);
	}

	uint64_t GetDescriptorUpdateCount()
	{
		return s_DescriptorUpdates.load();
	}

	Stats GetStats()
	{
		std::lock_guard<std::mutex> lock(s_StatsMutex);
		return s_Stats;
	}

	void CountFrame()
	{
		std::lock_guard<std::mutex> lock(s_StatsMutex);
		s_Stats.Frames++;
	}
}
