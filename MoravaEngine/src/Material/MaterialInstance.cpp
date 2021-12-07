#include "Material/MaterialInstance.h"


MaterialInstance::MaterialInstance(const H2M::RefH2M<H2M::MaterialH2M>& material, const std::string& name)
	: H2M::MaterialH2M(material, name)
{
}

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::Bind()
{
	m_Material->GetShader()->Bind();
}
