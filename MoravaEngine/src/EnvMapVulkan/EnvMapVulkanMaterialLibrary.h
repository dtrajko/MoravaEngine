#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanMaterialH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include <array>
#include <string>
#include <vector>


/**
 * A material of the Vulkan Material Library (SceneEnvMapVulkan). It exists independently of any model and can be used by
 * any number of submeshes of any meshes:
 * - values (albedo color, metalness, roughness, toggles...): the uniform storage of a VulkanMaterialH2M, pushed as push constants
 * - texture maps: the material's own descriptor set, set 1 of the mesh shaders (VulkanShaderH2M::MaterialDescriptorSet)
 */
class EnvMapVulkanMaterial : public H2M::RefCountedH2M
{
public:
	// Material map slots, in the order of the set 1 bindings in HazelPBR_Static.glsl / HazelPBR_Anim.glsl
	enum Map : uint32_t { Albedo = 0, Normal, Metalness, Roughness, Emissive, AmbientOcclusion, MapCount };

	static const char* GetMapTextureName(uint32_t slot); // "u_AlbedoTexture"...
	static const char* GetMapToggleName(uint32_t slot);  // "u_MaterialUniforms.AlbedoTexToggle"...
	static bool IsColorMap(uint32_t slot) { return slot == Albedo || slot == Emissive; } // sRGB data; the others are linear

	EnvMapVulkanMaterial(const std::string& name);

	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }

	// Values, edited in place by the Material Editor (e.g. Get<float>("u_MaterialUniforms.Roughness"))
	template<typename T>
	T& Get(const std::string& name) { return m_Values->Get<T>(name); }
	H2M::BufferH2M GetUniformStorageBuffer() { return m_Values->GetUniformStorageBuffer(); }

	VkDescriptorSet GetDescriptorSet();

	// A map is "present" when a texture is bound to its slot; whether the shader samples it is the slot's toggle value
	bool HasMap(uint32_t slot) const { return slot < MapCount && m_Maps[slot]; }
	H2M::RefH2M<H2M::Texture2D_H2M> GetMap(uint32_t slot) const { return slot < MapCount ? m_Maps[slot] : H2M::RefH2M<H2M::Texture2D_H2M>(); }
	// Binds a texture to the slot and turns its toggle on. The descriptor set may be in use by frames in flight:
	// call these between frames (the renderer applies them at the start of Draw)
	void SetMap(uint32_t slot, H2M::RefH2M<H2M::Texture2D_H2M> texture);
	// Binds the white placeholder and turns the toggle off: the shader uses the material's value instead
	void RemoveMap(uint32_t slot);

	// Where an imported material came from (empty for materials created in the editor)
	const std::string& GetSourceFile() const { return m_SourceFile; }
	uint32_t GetSourceIndex() const { return m_SourceIndex; }

private:
	void WriteMapDescriptor(uint32_t slot, H2M::RefH2M<H2M::Texture2D_H2M> texture);

private:
	std::string m_Name;
	H2M::RefH2M<H2M::VulkanMaterialH2M> m_Values; // created with the shared HazelPBR_Static shader; also owns the material descriptor set
	std::array<H2M::RefH2M<H2M::Texture2D_H2M>, MapCount> m_Maps; // keeps the bound images alive (null: placeholder bound)

	std::string m_SourceFile;
	uint32_t m_SourceIndex = 0;

	friend class EnvMapVulkanMaterialLibrary;
};

/**
 * All materials of the scene, in creation order. Materials loaded with a model are imported once per model file and
 * material index: loading the same model again reuses them (as a model asset's materials in Unity or Unreal).
 */
class EnvMapVulkanMaterialLibrary
{
public:
	static const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& GetMaterials() { return s_Materials; }

	// A new material with default values and no maps ("New Material")
	static H2M::RefH2M<EnvMapVulkanMaterial> CreateMaterial(const std::string& name = "New Material");
	// A copy with the same values and maps, independent of the original ("Duplicate")
	static H2M::RefH2M<EnvMapVulkanMaterial> Duplicate(H2M::RefH2M<EnvMapVulkanMaterial> material);
	// The library's materials for each of a mesh's own materials (same order as MeshH2M::GetMaterials), imported if needed
	static std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> ImportMeshMaterials(H2M::RefH2M<H2M::MeshH2M> mesh);
	// The fallback for submeshes whose model has no material for them (created on first use)
	static H2M::RefH2M<EnvMapVulkanMaterial> GetDefaultMaterial();

	// Removes the material from the library. Callers make sure no submesh uses it any more, and that the GPU is idle.
	static void Remove(H2M::RefH2M<EnvMapVulkanMaterial> material);
	static void Clear();

	// Renames the material; the name is trimmed and kept unique ("name (2)"...). An empty or unchanged name does nothing.
	static void Rename(H2M::RefH2M<EnvMapVulkanMaterial> material, const std::string& name);

	// "name", or "name (2)", "name (3)"... if the name is taken
	static std::string MakeUniqueName(const std::string& name);

private:
	static std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> s_Materials;
	static H2M::RefH2M<EnvMapVulkanMaterial> s_DefaultMaterial;
};
