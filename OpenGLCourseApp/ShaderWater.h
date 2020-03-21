#pragma once

#include "Shader.h"


class ShaderWater : public Shader
{
public:
	ShaderWater();
	~ShaderWater();

private:
	bool m_Validated = false;
};
