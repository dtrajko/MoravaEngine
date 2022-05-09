/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "ProjectH2M.h"

#include <string>


namespace H2M
{

	class ProjectSerializerH2M
	{
	public:
		ProjectSerializerH2M(RefH2M<ProjectH2M> project);

		void Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

	private:
		RefH2M<ProjectH2M> m_Project;
	};

}
