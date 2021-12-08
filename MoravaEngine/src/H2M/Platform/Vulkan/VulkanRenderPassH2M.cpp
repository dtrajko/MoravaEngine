/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanRenderPassH2M.h"

#include "H2M/Renderer/FramebufferH2M.h"


namespace H2M
{

	VulkanRenderPassH2M::VulkanRenderPassH2M(const RenderPassSpecificationH2M& spec)
		: m_Specification(spec) {}

	VulkanRenderPassH2M::~VulkanRenderPassH2M() {}

}
