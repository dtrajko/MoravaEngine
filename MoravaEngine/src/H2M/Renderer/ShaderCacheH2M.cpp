#include "ShaderCacheH2M.h"

#include "H2M/Core/HashH2M.h"

#include "Core/Log.h"

#include "yaml-cpp/yaml.h"

#include <fstream>


namespace H2M
{

	static const char* s_ShaderRegistryPath = "Resources/Cache/Shader/ShaderRegistry.cache";

	bool ShaderCacheH2M::HasChanged(const std::filesystem::path& shader, const std::string& source)
	{
		std::map<std::string, uint32_t> shaderCache;
		Deserialize(shaderCache);

		uint32_t hash = HashH2M::GenerateFNVHash(source.c_str());
		if (shaderCache.find(shader.string()) == shaderCache.end() || shaderCache.at(shader.string()) != hash)
		{
			shaderCache[shader.string()] = hash;
			Serialize(shaderCache);
			return true;
		}

		return false;
	}

	void ShaderCacheH2M::Serialize(const std::map<std::string, uint32_t>& shaderCache)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "ShaderRegistry" << YAML::BeginMap;

		for (auto& [filepath, hash] : shaderCache)
			out << YAML::Key << filepath << YAML::Value << hash;

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(s_ShaderRegistryPath);
		fout << out.c_str();
	}

	void ShaderCacheH2M::Deserialize(std::map<std::string, uint32_t>& shaderCache)
	{
		// Read registry
		std::ifstream stream(s_ShaderRegistryPath);
		if (!stream.good())
			return;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		auto handles = data["ShaderRegistry"];
		if (!handles)
		{
			MORAVA_CORE_ERROR("[ShaderCache] Shader Registry is invalid.");
			return;
		}

		for (auto entry : handles)
		{
			std::string path = entry.first.as<std::string>();
			uint32_t hash = entry.second.as<uint32_t>();
			shaderCache[path] = hash;
		}

	}

}
