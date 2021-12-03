#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/ImageH2M.h"
#include "H2M/Renderer/MaterialH2M.h"
#include "H2M/Renderer/PipelineH2M.h"

#include "DX11ConstantBuffer.h"
#include "DX11Texture2D.h"


class DX11Shader;
class DX11Pipeline;

enum class DX11CullMode
{
	None = 0,
	Front,
	Back,
};


class DX11Material : public H2M::HazelMaterial
{
public:
	DX11Material(const H2M::RefH2M<H2M::HazelShader>& shader, const std::string& name = "");
	DX11Material(H2M::RefH2M<DX11Pipeline> pipeline, const std::string& name = "");
	DX11Material(H2M::RefH2M<DX11Material> material, const std::string& name = "");
	virtual ~DX11Material();

	virtual void Invalidate() override;

	virtual void Set(const std::string& name, float value) override;
	virtual void Set(const std::string& name, int value) override;
	virtual void Set(const std::string& name, uint32_t value) override;
	virtual void Set(const std::string& name, bool value) override;
	virtual void Set(const std::string& name, const glm::ivec2& value) override;
	virtual void Set(const std::string& name, const glm::ivec3& value) override;
	virtual void Set(const std::string& name, const glm::ivec4& value) override;
	virtual void Set(const std::string& name, const glm::vec2& value) override;
	virtual void Set(const std::string& name, const glm::vec3& value) override;
	virtual void Set(const std::string& name, const glm::vec4& value) override;
	virtual void Set(const std::string& name, const glm::mat3& value) override;
	virtual void Set(const std::string& name, const glm::mat4& value) override;

	virtual void Set(const std::string& name, const H2M::RefH2M<H2M::Texture2D_H2M>& texture) override;
	virtual void Set(const std::string& name, const H2M::RefH2M<H2M::Texture2D_H2M>& texture, uint32_t arrayIndex) override;
	virtual void Set(const std::string& name, const H2M::RefH2M<H2M::TextureCubeH2M>& texture) override;
	virtual void Set(const std::string& name, const H2M::RefH2M<H2M::HazelImage2D>& image) override;

	virtual float& GetFloat(const std::string& name) override;
	virtual int32_t& GetInt(const std::string& name) override;
	virtual uint32_t& GetUInt(const std::string& name) override;
	virtual bool& GetBool(const std::string& name) override;
	virtual glm::vec2& GetVector2(const std::string& name) override;
	virtual glm::vec3& GetVector3(const std::string& name) override;
	virtual glm::vec4& GetVector4(const std::string& name) override;
	virtual glm::mat3& GetMatrix3(const std::string& name) override;
	virtual glm::mat4& GetMatrix4(const std::string& name) override;

	virtual H2M::RefH2M<H2M::Texture2D_H2M> GetTexture2D(const std::string& name) override;
	virtual H2M::RefH2M<H2M::TextureCubeH2M> GetTextureCube(const std::string& name) override;

	virtual H2M::RefH2M<H2M::Texture2D_H2M> TryGetTexture2D(const std::string& name) override;
	virtual H2M::RefH2M<H2M::TextureCubeH2M> TryGetTextureCube(const std::string& name) override;

	template <typename T>
	void Set(const std::string& name, const T& value)
	{
		auto decl = FindUniformDeclaration(name);
		if (!decl)
		{
			// H2M_CORE_ASSERT(decl, "Could not find uniform!");
			Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
			return;
		}

		auto& buffer = m_UniformStorageBuffer;
		buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());
	}

	void Set(const std::string& name, const H2M::RefH2M<H2M::HazelTexture>& texture)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			// HZ_CORE_WARN("Cannot find material property: ", name);
			Log::GetLogger()->error("Cannot find material property: ", name);
			return;
		}
		uint32_t slot = decl->GetRegister();
		if (m_Textures.size() <= slot)
		{
			m_Textures.resize((size_t)slot + 1);
		}
		m_Textures[slot] = texture;
	}

	template<typename T>
	T& Get(const std::string& name)
	{
		auto decl = FindUniformDeclaration(name);
		if (!decl)
		{
			// H2M_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
		}
		auto& buffer = m_UniformStorageBuffer;
		return buffer.Read<T>(decl->GetOffset());
	}

	template<typename T>
	Ref<T> GetResource(const std::string& name)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			// H2M_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			Log::GetLogger()->error("Could not find uniform with name '{0}'!", name);
		}
		uint32_t slot = decl->GetRegister();
		if (slot >= m_Textures.size())
		{
			// H2M_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
			Log::GetLogger()->error("Texture slot '{0}' is invalid!", slot);
		}
		return Ref<T>(m_Textures[slot]);
	}

	template<typename T>
	Ref<T> TryGetResource(const std::string& name)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			return Ref<T>();
		}

		uint32_t slot = decl->GetRegister();
		if (slot >= m_Textures.size())
		{
			return Ref<T>();
		}

		return Ref<T>(m_Textures[slot]);
	}

	virtual uint32_t GetFlags() const override { return uint32_t(); }
	virtual bool GetFlag(H2M::HazelMaterialFlag flag) const override { return bool(); };
	virtual void SetFlag(H2M::HazelMaterialFlag flag, bool value = true) override {};

	inline H2M::RefH2M<H2M::Pipeline> GetPipeline() { return m_Pipeline; }
	inline virtual H2M::RefH2M<H2M::HazelShader> GetShader() override { return m_Shader; }
	inline virtual const std::string& GetName() const override { return m_Name; }

	void UpdateForRendering();

	// DirectX Material
	void AddTexture(H2M::RefH2M<DX11Texture2D> texture);
	void RemoveTexture(uint32_t index);

	void SetData(void* data, uint32_t size);

	inline void SetCullMode(DX11CullMode cullMode) { m_CullMode = cullMode; }
	inline DX11CullMode GetCullMode() { return m_CullMode; }

	void Bind();

	const H2M::ShaderUniform* FindUniformDeclaration(const std::string& name);
	const H2M::ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);

private:
	H2M::RefH2M<H2M::Pipeline> m_Pipeline;
	H2M::RefH2M<H2M::HazelShader> m_Shader; // shader reference is a pipeline property
	std::string m_Name;

	std::vector<H2M::RefH2M<H2M::HazelTexture>> m_Textures;

	// DirectX Material
	H2M::RefH2M<DX11ConstantBuffer> m_ConstantBuffer;
	DX11CullMode m_CullMode = DX11CullMode::None;

};
