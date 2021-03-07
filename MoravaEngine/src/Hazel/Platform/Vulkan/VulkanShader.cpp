#include "VulkanShader.h"
#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Core/HazelLog.h"
#include "Hazel/Renderer/ShaderUniform.h"

#include <string>
#include <sstream>
#include <limits>
#include <fstream>
#include <filesystem>

#include <glm/gtc/type_ptr.hpp>

#include <shaderc/shaderc.hpp>


namespace Hazel {
	VulkanShader::VulkanShader(const std::string& path, bool forceCompile)
	{
	}
	Ref<VulkanShader> VulkanShader::CreateFromString(const std::string& source)
	{
		return Ref<VulkanShader>();
	}
	void VulkanShader::Reload(bool forceCompile)
	{
	}
	void VulkanShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
	{
	}
	void VulkanShader::Bind()
	{
	}
	void VulkanShader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size)
	{
	}
	void VulkanShader::SetUniform(const std::string& fullname, float value)
	{
	}
	void VulkanShader::SetUniform(const std::string& fullname, int value)
	{
	}
	void VulkanShader::SetUniform(const std::string& fullname, const glm::vec2& value)
	{
	}
	void VulkanShader::SetUniform(const std::string& fullname, const glm::vec3& value)
	{
	}
	void VulkanShader::SetUniform(const std::string& fullname, const glm::vec4& value)
	{
	}
	void VulkanShader::SetUniform(const std::string& fullname, const glm::mat3& value)
	{
	}
	void VulkanShader::SetUniform(const std::string& fullname, const glm::mat4& value)
	{
	}
	void VulkanShader::SetFloat(const std::string& name, float value)
	{
	}
	void VulkanShader::SetInt(const std::string& name, int value)
	{
	}
	void VulkanShader::SetBool(const std::string& name, bool value)
	{
	}
	void VulkanShader::SetFloat3(const std::string& name, const glm::vec3& value)
	{
	}
	void VulkanShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
	}
	void VulkanShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind)
	{
	}
	void VulkanShader::SetIntArray(const std::string& name, int* values, uint32_t size)
	{
	}
	void VulkanShader::Load(const std::string& source)
	{
	}
	void VulkanShader::Compile(const std::vector<uint32_t>& vertexBinary, const std::vector<uint32_t>& fragmentBinary)
	{
	}
	void VulkanShader::Reflect(std::vector<uint32_t>& data)
	{
	}
	void VulkanShader::CompileOrGetVulkanBinary(std::array<std::vector<uint32_t>, 2>& outputBinary, bool forceCompile)
	{
	}
	void VulkanShader::CompileOrGetOpenGLBinary(const std::array<std::vector<uint32_t>, 2>& vulkanBinaries, bool forceCompile)
	{
	}
	std::string VulkanShader::ReadShaderFromFile(const std::string& filepath) const
	{
		return std::string();
	}
	std::unordered_map<GLenum, std::string> VulkanShader::PreProcess(const std::string& source)
	{
		return std::unordered_map<GLenum, std::string>();
	}
	int32_t VulkanShader::GetUniformLocation(const std::string& name) const
	{
		return int32_t();
	}
	void VulkanShader::CompileAndUploadShader()
	{
	}
	GLenum VulkanShader::ShaderTypeFromString(const std::string& type)
	{
		return GLenum();
	}
	void VulkanShader::UploadUniformInt(uint32_t location, int32_t value)
	{
	}
	void VulkanShader::UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count)
	{
	}
	void VulkanShader::UploadUniformFloat(uint32_t location, float value)
	{
	}
	void VulkanShader::UploadUniformFloat2(uint32_t location, const glm::vec2& value)
	{
	}
	void VulkanShader::UploadUniformFloat3(uint32_t location, const glm::vec3& value)
	{
	}
	void VulkanShader::UploadUniformFloat4(uint32_t location, const glm::vec4& value)
	{
	}
	void VulkanShader::UploadUniformMat3(uint32_t location, const glm::mat3& values)
	{
	}
	void VulkanShader::UploadUniformMat4(uint32_t location, const glm::mat4& values)
	{
	}
	void VulkanShader::UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count)
	{
	}
	void VulkanShader::UploadUniformInt(const std::string& name, int32_t value)
	{
	}
	void VulkanShader::UploadUniformIntArray(const std::string& name, int32_t* values, uint32_t count)
	{
	}
	void VulkanShader::UploadUniformFloat(const std::string& name, float value)
	{
	}
	void VulkanShader::UploadUniformFloat2(const std::string& name, const glm::vec2& values)
	{
	}
	void VulkanShader::UploadUniformFloat3(const std::string& name, const glm::vec3& values)
	{
	}
	void VulkanShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
	{
	}
	void VulkanShader::UploadUniformMat4(const std::string& name, const glm::mat4& value)
	{
	}
}
