#include "EnvMapVulkanSceneRenderer.h"


EnvMapVulkanSceneRenderer::EnvMapVulkanSceneRenderer(H2M::RefH2M<H2M::SceneH2M> scene, EnvMapVulkanSceneRendererSpecification specification)
{
}

void EnvMapVulkanSceneRenderer::Init()
{
}

void EnvMapVulkanSceneRenderer::SetLineWidth(float width)
{
	m_LineWidth = width;
	if (m_GeometryWireframePipeline)
	{
		m_GeometryWireframePipeline->GetSpecification().LineWidth = width;
	}
}
