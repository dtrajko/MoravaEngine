#pragma once

#include "Hazel/Renderer/HazelImage.h"
#include "Hazel/Renderer/HazelMaterial.h"


class DX11Material : public Hazel::HazelMaterial
{
public:
	DX11Material(const Hazel::Ref<Hazel::HazelShader>& shader, const std::string& name = "");
	virtual ~DX11Material();

	virtual void Invalidate() override;

	virtual void Set(const std::string& name, float value) override;
	virtual void Set(const std::string& name, int value) override;
	virtual void Set(const std::string& name, uint32_t value) override;
	virtual void Set(const std::string& name, bool value) override;
	virtual void Set(const std::string& name, const glm::vec2& value) override;
	virtual void Set(const std::string& name, const glm::vec3& value) override;
	virtual void Set(const std::string& name, const glm::vec4& value) override;
	virtual void Set(const std::string& name, const glm::mat3& value) override;
	virtual void Set(const std::string& name, const glm::mat4& value) override;

	virtual void Set(const std::string& name, const Hazel::Ref<Hazel::HazelTexture2D>& texture) override;
	virtual void Set(const std::string& name, const Hazel::Ref<Hazel::HazelTextureCube>& texture) override;
	virtual void Set(const std::string& name, const Hazel::Ref<Hazel::HazelImage2D>& image) override;

	virtual float& GetFloat(const std::string& name) override;
	virtual int32_t& GetInt(const std::string& name) override;
	virtual uint32_t& GetUInt(const std::string& name) override;
	virtual bool& GetBool(const std::string& name) override;
	virtual glm::vec2& GetVector2(const std::string& name) override;
	virtual glm::vec3& GetVector3(const std::string& name) override;
	virtual glm::vec4& GetVector4(const std::string& name) override;
	virtual glm::mat3& GetMatrix3(const std::string& name) override;
	virtual glm::mat4& GetMatrix4(const std::string& name) override;

	virtual Hazel::Ref<Hazel::HazelTexture2D> GetTexture2D(const std::string& name) override;
	virtual Hazel::Ref<Hazel::HazelTextureCube> GetTextureCube(const std::string& name) override;

	virtual Hazel::Ref<Hazel::HazelTexture2D> TryGetTexture2D(const std::string& name) override;
	virtual Hazel::Ref<Hazel::HazelTextureCube> TryGetTextureCube(const std::string& name) override;

	template <typename T>
	void Set(const std::string& name, const T& value)
	{
		auto decl = FindUniformDeclaration(name);
		HZ_CORE_ASSERT(decl, "Could not find uniform!");
		if (!decl)
			return;

		auto& buffer = m_UniformStorageBuffer;
		buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());
	}

	template<typename T>
	T& Get(const std::string& name)
	{
		auto decl = FindUniformDeclaration(name);
		HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
		auto& buffer = m_UniformStorageBuffer;
		return buffer.Read<T>(decl->GetOffset());
	}

	template<typename T>
	Hazel::Ref<T> GetResource(const std::string& name)
	{
		auto decl = FindResourceDeclaration(name);
		HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
		uint32_t slot = decl->GetRegister();
		HZ_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
		return Hazel::Ref<T>(m_Textures[slot]);
	}

	template<typename T>
	Hazel::Ref<T> TryGetResource(const std::string& name)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl) {
			return Hazel::Ref<T>();
		}

		uint32_t slot = decl->GetRegister();
		if (slot >= m_Textures.size()) {
			return Hazel::Ref<T>();
		}

		return Hazel::Ref<T>(m_Textures[slot]);
	}

	virtual uint32_t GetFlags() const override { return m_MaterialFlags; }
	virtual bool GetFlag(Hazel::HazelMaterialFlag flag) const override { return (uint32_t)flag & m_MaterialFlags; }
	virtual void SetFlag(Hazel::HazelMaterialFlag flag, bool value = true) override
	{
		if (value)
		{
			m_MaterialFlags |= (uint32_t)flag;
		}
		else
		{
			m_MaterialFlags &= ~(uint32_t)flag;
		}
	}

	virtual Hazel::Ref<Hazel::HazelShader> GetShader() override { return m_Shader; }
	virtual const std::string& GetName() const override { return m_Name; }

	Hazel::Buffer GetUniformStorageBuffer() { return m_UniformStorageBuffer; }

	void UpdateForRendering();

private:
	void Init();
	void AllocateStorage();
	void OnShaderReloaded();

	const Hazel::ShaderUniform* FindUniformDeclaration(const std::string& name);
	const Hazel::ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);
private:
	Hazel::Ref<Hazel::HazelShader> m_Shader;
	std::string m_Name;

	uint32_t m_MaterialFlags = 0;

	Hazel::Buffer m_UniformStorageBuffer;
	std::vector<Hazel::Ref<Hazel::HazelTexture>> m_Textures; // TODO: Texture should only be stored as images
	std::vector<Hazel::Ref<Hazel::HazelImage>> m_Images;

	std::unordered_map<uint32_t, uint64_t> m_ImageHashes;

};
