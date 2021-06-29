#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"


class DX11VertexMesh
{
public:
	DX11VertexMesh() : m_Position(0.0f), m_TexCoord(0.0f)
	{
		m_Normal   = glm::vec3(0.0f);
		m_Tangent  = glm::vec3(0.0f);
		m_Binormal = glm::vec3(0.0f);
	}

	DX11VertexMesh(glm::vec3 position, glm::vec2 texCoord) : m_Position(position), m_TexCoord(texCoord)
	{
		m_Normal = glm::vec3(0.0f);
		m_Tangent = glm::vec3(0.0f);
		m_Binormal = glm::vec3(0.0f);
	}

	// DX11VertexMesh(const glm::vec3& position, const glm::vec2& texCoord) : m_Position(position), m_TexCoord(texCoord)
	// {
	// }

	~DX11VertexMesh()
	{
	}

public:
	glm::vec3 m_Position;
	glm::vec3 m_Normal;
	glm::vec3 m_Tangent;
	glm::vec3 m_Binormal;
	glm::vec2 m_TexCoord;

};
