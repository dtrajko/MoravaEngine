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

GLint VulkanMoravaShader::GetUniformLocation(const std::string& name)
{
	Log::GetLogger()->error("VulkanMoravaShader::GetUniformLocation() not implemented yet!");
	return GLint();
}

void VulkanMoravaShader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
{
	Log::GetLogger()->error("VulkanMoravaShader::UploadUniformMat4() not implemented yet!");
}

void VulkanMoravaShader::UploadUniformMat4(uint32_t location, const glm::mat4& value)
{
	Log::GetLogger()->error("VulkanMoravaShader::UploadUniformMat4() not implemented yet!");
}

void VulkanMoravaShader::SetLightMatrices(std::vector<glm::mat4> lightMatrices)
{
	Log::GetLogger()->error("VulkanMoravaShader::SetLightMatrices() not implemented yet!");
}

void VulkanMoravaShader::setLightMat4(std::vector<glm::mat4> lightMatrices)
{
	Log::GetLogger()->error("VulkanMoravaShader::setLightMat4() not implemented yet!");
}

void VulkanMoravaShader::Unbind()
{
	Log::GetLogger()->error("VulkanMoravaShader::Unbind() not implemented yet!");
}

void VulkanMoravaShader::SetVec2(const std::string& name, const glm::vec2& value)
{
	Log::GetLogger()->error("VulkanMoravaShader::SetVec2() not implemented yet!");
}

void VulkanMoravaShader::SetVec2(const std::string& name, float x, float y)
{
	Log::GetLogger()->error("VulkanMoravaShader::SetVec2() not implemented yet!");
}

void VulkanMoravaShader::SetMat2(const std::string& name, const glm::mat2& mat)
{
	Log::GetLogger()->error("VulkanMoravaShader::SetMat2() not implemented yet!");
}

void VulkanMoravaShader::SetMat3(const std::string& name, const glm::mat3& mat)
{
	Log::GetLogger()->error("VulkanMoravaShader::SetMat3() not implemented yet!");
}

void VulkanMoravaShader::SetIntArray(const std::string& name, int* values, uint32_t size) {}

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

void VulkanMoravaShader::SetFloat4(const std::string& name, const glm::vec4& value) {}

void VulkanMoravaShader::SetMat4(const std::string& name, const glm::mat4& value) {}

void VulkanMoravaShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind) {}

void VulkanMoravaShader::Bind() {}

void VulkanMoravaShader::Reload(bool forceCompile) {}

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
