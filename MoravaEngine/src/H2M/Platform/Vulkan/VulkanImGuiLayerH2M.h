#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/ImGui/ImGuiLayerH2M.h"


namespace H2M
{

	class VulkanImGuiLayerH2M : public ImGuiLayerH2M
	{
	public:
		VulkanImGuiLayerH2M();
		VulkanImGuiLayerH2M(const std::string& name);
		~VulkanImGuiLayerH2M();

		virtual void Begin() override;
		virtual void End() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(TimestepH2M ts) override;
		virtual void OnEvent(EventH2M& event) override;
		virtual void OnRender() override;
		virtual void OnImGuiRender() override;

	private:
		float m_Time = 0.0f;

		VkCommandBuffer m_CommandBuffer;
		VkDescriptorPool m_DescriptorPool;

	};

}
