#include "ShaderLibrary.h"


std::unordered_map<std::string, Hazel::Ref<Shader>> ShaderLibrary::s_Shaders;


void ShaderLibrary::Add(Hazel::Ref<Shader>& shader)
{
	auto name = shader->GetName();
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void ShaderLibrary::Load(const std::string& name, const std::string& vertexLocation, const std::string& fragmentLocation)
{
	auto shader = Hazel::Ref<Shader>::Create(vertexLocation.c_str(), fragmentLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void ShaderLibrary::Load(const std::string& name, const std::string& vertexLocation, const std::string& geometryLocation, const std::string& fragmentLocation)
{
	auto shader = Hazel::Ref<Shader>::Create(vertexLocation.c_str(), geometryLocation.c_str(), fragmentLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

void ShaderLibrary::Load(const std::string& name, const std::string& computeLocation)
{
	auto shader = Hazel::Ref<Shader>::Create(computeLocation.c_str());
	if (s_Shaders.find(name) == s_Shaders.end()) {
		s_Shaders[name] = shader;
	}
}

const Hazel::Ref<Shader>& ShaderLibrary::Get(const std::string& name)
{
	if (s_Shaders.find(name) != s_Shaders.end()) {
		return s_Shaders[name];
	}
	return Hazel::Ref<Shader>();
}
