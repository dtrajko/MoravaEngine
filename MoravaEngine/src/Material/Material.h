#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Renderer/MaterialH2M.h"
#include "H2M/Renderer/ShaderH2M.h"

#include "Shader/MoravaShader.h"
#include "Texture/MoravaTexture.h"

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


class Material : public H2M::MaterialH2M
{
public:
	Material();
	Material(float specularIntensity, float shininess);
	Material(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess); // based on LearnOpenGL material classint
	Material(int albedo, int specular, int normal, float shininess); // used in SceneNanosuit
	Material(H2M::RefH2M<H2M::ShaderH2M> shader); // based on H2M::Material contructor
	Material(TextureInfo textureInfoGold, float specularIntensity, float shininess); // PBR/IBL Material Workflow
	virtual ~Material() override;

	/**** BEGIN virtual methods defined in HazelMaterial ****/

	virtual void Invalidate() override {};

	virtual void Set(const std::string& name, float value) override {};
	virtual void Set(const std::string& name, int value) override {};
	virtual void Set(const std::string& name, uint32_t value) override {};
	virtual void Set(const std::string& name, bool value) override {};
	virtual void Set(const std::string& name, const glm::ivec2& value) override {};
	virtual void Set(const std::string& name, const glm::ivec3& value) override {};
	virtual void Set(const std::string& name, const glm::ivec4& value) override {};
	virtual void Set(const std::string& name, const glm::vec2& value) override {};
	virtual void Set(const std::string& name, const glm::vec3& value) override {};
	virtual void Set(const std::string& name, const glm::vec4& value) override {};
	virtual void Set(const std::string& name, const glm::mat3& value) override {};
	virtual void Set(const std::string& name, const glm::mat4& value) override {};

	virtual void Set(const std::string& name, const H2M::RefH2M<H2M::Texture2D_H2M>& texture) override {};
	virtual void Set(const std::string& name, const H2M::RefH2M<H2M::Texture2D_H2M>& texture, uint32_t arrayIndex) override {};
	virtual void Set(const std::string& name, const H2M::RefH2M<H2M::TextureCubeH2M>& texture) override {};
	virtual void Set(const std::string& name, const H2M::RefH2M<H2M::Image2D_H2M>& image) override {};

	virtual float& GetFloat(const std::string& name) override { return m_Float; }
	virtual int32_t& GetInt(const std::string& name) override { return m_int32_t; }
	virtual uint32_t& GetUInt(const std::string& name) override { return m_uint32_t; }
	virtual bool& GetBool(const std::string& name) override { return m_bool; }
	virtual glm::vec2& GetVector2(const std::string& name) override { return m_glm_vec2; }
	virtual glm::vec3& GetVector3(const std::string& name) override { return m_glm_vec3; }
	virtual glm::vec4& GetVector4(const std::string& name) override { return m_glm_vec4; }
	virtual glm::mat3& GetMatrix3(const std::string& name) override { return m_glm_mat3; }
	virtual glm::mat4& GetMatrix4(const std::string& name) override { return m_glm_mat4; }

	virtual H2M::RefH2M<H2M::Texture2D_H2M> GetTexture2D(const std::string& name) override { return H2M::RefH2M<H2M::Texture2D_H2M>(); };
	virtual H2M::RefH2M<H2M::TextureCubeH2M> GetTextureCube(const std::string& name) override { return H2M::RefH2M<H2M::TextureCubeH2M>(); };

	virtual H2M::RefH2M<H2M::Texture2D_H2M> TryGetTexture2D(const std::string& name) override { return H2M::RefH2M<H2M::Texture2D_H2M>(); };
	virtual H2M::RefH2M<H2M::TextureCubeH2M> TryGetTextureCube(const std::string& name) override { return H2M::RefH2M<H2M::TextureCubeH2M>(); };

	virtual uint32_t GetFlags() const override { return uint32_t(); };
	virtual bool GetFlag(H2M::MaterialFlagH2M flag) const override { return bool(); }
	virtual void SetFlag(H2M::MaterialFlagH2M flag, bool value = true) override {}

	virtual H2M::RefH2M<H2M::ShaderH2M> GetShader() override { return H2M::RefH2M<H2M::ShaderH2M>(); }
	virtual const std::string& GetName() const override { return m_Name; }

	/**** END virtual methods defined in HazelMaterial ****/

	void UseMaterial(int specularIntensityLocation, int shininessLocation);
	void AddMap(MapType mapType, int textureSlot);

	// PBR/IBL Material Workflow
	void BindTextures(unsigned int slot);
	inline H2M::RefH2M<MoravaTexture> GetTextureAlbedo()    const { return m_TextureAlbedo;    };
	inline H2M::RefH2M<MoravaTexture> GetTextureNormal()    const { return m_TextureNormal;    };
	inline H2M::RefH2M<MoravaTexture> GetTextureMetallic()  const { return m_TextureMetallic;  };
	inline H2M::RefH2M<MoravaTexture> GetTextureRoughness() const { return m_TextureRoughness; };
	inline H2M::RefH2M<MoravaTexture> GetTextureEmissive()  const { return m_TextureEmissive;  };
	inline H2M::RefH2M<MoravaTexture> GetTextureAO()        const { return m_TextureAO;        };

	// From Hazel/Renderer/Material
	bool GetFlag(MaterialFlag flag) const { return (uint32_t)flag & m_MaterialFlags; }
	void SetFlag(MaterialFlag flag, bool value);

public:
	int m_AlbedoMap;      // sampler2D, texture slot - diffuse/albedo
	int m_SpecularMap;    // sampler2D, texture slot
	int m_NormalMap;      // sampler2D, texture slot
	float m_Shininess;

private:
	H2M::RefH2M<MoravaTexture> m_TextureAlbedo;
	H2M::RefH2M<MoravaTexture> m_TextureNormal;
	H2M::RefH2M<MoravaTexture> m_TextureMetallic;
	H2M::RefH2M<MoravaTexture> m_TextureRoughness;
	H2M::RefH2M<MoravaTexture> m_TextureEmissive;
	H2M::RefH2M<MoravaTexture> m_TextureAO;

	H2M::RefH2M<MoravaTexture> m_TexturePlaceholder;

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
	H2M::RefH2M<H2M::HazelShader> m_Shader;

	float m_Float;
	int32_t m_int32_t;
	uint32_t m_uint32_t;
	bool m_bool;
	glm::vec2 m_glm_vec2;
	glm::vec3 m_glm_vec3;
	glm::vec4 m_glm_vec4;
	glm::mat3 m_glm_mat3;
	glm::mat4 m_glm_mat4;
	std::string m_Name;

};
