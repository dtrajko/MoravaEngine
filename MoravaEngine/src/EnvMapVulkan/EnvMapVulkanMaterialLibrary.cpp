#include "EnvMapVulkanMaterialLibrary.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Log.h"
#include "Core/ResourceManager.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <random>


std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> EnvMapVulkanMaterialLibrary::s_Materials;
H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::s_DefaultMaterial;

static const char* s_MapTextureNames[EnvMapVulkanMaterial::MapCount] = {
	"u_AlbedoTexture", "u_NormalTexture", "u_MetalnessTexture", "u_RoughnessTexture", "u_EmissiveTexture", "u_AOTexture" };

static const char* s_MapToggleNames[EnvMapVulkanMaterial::MapCount] = {
	"u_MaterialUniforms.AlbedoTexToggle",
	"u_MaterialUniforms.NormalTexToggle",
	"u_MaterialUniforms.MetalnessTexToggle",
	"u_MaterialUniforms.RoughnessTexToggle",
	"u_MaterialUniforms.EmissiveTexToggle",
	"u_MaterialUniforms.AOTexToggle",
};

// The material values (the Material push constant block of the mesh shaders), copied by name on import and duplicate
static const char* s_FloatValueNames[] = {
	"u_MaterialUniforms.Metalness",
	"u_MaterialUniforms.Roughness",
	"u_MaterialUniforms.RadiancePrefilter",
	"u_MaterialUniforms.AlbedoTexToggle",
	"u_MaterialUniforms.NormalTexToggle",
	"u_MaterialUniforms.MetalnessTexToggle",
	"u_MaterialUniforms.RoughnessTexToggle",
	"u_MaterialUniforms.TilingFactor",
	"u_MaterialUniforms.EmissiveTexToggle",
	"u_MaterialUniforms.AOTexToggle",
	"u_MaterialUniforms.EmissiveIntensity",
	"u_MaterialUniforms.MetalRoughPacked",
};

// Library materials are created with the shared HazelPBR_Static shader (shader library), so their descriptor sets don't depend
// on any model's own shader instance. Set 1 is declared identically in HazelPBR_Anim, so skinned models can bind them too.
static H2M::RefH2M<H2M::VulkanShaderH2M> GetMaterialShader()
{
	return H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
}

const char* EnvMapVulkanMaterial::GetMapTextureName(uint32_t slot)
{
	return slot < MapCount ? s_MapTextureNames[slot] : "";
}

const char* EnvMapVulkanMaterial::GetMapToggleName(uint32_t slot)
{
	return slot < MapCount ? s_MapToggleNames[slot] : "";
}

// A random material ID (0 is "none")
static uint64_t NewMaterialID()
{
	static std::mt19937_64 s_Generator(std::random_device{}());
	uint64_t id = 0;
	while (id == 0)
	{
		id = s_Generator();
	}
	return id;
}

EnvMapVulkanMaterial::EnvMapVulkanMaterial(const std::string& name)
	: m_ID(NewMaterialID()), m_Name(name)
{
	H2M::RefH2M<H2M::ShaderH2M> shader = GetMaterialShader().As<H2M::ShaderH2M>();
	m_Values = H2M::RefH2M<H2M::VulkanMaterialH2M>::Create(shader, name);

	// Defaults (the storage starts zeroed: no maps, metalness 0)
	Get<glm::vec3>("u_MaterialUniforms.AlbedoColor") = glm::vec3(0.8f);
	Get<float>("u_MaterialUniforms.Roughness") = 0.5f;
	Get<float>("u_MaterialUniforms.TilingFactor") = 1.0f;
	Get<float>("u_MaterialUniforms.EmissiveIntensity") = 1.0f;

	// Every binding of the set must be written before the set is used
	for (uint32_t slot = 0; slot < MapCount; slot++)
	{
		WriteMapDescriptor(slot, H2M::RendererH2M::GetWhiteTexture());
	}
}

VkDescriptorSet EnvMapVulkanMaterial::GetDescriptorSet()
{
	const auto& descriptorSets = m_Values->GetDescriptorSet().DescriptorSets;
	return descriptorSets.empty() ? VK_NULL_HANDLE : descriptorSets[0];
}

void EnvMapVulkanMaterial::WriteMapDescriptor(uint32_t slot, H2M::RefH2M<H2M::Texture2D_H2M> texture)
{
	VkDescriptorSet descriptorSet = GetDescriptorSet();
	const VkWriteDescriptorSet* binding = GetMaterialShader()->GetDescriptorSet(s_MapTextureNames[slot], H2M::VulkanShaderH2M::MaterialDescriptorSet);
	if (descriptorSet == VK_NULL_HANDLE || !binding)
	{
		Log::GetLogger()->error("Material '{0}': '{1}' could not be written (no material descriptor set or binding).", m_Name, s_MapTextureNames[slot]);
		return;
	}

	VkWriteDescriptorSet write = *binding;
	write.dstSet = descriptorSet;
	write.descriptorCount = 1;
	write.pImageInfo = &texture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();

	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void EnvMapVulkanMaterial::SetMap(uint32_t slot, H2M::RefH2M<H2M::Texture2D_H2M> texture)
{
	if (slot >= MapCount || !texture)
	{
		return;
	}
	WriteMapDescriptor(slot, texture);
	m_Maps[slot] = texture;
	Get<float>(s_MapToggleNames[slot]) = 1.0f;
}

void EnvMapVulkanMaterial::RemoveMap(uint32_t slot)
{
	if (slot >= MapCount)
	{
		return;
	}
	WriteMapDescriptor(slot, H2M::RendererH2M::GetWhiteTexture());
	m_Maps[slot] = H2M::RefH2M<H2M::Texture2D_H2M>();
	Get<float>(s_MapToggleNames[slot]) = 0.0f;
}

H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::CreateMaterial(const std::string& name)
{
	H2M::RefH2M<EnvMapVulkanMaterial> material = H2M::RefH2M<EnvMapVulkanMaterial>::Create(MakeUniqueName(name));
	s_Materials.push_back(material);
	return material;
}

H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::Duplicate(H2M::RefH2M<EnvMapVulkanMaterial> source)
{
	H2M::RefH2M<EnvMapVulkanMaterial> material = CreateMaterial(source->GetName());

	material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor") = source->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
	for (const char* name : s_FloatValueNames)
	{
		material->Get<float>(name) = source->Get<float>(name);
	}
	for (uint32_t slot = 0; slot < EnvMapVulkanMaterial::MapCount; slot++)
	{
		if (source->HasMap(slot))
		{
			float toggle = source->Get<float>(s_MapToggleNames[slot]); // the map may be present but switched off
			material->SetMap(slot, source->GetMap(slot));
			material->Get<float>(s_MapToggleNames[slot]) = toggle;
		}
	}
	return material;
}

std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> EnvMapVulkanMaterialLibrary::ImportModelMaterials(H2M::RefH2M<H2M::ModelH2M> model)
{
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> result;
	H2M::Texture2D_H2M* whiteTexture = H2M::RendererH2M::GetWhiteTexture().Raw();

	auto& modelMaterials = model->GetMaterials();
	for (uint32_t i = 0; i < (uint32_t)modelMaterials.size(); i++)
	{
		// Already imported with an earlier copy of this model
		H2M::RefH2M<EnvMapVulkanMaterial> material;
		for (auto& existing : s_Materials)
		{
			std::error_code error;
			if (existing->m_SourceIndex == i && !existing->m_SourceFile.empty() &&
				std::filesystem::weakly_canonical(existing->m_SourceFile, error) == std::filesystem::weakly_canonical(model->GetFilePath(), error))
			{
				material = existing;
				break;
			}
		}
		if (material)
		{
			result.push_back(material);
			continue;
		}

		H2M::RefH2M<H2M::VulkanMaterialH2M> source = modelMaterials[i].As<H2M::VulkanMaterialH2M>();
		// Unnamed materials are named after the model ("boblampclean Material 2"), so they can be told apart in the library
		std::string name = !source->GetName().empty() ? source->GetName() :
			std::filesystem::path(model->GetFilePath()).stem().string() + " Material " + std::to_string(i);
		material = CreateMaterial(name);
		material->m_SourceFile = model->GetFilePath();
		material->m_SourceIndex = i;

		material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor") = source->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
		for (const char* valueName : s_FloatValueNames)
		{
			material->Get<float>(valueName) = source->Get<float>(valueName);
		}

		// The maps the model loader bound to the model's own material set (missing maps got the white placeholder)
		const H2M::ModelH2M::MaterialDescriptor* modelDescriptor = model->FindDescriptorSet(i);
		for (uint32_t slot = 0; slot < EnvMapVulkanMaterial::MapCount; slot++)
		{
			float toggle = material->Get<float>(s_MapToggleNames[slot]);
			H2M::RefH2M<H2M::Texture2D_H2M> texture;
			if (modelDescriptor)
			{
				auto textureIt = modelDescriptor->Textures.find(s_MapTextureNames[slot]);
				if (textureIt != modelDescriptor->Textures.end()) texture = textureIt->second;
			}
			if (texture && texture.Raw() != whiteTexture)
			{
				material->SetMap(slot, texture);
				material->Get<float>(s_MapToggleNames[slot]) = toggle;
			}
			else
			{
				material->Get<float>(s_MapToggleNames[slot]) = 0.0f; // nothing to sample
			}
		}

		result.push_back(material);
	}

	return result;
}

H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::GetDefaultMaterial()
{
	if (!s_DefaultMaterial)
	{
		s_DefaultMaterial = CreateMaterial("Default");
	}
	return s_DefaultMaterial;
}

void EnvMapVulkanMaterialLibrary::Remove(H2M::RefH2M<EnvMapVulkanMaterial> material)
{
	s_Materials.erase(std::remove(s_Materials.begin(), s_Materials.end(), material), s_Materials.end());
	if (s_DefaultMaterial == material)
	{
		s_DefaultMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
	}
}

void EnvMapVulkanMaterialLibrary::Clear()
{
	s_Materials.clear();
	s_DefaultMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
}

void EnvMapVulkanMaterialLibrary::Rename(H2M::RefH2M<EnvMapVulkanMaterial> material, const std::string& name)
{
	size_t first = name.find_first_not_of(" \t");
	if (!material || first == std::string::npos)
	{
		return;
	}
	std::string trimmed = name.substr(first, name.find_last_not_of(" \t") - first + 1);
	if (trimmed != material->GetName())
	{
		material->SetName(MakeUniqueName(trimmed));
	}
}

std::string EnvMapVulkanMaterialLibrary::MakeUniqueName(const std::string& name)
{
	auto taken = [](const std::string& candidate) {
		for (auto& material : s_Materials)
		{
			if (material->GetName() == candidate) return true;
		}
		return false;
	};

	if (!taken(name))
	{
		return name;
	}
	for (uint32_t n = 2; ; n++)
	{
		std::string candidate = name + " (" + std::to_string(n) + ")";
		if (!taken(candidate))
		{
			return candidate;
		}
	}
}

// Material files

static const char* s_MapKeys[EnvMapVulkanMaterial::MapCount] = { "Albedo", "Normal", "Metalness", "Roughness", "Emissive", "AmbientOcclusion" };

// The values in a file, by key (the uniform without "u_MaterialUniforms."); the map toggles are kept with the maps
struct MaterialValueKey
{
	const char* Key;
	const char* Uniform;
};
static const MaterialValueKey s_ValueKeys[] = {
	{ "Metalness",         "u_MaterialUniforms.Metalness" },
	{ "Roughness",         "u_MaterialUniforms.Roughness" },
	{ "TilingFactor",      "u_MaterialUniforms.TilingFactor" },
	{ "EmissiveIntensity", "u_MaterialUniforms.EmissiveIntensity" },
	{ "MetalRoughPacked",  "u_MaterialUniforms.MetalRoughPacked" },
	{ "RadiancePrefilter", "u_MaterialUniforms.RadiancePrefilter" },
};

// A path as stored in a file: relative to the working directory (the project folder) when it is inside it, with forward slashes
static std::string ToStoredPath(const std::string& path)
{
	if (path.empty())
	{
		return path;
	}
	std::error_code error;
	std::filesystem::path absolute = std::filesystem::weakly_canonical(path, error);
	if (error)
	{
		return std::filesystem::path(path).generic_string();
	}
	std::filesystem::path relative = absolute.lexically_relative(std::filesystem::current_path(error));
	std::filesystem::path stored = (!relative.empty() && *relative.begin() != "..") ? relative : absolute;
	return stored.generic_string();
}

static uint64_t HashBytes(uint64_t hash, const void* data, size_t size)
{
	const unsigned char* bytes = (const unsigned char*)data;
	for (size_t i = 0; i < size; i++)
	{
		hash = (hash ^ bytes[i]) * 1099511628211ull; // FNV-1a
	}
	return hash;
}

uint64_t EnvMapVulkanMaterial::ComputeContentHash() const
{
	// The values are read through the material's uniform storage, which has no const access
	EnvMapVulkanMaterial& self = const_cast<EnvMapVulkanMaterial&>(*this);
	uint64_t hash = 14695981039346656037ull;
	hash = HashBytes(hash, m_Name.data(), m_Name.size());
	const glm::vec3 albedo = self.Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
	hash = HashBytes(hash, &albedo, sizeof(albedo));
	for (const char* name : s_FloatValueNames)
	{
		float value = self.Get<float>(name);
		hash = HashBytes(hash, &value, sizeof(value));
	}
	for (uint32_t slot = 0; slot < MapCount; slot++)
	{
		std::string path = m_Maps[slot] ? m_Maps[slot]->GetPath() : std::string();
		hash = HashBytes(hash, &slot, sizeof(slot));
		hash = HashBytes(hash, path.data(), path.size());
	}
	return hash;
}

// Whether a material file is named after the material: <name>.mmat, or <name>_2.mmat... (taken when it was saved)
static bool FileFollowsName(const std::string& filepath, const std::string& materialName)
{
	const std::string wanted = EnvMapVulkanMaterialLibrary::MakeFileName(materialName, "Material");
	const std::string stem = std::filesystem::path(filepath).stem().string();
	if (stem == wanted)
	{
		return true;
	}
	if (stem.size() <= wanted.size() + 1 || stem.compare(0, wanted.size() + 1, wanted + "_") != 0)
	{
		return false;
	}
	const std::string suffix = stem.substr(wanted.size() + 1);
	return std::all_of(suffix.begin(), suffix.end(), [](char c) { return std::isdigit((unsigned char)c) != 0; });
}

bool EnvMapVulkanMaterial::HasUnsavedChanges() const
{
	// Never saved, changed since, or its file is still named after an earlier name (Save renames it)
	return m_FilePath.empty() || ComputeContentHash() != m_SavedHash || !FileFollowsName(m_FilePath, m_Name);
}

H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::FindByID(uint64_t id)
{
	for (auto& material : s_Materials)
	{
		if (material->m_ID == id)
		{
			return material;
		}
	}
	return H2M::RefH2M<EnvMapVulkanMaterial>();
}

std::string EnvMapVulkanMaterialLibrary::MakeFileName(const std::string& name, const std::string& fallback)
{
	std::string result;
	bool pendingSeparator = false;
	for (char c : name)
	{
		if (!(std::isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.'))
		{
			// A space or a character not allowed: a run of them is one '_' (none at the start or the end)
			pendingSeparator = !result.empty();
			continue;
		}
		if (pendingSeparator)
		{
			result += '_';
			pendingSeparator = false;
		}
		result += c;
	}
	return result.empty() ? fallback : result;
}

bool EnvMapVulkanMaterialLibrary::Save(H2M::RefH2M<EnvMapVulkanMaterial> material, const std::string& filepath)
{
	if (!material)
	{
		return false;
	}
	const std::string wantedName = MakeFileName(material->m_Name, "Material");
	std::string path = filepath.empty() ? material->m_FilePath : filepath;
	std::string folder = MaterialsFolder;
	std::string previousFile; // the material's file, replaced by a file with its new name (renamed material)
	if (!filepath.empty())
	{
		// A file name chosen in a dialog follows the same rule as the generated ones (the folder is kept as it is)
		std::filesystem::path chosen(filepath);
		chosen.replace_filename(MakeFileName(chosen.stem().string(), "Material") + chosen.extension().string());
		path = chosen.string();
	}
	else if (!path.empty())
	{
		// The file follows the material's name: a renamed material moves to <new name>.mmat in the same folder
		if (!FileFollowsName(path, material->m_Name))
		{
			previousFile = path;
			folder = std::filesystem::path(path).parent_path().generic_string();
			path.clear();
		}
	}
	if (path.empty())
	{
		// A file of its own: <name>.mmat in the materials folder (or the folder of its previous file), <name>_2.mmat...
		// when that one is taken
		const std::string base = (folder.empty() ? std::string(".") : folder) + "/" + wantedName;
		path = base + FileExtension;
		for (uint32_t n = 2; ; n++)
		{
			std::error_code existsError;
			// The material's own previous file doesn't count (a rename that only changes upper / lower case)
			bool taken = std::filesystem::exists(path) &&
				!(!previousFile.empty() && std::filesystem::exists(previousFile) && std::filesystem::equivalent(path, previousFile, existsError));
			for (auto& other : s_Materials)
			{
				taken |= other != material && !other->m_FilePath.empty() && std::filesystem::path(other->m_FilePath) == std::filesystem::path(path);
			}
			if (!taken)
			{
				break;
			}
			path = base + "_" + std::to_string(n) + FileExtension;
		}
	}

	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Version" << YAML::Value << 1;
	out << YAML::Key << "ID" << YAML::Value << material->m_ID;
	out << YAML::Key << "Name" << YAML::Value << material->m_Name;
	if (!material->m_SourceFile.empty())
	{
		out << YAML::Key << "Source" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "File" << YAML::Value << ToStoredPath(material->m_SourceFile);
		out << YAML::Key << "Index" << YAML::Value << material->m_SourceIndex;
		out << YAML::EndMap;
	}
	const glm::vec3 albedo = material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
	out << YAML::Key << "AlbedoColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << albedo.r << albedo.g << albedo.b << YAML::EndSeq;
	for (const MaterialValueKey& value : s_ValueKeys)
	{
		out << YAML::Key << value.Key << YAML::Value << material->Get<float>(value.Uniform);
	}
	out << YAML::Key << "Maps" << YAML::Value << YAML::BeginMap;
	for (uint32_t slot = 0; slot < EnvMapVulkanMaterial::MapCount; slot++)
	{
		H2M::RefH2M<H2M::Texture2D_H2M> map = material->m_Maps[slot];
		if (!map)
		{
			continue;
		}
		if (map->GetPath().empty())
		{
			Log::GetLogger()->warn("Material '{0}': its {1} map has no file (embedded in the model?) and isn't saved", material->m_Name, s_MapKeys[slot]);
			continue;
		}
		out << YAML::Key << s_MapKeys[slot] << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "File" << YAML::Value << ToStoredPath(map->GetPath());
		out << YAML::Key << "Enabled" << YAML::Value << (material->Get<float>(s_MapToggleNames[slot]) > 0.5f);
		out << YAML::EndMap;
	}
	out << YAML::EndMap; // Maps
	out << YAML::EndMap; // Material
	out << YAML::EndMap;

	std::error_code error;
	std::filesystem::path parent = std::filesystem::path(path).parent_path();
	if (!parent.empty())
	{
		std::filesystem::create_directories(parent, error);
	}
	std::ofstream file(path);
	if (!file)
	{
		Log::GetLogger()->error("Material '{0}' could not be saved: '{1}' can't be written", material->m_Name, path);
		return false;
	}
	file << out.c_str() << "\n";
	file.close();

	// A renamed material: its old file goes (the new one has everything)
	if (!previousFile.empty() && std::filesystem::exists(previousFile) && !std::filesystem::equivalent(previousFile, path, error))
	{
		std::filesystem::remove(previousFile, error);
		Log::GetLogger()->info("Material '{0}': its previous file '{1}' was removed (renamed)", material->m_Name, previousFile);
	}

	material->m_FilePath = ToStoredPath(path);
	material->m_SavedHash = material->ComputeContentHash();
	Log::GetLogger()->info("Material '{0}' saved to '{1}'", material->m_Name, material->m_FilePath);
	return true;
}

H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::Load(const std::string& filepath)
{
	YAML::Node root;
	try
	{
		root = YAML::LoadFile(filepath);
	}
	catch (const std::exception& e)
	{
		Log::GetLogger()->error("Material file '{0}' could not be read: {1}", filepath, e.what());
		return H2M::RefH2M<EnvMapVulkanMaterial>();
	}
	YAML::Node node = root["Material"];
	if (!node || !node["ID"])
	{
		Log::GetLogger()->error("'{0}' is not a material file (it has no Material with an ID)", filepath);
		return H2M::RefH2M<EnvMapVulkanMaterial>();
	}

	const uint64_t id = node["ID"].as<uint64_t>();
	const std::string name = node["Name"] ? node["Name"].as<std::string>() : std::filesystem::path(filepath).stem().string();

	// The library's material with this ID (reloaded in place), or a new one
	H2M::RefH2M<EnvMapVulkanMaterial> material = FindByID(id);
	if (material)
	{
		if (material->m_Name != name)
		{
			material->m_Name.clear(); // the material's own name doesn't count as taken
			material->m_Name = MakeUniqueName(name);
		}
	}
	else
	{
		material = CreateMaterial(name);
		material->m_ID = id;
	}

	if (YAML::Node source = node["Source"])
	{
		material->m_SourceFile = source["File"].as<std::string>("");
		material->m_SourceIndex = source["Index"].as<uint32_t>(0);
	}
	YAML::Node albedo = node["AlbedoColor"];
	if (albedo && albedo.IsSequence() && albedo.size() == 3)
	{
		material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor") = glm::vec3(albedo[0].as<float>(), albedo[1].as<float>(), albedo[2].as<float>());
	}
	for (const MaterialValueKey& value : s_ValueKeys)
	{
		if (node[value.Key])
		{
			material->Get<float>(value.Uniform) = node[value.Key].as<float>();
		}
	}

	YAML::Node maps = node["Maps"];
	for (uint32_t slot = 0; slot < EnvMapVulkanMaterial::MapCount; slot++)
	{
		YAML::Node map = maps ? maps[s_MapKeys[slot]] : YAML::Node();
		const std::string mapFile = map ? map["File"].as<std::string>("") : std::string();
		H2M::RefH2M<H2M::Texture2D_H2M> texture;
		if (!mapFile.empty())
		{
			if (std::filesystem::exists(mapFile))
			{
				// Through the texture cache: an image already loaded in this slot's color space is shared
				texture = ResourceManager::LoadTexture2D_H2M(mapFile, EnvMapVulkanMaterial::IsColorMap(slot));
			}
			if (!texture || !texture->Loaded())
			{
				Log::GetLogger()->warn("Material '{0}': its {1} map '{2}' could not be loaded", material->m_Name, s_MapKeys[slot], mapFile);
				texture = H2M::RefH2M<H2M::Texture2D_H2M>();
			}
		}
		if (texture)
		{
			material->SetMap(slot, texture);
			material->Get<float>(s_MapToggleNames[slot]) = map["Enabled"].as<bool>(true) ? 1.0f : 0.0f;
		}
		else if (material->HasMap(slot))
		{
			material->RemoveMap(slot);
		}
		else
		{
			material->Get<float>(s_MapToggleNames[slot]) = 0.0f;
		}
	}

	material->m_FilePath = ToStoredPath(filepath);
	material->m_SavedHash = material->ComputeContentHash();
	return material;
}

uint32_t EnvMapVulkanMaterialLibrary::LoadFolder(const std::string& folder)
{
	std::error_code error;
	if (!std::filesystem::is_directory(folder, error))
	{
		return 0;
	}
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator(folder, error))
	{
		if (entry.is_regular_file() && entry.path().extension() == FileExtension)
		{
			files.push_back(entry.path());
		}
	}
	std::sort(files.begin(), files.end()); // a stable order in the library
	uint32_t loaded = 0;
	for (const auto& file : files)
	{
		loaded += Load(file.string()) ? 1 : 0;
	}
	return loaded;
}
