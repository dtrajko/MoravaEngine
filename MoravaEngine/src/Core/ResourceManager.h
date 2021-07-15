#pragma once

#include "Hazel/Renderer/HazelTexture.h"

#include "Material/Material.h"
#include "Texture/Texture.h"

#include <map>


class ResourceManager
{
public:
	static void Init();
	static void LoadTexture(std::string name, std::string filePath);
	static void LoadTexture(std::string name, std::string filePath, GLenum filter, bool force);
	static void LoadMaterial(std::string name, TextureInfo textureInfo);
	static Hazel::Ref<Texture> HotLoadTexture(std::string textureName);
	static Hazel::Ref<Material> HotLoadMaterial(std::string materialName);

	// Getters
	static inline Hazel::Ref<Texture> GetTexture(std::string textureName) { return s_Textures[textureName]; };
	static inline std::map<std::string, Hazel::Ref<Texture>>* GetTextures() { return &s_Textures; };
	static inline std::map<std::string, Hazel::Ref<Material>>* GetMaterials() { return &s_Materials; };
	static inline std::map<std::string, std::string>* GetTextureInfo() { return &s_TextureInfo; };
	static inline std::map<std::string, TextureInfo>* GetMaterialInfo() { return &s_MaterialInfo; };
	static inline std::map<std::string, Hazel::Ref<MoravaShader>>* GetShaders() { return &s_ShaderCacheByTitle; };

	// Loading HazelTexture2D
	static Hazel::Ref<Hazel::HazelTexture2D> LoadHazelTexture2D(std::string filePath);

	static void AddShader(std::string name, Hazel::Ref<MoravaShader> shader);
	static const Hazel::Ref<MoravaShader>& GetShader(std::string name);

	// Caching shaders
	static const Hazel::Ref<MoravaShader>& CreateOrLoadShader(MoravaShaderSpecification moravaShaderSpecification);

public:
	static float s_MaterialSpecular;
	static float s_MaterialShininess;

private:
	// Asset loading
	static std::map<std::string, std::string> s_TextureInfo;
	static std::map<std::string, TextureInfo> s_MaterialInfo;

	static std::map<std::string, Hazel::Ref<Texture>> s_Textures;
	static std::map<std::string, Hazel::Ref<Material>> s_Materials;

	static std::map<std::string, Hazel::Ref<Hazel::HazelTexture2D>> s_HazelTextures2D;

	static std::map<std::string, Hazel::Ref<MoravaShader>> s_ShaderCacheByTitle;

	static std::map<std::string, Hazel::Ref<MoravaShader>> s_ShadersCacheByFilepath;

};
