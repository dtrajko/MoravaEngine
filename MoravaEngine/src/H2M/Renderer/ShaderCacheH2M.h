#pragma once

#include <filesystem>
#include <map>


namespace H2M
{

	class ShaderCacheH2M
	{
	public:
		static bool HasChanged(const std::filesystem::path& shader, const std::string& source);
	private:
		static void Serialize(const std::map<std::string, uint32_t>& shaderCache);
		static void Deserialize(std::map<std::string, uint32_t>& shaderCache);
	};

}
