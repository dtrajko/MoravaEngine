#include "HazelMaterial.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLMaterial.h"
#include "Hazel/Platform/Vulkan/VulkanMaterial.h"
#include "Platform/DX11/DX11Material.h"


namespace Hazel {

	HazelMaterial::HazelMaterial()
	{
	}

	HazelMaterial::HazelMaterial(const Ref<HazelShader>& shader, const std::string& name)
	{
		// Create(shader, name);
	}

	Ref<HazelMaterial> HazelMaterial::Create(const Ref<HazelShader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:   return Ref<HazelMaterial>();
			case RendererAPIType::OpenGL: return Ref<OpenGLMaterial>::Create(shader, name);
			case RendererAPIType::Vulkan: return Ref<VulkanMaterial>::Create(shader, name);
			case RendererAPIType::DX11:   return Ref<DX11Material>::Create(shader, name);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelMaterial>();
	}

	void HazelMaterial::AllocateStorage()
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
			m_UniformStorageBuffer.Allocate(buffer.Size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void HazelMaterial::Bind()
	{
		auto& shader = m_Shader;
		shader->Bind();
		const auto& shaderBuffers = GetShader()->GetShaderBuffers();
		HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

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
		BindTextures();
	}

	void HazelMaterial::BindTextures()
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

	Ref<HazelMaterialInstance> HazelMaterialInstance::Create(const Ref<HazelMaterial>& material)
	{
		return Ref<HazelMaterialInstance>::Create(material);
	}

	HazelMaterialInstance::HazelMaterialInstance(const Ref<HazelMaterial>& material, const std::string& name)
		: m_Material(material), m_Name(name)
	{
		m_Material->m_MaterialInstances.insert(this);
		AllocateStorage();
	}

	HazelMaterialInstance::~HazelMaterialInstance()
	{
		m_Material->m_MaterialInstances.erase(this);
	}

	void HazelMaterialInstance::AllocateStorage()
	{
		const auto& shaderBuffers = GetShader()->GetShaderBuffers();

		HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
			m_UniformStorageBuffer.Allocate(buffer.Size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void HazelMaterialInstance::SetFlag(HazelMaterialFlag flag, bool value)
	{
	}

	void HazelMaterialInstance::Bind()
	{
		auto& shader = m_Material->m_Shader;
		shader->Bind();

		const auto& shaderBuffers = GetShader()->GetShaderBuffers();
		HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

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
