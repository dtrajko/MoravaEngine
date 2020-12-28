#include "ShaderLibrary.h"


std::unordered_map<std::string, Ref<Shader>> ShaderLibrary::s_Shaders;


void ShaderLibrary::Add(Ref<Shader>& shader)
{
	auto name = shader->GetName();
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void ShaderLibrary::Load(const std::string& name, const std::string& vertexLocation, const std::string& fragmentLocation)
{
	auto shader = CreateRef<Shader>(vertexLocation.c_str(), fragmentLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void ShaderLibrary::Load(const std::string& name, const std::string& vertexLocation, const std::string& geometryLocation, const std::string& fragmentLocation)
{
	auto shader = CreateRef<Shader>(vertexLocation.c_str(), geometryLocation.c_str(), fragmentLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void ShaderLibrary::Load(const std::string& name, const std::string& computeLocation)
{
	auto shader = CreateRef<Shader>(computeLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

Ref<Shader> ShaderLibrary::Get(const std::string& name)
{
	if (s_Shaders.find(name) != s_Shaders.end()) {
		return s_Shaders[name];
	}
	return CreateRef<Shader>();
}
