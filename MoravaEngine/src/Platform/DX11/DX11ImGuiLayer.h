#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/ImGui/ImGuiLayer.h"


namespace Hazel {

	class VulkanImGuiLayer : public ImGuiLayer
	{
	public:
		VulkanImGuiLayer();
		VulkanImGuiLayer(const std::string& name);
		~VulkanImGuiLayer();

		virtual void Begin() override;
		virtual void End() override;

		virtual void OnAttach();
		virtual void OnDetach();
		virtual void OnImGuiRender();

	private:
		float m_Time = 0.0f;

		VkCommandBuffer m_CommandBuffer;
		VkDescriptorPool m_DescriptorPool;

	};

}
