#pragma once

#include "Texture.h"
#include "Material.h"

#include <map>


class ResourceManager
{
public:
	static void Init();
	static void LoadTexture(std::string name, std::string filePath);
	static void LoadTexture(std::string name, std::string filePath, GLenum filter, bool force);
	static void LoadMaterial(std::string name, TextureInfo textureInfo);
	static Texture* HotLoadTexture(std::string textureName);
	static Material* HotLoadMaterial(std::string materialName);
	// Getters
	static inline Texture* GetTexture(std::string textureName) { return s_Textures[textureName]; };
	static inline std::map<std::string, Texture*>* GetTextures() { return &s_Textures; };
	static inline std::map<std::string, Material*>* GetMaterials() { return &s_Materials; };
	static inline std::map<std::string, std::string>* GetTextureInfo() { return &s_TextureInfo; };
	static inline std::map<std::string, TextureInfo>* GetMaterialInfo() { return &s_MaterialInfo; };

public:
	static float s_MaterialSpecular;
	static float s_MaterialShininess;

private:
	// Asset loading
	static std::map<std::string, std::string> s_TextureInfo;
	static std::map<std::string, TextureInfo> s_MaterialInfo;

	static std::map<std::string, Texture*> s_Textures;
	static std::map<std::string, Material*> s_Materials;
};
