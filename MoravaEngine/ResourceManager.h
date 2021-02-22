#pragma once

#include "Texture.h"
#include "Material.h"
#include "Hazel/Renderer/HazelTexture.h"

#include <map>


class ResourceManager
{
public:
	static void Init();
	static void LoadTexture(std::string name, std::string filePath);
	static void LoadTexture(std::string name, std::string filePath, GLenum filter, bool force);
	static void LoadMaterial(std::string name, TextureInfo textureInfo);
	static Texture* HotLoadTexture(std::string textureName);
	static Ref<Material> HotLoadMaterial(std::string materialName);

	// Getters
	static inline Texture* GetTexture(std::string textureName) { return s_Textures[textureName]; };
	static inline std::map<std::string, Texture*>* GetTextures() { return &s_Textures; };
	static inline std::map<std::string, Ref<Material>>* GetMaterials() { return &s_Materials; };
	static inline std::map<std::string, std::string>* GetTextureInfo() { return &s_TextureInfo; };
	static inline std::map<std::string, TextureInfo>* GetMaterialInfo() { return &s_MaterialInfo; };
	static inline std::map<std::string, Hazel::Ref<Shader>>* GetShaders() { return &s_Shaders; };

	// Loading HazelTexture2D
	static Hazel::Ref<Hazel::HazelTexture2D> LoadHazelTexture2D(std::string filePath);

	static void AddShader(std::string name, Hazel::Ref<Shader> shader);
	static const Hazel::Ref<Shader>& GetShader(std::string name);

public:
	static float s_MaterialSpecular;
	static float s_MaterialShininess;

private:
	// Asset loading
	static std::map<std::string, std::string> s_TextureInfo;
	static std::map<std::string, TextureInfo> s_MaterialInfo;

	static std::map<std::string, Texture*> s_Textures;
	static std::map<std::string, Ref<Material>> s_Materials;

	static std::map<std::string, Hazel::Ref<Hazel::HazelTexture2D>> s_HazelTextures2D;

	static std::map<std::string, Hazel::Ref<Shader>> s_Shaders;

};
