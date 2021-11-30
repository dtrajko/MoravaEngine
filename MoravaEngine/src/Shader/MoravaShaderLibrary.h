#pragma once

#include "H2M/Core/Base.h"

#include "Shader/MoravaShader.h"

#include <unordered_map>


class MoravaShaderLibrary
{
public:
	static void Add(H2M::Ref<MoravaShader>& shader);
	static void Load(const std::string& name, const std::string& vertexLocation, const std::string& fragmentLocation);
	static void Load(const std::string& name, const std::string& vertexLocation, const std::string& geometryLocation, const std::string& fragmentLocation);
	static void Load(const std::string& name, const std::string& computeLocation);
	static const H2M::Ref<MoravaShader>& Get(const std::string& name);

private:
	static std::unordered_map<std::string, H2M::Ref<MoravaShader>> s_Shaders;

};
