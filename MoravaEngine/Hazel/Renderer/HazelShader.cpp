#include "HazelShader.h"
#include "../Core/Assert.h"

#include "../Platform/OpenGL/OpenGLShader.h"


namespace Hazel {

	std::vector<HazelShader*> HazelShader::s_AllShaders;

	HazelShader* HazelShader::Create(const std::string& filepath)
	{
		HazelShader* result = nullptr;

		result = new OpenGLShader(filepath);

		s_AllShaders.push_back(result);
		return result;
	}

	HazelShader* HazelShader::CreateFromString(const std::string& source)
	{
		HazelShader* result = nullptr;

		result = OpenGLShader::CreateFromString(source);

		s_AllShaders.push_back(result);
		return result;
	}

	ShaderLibrary::ShaderLibrary()
	{
	}

	ShaderLibrary::~ShaderLibrary()
	{
	}

	void ShaderLibrary::Add(const Ref<HazelShader>& shader)
	{
		auto& name = shader->GetName();
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(const std::string& path)
	{
		auto shader = Ref<HazelShader>(HazelShader::Create(path));
		auto& name = shader->GetName();
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(const std::string& name, const std::string& path)
	{
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = Ref<HazelShader>(HazelShader::Create(path));
	}

	Ref<HazelShader>& ShaderLibrary::Get(const std::string& name)
	{
		HZ_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		return m_Shaders[name];
	}

}
