#pragma once

#include "Hazel/Renderer/HazelMaterial.h"

#include "Material/Material.h"


class MaterialInstance : public Hazel::HazelMaterialInstance
{
public:
	MaterialInstance(const Hazel::Ref<Hazel::HazelMaterial>& material, const std::string& name = "");
	~MaterialInstance();

	void Bind();
	Hazel::Ref<Hazel::HazelShader> GetShader() { m_Material->GetShader(); }

private:
	Hazel::Ref<Hazel::HazelMaterial> m_Material;
	std::string m_Name;

};
