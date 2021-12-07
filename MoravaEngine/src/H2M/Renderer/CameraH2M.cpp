/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "CameraH2M.h"

#include <glm/gtc/matrix_transform.hpp>


namespace H2M
{

	void CameraH2M::RecalculateProjection()
	{
		if (m_ProjectionType == ProjectionType::Perspective)
		{
			// TODO...
		}
		else if (m_ProjectionType == ProjectionType::Orthographic)
		{
			// TODO...
		}
	}

}
