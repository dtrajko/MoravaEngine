/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "ProjectH2M.h"

#include "H2M/Asset/AssetManagerH2M.h"
#include "H2M/Physics/3D/PhysicsSystemH2M.h"


namespace H2M
{

	ProjectH2M::ProjectH2M()
	{

	}

	ProjectH2M::~ProjectH2M()
	{

	}

	void ProjectH2M::SetActive(RefH2M<ProjectH2M> project)
	{
		if (s_ActiveProject)
		{
			// AssetManagerH2M::Shutdown();
			// PhysicsSystemH2M::Shutdown();
		}

		s_ActiveProject = project;
		if (s_ActiveProject)
		{
			// AssetManagerH2M::Init();
			// PhysicsSystemH2M::Init();
			// AudioCommandRegistryH2M::Init();
		}
	}

}
