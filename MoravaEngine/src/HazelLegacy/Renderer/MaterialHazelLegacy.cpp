#include "MaterialHazelLegacy.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLMaterial.h"
#include "Hazel/Platform/Vulkan/VulkanMaterial.h"
#include "Platform/DX11/DX11Material.h"


namespace HazelLegacy {

	MaterialHazelLegacy::MaterialHazelLegacy()
	{
	}

	MaterialHazelLegacy::MaterialHazelLegacy(const Hazel::Ref<ShaderHazelLegacy>& shader, const std::string& name)
		: m_Shader(shader), m_Name(name)
	{
		// Create(shader, name);
	}

	Hazel::Ref<MaterialHazelLegacy> MaterialHazelLegacy::Create(const Hazel::Ref<ShaderHazelLegacy>& shader, const std::string& name)
	{
		switch (Hazel::RendererAPI::Current())
		{
			case Hazel::RendererAPIType::None:   return Hazel::Ref<MaterialHazelLegacy>();
			case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<Hazel::OpenGLMaterial>::Create(shader, name);
			case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<Hazel::VulkanMaterial>::Create(shader, name);
			case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11Material>::Create(shader, name);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Hazel::Ref<MaterialHazelLegacy>();
	}

	MaterialHazelLegacy::~MaterialHazelLegacy()
	{
		Log::GetLogger()->debug("Destroy MaterialHazelLegacy!");
	}

	void MaterialHazelLegacy::AllocateStorage()
	{
		Log::GetLogger()->error("AllocateStorage() not implemented in base class HazelMaterial!");

		//	const auto& shaderBuffers = m_Shader->GetShaderBuffers();
		//	
		//	HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");
		//	
		//	if (shaderBuffers.size() > 0)
		//	{
		//		const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
		//		m_UniformStorageBuffer.Allocate(buffer.Size);
		//		m_UniformStorageBuffer.ZeroInitialize();
		//	}
	}

	void MaterialHazelLegacy::Bind()
	{
		Log::GetLogger()->error("Bind() not implemented in base class HazelMaterial!");

		auto& shader = m_Shader;
		shader->Bind();
		const auto& shaderBuffers = GetShader()->GetShaderBuffers();
		HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

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

	void MaterialHazelLegacy::BindTextures()
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

	Hazel::Ref<MaterialInstanceHazelLegacy> MaterialInstanceHazelLegacy::Create(const Hazel::Ref<MaterialHazelLegacy>& material)
	{
		return Hazel::Ref<MaterialInstanceHazelLegacy>::Create(material);
	}

	MaterialInstanceHazelLegacy::MaterialInstanceHazelLegacy(const Hazel::Ref<MaterialHazelLegacy>& material, const std::string& name)
		: m_Material(material), m_Name(name)
	{
		m_Material->m_MaterialInstances.insert(this);
		AllocateStorage();
	}

	MaterialInstanceHazelLegacy::~MaterialInstanceHazelLegacy()
	{
		m_Material->m_MaterialInstances.erase(this);
	}

	void MaterialInstanceHazelLegacy::AllocateStorage()
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

	void MaterialInstanceHazelLegacy::SetFlag(HazelMaterialFlag flag, bool value)
	{
	}

	void MaterialInstanceHazelLegacy::Bind()
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
