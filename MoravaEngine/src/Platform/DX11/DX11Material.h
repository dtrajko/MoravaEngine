#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelImage.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/Pipeline.h"

#include "DX11Shader.h"
#include "DX11ConstantBuffer.h"
#include "DX11Texture2D.h"


class DX11Material : public Hazel::HazelMaterial
{
public:
	enum class CullMode
	{
		Front = 0,
		Back,
		Both,
	};

	DX11Material(const Hazel::Ref<Hazel::Pipeline>& pipeline, const std::string& name = "");
	DX11Material(Hazel::Ref<DX11Material> material);
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

	virtual uint32_t GetFlags() const override { return uint32_t(); }
	virtual bool GetFlag(Hazel::HazelMaterialFlag flag) const override { return bool(); };
	virtual void SetFlag(Hazel::HazelMaterialFlag flag, bool value = true) override {};

	Hazel::Ref<Hazel::Pipeline> GetPipeline() { return m_Pipeline; }
	virtual Hazel::Ref<Hazel::HazelShader> GetShader() override { return m_Shader; }
	virtual const std::string& GetName() const override { return m_Name; }

	void UpdateForRendering();

	// DirectX Material
	void AddTexture(Hazel::Ref<Hazel::HazelTexture> texture);
	void RemoveTexture(uint32_t index);

	void SetData(void* data, uint32_t size);

	void SetCullMode(CullMode cullMode);
	CullMode GetCullMode();

private:
	Hazel::Ref<Hazel::Pipeline> m_Pipeline;
	Hazel::Ref<Hazel::HazelShader> m_Shader; // shader reference is a pipeline property
	std::string m_Name;

	std::vector<Hazel::Ref<Hazel::HazelTexture>> m_Textures;

	// DirectX Material
	Hazel::Ref<DX11Shader> m_DX11Shader;
	Hazel::Ref<DX11ConstantBuffer> m_ConstantBuffer;
	CullMode m_CullMode = CullMode::Back;

};
