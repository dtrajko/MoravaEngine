#include "DX11Material.h"

#include "H2M/Renderer/HazelRenderer.h"

#include "DX11Context.h"
#include "DX11Texture2D.h"
#include "DX11Image.h"
#include "DX11Shader.h"
#include "DX11Pipeline.h"


DX11Material::DX11Material(const H2M::Ref<H2M::HazelShader>& shader, const std::string& name)
	: m_Shader(shader), m_Name(name)
{
	Invalidate();

	Log::GetLogger()->info("DX11Material created (name: '{0}')", m_Name);
}

DX11Material::DX11Material(H2M::Ref<DX11Pipeline> pipeline, const std::string& name)
	: m_Pipeline(pipeline), m_Name(name)
{
	m_Shader = pipeline->GetSpecification().Shader;

	Invalidate();

	Log::GetLogger()->info("DX11Material created (name: '{0}', pipeline specification debug name: '{1}')",
		m_Name, m_Pipeline->GetSpecification().DebugName);
}

DX11Material::DX11Material(H2M::Ref<DX11Material> material, const std::string& name)
	: m_Pipeline(material->GetPipeline()), m_Shader(material->GetShader()), m_CullMode(material->GetCullMode())
{
	m_Name = name != "" ? name : material->GetName();

	Invalidate();

	Log::GetLogger()->info("DX11Material created (name: '{0}', pipeline specification debug name: '{1}')",
		m_Name, m_Pipeline->GetSpecification().DebugName);
}

DX11Material::~DX11Material() {}

void DX11Material::Invalidate() {}

void DX11Material::Set(const std::string& name, float value) {}
void DX11Material::Set(const std::string& name, int value) {}
void DX11Material::Set(const std::string& name, uint32_t value) {}
void DX11Material::Set(const std::string& name, bool value) {}
void DX11Material::Set(const std::string& name, const glm::ivec2& value) {}
void DX11Material::Set(const std::string& name, const glm::ivec3& value) {}
void DX11Material::Set(const std::string& name, const glm::ivec4& value) {}
void DX11Material::Set(const std::string& name, const glm::vec2& value) {}
void DX11Material::Set(const std::string& name, const glm::vec3& value) {}
void DX11Material::Set(const std::string& name, const glm::vec4& value) {}
void DX11Material::Set(const std::string& name, const glm::mat3& value) {}
void DX11Material::Set(const std::string& name, const glm::mat4& value) {}

void DX11Material::Set(const std::string& name, const H2M::Ref<H2M::Texture2DH2M>& texture) {}
void DX11Material::Set(const std::string& name, const H2M::Ref<H2M::Texture2DH2M>& texture, uint32_t arrayIndex) {}
void DX11Material::Set(const std::string& name, const H2M::Ref<H2M::TextureCubeH2M>& texture) {}
void DX11Material::Set(const std::string& name, const H2M::Ref<H2M::HazelImage2D>& image) {}

float& DX11Material::GetFloat(const std::string& name) { float v;  return v; }
int32_t& DX11Material::GetInt(const std::string& name) { int32_t v;  return v; }
uint32_t& DX11Material::GetUInt(const std::string& name) { uint32_t v;  return v; }
bool& DX11Material::GetBool(const std::string& name) { bool v;  return v; }
glm::vec2& DX11Material::GetVector2(const std::string& name) { glm::vec2 v;  return v; }
glm::vec3& DX11Material::GetVector3(const std::string& name) { glm::vec3 v;  return v; }
glm::vec4& DX11Material::GetVector4(const std::string& name) { glm::vec4 v;  return v; }
glm::mat3& DX11Material::GetMatrix3(const std::string& name) { glm::mat3 v;  return v; }
glm::mat4& DX11Material::GetMatrix4(const std::string& name) { glm::mat4 v;  return v; }
H2M::Ref<H2M::Texture2DH2M> DX11Material::GetTexture2D(const std::string& name) { std::string path = ""; return H2M::Texture2DH2M::Create(path); }
H2M::Ref<H2M::TextureCubeH2M> DX11Material::TryGetTextureCube(const std::string& name) { std::string path = ""; return H2M::TextureCubeH2M::Create(path); }
H2M::Ref<H2M::Texture2DH2M> DX11Material::TryGetTexture2D(const std::string& name) { std::string path = ""; return H2M::Texture2DH2M::Create(path); }
H2M::Ref<H2M::TextureCubeH2M> DX11Material::GetTextureCube(const std::string& name) { std::string path = ""; return H2M::TextureCubeH2M::Create(path); }
void DX11Material::UpdateForRendering() {}

void DX11Material::AddTexture(H2M::Ref<DX11Texture2D> texture)
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
		m_ConstantBuffer = H2M::Ref<DX11ConstantBuffer>::Create(data, size);
	}
	else
	{
		m_ConstantBuffer->Update(data);
	}
}

void DX11Material::Bind()
{
	DX11Context::Get()->SetRasterizerState(m_CullMode);

	m_Shader.As<DX11Shader>()->GetVertexShader()->BindConstantBuffer(m_ConstantBuffer);
	m_Shader.As<DX11Shader>()->GetPixelShader()->BindConstantBuffer(m_ConstantBuffer);

	m_Shader.As<DX11Shader>()->GetVertexShader()->Bind();
	m_Shader.As<DX11Shader>()->GetPixelShader()->Bind();

	m_Shader.As<DX11Shader>()->GetVertexShader()->SetTextures(m_Textures);
	m_Shader.As<DX11Shader>()->GetPixelShader()->SetTextures(m_Textures);
}

const H2M::ShaderUniform* DX11Material::FindUniformDeclaration(const std::string& name)
{
	const auto& shaderBuffers = m_Shader->GetShaderBuffers();

	HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

	if (shaderBuffers.size() > 0)
	{
		const H2M::ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
		if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
			return nullptr;

		return &buffer.Uniforms.at(name);
	}
	return nullptr;
}

const H2M::ShaderResourceDeclaration* DX11Material::FindResourceDeclaration(const std::string& name)
{
	auto& resources = m_Shader->GetResources();

	if (!resources.size())
	{
		Log::GetLogger()->error("DX11Material::FindResourceDeclaration - no resources found (name '{0}')!", name);
		return nullptr;
	}

	for (const auto& [n, resource] : resources)
	{
		if (resource.GetName() == name)
			return &resource;
	}
	return nullptr;
}
