/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/ShaderH2M.h"

#include <spirv_glsl.hpp>


namespace H2M
{

	class OpenGLShaderH2M : public ShaderH2M
	{
	public:
		OpenGLShaderH2M() = default;
		OpenGLShaderH2M(const std::string& filepath, bool forceRecompile);
		static RefH2M<OpenGLShaderH2M> CreateFromString(const std::string& source);

		virtual void Reload(bool forceCompile = false) override;
		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

		virtual void Bind() override;
		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual size_t GetHash() const override;

		virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) override;
		virtual void SetUniform(const std::string& fullname, float value) override;
		virtual void SetUniform(const std::string& fullname, int value) override;
		virtual void SetUniform(const std::string& fullname, uint32_t value) override;
		virtual void SetUniform(const std::string& fullname, const glm::vec2& value) override;
		virtual void SetUniform(const std::string& fullname, const glm::vec3& value) override;
		virtual void SetUniform(const std::string& fullname, const glm::vec4& value) override;
		virtual void SetUniform(const std::string& fullname, const glm::mat3& value) override;
		virtual void SetUniform(const std::string& fullname, const glm::mat4& value) override;

		virtual void SetUInt(const std::string& name, uint32_t value) override;
		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
		virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;

		virtual void SetIntArray(const std::string& name, int* values, uint32_t size) override;

		virtual const std::string& GetName() const override { return m_Name; }

		virtual const std::unordered_map<std::string, ShaderBufferH2M>& GetShaderBuffers() const override { return m_Buffers; }
		virtual const std::unordered_map<std::string, ShaderResourceDeclarationH2M>& GetResources() const override { return m_Resources; }

		const ShaderResourceDeclarationH2M* GetShaderResource(const std::string& name);

		static void ClearUniformBuffers();

	private:
		void Load(const std::string& source, bool forceCompile);
		void Compile(const std::vector<uint32_t>& vertexBinary, const std::vector<uint32_t>& fragmentBinary);
		void Reflect(std::vector<uint32_t>& data);

		void CompileOrGetVulkanBinary(std::unordered_map<uint32_t, std::vector<uint32_t>>& outputBinary, bool forceCompile = false);
		void CompileOrGetOpenGLBinary(const std::unordered_map<uint32_t, std::vector<uint32_t>>&, bool forceCompile = false);

		std::string ReadShaderFromFile(const std::string& filepath) const;
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);

		void ParseConstantBuffers(const spirv_cross::CompilerGLSL& compiler);

		int32_t GetUniformLocation(const std::string& name) const;

		static GLenum ShaderTypeFromString(const std::string& type);

		void UploadUniformInt(uint32_t location, int32_t value);
		void UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count);
		void UploadUniformFloat(uint32_t location, float value);
		void UploadUniformFloat2(uint32_t location, const glm::vec2& value);
		void UploadUniformFloat3(uint32_t location, const glm::vec3& value);
		void UploadUniformFloat4(uint32_t location, const glm::vec4& value);
		void UploadUniformMat3(uint32_t location, const glm::mat3& values);
		void UploadUniformMat4(uint32_t location, const glm::mat4& values);
		void UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count);

		void UploadUniformInt(const std::string& name, int32_t value);
		void UploadUniformUInt(const std::string& name, uint32_t value);
		void UploadUniformIntArray(const std::string& name, int32_t* values, uint32_t count);

		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value);

		void UploadUniformMat4(const std::string& name, const glm::mat4& value);

	private:
		uint32_t m_RendererID = 0;
		bool m_Loaded = false;
		bool m_IsCompute = false;

		uint32_t m_ConstantBufferOffset = 0;

		std::string m_Name, m_AssetPath;
		std::unordered_map<GLenum, std::string> m_ShaderSource;

		std::vector<ShaderReloadedCallback> m_ShaderReloadedCallbacks;
		inline static std::unordered_map<uint32_t, ShaderUniformBufferH2M> s_UniformBuffers;

		std::unordered_map<std::string, ShaderBufferH2M> m_Buffers;
		std::unordered_map<std::string, ShaderResourceDeclarationH2M> m_Resources;
		std::unordered_map<std::string, GLint> m_UniformLocations;

	};

}
