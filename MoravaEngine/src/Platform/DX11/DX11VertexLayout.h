#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"


class DX11VertexLayout
{
public:
	DX11VertexLayout(glm::vec3 position, glm::vec3 normal, glm::vec3 tangent, glm::vec3 binormal, glm::vec2 texCoord)
		: Position(position), Normal(normal), Tangent(tangent), Binormal(binormal), TexCoord(texCoord)
	{
	}

	~DX11VertexLayout()
	{
	}

public:
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec3 Binormal;
	glm::vec2 TexCoord;

};
