#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLShader.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"


namespace Hazel {

	std::vector<Ref<HazelShader>> HazelShader::s_AllShaders;

	Ref<HazelShader> HazelShader::Create(const std::string& filepath, bool forceCompile)
	{
		Log::GetLogger()->info("HazelShader::Create('{0}')", filepath.c_str());

		Ref<HazelShader> result = nullptr;

		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL:
				result = Ref<OpenGLShader>::Create(filepath, forceCompile);
				break;
			case RendererAPIType::Vulkan:
				result = Ref<VulkanShader>::Create(filepath, forceCompile);
				break;
		}
		s_AllShaders.push_back(result);
		return result;
	}

	Ref<HazelShader> HazelShader::CreateFromString(const std::string& source)
	{
		Log::GetLogger()->info("HazelShader::CreateFromString('{0}')", source.c_str());

		Ref<HazelShader> result = nullptr;

		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: result = OpenGLShader::CreateFromString(source);
		}
		s_AllShaders.push_back(result);
		return result;
	}

	bool HazelShader::HasVSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("HazelShader::HasVSMaterialUniformBuffer - Method not implemented!");
		return false;
	}

	bool HazelShader::HasPSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("HazelShader::HasPSMaterialUniformBuffer - Method not implemented!");
		return false;
	}

	Buffer HazelShader::GetVSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("HazelShader::GetVSMaterialUniformBuffer - Method not implemented!");
		return Buffer();
	}

	Buffer HazelShader::GetPSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("HazelShader::GetPSMaterialUniformBuffer - Method not implemented!");
		return Buffer();
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

	void HazelShaderLibrary::Load(const std::string& path, bool forceCompile)
	{
		Log::GetLogger()->info("HazelShaderLibrary::Load(path: '{0}')", path);

		auto shader = HazelShader::Create(path, forceCompile);
		auto& name = shader->GetName();
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void HazelShaderLibrary::Load(const std::string& name, const std::string& path)
	{
		Log::GetLogger()->info("HazelShaderLibrary::Load(name: '{0}', path: '{1}')", name, path);

		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = Ref<HazelShader>(HazelShader::Create(path));
	}

	Ref<HazelShader>& HazelShaderLibrary::Get(const std::string& name)
	{
		HZ_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		return m_Shaders.at(name);
	}

	ShaderUniform::ShaderUniform(const std::string& name, ShaderUniformType type, uint32_t size, uint32_t offset)
		: m_Name(name), m_Type(type), m_Size(size), m_Offset(offset)
	{
	}

	const std::string& ShaderUniform::UniformTypeToString(ShaderUniformType type)
	{
		if (type == ShaderUniformType::Bool)
		{
			return "Boolean";
		}
		else if (type == ShaderUniformType::Int)
		{
			return "Int";
		}
		else if (type == ShaderUniformType::Float)
		{
			return "Float";
		}

		return "None";
	}

}
