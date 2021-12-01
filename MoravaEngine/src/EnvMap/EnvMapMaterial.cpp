#include "EnvMapMaterial.h"

#include "Core/Util.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Material/MaterialLibrary.h"


EnvMapMaterial::EnvMapMaterial()
{
	m_Name = MaterialLibrary::NewMaterialName();
	m_UUID = NewMaterialUUID();
}

EnvMapMaterial::EnvMapMaterial(std::string name)
{
	m_Name = name;
	m_UUID = NewMaterialUUID();
}

EnvMapMaterial::EnvMapMaterial(std::string name, H2M::RefH2M<EnvMapMaterial> other)
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
