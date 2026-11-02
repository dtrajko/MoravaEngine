/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLRenderPassH2M.h"

#include "H2M/Renderer/FramebufferH2M.h"


namespace H2M
{

	OpenGLRenderPassH2M::OpenGLRenderPassH2M(const RenderPassSpecificationH2M& spec)
		: m_Specification(spec) {}

	OpenGLRenderPassH2M::~OpenGLRenderPassH2M() {}

}
