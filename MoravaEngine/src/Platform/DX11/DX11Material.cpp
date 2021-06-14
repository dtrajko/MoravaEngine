#include "DX11Material.h"

#include "Hazel/Renderer/HazelRenderer.h"

#include "Platform/DX11/DX11Context.h"
#include "Platform/DX11/DX11Texture2D.h"
#include "Platform/DX11/DX11Image.h"


DX11Material::DX11Material(const Hazel::Ref<Hazel::HazelShader>& shader, const std::string& name)
	: m_Shader(shader), m_Name(name)
{
	Init();
	Hazel::HazelRenderer::RegisterShaderDependency(shader, Hazel::Ref<HazelMaterial>(this));
}

DX11Material::DX11Material(const Hazel::Ref<DX11Shader>& shader)
{
	m_Shader = shader;
}

DX11Material::DX11Material(Hazel::Ref<DX11Material> material)
{
	m_Shader = material->GetShader();
	// m_Shader.As<DX11Shader>()->m_VertexShaderDX11 = material->GetShader().As<DX11Shader>()->m_VertexShaderDX11;
	// m_Shader.As<DX11Shader>()->m_PixelShaderDX11 = material->GetShader().As<DX11Shader>()->m_PixelShaderDX11;
}

DX11Material::~DX11Material()
{
}

void DX11Material::Init()
{
	AllocateStorage();

	m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
	m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;
}

void DX11Material::Invalidate()
{
}

void DX11Material::AllocateStorage()
{
	Log::GetLogger()->warn("DX11Material::AllocateStorage: method not yet implemented!"); return;

	const auto& shaderBuffers = m_Shader->GetShaderBuffers();

	if (shaderBuffers.size() > 0)
	{
		uint32_t size = 0;
		for (auto [name, shaderBuffer] : shaderBuffers)
			size += shaderBuffer.Size;

		m_UniformStorageBuffer.Allocate(size);
		m_UniformStorageBuffer.ZeroInitialize();
	}
}

void DX11Material::OnShaderReloaded()
{
	return;
	AllocateStorage();
}

const Hazel::ShaderUniform* DX11Material::FindUniformDeclaration(const std::string& name)
{
	const auto& shaderBuffers = m_Shader->GetShaderBuffers();

	HZ_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

	if (shaderBuffers.size() > 0)
	{
		const Hazel::ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
		if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
			return nullptr;

		return &buffer.Uniforms.at(name);
	}
	return nullptr;
}

const Hazel::ShaderResourceDeclaration* DX11Material::FindResourceDeclaration(const std::string& name)
{
	auto& resources = m_Shader->GetResources();
	for (const auto& [n, resource] : resources)
	{
		if (resource.GetName() == name)
			return &resource;
	}
	return nullptr;
}

void DX11Material::Set(const std::string& name, float value)
{
	Set<float>(name, value);
}

void DX11Material::Set(const std::string& name, int value)
{
	Set<int>(name, value);
}

void DX11Material::Set(const std::string& name, uint32_t value)
{
	Set<uint32_t>(name, value);
}

void DX11Material::Set(const std::string& name, bool value)
{
	// Bools are 4-byte ints
	Set<int>(name, (int)value);
}

void DX11Material::Set(const std::string& name, const glm::vec2& value)
{
	Set<glm::vec2>(name, value);
}

void DX11Material::Set(const std::string& name, const glm::vec3& value)
{
	Set<glm::vec3>(name, value);
}

void DX11Material::Set(const std::string& name, const glm::vec4& value)
{
	Set<glm::vec4>(name, value);
}

void DX11Material::Set(const std::string& name, const glm::mat3& value)
{
	Set<glm::mat3>(name, value);
}

void DX11Material::Set(const std::string& name, const glm::mat4& value)
{
	Set<glm::mat4>(name, value);
}

void DX11Material::Set(const std::string& name, const Hazel::Ref<Hazel::HazelTexture2D>& texture)
{
}

void DX11Material::Set(const std::string& name, const Hazel::Ref<Hazel::HazelTextureCube>& texture)
{
}

void DX11Material::Set(const std::string& name, const Hazel::Ref<Hazel::HazelImage2D>& image)
{
}

float& DX11Material::GetFloat(const std::string& name)
{
	return Get<float>(name);
}

int32_t& DX11Material::GetInt(const std::string& name)
{
	return Get<int32_t>(name);
}

uint32_t& DX11Material::GetUInt(const std::string& name)
{
	return Get<uint32_t>(name);
}

bool& DX11Material::GetBool(const std::string& name)
{
	return Get<bool>(name);
}

glm::vec2& DX11Material::GetVector2(const std::string& name)
{
	return Get<glm::vec2>(name);
}

glm::vec3& DX11Material::GetVector3(const std::string& name)
{
	return Get<glm::vec3>(name);
}

glm::vec4& DX11Material::GetVector4(const std::string& name)
{
	return Get<glm::vec4>(name);
}

glm::mat3& DX11Material::GetMatrix3(const std::string& name)
{
	return Get<glm::mat3>(name);
}

glm::mat4& DX11Material::GetMatrix4(const std::string& name)
{
	return Get<glm::mat4>(name);
}

Hazel::Ref<Hazel::HazelTexture2D> DX11Material::GetTexture2D(const std::string& name)
{
	return GetResource<Hazel::HazelTexture2D>(name);
}

Hazel::Ref<Hazel::HazelTextureCube> DX11Material::TryGetTextureCube(const std::string& name)
{
	return TryGetResource<Hazel::HazelTextureCube>(name);
}

Hazel::Ref<Hazel::HazelTexture2D> DX11Material::TryGetTexture2D(const std::string& name)
{
	return TryGetResource<Hazel::HazelTexture2D>(name);
}

Hazel::Ref<Hazel::HazelTextureCube> DX11Material::GetTextureCube(const std::string& name)
{
	return GetResource<Hazel::HazelTextureCube>(name);
}

void DX11Material::UpdateForRendering()
{
}

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
