/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/RefH2M.h"

#include <string>
#include <map>
#include <filesystem>


namespace H2M
{

	struct RecentProjectH2M
	{
		std::string Name;
		std::string FilePath;
		time_t LastOpened;
	};

	struct UserPreferencesH2M : public RefCountedH2M
	{
		bool ShowWelcomeScreen = true;
		std::string StartupProject;
		std::map<time_t, RecentProjectH2M, std::greater<time_t>> RecentProjects;

		// Not Serialized
		std::string FilePath;
	};

	class UserPreferencesSerializerH2M
	{
	public:
		UserPreferencesSerializerH2M(const RefH2M<UserPreferencesH2M>& preferences);
		~UserPreferencesSerializerH2M();

		void Serialize(const std::filesystem::path& filepath);
		void Deserialize(const std::filesystem::path& filepath);

	private:
		RefH2M<UserPreferencesH2M> m_Preferences;

	};
}
