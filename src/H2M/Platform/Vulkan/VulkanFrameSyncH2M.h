#pragma once

// Frames in flight: the CPU records the next frame while the GPU still draws the last one (two frames), or waits for
// each frame to finish before starting the next (one frame, as before: --frames-in-flight 1).
//
// With two, what a frame on the GPU uses must not change under it while the CPU prepares the next:
// - Writes into GPU memory the frames read (uniform buffers, instance and vertex data the CPU fills each frame) are kept
//   here and done at Present, after the GPU has finished the last frame and before the next is submitted
//   (BeginUniformWrite / EndUniformWrite: VulkanShaderH2M's uniform buffers; WriteMemory, WriteMapped).
// - Whatever changes or destroys a resource first waits for the GPU (WaitForGpu): VulkanSyncShimH2M.h, in every
//   source file of the engine (the precompiled header), sends vkDestroy*, vkFree*, vkUpdateDescriptorSets and
//   vkResetDescriptorPool through it. That costs nothing when the GPU is done already; a wait that blocks is counted
//   (GetStats): one each frame means a part of the frame changes what the GPU uses and loses the overlap.

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>


namespace H2M::VulkanFrameSync
{
	constexpr uint32_t MaxFramesInFlight = 2;

	// 1 or 2 (the command line's --frames-in-flight), set before the swapchain is made
	void SetFramesInFlight(uint32_t count);
	uint32_t GetFramesInFlight();
	bool IsDeferring(); // two frames in flight: the writes wait for Present

	// The swapchain: the device, and the fence of the frame just submitted (not waited for yet)
	void SetDevice(VkDevice device);
	void SetInFlight(VkFence fence);

	// Waits until the GPU has finished the frame in flight, if there is one. Before anything that changes or destroys
	// what that frame uses
	void WaitForGpu();
	// Present: the wait for the last frame before the next is submitted (not counted as a blocking wait)
	void WaitForLastFrame();

	// A uniform buffer's contents, written by the CPU each frame (VulkanShaderH2M::MapUniformBuffer): a copy kept here,
	// whole, copied into the memory at Present. Begin returns the copy to write into; End marks it to be copied
	void* BeginUniformWrite(VkDeviceMemory memory, VkDeviceSize size);
	void EndUniformWrite(VkDeviceMemory memory);
	// Bytes for host-visible memory, mapped and written at Present (at once with one frame in flight)
	void WriteMemory(VkDeviceMemory memory, VkDeviceSize offset, const void* data, size_t size);
	// Bytes for memory the caller keeps mapped (destination: inside memory's mapping), written at Present
	void WriteMapped(VkDeviceMemory memory, void* destination, const void* data, size_t size);
	// The memory is freed (vkFreeMemory, through the shim): its copy and pending writes go
	void ForgetMemory(VkDeviceMemory memory);
	// Waits for the GPU, then does the kept writes. At Present, and before a submit outside the frame (an upload, a
	// compute pass) that may read them
	void FlushWrites();

	// Descriptor set updates so far (each vkUpdateDescriptorSets, through the shim): a copy of a set made when the count
	// was the same is still up to date
	void CountDescriptorUpdate();
	uint64_t GetDescriptorUpdateCount();

	struct Stats
	{
		uint64_t Frames = 0;          // frames presented
		uint64_t BlockingWaits = 0;   // WaitForGpu calls that had to wait (the GPU was still busy)
		double BlockedMs = 0.0;       // the time they waited
		uint64_t DeferredBytes = 0;   // bytes written at Present
	};
	Stats GetStats();
	void CountFrame();
}
