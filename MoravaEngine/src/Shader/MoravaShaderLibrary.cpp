#include "Shader/MoravaShaderLibrary.h"


std::unordered_map<std::string, H2M::Ref<MoravaShader>> MoravaShaderLibrary::s_Shaders;


void MoravaShaderLibrary::Add(H2M::Ref<MoravaShader>& shader)
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

const H2M::Ref<MoravaShader>& MoravaShaderLibrary::Get(const std::string& name)
{
	if (s_Shaders.find(name) != s_Shaders.end()) {
		return s_Shaders[name];
	}
	return H2M::Ref<MoravaShader>();
}
