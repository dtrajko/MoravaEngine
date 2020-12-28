#include "HazelShader.h"
#include "../Core/Assert.h"

#include "../Platform/OpenGL/OpenGLShader.h"


namespace Hazel {

	std::vector<Ref<HazelShader>> HazelShader::s_AllShaders;

	Ref<HazelShader> HazelShader::Create(const std::string& filepath)
	{
		Ref<HazelShader> result = nullptr;

		result = Ref<OpenGLShader>::Create(filepath);

		s_AllShaders.push_back(result);
		return result;
	}

	Ref<HazelShader> HazelShader::CreateFromString(const std::string& source)
	{
		Ref<HazelShader> result = nullptr;

		result = OpenGLShader::CreateFromString(source);

		s_AllShaders.push_back(result);
		return result;
	}

	HazelShaderLibrary::HazelShaderLibrary()
	{
	}

	HazelShaderLibrary::~HazelShaderLibrary()
	{
	}

	void HazelShaderLibrary::Add(const Ref<HazelShader>& shader)
	{
		auto& name = shader->GetName();
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void HazelShaderLibrary::Load(const std::string& path)
	{
		auto shader = Ref<HazelShader>(HazelShader::Create(path));
		auto& name = shader->GetName();
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void HazelShaderLibrary::Load(const std::string& name, const std::string& path)
	{
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = Ref<HazelShader>(HazelShader::Create(path));
	}

	Ref<HazelShader>& HazelShaderLibrary::Get(const std::string& name)
	{
		HZ_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		return m_Shaders[name];
	}

}
