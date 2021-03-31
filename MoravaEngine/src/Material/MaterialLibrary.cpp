#include "MaterialLibrary.h"

#include "EnvMap/EnvMapEditorLayer.h"

#include <map>


std::vector<Hazel::Ref<MaterialData>> MaterialLibrary::s_MaterialData;
std::map<MaterialUUID, Hazel::Ref<EnvMapMaterial>> MaterialLibrary::s_EnvMapMaterials; // MaterialUUID, EnvMapMaterial*
std::map<SubmeshUUID, MaterialUUID> MaterialLibrary::s_SubmeshMaterialUUIDs; // SubmeshUUID, MaterialUUID

Hazel::Ref<MaterialData> MaterialLibrary::AddNew()
{
	Hazel::Ref<MaterialData> materialData = Hazel::Ref<MaterialData>::Create();
	s_MaterialData.push_back(materialData);



	return materialData;
}

void MaterialLibrary::AddEnvMapMaterial(MaterialUUID UUID, Hazel::Ref<EnvMapMaterial> envMapMaterial)
{
	if (s_EnvMapMaterials.find(envMapMaterial->GetName()) != s_EnvMapMaterials.end())
	{
		Log::GetLogger()->warn("Material name '{0}' already taken!", envMapMaterial->GetName());
		return;
	}

	s_EnvMapMaterials.insert(std::make_pair(UUID, envMapMaterial));
}

void MaterialLibrary::Cleanup()
{
	s_EnvMapMaterials.clear();
	s_MaterialData.clear();
	s_SubmeshMaterialUUIDs.clear();
}

void MaterialLibrary::RenameMaterial(Hazel::Ref<EnvMapMaterial> envMapMaterial, std::string newName)
{
    Log::GetLogger()->debug("MaterialLibrary::RenameMaterial from '{0}' to '{1}'", envMapMaterial->GetName(), newName);

    std::string oldName = envMapMaterial->GetName();
    MaterialUUID materialUUID = envMapMaterial->GetUUID();

    // TODO: Make sure that the new material name is not already taken
    for (auto emm_it = s_EnvMapMaterials.begin(); emm_it != s_EnvMapMaterials.end(); emm_it++) {
        if (emm_it->second->GetName() == newName) {
            Log::GetLogger()->error("Material name is already taken [Name: '{0}', UUID: '{1}']!", newName, emm_it->second->GetUUID());
            return;
        }
    }

    // TODO: Rename object attribute
    envMapMaterial->SetName(newName);

    // TODO: Rename in s_EnvMapMaterials
    for (auto emm_it = s_EnvMapMaterials.begin(); emm_it != s_EnvMapMaterials.end();) {
        if (emm_it->second->GetUUID() == materialUUID) {
            emm_it->second->SetName(newName);
            Log::GetLogger()->error("s_EnvMapMaterials: '{0}' => '{1}'", oldName, newName);
            break;
        }
        else {
            ++emm_it;
        }
    }
}

void MaterialLibrary::AddSubmeshMaterialRelation(SubmeshUUID submeshUUID, MaterialUUID materialUUID)
{
    s_SubmeshMaterialUUIDs.insert(std::make_pair(submeshUUID, materialUUID));
}
