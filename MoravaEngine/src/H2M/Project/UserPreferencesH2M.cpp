/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "UserPreferencesH2M.h"

#include "Core/Log.h"


namespace H2M
{

	UserPreferencesSerializerH2M::UserPreferencesSerializerH2M(const RefH2M<UserPreferencesH2M>& preferences)
	{
		Log::GetLogger()->warn("UserPreferencesSerializerH2M::UserPreferencesSerializer: Method not yet implemented!");
	}

	UserPreferencesSerializerH2M::~UserPreferencesSerializerH2M()
	{
		Log::GetLogger()->warn("UserPreferencesSerializerH2M::~UserPreferencesSerializer: Method not yet implemented!");
	}

	void UserPreferencesSerializerH2M::Serialize(const std::filesystem::path& filepath)
	{
		Log::GetLogger()->warn("UserPreferencesSerializerH2M::Serialize: Method not yet implemented!");
	}

	void UserPreferencesSerializerH2M::Deserialize(const std::filesystem::path& filepath)
	{
		Log::GetLogger()->warn("UserPreferencesSerializerH2M::Deserialize: Method not yet implemented!");
	}

}
