#include "MaterialHazelVulkan.h"

#include "Core/Util.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Material/MaterialLibrary.h"


MaterialHazelVulkan::MaterialHazelVulkan()
{
	m_Name = MaterialLibrary::NewMaterialName();
	m_UUID = NewMaterialUUID();
}

MaterialHazelVulkan::MaterialHazelVulkan(std::string name)
{
	m_Name = name;
	m_UUID = NewMaterialUUID();
}

MaterialHazelVulkan::MaterialHazelVulkan(std::string name, Hazel::Ref<MaterialHazelVulkan> other)
	: MaterialHazelVulkan(name)
{
	m_AlbedoInput = other->m_AlbedoInput;
	m_NormalInput = other->m_NormalInput;
	m_MetalnessInput = other->m_MetalnessInput;
	m_RoughnessInput = other->m_RoughnessInput;
	m_EmissiveInput = other->m_EmissiveInput;
	m_AOInput = other->m_AOInput;
	m_TilingFactor = other->m_TilingFactor;
}

MaterialUUID MaterialHazelVulkan::NewMaterialUUID()
{
	return Util::randomString(10);
}
