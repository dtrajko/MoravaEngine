#include "EnvMapVulkanTextureTable.h"

#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"

#include "Core/Log.h"

#include <algorithm>


void EnvMapVulkanTextureTable::BeginUse()
{
	for (Slot& slot : m_Slots)
	{
		slot.Used = false;
	}
}

uint32_t EnvMapVulkanTextureTable::GetSlot(const H2M::RefH2M<H2M::Texture2D_H2M>& textureRef)
{
	H2M::RefH2M<H2M::Texture2D_H2M> texture = textureRef;
	auto found = m_Index.find(texture.Raw());
	if (found != m_Index.end())
	{
		m_Slots[found->second].Used = true;
		return found->second;
	}

	uint32_t index;
	if (!m_FreeSlots.empty())
	{
		index = m_FreeSlots.back();
		m_FreeSlots.pop_back();
	}
	else if (m_Slots.size() < Capacity)
	{
		index = (uint32_t)m_Slots.size();
		m_Slots.emplace_back();
	}
	else
	{
		static bool s_Warned = false;
		if (!s_Warned)
		{
			Log::GetLogger()->warn("The array of all textures is full ({0}): ray hits show the other materials' values instead of their maps", Capacity);
			s_Warned = true;
		}
		return NoSlot;
	}
	m_Slots[index].Texture = texture;
	m_Slots[index].Used = true;
	m_Index[texture.Raw()] = index;
	m_Writes.push_back({ index, texture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo() });
	return index;
}

void EnvMapVulkanTextureTable::EndUse()
{
	for (uint32_t index = 0; index < (uint32_t)m_Slots.size(); index++)
	{
		Slot& slot = m_Slots[index];
		if (!slot.Used && slot.Texture)
		{
			// No material's entry has this element any more, so no ray hit reads it: the descriptor may stay as it is
			m_Index.erase(slot.Texture.Raw());
			slot.Texture = H2M::RefH2M<H2M::Texture2D_H2M>();
			m_FreeSlots.push_back(index);
			// A write still waiting for this element would write an image that may be gone
			m_Writes.erase(std::remove_if(m_Writes.begin(), m_Writes.end(), [index](const Write& write) { return write.Slot == index; }), m_Writes.end());
		}
	}
}

std::vector<EnvMapVulkanTextureTable::Write> EnvMapVulkanTextureTable::TakeWrites()
{
	std::vector<Write> writes = std::move(m_Writes);
	m_Writes.clear();
	return writes;
}

void EnvMapVulkanTextureTable::Clear()
{
	m_Slots.clear();
	m_FreeSlots.clear();
	m_Index.clear();
	m_Writes.clear();
}
