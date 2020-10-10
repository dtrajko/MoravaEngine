#pragma once

#include "ShaderUniform.h"


namespace Hazel {


	ShaderStruct::ShaderStruct(const std::string& name)
		: m_Name(name), m_Size(0), m_Offset(0)
	{
	}

	void ShaderStruct::AddField(ShaderUniformDeclaration* field)
	{
		m_Size += field->GetSize();
		uint32_t offset = 0;
		if (m_Fields.size())
		{
			ShaderUniformDeclaration* previous = m_Fields.back();
			offset = previous->GetOffset() + previous->GetSize();
		}
		field->SetOffset(offset);
		m_Fields.push_back(field);
	}

}
