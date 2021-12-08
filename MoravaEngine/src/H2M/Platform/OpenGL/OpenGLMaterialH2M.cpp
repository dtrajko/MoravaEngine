/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLMaterialH2M.h"

#include "H2M/Renderer/RendererH2M.h"

#include "OpenGLImageH2M.h"
#include "OpenGLShaderH2M.h"
#include "OpenGLTextureH2M.h"


namespace H2M
{

	OpenGLMaterialH2M::OpenGLMaterialH2M(const RefH2M<ShaderH2M>& shader, const std::string& name)
		: m_Shader(shader), m_Name(name)
	{
		m_Shader->AddShaderReloadedCallback(std::bind(&OpenGLMaterialH2M::OnShaderReloaded, this));
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;
	}

	OpenGLMaterialH2M::~OpenGLMaterialH2M()
	{
		Log::GetLogger()->debug("Destroy OpenGLMaterialH2M!");
	}

	void OpenGLMaterialH2M::Invalidate()
	{
	}

	/*
	 * Made in Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021)
	 */
	void OpenGLMaterialH2M::AllocateStorage()
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		if (shaderBuffers.size() > 100000) return; // temporary check for properly allocated resource

		if (shaderBuffers.size() > 0)
		{
			uint32_t size = 0;
			for (auto [name, shaderBuffer] : shaderBuffers)
			{
				size += shaderBuffer.Size;
			}

			m_UniformStorageBuffer.Allocate(size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void OpenGLMaterialH2M::OnShaderReloaded()
	{
		return;
		AllocateStorage();
	}

	const ShaderUniformH2M* OpenGLMaterialH2M::FindUniformDeclaration(const std::string& name)
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		H2M_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBufferH2M& buffer = (*shaderBuffers.begin()).second;
			if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
			{
				return nullptr;
			}

			return &buffer.Uniforms.at(name);
		}
		return nullptr;
	}

	const ShaderResourceDeclarationH2M* OpenGLMaterialH2M::FindResourceDeclaration(const std::string& name)
	{
		auto& resources = m_Shader->GetResources();

		if (!resources.size())
		{
			Log::GetLogger()->error("OpenGLMaterialH2M::FindResourceDeclaration - no resources found (name '{0}')!", name);
			return nullptr;
		}

		for (const auto& [n, resource] : resources)
		{
			if (resource.GetName() == name)
			{
				return &resource;
			}
		}
		return nullptr;
	}

	void OpenGLMaterialH2M::Set(const std::string& name, float value)
	{
		Set<float>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, int value)
	{
		Set<int>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, uint32_t value)
	{
		Set<uint32_t>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, bool value)
	{
		// Bools are uints
		Set<uint32_t>(name, (int)value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const glm::ivec2& value)
	{
		Set<glm::ivec2>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const glm::ivec3& value)
	{
		Set<glm::ivec3>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const glm::ivec4& value)
	{
		Set<glm::ivec4>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const glm::vec2& value)
	{
		Set<glm::vec2>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const glm::vec3& value)
	{
		Set<glm::vec3>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const glm::vec4& value)
	{
		Set<glm::vec4>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const glm::mat3& value)
	{
		Set<glm::mat3>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const glm::mat4& value)
	{
		Set<glm::mat4>(name, value);
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const RefH2M<Texture2D_H2M>& texture, uint32_t arrayIndex)
	{
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const RefH2M<Texture2D_H2M>& texture)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			// HZ_CORE_WARN("Cannot find material property: ", name);
			Log::GetLogger()->error("Cannot find material property '{0}'!", name);
			return;
		}
		uint32_t slot = decl->GetRegister();
		m_Texture2Ds[slot] = texture;
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const RefH2M<TextureCubeH2M>& texture)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			// HZ_CORE_WARN("Cannot find material property: ", name);
			Log::GetLogger()->error("Cannot find material property '{0}'!", name);
			return;
		}
		uint32_t slot = decl->GetRegister();
		if (m_Textures.size() <= slot)
		{
			m_Textures.resize((size_t)slot + 1);
		}
		m_Textures[slot] = texture;
	}

	void OpenGLMaterialH2M::Set(const std::string& name, const RefH2M<Image2D_H2M>& image)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			// HZ_CORE_WARN("Cannot find material property: ", name);
			Log::GetLogger()->error("Cannot find material property '{0}'!", name);
			return;
		}
		uint32_t slot = decl->GetRegister();
		m_Images[slot] = image;
	}

	float& OpenGLMaterialH2M::GetFloat(const std::string& name)
	{
		return Get<float>(name);
	}

	int32_t& OpenGLMaterialH2M::GetInt(const std::string& name)
	{
		return Get<int32_t>(name);
	}

	uint32_t& OpenGLMaterialH2M::GetUInt(const std::string& name)
	{
		return Get<uint32_t>(name);
	}

	bool& OpenGLMaterialH2M::GetBool(const std::string& name)
	{
		return Get<bool>(name);
	}

	glm::vec2& OpenGLMaterialH2M::GetVector2(const std::string& name)
	{
		return Get<glm::vec2>(name);
	}

	glm::vec3& OpenGLMaterialH2M::GetVector3(const std::string& name)
	{
		return Get<glm::vec3>(name);
	}

	glm::vec4& OpenGLMaterialH2M::GetVector4(const std::string& name)
	{
		return Get<glm::vec4>(name);
	}

	glm::mat3& OpenGLMaterialH2M::GetMatrix3(const std::string& name)
	{
		return Get<glm::mat3>(name);
	}

	glm::mat4& OpenGLMaterialH2M::GetMatrix4(const std::string& name)
	{
		return Get<glm::mat4>(name);
	}

	RefH2M<Texture2D_H2M> OpenGLMaterialH2M::GetTexture2D(const std::string& name)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			// H2M_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
		}
		uint32_t slot = decl->GetRegister();
		if (slot >= m_Texture2Ds.size())
		{
			// H2M_CORE_ASSERT(slot < m_Texture2Ds.size(), "Texture slot is invalid");
			Log::GetLogger()->error("Texture slot '{0}' is invalid!", slot);
		}

		return m_Texture2Ds[slot];
	}

	RefH2M<TextureCubeH2M> OpenGLMaterialH2M::TryGetTextureCube(const std::string& name)
	{
		return TryGetResource<TextureCubeH2M>(name);
	}

	RefH2M<Texture2D_H2M> OpenGLMaterialH2M::TryGetTexture2D(const std::string& name)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			return RefH2M<Texture2D_H2M>();
		}

		uint32_t slot = decl->GetRegister();
		if (m_Texture2Ds.find(slot) == m_Texture2Ds.end())
		{
			return RefH2M<Texture2D_H2M>();
		}

		return m_Texture2Ds[slot];
	}

	RefH2M<TextureCubeH2M> OpenGLMaterialH2M::GetTextureCube(const std::string& name)
	{
		return GetResource<TextureCubeH2M>(name);
	}

	void OpenGLMaterialH2M::UpdateForRendering()
	{
		RefH2M<OpenGLShaderH2M> shader = m_Shader.As<OpenGLShaderH2M>();

		// HazelRenderer::Submit([shader]() {});
		{
			glUseProgram(shader->GetRendererID());
		}

		const auto& shaderBuffers = GetShader()->GetShaderBuffers();
		H2M_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBufferH2M& buffer = (*shaderBuffers.begin()).second;

			for (auto& [name, uniform] : buffer.Uniforms)
			{
				switch (uniform.GetType())
				{
				case ShaderUniformTypeH2M::Bool:
				case ShaderUniformTypeH2M::UInt:
				{
					uint32_t value = m_UniformStorageBuffer.Read<uint32_t>(uniform.GetOffset());
					shader->SetUniform(name, (int)value);
					break;
				}
				case ShaderUniformTypeH2M::Int:
				{
					int value = m_UniformStorageBuffer.Read<int>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformTypeH2M::Float:
				{
					float value = m_UniformStorageBuffer.Read<float>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformTypeH2M::Vec2:
				{
					const glm::vec2& value = m_UniformStorageBuffer.Read<glm::vec2>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformTypeH2M::Vec3:
				{
					const glm::vec3& value = m_UniformStorageBuffer.Read<glm::vec3>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformTypeH2M::Vec4:
				{
					const glm::vec4& value = m_UniformStorageBuffer.Read<glm::vec4>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformTypeH2M::Mat3:
				{
					const glm::mat3& value = m_UniformStorageBuffer.Read<glm::mat3>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformTypeH2M::Mat4:
				{
					const glm::mat4& value = m_UniformStorageBuffer.Read<glm::mat4>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				default:
				{
					H2M_CORE_ASSERT(false);
					break;
				}
				}
			}
		}

#if 1
		for (size_t i = 0; i < m_Textures.size(); i++)
		{
			auto& texture = m_Textures[i];
			if (texture)
			{
				// HazelRenderer::Submit([i, texture]() {});
				{
					if (texture->GetType() == TextureTypeH2M::TextureCube)
					{
						RefH2M<OpenGLTextureCubeH2M> glTexture = texture.As<OpenGLTextureCubeH2M>();
						glBindTextureUnit((GLuint)i, glTexture->GetRendererID());
					}
					else
					{
						RefH2M<OpenGLTexture2D_H2M> glTexture = texture.As<OpenGLTexture2D_H2M>();
						RefH2M<OpenGLImage2D_H2M> image = glTexture->GetImage().As<OpenGLImage2D_H2M>();
						glBindTextureUnit((GLuint)i, image->GetRendererID());
					}
				}
			}
		}
#else
		/**** BEGIN too early for this code version ****
		for (size_t i = 0; i < m_Textures.size(); i++)
		{
			auto& texture = m_Textures[i];
			if (texture)
			{
				// HazelRenderer::Submit([i, texture]() {});
				{
					H2M_CORE_ASSERT(texture->GetType() == TextureType::TextureCube);
					RefH2M<OpenGLTextureCube> glTexture = texture.As<OpenGLTextureCube>();
					glBindTextureUnit((GLuint)i, glTexture->GetID());
				}
			}
		}

		for (auto [slot, texture] : m_Texture2Ds)
		{
			if (texture)
			{
				uint32_t textureSlot = slot;
				RefH2M<Image2D_H2M> image = texture->GetImage();
				RefH2M<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
				// HazelRenderer::Submit([textureSlot, glImage]() {});
				{
					glBindSampler(textureSlot, glImage->GetSamplerRendererID());
					glBindTextureUnit(textureSlot, glImage->GetRendererID());
				}
			}
		}

		for (auto [slot, image] : m_Images)
		{
			if (image)
			{
				uint32_t textureSlot = slot;
				RefH2M<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
				// HazelRenderer::Submit([textureSlot, glImage]() {});
				{
					glBindSampler(textureSlot, glImage->GetSamplerRendererID());
					glBindTextureUnit(textureSlot, glImage->GetRendererID());
				}
			}
		}
		/**** END too early for this code version ****/
#endif
	}

}
