#include "VulkanUIImagesH2M.h"


namespace H2M
{

	static VulkanUIImagesH2M::AddFunction s_Add = nullptr;
	static VulkanUIImagesH2M::RemoveFunction s_Remove = nullptr;

	void VulkanUIImagesH2M::SetBackend(AddFunction add, RemoveFunction remove)
	{
		s_Add = add;
		s_Remove = remove;
	}

	bool VulkanUIImagesH2M::HasBackend()
	{
		return s_Add != nullptr;
	}

	uint64_t VulkanUIImagesH2M::Add(VkSampler sampler, VkImageView view, VkImageLayout layout)
	{
		return s_Add ? s_Add(sampler, view, layout) : 0;
	}

	void VulkanUIImagesH2M::Remove(uint64_t id)
	{
		if (s_Remove && id)
		{
			s_Remove(id);
		}
	}

}
