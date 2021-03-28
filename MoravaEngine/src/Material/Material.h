#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/HazelShader.h"

#include "Shader/Shader.h"
#include "Texture/Texture.h"

#include <glm/glm.hpp>

#include <map>
#include <string>


enum class MapType
{
	Albedo           =  0,
	Normal           =  1,
	Specular         =  2,
	Metalness        =  3,
	Roughness        =  4,
	AmbientOcclusion =  5,
	Height           =  6,
	Emission         =  7,
};

struct TextureInfo
{
	std::string albedo;
	std::string normal;
	std::string metallic;
	std::string roughness;
	std::string emissive;
	std::string ao;
};

enum class MaterialFlag
{
	None      = 1 << 0,
	DepthTest = 1 << 1,
	Blend     = 1 << 2,
};


class Material : public Hazel::HazelMaterial
{
public:
	Material();
	Material(float specularIntensity, float shininess);
	Material(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess); // based on LearnOpenGL material classint
	Material(int albedo, int specular, int normal, float shininess); // used in SceneNanosuit
	Material(Hazel::Ref<Hazel::HazelShader> shader); // based on Hazel::Material contructor
	virtual ~Material() override;

	/**** BEGIN virtual methods defined in HazelMaterial ****/

	virtual void Set(const std::string& name, float value) override {};
	virtual void Set(const std::string& name, int value) override {};
	virtual void Set(const std::string& name, uint32_t value) override {};
	virtual void Set(const std::string& name, bool value) override {};
	virtual void Set(const std::string& name, const glm::vec2& value) override {};
	virtual void Set(const std::string& name, const glm::vec3& value) override {};
	virtual void Set(const std::string& name, const glm::vec4& value) override {};
	virtual void Set(const std::string& name, const glm::mat3& value) override {};
	virtual void Set(const std::string& name, const glm::mat4& value) override {};

	virtual void Set(const std::string& name, const Hazel::Ref<Hazel::HazelTexture2D>& texture) override {};
	virtual void Set(const std::string& name, const Hazel::Ref<Hazel::HazelTextureCube>& texture) override {};
	virtual void Set(const std::string& name, const Hazel::Ref<Hazel::HazelImage2D>& image) override {};

	virtual float& GetFloat(const std::string& name) override {};
	virtual int32_t& GetInt(const std::string& name) override {};
	virtual uint32_t& GetUInt(const std::string& name) override {};
	virtual bool& GetBool(const std::string& name) override {};
	virtual glm::vec2& GetVector2(const std::string& name) override {};
	virtual glm::vec3& GetVector3(const std::string& name) override {};
	virtual glm::vec4& GetVector4(const std::string& name) override {};
	virtual glm::mat3& GetMatrix3(const std::string& name) override {};
	virtual glm::mat4& GetMatrix4(const std::string& name) override {};

	virtual Hazel::Ref<Hazel::HazelTexture2D> GetTexture2D(const std::string& name) override {};
	virtual Hazel::Ref<Hazel::HazelTextureCube> GetTextureCube(const std::string& name) override {};

	virtual Hazel::Ref<Hazel::HazelTexture2D> TryGetTexture2D(const std::string& name) override {};
	virtual Hazel::Ref<Hazel::HazelTextureCube> TryGetTextureCube(const std::string& name) override {};

	virtual uint32_t GetFlags() const override {};
	virtual bool GetFlag(Hazel::HazelMaterialFlag flag) const override {};
	virtual void SetFlag(Hazel::HazelMaterialFlag flag, bool value = true) override {};

	virtual Hazel::Ref<Hazel::HazelShader> GetShader() override {};
	virtual const std::string& GetName() const override {};

	/**** END virtual methods defined in HazelMaterial ****/

	void UseMaterial(int specularIntensityLocation, int shininessLocation);
	void AddMap(MapType mapType, int textureSlot);

	// PBR/IBL Material Workflow
	Material(TextureInfo textureInfoGold, float specularIntensity, float shininess);
	void BindTextures(unsigned int slot);
	inline Texture* GetTextureAlbedo()    const { return m_TextureAlbedo;    };
	inline Texture* GetTextureNormal()    const { return m_TextureNormal;    };
	inline Texture* GetTextureMetallic()  const { return m_TextureMetallic;  };
	inline Texture* GetTextureRoughness() const { return m_TextureRoughness; };
	inline Texture* GetTextureEmissive()  const { return m_TextureEmissive;  };
	inline Texture* GetTextureAO()        const { return m_TextureAO;        };

	// From Hazel/Renderer/Material
	bool GetFlag(MaterialFlag flag) const { return (uint32_t)flag & m_MaterialFlags; }
	void SetFlag(MaterialFlag flag, bool value);

public:
	int m_AlbedoMap;      // sampler2D, texture slot - diffuse/albedo
	int m_SpecularMap;    // sampler2D, texture slot
	int m_NormalMap;      // sampler2D, texture slot
	float m_Shininess;

private:
	Texture* m_TextureAlbedo;
	Texture* m_TextureNormal;
	Texture* m_TextureMetallic;
	Texture* m_TextureRoughness;
	Texture* m_TextureEmissive;
	Texture* m_TextureAO;

	Texture* m_TexturePlaceholder;

	std::map<int, MapType> m_Maps;

	glm::vec3 m_Ambient;  // color value
	glm::vec3 m_Diffuse;  // color value
	glm::vec3 m_Specular; // color value

	glm::vec3 m_AlbedoColor;   // diffuse/albedo color

	int m_MetalnessMap;        // sampler2D, texture slot
	int m_RoughnessMap;        // sampler2D, texture slot
	int m_HeightMap;           // sampler2D, texture slot
	int m_AmbientOcclusionMap; // sampler2D, texture slot
	int m_EmissionMap;         // sampler2D, texture slot

	float m_SpecularIntensity;

	// From Hazel/Renderer/Material
	uint32_t m_MaterialFlags;
	Hazel::Ref<Hazel::HazelShader> m_Shader;

};
