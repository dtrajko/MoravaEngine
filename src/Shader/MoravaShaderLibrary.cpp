#include "Shader/MoravaShaderLibrary.h"


std::unordered_map<std::string, H2M::RefH2M<MoravaShader>> MoravaShaderLibrary::s_Shaders;


void MoravaShaderLibrary::Add(H2M::RefH2M<MoravaShader>& shader)
{
	auto name = shader->GetName();
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void MoravaShaderLibrary::Load(const std::string& name, const std::string& vertexLocation, const std::string& fragmentLocation)
{
	auto shader = MoravaShader::Create(vertexLocation.c_str(), fragmentLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void MoravaShaderLibrary::Load(const std::string& name, const std::string& vertexLocation, const std::string& geometryLocation, const std::string& fragmentLocation)
{
	auto shader = MoravaShader::Create(vertexLocation.c_str(), geometryLocation.c_str(), fragmentLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void MoravaShaderLibrary::Load(const std::string& name, const std::string& computeLocation)
{
	auto shader = MoravaShader::Create(computeLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

H2M::RefH2M<MoravaShader> MoravaShaderLibrary::Get(const std::string& name)
{
	if (auto it = s_Shaders.find(name); it != s_Shaders.end())
	{
		return it->second;
	}
	return {};
}
