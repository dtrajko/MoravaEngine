#include "ShaderHazelLegacy.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/OpenGL/OpenGLShader.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Platform/DX11/DX11Shader.h"


namespace HazelLegacy {

	std::vector<Hazel::Ref<ShaderHazelLegacy>> ShaderHazelLegacy::s_AllShaders;

	Hazel::Ref<ShaderHazelLegacy> ShaderHazelLegacy::Create(const std::string& filepath, bool forceCompile)
	{
		Log::GetLogger()->info("ShaderHazelLegacy::Create('{0}')", filepath.c_str());

		Hazel::Ref<ShaderHazelLegacy> result = Hazel::Ref<ShaderHazelLegacy>();

		switch (Hazel::RendererAPI::Current())
		{
			case Hazel::RendererAPIType::None: return Hazel::Ref<ShaderHazelLegacy>();
			case Hazel::RendererAPIType::OpenGL:
				result = Hazel::Ref<Hazel::OpenGLShader>::Create(filepath, forceCompile);
				break;
			case Hazel::RendererAPIType::Vulkan:
				result = Hazel::Ref<Hazel::VulkanShader>::Create(filepath, forceCompile);
				break;
			case Hazel::RendererAPIType::DX11:
				result = Hazel::Ref<DX11Shader>::Create(filepath, forceCompile);
				break;
		}
		s_AllShaders.push_back(result);
		return result;
	}

	Hazel::Ref<ShaderHazelLegacy> ShaderHazelLegacy::CreateFromString(const std::string& source)
	{
		Log::GetLogger()->info("ShaderHazelLegacy::CreateFromString('{0}')", source.c_str());

		Hazel::Ref<ShaderHazelLegacy> result = Hazel::Ref<ShaderHazelLegacy>();

		switch (Hazel::RendererAPI::Current())
		{
			case Hazel::RendererAPIType::None:   return Hazel::Ref<ShaderHazelLegacy>();
			case Hazel::RendererAPIType::OpenGL: result = Hazel::OpenGLShader::CreateFromString(source);
			case Hazel::RendererAPIType::Vulkan: result = Hazel::VulkanShader::CreateFromString(source);
			case Hazel::RendererAPIType::DX11:   result = DX11Shader::CreateFromString(source);
		}
		s_AllShaders.push_back(result);
		return result;
	}

	ShaderLibraryHazelLegacy::ShaderLibraryHazelLegacy()
	{
	}

	ShaderLibraryHazelLegacy::~ShaderLibraryHazelLegacy()
	{
	}

	void ShaderLibraryHazelLegacy::Add(const Hazel::Ref<ShaderHazelLegacy>& shader)
	{
		auto& name = shader->GetName();
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;

		// m_Shaders.insert(std::make_pair(name, shader));
	}

	void ShaderLibraryHazelLegacy::Load(const std::string& path, bool forceCompile)
	{
		Log::GetLogger()->info("ShaderLibraryHazelLegacy::Load(path: '{0}')", path);

		auto shader = ShaderHazelLegacy::Create(path, forceCompile);
		auto& name = shader->GetName();
		// HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());

		if (m_Shaders.find(name) != m_Shaders.end())
		{
			Log::GetLogger()->warn("ShaderLibraryHazelLegacy::Load: the shader (path: '{0}', name: '{1}') already exists in ShaderLibraryHazelLegacy!", path, name);
			return;
		}

		m_Shaders[name] = shader;

		// m_Shaders.insert(std::make_pair(name, shader));
	}

	void ShaderLibraryHazelLegacy::Load(const std::string& name, const std::string& path)
	{
		Log::GetLogger()->info("ShaderLibraryHazelLegacy::Load(name: '{0}', path: '{1}')", name, path);

		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = ShaderHazelLegacy::Create(path, true);

		// m_Shaders.insert(std::make_pair(name, shader));
	}

	Hazel::Ref<ShaderHazelLegacy> ShaderLibraryHazelLegacy::Get(const std::string& name)
	{
		// HZ_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		if (m_Shaders.find(name) == m_Shaders.end())
		{
			Log::GetLogger()->error("ShaderLibraryHazelLegacy::Get - shader '{0}' not found in ShaderLibrary!", name);
			return Hazel::Ref<ShaderHazelLegacy>();
		}
		return m_Shaders.at(name);
	}

	// ---------------------------------------------------------------

	bool ShaderHazelLegacy::HasVSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("ShaderHazelLegacy::HasVSMaterialUniformBuffer - Method not implemented!");
		return false;
	}

	bool ShaderHazelLegacy::HasPSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("ShaderHazelLegacy::HasPSMaterialUniformBuffer - Method not implemented!");
		return false;
	}

	Hazel::Buffer ShaderHazelLegacy::GetVSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("ShaderHazelLegacy::GetVSMaterialUniformBuffer - Method not implemented!");
		return Hazel::Buffer();
	}

	Hazel::Buffer ShaderHazelLegacy::GetPSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("ShaderHazelLegacy::GetPSMaterialUniformBuffer - Method not implemented!");
		return Hazel::Buffer();
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
