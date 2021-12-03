#pragma once

#include "H2M/Renderer/MaterialH2M.h"

#include "Material/Material.h"


class MaterialInstance : public H2M::HazelMaterial
{
public:
	MaterialInstance(const H2M::RefH2M<H2M::HazelMaterial>& material, const std::string& name = "");
	~MaterialInstance();

	void Bind();
	H2M::RefH2M<H2M::HazelShader> GetShader() { return m_Material->GetShader(); }

private:
	H2M::RefH2M<H2M::HazelMaterial> m_Material;
	std::string m_Name;

};
