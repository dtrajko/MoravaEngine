#include "EnvMapMaterial.h"

#include "EnvMapEditorLayer.h"
#include "Util.h"


EnvMapMaterial::EnvMapMaterial()
{
	m_Name = EnvMapEditorLayer::NewMaterialName();
	m_UUID = NewMaterialUUID();
}

EnvMapMaterial::EnvMapMaterial(std::string name)
{
	m_Name = name;
	m_UUID = NewMaterialUUID();
}

EnvMapMaterial::EnvMapMaterial(std::string name, EnvMapMaterial* other)
	: EnvMapMaterial(name)
{
	m_AlbedoInput = other->m_AlbedoInput;
	m_NormalInput = other->m_NormalInput;
	m_MetalnessInput = other->m_MetalnessInput;
	m_RoughnessInput = other->m_RoughnessInput;
	m_EmissiveInput = other->m_EmissiveInput;
	m_AOInput = other->m_AOInput;
	m_TilingFactor = other->m_TilingFactor;
}

MaterialUUID EnvMapMaterial::NewMaterialUUID()
{
	return Util::randomString(10);
}
