/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

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
