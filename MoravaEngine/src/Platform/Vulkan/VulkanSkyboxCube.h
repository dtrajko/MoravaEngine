#pragma once

#include "Mesh/Mesh.h"

#include "Hazel/Renderer/VertexBuffer.h"
#include "Hazel/Renderer/IndexBuffer.h"


class VulkanSkyboxCube : public Mesh
{
public:
	VulkanSkyboxCube();
	~VulkanSkyboxCube();

public:
	Hazel::Ref<Hazel::VertexBuffer> m_VertexBuffer;
	Hazel::Ref<Hazel::IndexBuffer> m_IndexBuffer;

	uint32_t m_IndexCount;

};
