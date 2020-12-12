#include "HazelCamera.h"

#include <glm/gtc/matrix_transform.hpp>


namespace Hazel {

	void HazelCamera::RecalculateProjection()
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
