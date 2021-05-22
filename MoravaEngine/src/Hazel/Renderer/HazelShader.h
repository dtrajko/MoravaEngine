#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Buffer.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/RendererTypes.h"
#include "Hazel/Renderer/ShaderUniform.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <functional>


namespace Hazel
{
	enum class UniformType
	{
		None = 0,
		Float, Float2, Float3, Float4,
		Matrix3x3, Matrix4x4,
		Int32, Uint32
	};

	struct UniformDecl
	{
		UniformType Type;
		std::ptrdiff_t Offset;
		std::string Name;
	};

	struct UniformBuffer
	{
		// TODO: This currently represents a byte buffer that has been
		// packed with uniforms. This was primarily created for OpenGL,
		// and needs to be revisted for other rendering APIs. Furthermore,
		// this currently does not assume any alignment. This also has
		// nothing to do with GL uniform buffers, this is simply a CPU-side
		// buffer abstraction.
		uint8_t* Buffer;
		std::vector<UniformDecl> Uniforms;
	};

	struct UniformBufferBase
	{
		virtual const uint8_t* GetBuffer() const = 0;
		virtual const UniformDecl* GetUniforms() const = 0;
		virtual unsigned int GetUniformCount() const = 0;
	};

	template<unsigned int N, unsigned int U>
	struct UniformBufferDeclaration : public UniformBufferBase
	{
		uint8_t Buffer[N];
		UniformDecl Uniforms[U];
		std::ptrdiff_t Cursor = 0;
		int Index = 0;

		virtual const uint8_t* GetBuffer() const override { return Buffer; }
		virtual const UniformDecl* GetUniforms() const override { return Uniforms; }
		virtual unsigned int GetUniformCount() const { return U; }

		template<typename T>
		void Push(const std::string& name, const T& data) {}

		template<typename T>
		void Push(const std::string& name, const float& data)
		{
			Uniforms[Index++] = { UniformType::Float, Cursor, name };
			memcpy(Buffer + Cursor, &data, sizeof(float));
			Cursor += sizeof(float);
		}

		template<typename T>
		void Push(const std::string& name, const glm::vec3& data)
		{
			Uniforms[Index++] = { UniformType::Float3, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::vec3));
			Cursor += sizeof(glm::vec3);
		}

		template<typename T>
		void Push(const std::string& name, const glm::vec4& data)
		{
			Uniforms[Index++] = { UniformType::Float4, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::vec4));
			Cursor += sizeof(glm::vec4);
		}

		template<typename T>
		void Push(const std::string& name, const glm::mat4& data)
		{
			Uniforms[Index++] = { UniformType::Matrix4x4, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::mat4));
			Cursor += sizeof(glm::mat4);
		}

	};

	enum class ShaderUniformType
	{
		None = 0, Bool, Int, UInt, Float, Vec2, Vec3, Vec4, Mat3, Mat4
	};

	class ShaderUniform
	{
	public:
		ShaderUniform() = default;
		ShaderUniform(const std::string& name, ShaderUniformType type, uint32_t size, uint32_t offset);

		const std::string& GetName() const { return m_Name; }
		ShaderUniformType GetType() const { return m_Type; }
		uint32_t GetSize() const { return m_Size; }
		uint32_t GetOffset() const { return m_Offset; }

		static const std::string& UniformTypeToString(ShaderUniformType type);

	private:
		std::string m_Name;
		ShaderUniformType m_Type = ShaderUniformType::None;
		uint32_t m_Size = 0;
		uint32_t m_Offset = 0;
	};

	struct ShaderUniformBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		std::vector<ShaderUniform> Uniforms;
	};

	struct ShaderBuffer
	{
		std::string Name;
		uint32_t Size = 0;
		std::unordered_map<std::string, ShaderUniform> Uniforms;
	};

	class HazelShader : public RefCounted
	{
	public:
		using ShaderReloadedCallback = std::function<void()>;

		virtual void Reload(bool forceCompile = false) = 0;

		virtual void Bind() = 0;
		virtual RendererID GetRendererID() const = 0;

		virtual size_t GetHash() const = 0;

		// NEW shader system
		virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) = 0;

		virtual void SetUniform(const std::string& fullname, float value) = 0;
		virtual void SetUniform(const std::string& fullname, uint32_t value) = 0;
		virtual void SetUniform(const std::string& fullname, int value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec2& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec3& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec4& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::mat3& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::mat4& value) = 0;

		// Temporary while we don't have materials
		virtual void SetFloat(const std::string& name, float value) = 0;
		virtual void SetUInt(const std::string& name, uint32_t value) = 0;
		virtual void SetInt(const std::string& name, int value) = 0;
		// virtual void SetBool(const std::string& name, bool value) = 0;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;
		virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) = 0;

		virtual void SetIntArray(const std::string& name, int* values, uint32_t size) = 0;

		virtual const std::string& GetName() const = 0;

		// Represents a complete shader program stored in a single file.
		// Note: currently for simplicity this is simply a string filepath, however
		//       in the future this will be an asset object + metadata
		static Ref<HazelShader> Create(const std::string& filepath, bool forceCompile = false);
		static Ref<HazelShader> CreateFromString(const std::string& source);

		virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const = 0;

		virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const = 0;

		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) = 0;

		// Temporary, before we have an asset manager
		static std::vector<Ref<HazelShader>> s_AllShaders;

		// Methods from Vulkan Week Day 1
		bool HasVSMaterialUniformBuffer();
		bool HasPSMaterialUniformBuffer();
		Buffer GetVSMaterialUniformBuffer();
		Buffer GetPSMaterialUniformBuffer();
	};

	// This should be eventually handled by the Asset Manager
	class HazelShaderLibrary : public RefCounted
	{
	public:
		HazelShaderLibrary();
		~HazelShaderLibrary();

		void Add(const Ref<HazelShader>& shader);
		void Load(const std::string& path, bool forceCompile = false);
		void Load(const std::string& name, const std::string& path);

		Ref<HazelShader>& Get(const std::string& name);

	private:
		std::unordered_map<std::string, Ref<HazelShader>> m_Shaders = std::unordered_map<std::string, Ref<HazelShader>>();

	};

}
