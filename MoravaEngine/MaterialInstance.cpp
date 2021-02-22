#include "MaterialInstance.h"


MaterialInstance::MaterialInstance(const Hazel::Ref<Hazel::HazelMaterial>& material, const std::string& name)
	: Hazel::HazelMaterialInstance(material, name)
{
}

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::Bind()
{
	m_Material->GetShader()->Bind();
}
