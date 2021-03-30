#include "Material/MaterialInstance.h"


MaterialInstance::MaterialInstance(const Hazel::Ref<Hazel::HazelMaterial>& material, const std::string& name)
	: Hazel::HazelMaterial(material, name)
{
}

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::Bind()
{
	m_Material->GetShader()->Bind();
}
