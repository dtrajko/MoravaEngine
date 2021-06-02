#include "VulkanMoravaShader.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Core/Log.h"
#include "Core/Util.h"


VulkanMoravaShader::VulkanMoravaShader()
{
	shaderID = 0;
	m_Name = "Untitled";
	m_UniformLocations = std::map<std::string, int>();
}

VulkanMoravaShader::VulkanMoravaShader(const char* vertexLocation, const char* fragmentLocation, bool forceCompile)
	: VulkanMoravaShader() {}

VulkanMoravaShader::VulkanMoravaShader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile)
	: VulkanMoravaShader() {}

VulkanMoravaShader::VulkanMoravaShader(const char* computeLocation, bool forceCompile)
	: VulkanMoravaShader() {}

Hazel::Ref<VulkanMoravaShader> VulkanMoravaShader::Create(const char* vertexLocation, const char* fragmentLocation, bool forceCompile)
{
	return Hazel::Ref<VulkanMoravaShader>();
}

Hazel::Ref<VulkanMoravaShader> VulkanMoravaShader::Create(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile)
{
	return Hazel::Ref<VulkanMoravaShader>();
}

Hazel::Ref<VulkanMoravaShader> VulkanMoravaShader::Create(const char* computeLocation, bool forceCompile) { return Hazel::Ref<VulkanMoravaShader>(); }

VulkanMoravaShader::~VulkanMoravaShader() {}

void VulkanMoravaShader::CreateFromString(const char* vertexCode, const char* fragmentCode) {}

void VulkanMoravaShader::CreateFromFiles(const char* vertexLocation, const char* fragmentLocation) {}

void VulkanMoravaShader::CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation) {}

void VulkanMoravaShader::CreateFromFileVertex(const char* vertexLocation) {}

void VulkanMoravaShader::CreateFromFileFragment(const char* fragmentLocation) {}

void VulkanMoravaShader::CreateFromFileGeometry(const char* geometryLocation) {}

void VulkanMoravaShader::CreateFromFileCompute(const char* computeLocation) {}

std::string VulkanMoravaShader::ReadFile(const char* fileLocation) { return std::string(); }

void VulkanMoravaShader::Validate() {}

GLuint VulkanMoravaShader::GetProgramID() { return GLuint(); }

void VulkanMoravaShader::setBool(const std::string& name, bool value) {}

void VulkanMoravaShader::setInt(const std::string& name, int value) {}

void VulkanMoravaShader::setFloat(const std::string& name, float value) {}

void VulkanMoravaShader::setVec2(const std::string& name, const glm::vec2& value) {}

void VulkanMoravaShader::setVec2(const std::string& name, float x, float y) {}

void VulkanMoravaShader::setVec3(const std::string& name, const glm::vec3& value) {}

void VulkanMoravaShader::setVec3(const std::string& name, float x, float y, float z) {}

void VulkanMoravaShader::setVec4(const std::string& name, const glm::vec4& value) {}

void VulkanMoravaShader::setVec4(const std::string& name, float x, float y, float z, float w) {}

void VulkanMoravaShader::setMat2(const std::string& name, const glm::mat2& mat) {}

void VulkanMoravaShader::setMat3(const std::string& name, const glm::mat3& mat) {}

void VulkanMoravaShader::setMat4(const std::string& name, const glm::mat4& mat) {}

GLint VulkanMoravaShader::GetUniformLocation(const std::string& name) { return GLint(); }

void VulkanMoravaShader::SetIntArray(const std::string& name, int* values, uint32_t size) {}

const std::string& VulkanMoravaShader::GetName() const { return ""; }

const std::unordered_map<std::string, Hazel::ShaderBuffer>& VulkanMoravaShader::GetShaderBuffers() const
{
	return std::unordered_map<std::string, Hazel::ShaderBuffer>();
}

const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& VulkanMoravaShader::GetResources() const
{
	return std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>();
}

void VulkanMoravaShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback) {}

uint32_t VulkanMoravaShader::GetRendererID() const { return uint32_t(); }

size_t VulkanMoravaShader::GetHash() const { return size_t(); }

void VulkanMoravaShader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size) {}

void VulkanMoravaShader::SetUniform(const std::string& fullname, float value) {}

void VulkanMoravaShader::SetUniform(const std::string& fullname, uint32_t value) {}

void VulkanMoravaShader::SetUniform(const std::string& fullname, int value) {}

void VulkanMoravaShader::SetUniform(const std::string& fullname, const glm::vec2& value) {}

void VulkanMoravaShader::SetUniform(const std::string& fullname, const glm::vec3& value) {}

void VulkanMoravaShader::SetUniform(const std::string& fullname, const glm::vec4& value) {}

void VulkanMoravaShader::SetUniform(const std::string& fullname, const glm::mat3& value) {}

void VulkanMoravaShader::SetUniform(const std::string& fullname, const glm::mat4& value) {}

void VulkanMoravaShader::SetFloat(const std::string& name, float value) {}

void VulkanMoravaShader::SetUInt(const std::string& name, uint32_t value) {}

void VulkanMoravaShader::SetInt(const std::string& name, int value) {}

void VulkanMoravaShader::SetBool(const std::string& name, bool value) {}

void VulkanMoravaShader::SetFloat2(const std::string& name, const glm::vec2& value) {}

void VulkanMoravaShader::SetFloat3(const std::string& name, const glm::vec3& value) {}

void VulkanMoravaShader::SetMat4(const std::string& name, const glm::mat4& value) {}

void VulkanMoravaShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind) {}

void VulkanMoravaShader::UploadUniformMat4(const std::string& name, const glm::mat4& values) {}

void VulkanMoravaShader::UploadUniformMat4(uint32_t location, const glm::mat4& value) {}

void VulkanMoravaShader::SetLightMatrices(std::vector<glm::mat4> lightMatrices) {}

void VulkanMoravaShader::setLightMat4(std::vector<glm::mat4> lightMatrices) {}

void VulkanMoravaShader::Bind() {}

void VulkanMoravaShader::Reload(bool forceCompile) {}

void VulkanMoravaShader::Unbind() {}

void VulkanMoravaShader::ClearShader() {}

void VulkanMoravaShader::CompileShader(const char* vertexCode, const char* fragmentCode) {}

void VulkanMoravaShader::CompileShader(const char* vertexCode, const char* geometryCode, const char* fragmentCode) {}

void VulkanMoravaShader::AddShaderVertex(const char* vertexCode) {}

void VulkanMoravaShader::AddShaderFragment(const char* fragmentCode) {}

void VulkanMoravaShader::AddShaderGeometry(const char* geometryCode) {}

void VulkanMoravaShader::AddShaderCompute(const char* computeCode) {}

const char* VulkanMoravaShader::GetShaderTypeNameFromEnum(const GLenum shaderType) { return ""; }

void VulkanMoravaShader::AddShader(GLuint programID, const char* shaderCode, GLenum shaderType) {}

void VulkanMoravaShader::CompileProgram() {}

void VulkanMoravaShader::GetUniformLocations() {}
