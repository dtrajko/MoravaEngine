#pragma once

#include "Hazel/Core/Base.h"

#include "Shader/MoravaShader.h"

#include <unordered_map>


class MoravaShaderLibrary
{
public:
	static void Add(Hazel::Ref<MoravaShader>& shader);
	static void Load(const std::string& name, const std::string& vertexLocation, const std::string& fragmentLocation);
	static void Load(const std::string& name, const std::string& vertexLocation, const std::string& geometryLocation, const std::string& fragmentLocation);
	static void Load(const std::string& name, const std::string& computeLocation);
	static const Hazel::Ref<MoravaShader>& Get(const std::string& name);

private:
	static std::unordered_map<std::string, Hazel::Ref<MoravaShader>> s_Shaders;

};
