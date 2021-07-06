#include "DX11Material.h"

#include "Hazel/Renderer/HazelRenderer.h"

#include "Platform/DX11/DX11Context.h"
#include "Platform/DX11/DX11Texture2D.h"
#include "Platform/DX11/DX11Image.h"


DX11Material::DX11Material(const Hazel::Ref<Hazel::Pipeline>& pipeline, const std::string& name)
// 	: m_Pipeline(pipeline), m_Shader(pipeline->GetSpecification().Shader), m_Name(name)
{
	m_Pipeline = pipeline;
	// m_Shader = pipeline->GetSpecification().Shader; // TODO: fix the Ref problem
	m_Name = name;

	Invalidate();
}

DX11Material::DX11Material(Hazel::Ref<DX11Material> material)
	: m_Pipeline(material->GetPipeline()), m_Shader(m_Pipeline->GetSpecification().Shader)
{
	Invalidate();
}

DX11Material::~DX11Material() {}

void DX11Material::Invalidate() {}
void DX11Material::Set(const std::string& name, float value) {}
void DX11Material::Set(const std::string& name, int value) {}
void DX11Material::Set(const std::string& name, uint32_t value) {}
void DX11Material::Set(const std::string& name, bool value) {}
void DX11Material::Set(const std::string& name, const glm::vec2& value) {}
void DX11Material::Set(const std::string& name, const glm::vec3& value) {}
void DX11Material::Set(const std::string& name, const glm::vec4& value) {}
void DX11Material::Set(const std::string& name, const glm::mat3& value) {}
void DX11Material::Set(const std::string& name, const glm::mat4& value) {}
void DX11Material::Set(const std::string& name, const Hazel::Ref<Hazel::HazelTexture2D>& texture) {}
void DX11Material::Set(const std::string& name, const Hazel::Ref<Hazel::HazelTextureCube>& texture) {}
void DX11Material::Set(const std::string& name, const Hazel::Ref<Hazel::HazelImage2D>& image) {}
float& DX11Material::GetFloat(const std::string& name) { float v;  return v; }
int32_t& DX11Material::GetInt(const std::string& name) { int32_t v;  return v; }
uint32_t& DX11Material::GetUInt(const std::string& name) { uint32_t v;  return v; }
bool& DX11Material::GetBool(const std::string& name) { bool v;  return v; }
glm::vec2& DX11Material::GetVector2(const std::string& name) { glm::vec2 v;  return v; }
glm::vec3& DX11Material::GetVector3(const std::string& name) { glm::vec3 v;  return v; }
glm::vec4& DX11Material::GetVector4(const std::string& name) { glm::vec4 v;  return v; }
glm::mat3& DX11Material::GetMatrix3(const std::string& name) { glm::mat3 v;  return v; }
glm::mat4& DX11Material::GetMatrix4(const std::string& name) { glm::mat4 v;  return v; }
Hazel::Ref<Hazel::HazelTexture2D> DX11Material::GetTexture2D(const std::string& name) { std::string path = ""; return Hazel::HazelTexture2D::Create(path); }
Hazel::Ref<Hazel::HazelTextureCube> DX11Material::TryGetTextureCube(const std::string& name) { std::string path = ""; return Hazel::HazelTextureCube::Create(path); }
Hazel::Ref<Hazel::HazelTexture2D> DX11Material::TryGetTexture2D(const std::string& name) { std::string path = ""; return Hazel::HazelTexture2D::Create(path); }
Hazel::Ref<Hazel::HazelTextureCube> DX11Material::GetTextureCube(const std::string& name) { std::string path = ""; return Hazel::HazelTextureCube::Create(path); }
void DX11Material::UpdateForRendering() {}

void DX11Material::AddTexture(Hazel::Ref<DX11Texture2D> texture)
{
	m_Textures.push_back(texture);
}

void DX11Material::RemoveTexture(uint32_t index)
{
	if (index >= m_Textures.size()) return;

	m_Textures.erase(m_Textures.begin() + index);
}

void DX11Material::SetData(void* data, uint32_t size)
{
	if (!m_ConstantBuffer)
	{
		m_ConstantBuffer = Hazel::Ref<DX11ConstantBuffer>::Create(data, size);
	}
	else
	{
		m_ConstantBuffer->Update(data);
	}
}

void DX11Material::SetCullMode(CullMode mode)
{
	m_CullMode = mode;
}

DX11Material::CullMode DX11Material::GetCullMode()
{
	return m_CullMode;
}
