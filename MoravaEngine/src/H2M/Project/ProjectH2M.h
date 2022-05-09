/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Core/AssertH2M.h"

#include <filesystem>


namespace H2M
{

	struct ProjectConfigH2M
	{
		std::string Name;

		std::string AssetDirectory;

		std::string StartScene;

		std::string ProjectDirectory;
	};

	class ProjectH2M : public RefCountedH2M
	{
	public:
		ProjectH2M();
		~ProjectH2M();

		const ProjectConfigH2M& GetConfig() const { return m_Config; }

		static RefH2M<ProjectH2M> GetActive() { return s_ActiveProject; }
		static void SetActive(RefH2M<ProjectH2M> project);

		static std::filesystem::path GetAssetDirectory()
		{
			H2M_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().AssetDirectory;
		}

	private:
		ProjectConfigH2M m_Config;

		inline static RefH2M<ProjectH2M> s_ActiveProject;

	};

}
