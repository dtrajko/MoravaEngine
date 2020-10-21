#pragma once

#include <glm/glm.hpp>

namespace Hazel {

	class HazelCamera
	{
	public:
		HazelCamera() = default;
		HazelCamera(const glm::mat4& projection)
			: m_Projection(projection) {}

		virtual ~HazelCamera() = default;

		const glm::mat4& GetProjection() const { return m_Projection; }

	protected:
		glm::mat4 m_Projection = glm::mat4(1.0f);

	};

}
