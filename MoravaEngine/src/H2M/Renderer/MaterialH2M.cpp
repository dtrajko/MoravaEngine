#include "MaterialH2M.h"

#include "H2M/Renderer/RendererAPI_H2M.h"

#include "H2M/Platform/OpenGL/OpenGLMaterialH2M.h"
#include "H2M/Platform/Vulkan/VulkanMaterialH2M.h"
#include "Platform/DX11/DX11Material.h"


namespace H2M {

	MaterialH2M::MaterialH2M()
	{
	}

	MaterialH2M::MaterialH2M(const RefH2M<ShaderH2M>& shader, const std::string& name)
		: m_Shader(shader), m_Name(name)
	{
		// Create(shader, name);
	}

	RefH2M<MaterialH2M> MaterialH2M::Create(const RefH2M<ShaderH2M>& shader, const std::string& name)
	{
		switch (RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return RefH2M<MaterialH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLMaterialH2M>::Create(shader, name);
			case H2M::RendererAPITypeH2M::Vulkan: return RefH2M<VulkanMaterialH2M>::Create(shader, name);
			case H2M::RendererAPITypeH2M::DX11:   return RefH2M<DX11Material>::Create(shader, name);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<MaterialH2M>();
	}

	MaterialH2M::~MaterialH2M()
	{
		Log::GetLogger()->debug("Destroy MaterialH2M!");
	}

	void MaterialH2M::AllocateStorage()
	{
		Log::GetLogger()->error("AllocateStorage() not implemented in base class HazelMaterial!");

		//	const auto& shaderBuffers = m_Shader->GetShaderBuffers();
		//	
		//	H2M_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");
		//	
		//	if (shaderBuffers.size() > 0)
		//	{
		//		const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
		//		m_UniformStorageBuffer.Allocate(buffer.Size);
		//		m_UniformStorageBuffer.ZeroInitialize();
		//	}
	}

	void MaterialH2M::Bind()
	{
		Log::GetLogger()->error("Bind() not implemented in base class HazelMaterial!");

		auto& shader = m_Shader;
		shader->Bind();
		const auto& shaderBuffers = GetShader()->GetShaderBuffers();
		H2M_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		//	if (shaderBuffers.size() > 0)
		//	{
		//		const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
		//	
		//		for (auto& [name, uniform] : buffer.Uniforms)
		//		{
		//			switch (uniform.GetType())
		//			{
		//				//	None = 0, Bool, Int, Float, Vec2, Vec3, Vec4, Mat3, Mat4
		//				case ShaderUniformType::Bool:
		//				{
		//					bool value = m_UniformStorageBuffer.Read<bool>(uniform.GetOffset());
		//					shader->SetUniform(name, value);
		//					break;
		//				}
		//				case ShaderUniformType::Int:
		//				{
		//					int value = m_UniformStorageBuffer.Read<int>(uniform.GetOffset());
		//					shader->SetUniform(name, value);
		//					break;
		//				}
		//				case ShaderUniformType::Float:
		//				{
		//					float value = m_UniformStorageBuffer.Read<float>(uniform.GetOffset());
		//					shader->SetUniform(name, value);
		//					break;
		//				}
		//				case ShaderUniformType::Vec2:
		//				{
		//					const glm::vec2& value = m_UniformStorageBuffer.Read<glm::vec2>(uniform.GetOffset());
		//					shader->SetUniform(name, value);
		//					break;
		//				}
		//				case ShaderUniformType::Vec3:
		//				{
		//					const glm::vec3& value = m_UniformStorageBuffer.Read<glm::vec3>(uniform.GetOffset());
		//					shader->SetUniform(name, value);
		//					break;
		//				}
		//				case ShaderUniformType::Vec4:
		//				{
		//					const glm::vec4& value = m_UniformStorageBuffer.Read<glm::vec4>(uniform.GetOffset());
		//					shader->SetUniform(name, value);
		//					break;
		//				}
		//				case ShaderUniformType::Mat3:
		//				{
		//					const glm::mat3& value = m_UniformStorageBuffer.Read<glm::mat3>(uniform.GetOffset());
		//					shader->SetUniform(name, value);
		//					break;
		//				}
		//				case ShaderUniformType::Mat4:
		//				{
		//					const glm::mat4& value = m_UniformStorageBuffer.Read<glm::mat4>(uniform.GetOffset());
		//					shader->SetUniform(name, value);
		//					break;
		//				}
		//			}
		//		}
		//	}

		BindTextures();
	}

	void MaterialH2M::BindTextures()
	{
		for (size_t i = 0; i < m_Textures.size(); i++)
		{
			auto& texture = m_Textures[i];
			if (texture)
			{
				texture->Bind((uint32_t)i);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// MaterialInstance
	//////////////////////////////////////////////////////////////////////////////////

	RefH2M<MaterialInstanceH2M> MaterialInstanceH2M::Create(const RefH2M<MaterialH2M>& material)
	{
		return RefH2M<MaterialInstanceH2M>::Create(material);
	}

	MaterialInstanceH2M::MaterialInstanceH2M(const RefH2M<MaterialH2M>& material, const std::string& name)
		: m_Material(material), m_Name(name)
	{
		m_Material->m_MaterialInstances.insert(this);
		AllocateStorage();
	}

	MaterialInstanceH2M::~MaterialInstanceH2M()
	{
		m_Material->m_MaterialInstances.erase(this);
	}

	void MaterialInstanceH2M::AllocateStorage()
	{
		const auto& shaderBuffers = GetShader()->GetShaderBuffers();

		H2M_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
			m_UniformStorageBuffer.Allocate(buffer.Size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void MaterialInstanceH2M::SetFlag(HazelMaterialFlag flag, bool value)
	{
	}

	void MaterialInstanceH2M::Bind()
	{
		auto& shader = m_Material->m_Shader;
		shader->Bind();

		const auto& shaderBuffers = GetShader()->GetShaderBuffers();
		H2M_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;

			for (auto& [name, uniform] : buffer.Uniforms)
			{
				switch (uniform.GetType())
				{
					//	None = 0, Bool, Int, Float, Vec2, Vec3, Vec4, Mat3, Mat4
				case ShaderUniformType::Bool:
				{
					bool value = m_UniformStorageBuffer.Read<bool>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Int:
				{
					int value = m_UniformStorageBuffer.Read<int>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Float:
				{
					float value = m_UniformStorageBuffer.Read<float>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Vec2:
				{
					const glm::vec2& value = m_UniformStorageBuffer.Read<glm::vec2>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Vec3:
				{
					const glm::vec3& value = m_UniformStorageBuffer.Read<glm::vec3>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Vec4:
				{
					const glm::vec4& value = m_UniformStorageBuffer.Read<glm::vec4>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Mat3:
				{
					const glm::mat3& value = m_UniformStorageBuffer.Read<glm::mat3>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Mat4:
				{
					const glm::mat4& value = m_UniformStorageBuffer.Read<glm::mat4>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				}
			}
		}

		m_Material->BindTextures();
		for (size_t i = 0; i < m_Textures.size(); i++)
		{
			auto& texture = m_Textures[i];
			if (texture)
			{
				texture->Bind((uint32_t)i);
			}
		}
	}
}
