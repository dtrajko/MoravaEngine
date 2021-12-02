#include "ShaderH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Platform/OpenGL/OpenGLShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Platform/DX11/DX11Shader.h"


namespace H2M
{

	std::vector<RefH2M<ShaderH2M>> ShaderH2M::s_AllShaders;

	RefH2M<ShaderH2M> ShaderH2M::Create(const std::string& filepath, bool forceCompile)
	{
		Log::GetLogger()->info("ShaderH2M::Create('{0}')", filepath.c_str());

		RefH2M<ShaderH2M> result = RefH2M<ShaderH2M>();

		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None: return RefH2M<ShaderH2M>();
			case H2M::RendererAPITypeH2M::OpenGL:
				result = RefH2M<H2M::OpenGLShader>::Create(filepath, forceCompile);
				break;
			case H2M::RendererAPITypeH2M::Vulkan:
				result = RefH2M<H2M::VulkanShaderH2M>::Create(filepath, forceCompile);
				break;
			case H2M::RendererAPITypeH2M::DX11:
				result = RefH2M<DX11Shader>::Create(filepath, forceCompile);
				break;
		}
		s_AllShaders.push_back(result);
		return result;
	}

	RefH2M<ShaderH2M> ShaderH2M::CreateFromString(const std::string& source)
	{
		Log::GetLogger()->info("ShaderH2M::CreateFromString('{0}')", source.c_str());

		RefH2M<ShaderH2M> result = RefH2M<ShaderH2M>();

		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return RefH2M<ShaderH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: result = H2M::OpenGLShader::CreateFromString(source);
			case H2M::RendererAPITypeH2M::Vulkan: result = H2M::VulkanShader::CreateFromString(source);
			case H2M::RendererAPITypeH2M::DX11:   result = DX11Shader::CreateFromString(source);
		}
		s_AllShaders.push_back(result);
		return result;
	}

	ShaderLibraryH2M::ShaderLibraryH2M()
	{
	}

	ShaderLibraryH2M::~ShaderLibraryH2M()
	{
	}

	void ShaderLibraryH2M::Add(const RefH2M<ShaderH2M>& shader)
	{
		auto& name = shader->GetName();
		H2M_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;

		// m_Shaders.insert(std::make_pair(name, shader));
	}

	void ShaderLibraryH2M::Load(const std::string& path, bool forceCompile)
	{
		Log::GetLogger()->info("ShaderLibraryH2M::Load(path: '{0}')", path);

		auto shader = ShaderH2M::Create(path, forceCompile);
		auto& name = shader->GetName();
		// H2M_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());

		if (m_Shaders.find(name) != m_Shaders.end())
		{
			Log::GetLogger()->warn("ShaderLibraryH2M::Load: the shader (path: '{0}', name: '{1}') already exists in ShaderLibraryH2M!", path, name);
			return;
		}

		m_Shaders[name] = shader;

		// m_Shaders.insert(std::make_pair(name, shader));
	}

	void ShaderLibraryH2M::Load(const std::string& name, const std::string& path)
	{
		Log::GetLogger()->info("ShaderLibraryH2M::Load(name: '{0}', path: '{1}')", name, path);

		H2M_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = ShaderH2M::Create(path, true);

		// m_Shaders.insert(std::make_pair(name, shader));
	}

	RefH2M<ShaderH2M> ShaderLibraryH2M::Get(const std::string& name)
	{
		// H2M_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		if (m_Shaders.find(name) == m_Shaders.end())
		{
			Log::GetLogger()->error("ShaderLibraryH2M::Get - shader '{0}' not found in ShaderLibrary!", name);
			return RefH2M<ShaderH2M>();
		}
		return m_Shaders.at(name);
	}

	// ---------------------------------------------------------------

	bool ShaderH2M::HasVSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("ShaderH2M::HasVSMaterialUniformBuffer - Method not implemented!");
		return false;
	}

	bool ShaderH2M::HasPSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("ShaderH2M::HasPSMaterialUniformBuffer - Method not implemented!");
		return false;
	}

	H2M::Buffer ShaderH2M::GetVSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("ShaderH2M::GetVSMaterialUniformBuffer - Method not implemented!");
		return H2M::Buffer();
	}

	H2M::Buffer ShaderH2M::GetPSMaterialUniformBuffer()
	{
		Log::GetLogger()->warn("ShaderH2M::GetPSMaterialUniformBuffer - Method not implemented!");
		return H2M::Buffer();
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
