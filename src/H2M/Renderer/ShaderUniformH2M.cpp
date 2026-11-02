#include "ShaderUniformH2M.h"


namespace H2M
{

	ShaderStructH2M::ShaderStructH2M(const std::string& name)
		: m_Name(name), m_Size(0), m_Offset(0)
	{
	}

	void ShaderStructH2M::AddField(ShaderUniformDeclarationH2M* field)
	{
		m_Size += field->GetSize();
		uint32_t offset = 0;
		if (m_Fields.size())
		{
			ShaderUniformDeclarationH2M* previous = m_Fields.back();
			offset = previous->GetOffset() + previous->GetSize();
		}
		field->SetOffset(offset);
		m_Fields.push_back(field);
	}

}
