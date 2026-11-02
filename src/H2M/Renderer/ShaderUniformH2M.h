#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/RefH2M.h"

#include "Core/Log.h"

#include <string>
#include <vector>


namespace H2M
{

	enum class ShaderDomainH2M
	{
		None = 0, Vertex = 0, Pixel = 1
	};

	class ShaderUniformDeclarationH2M
	{
	private:
		friend class ShaderH2M;
		friend class OpenGLShaderH2M;
		friend class ShaderStructH2M;

	public:
		virtual const std::string& GetName() const = 0;
		virtual uint32_t GetSize() const = 0;
		virtual uint32_t GetCount() const = 0;
		virtual uint32_t GetOffset() const = 0;
		virtual ShaderDomainH2M GetDomain() const = 0;

	protected:
		virtual void SetOffset(uint32_t offset) = 0;

	};

	typedef std::vector<ShaderUniformDeclarationH2M*> ShaderUniformList;


	class ShaderUniformBufferDeclarationH2M : public RefCountedH2M
	{
	public:
		virtual const std::string& GetName() const = 0;
		virtual uint32_t GetRegister() const = 0;
		virtual uint32_t GetSize() const = 0;
		virtual const ShaderUniformList& GetUniformDeclarations() const = 0;
		virtual ShaderUniformDeclarationH2M* FindUniform(const std::string& name) = 0;
	};

	typedef std::vector<ShaderUniformBufferDeclarationH2M*> ShaderUniformBufferList;


	class ShaderStructH2M
	{
	private:
		friend class ShaderH2M;

	private:
		std::string m_Name;
		std::vector<ShaderUniformDeclarationH2M*> m_Fields;
		uint32_t m_Size;
		uint32_t m_Offset;

	public:
		ShaderStructH2M(const std::string& name);

		void AddField(ShaderUniformDeclarationH2M* field);

		inline void SetOffset(uint32_t offset) { m_Offset = offset; }
		inline const std::string& GetName() const { return m_Name; }
		inline uint32_t GetSize() const { return m_Size; }
		inline uint32_t GetOffset() const { return m_Offset; }
		inline const std::vector<ShaderUniformDeclarationH2M*>& GetFields() const { return m_Fields; }
	};

	typedef std::vector<ShaderStructH2M*> ShaderStructList;

	class ShaderResourceDeclarationH2M
	{
	public:
		ShaderResourceDeclarationH2M() = default;
		ShaderResourceDeclarationH2M(const std::string& name, uint32_t resourceRegister, uint32_t count)
			: m_Name(name), m_Register(resourceRegister), m_Count(count) {}

		virtual const std::string& GetName() const { return m_Name; }
		virtual uint32_t GetRegister() const { return m_Register; }
		virtual uint32_t GetCount() const { return m_Count; }
	private:
		std::string m_Name;
		uint32_t m_Register = 0;
		uint32_t m_Count = 0;
	};

	typedef std::vector<ShaderResourceDeclarationH2M*> ShaderResourceList;

}
