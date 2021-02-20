#pragma once

#include "../Core/Base.h"
#include "../../Log.h"

#include <string>
#include <vector>

namespace Hazel {


	enum class ShaderDomain
	{
		None = 0, Vertex = 0, Pixel = 1
	};

	class ShaderUniformDeclaration
	{
	private:
		friend class HazelShader;
		friend class OpenGLShader;
		friend class ShaderStruct;

	public:
		virtual const std::string& GetName() const = 0;
		virtual uint32_t GetSize() const = 0;
		virtual uint32_t GetCount() const = 0;
		virtual uint32_t GetOffset() const = 0;
		virtual ShaderDomain GetDomain() const = 0;

	protected:
		virtual void SetOffset(uint32_t offset) = 0;

	};

	typedef std::vector<ShaderUniformDeclaration*> ShaderUniformList;


	class ShaderUniformBufferDeclaration
	{
	public:
		virtual const std::string& GetName() const = 0;
		virtual uint32_t GetRegister() const = 0;
		virtual uint32_t GetSize() const = 0;
		virtual const ShaderUniformList& GetUniformDeclarations() const = 0;
		virtual ShaderUniformDeclaration* FindUniform(const std::string& name) = 0;
	};

	typedef std::vector<ShaderUniformBufferDeclaration*> ShaderUniformBufferList;


	class ShaderStruct
	{
	private:
		friend class HazelShader;

	private:
		std::string m_Name;
		std::vector<ShaderUniformDeclaration*> m_Fields;
		uint32_t m_Size;
		uint32_t m_Offset;

	public:
		ShaderStruct(const std::string& name);

		void AddField(ShaderUniformDeclaration* field);

		inline void SetOffset(uint32_t offset) { m_Offset = offset; }
		inline const std::string& GetName() const { return m_Name; }
		inline uint32_t GetSize() const { return m_Size; }
		inline uint32_t GetOffset() const { return m_Offset; }
		inline const std::vector<ShaderUniformDeclaration*>& GetFields() const { return m_Fields; }
	};

	typedef std::vector<ShaderStruct*> ShaderStructList;

	class ShaderResourceDeclaration
	{
	public:
		ShaderResourceDeclaration() = default;
		ShaderResourceDeclaration(const std::string& name, uint32_t resourceRegister, uint32_t count)
			: m_Name(name), m_Register(resourceRegister), m_Count(count) {}

		virtual const std::string& GetName() const { return m_Name; }
		virtual uint32_t GetRegister() const { return m_Register; }
		virtual uint32_t GetCount() const { return m_Count; }
	private:
		std::string m_Name;
		uint32_t m_Register = 0;
		uint32_t m_Count = 0;
	};

	typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;

}
