#include "Material/MaterialInstance.h"


MaterialInstance::MaterialInstance(const H2M::RefH2M<H2M::HazelMaterial>& material, const std::string& name)
	: H2M::HazelMaterial(material, name)
{
}

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::Bind()
{
	m_Material->GetShader()->Bind();
}
