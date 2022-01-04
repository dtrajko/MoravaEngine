/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/ImGui/ImGuiLayerH2M.h"


class EnvMapVulkanImGuiLayer : public H2M::ImGuiLayerH2M
{
public:
	EnvMapVulkanImGuiLayer();
	EnvMapVulkanImGuiLayer(const std::string& name);
	~EnvMapVulkanImGuiLayer();

	virtual void Begin() override;
	virtual void End() override;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnUpdate(H2M::TimestepH2M ts) override;
	virtual void OnEvent(H2M::EventH2M& event) override;
	virtual void OnRender() override;
	virtual void OnImGuiRender() override;

private:
	float m_Time = 0.0f;

	VkCommandBuffer m_CommandBuffer;
	VkDescriptorPool m_DescriptorPool;

};
