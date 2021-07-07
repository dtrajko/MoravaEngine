#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelImage.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/Pipeline.h"

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

class DX11Material : public Hazel::HazelMaterial
{
public:
	DX11Material(const Hazel::Ref<Hazel::HazelShader>& shader, const std::string& name = "");
	DX11Material(Hazel::Ref<DX11Pipeline> pipeline, const std::string& name = "");
	DX11Material(Hazel::Ref<DX11Material> material, const std::string& name = "");
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

	inline Hazel::Ref<Hazel::Pipeline> GetPipeline() { return m_Pipeline; }
	inline virtual Hazel::Ref<Hazel::HazelShader> GetShader() override { return m_Shader; }
	inline virtual const std::string& GetName() const override { return m_Name; }

	void UpdateForRendering();

	// DirectX Material
	void AddTexture(Hazel::Ref<DX11Texture2D> texture);
	void RemoveTexture(uint32_t index);

	void SetData(void* data, uint32_t size);

	inline void SetCullMode(DX11CullMode cullMode) { m_CullMode = cullMode; }
	inline DX11CullMode GetCullMode() { return m_CullMode; }

	void Bind();

private:
	Hazel::Ref<Hazel::Pipeline> m_Pipeline;
	Hazel::Ref<Hazel::HazelShader> m_Shader; // shader reference is a pipeline property
	std::string m_Name;

	std::vector<Hazel::Ref<DX11Texture2D>> m_Textures;

	// DirectX Material
	Hazel::Ref<DX11Shader> m_DX11Shader;
	Hazel::Ref<DX11ConstantBuffer> m_ConstantBuffer;
	DX11CullMode m_CullMode = DX11CullMode::None;

};
