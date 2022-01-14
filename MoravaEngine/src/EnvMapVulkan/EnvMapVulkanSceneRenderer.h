#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Scene/SceneH2M.h"


struct EnvMapVulkanSceneRendererSpecification
{
	bool SwapChainTarget = false;
};

class EnvMapVulkanSceneRenderer : public H2M::RefCountedH2M
{
public:
	EnvMapVulkanSceneRenderer(H2M::RefH2M<H2M::SceneH2M> scene, EnvMapVulkanSceneRendererSpecification specification = EnvMapVulkanSceneRendererSpecification());

	void Init();

	// TODO Implement all SceneRenderer methods

	void SetLineWidth(float width);


private:
	float m_LineWidth = 2.0f;

	H2M::RefH2M<H2M::PipelineH2M> m_GeometryWireframePipeline;

};
