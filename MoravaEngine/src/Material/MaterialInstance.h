#pragma once

#include "H2M/Renderer/MaterialH2M.h"

#include "Material/Material.h"


class MaterialInstance : public H2M::MaterialH2M
{
public:
	MaterialInstance(const H2M::RefH2M<H2M::MaterialH2M>& material, const std::string& name = "");
	~MaterialInstance();

	void Bind();
	H2M::RefH2M<H2M::ShaderH2M> GetShader() { return m_Material->GetShader(); }

private:
	H2M::RefH2M<H2M::MaterialH2M> m_Material;
	std::string m_Name;

};
