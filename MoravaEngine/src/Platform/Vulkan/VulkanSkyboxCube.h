#pragma once

#include "Mesh/Mesh.h"

#include "H2M/Renderer/VertexBuffer.h"
#include "H2M/Renderer/IndexBuffer.h"


class VulkanSkyboxCube : public Mesh
{
public:
	VulkanSkyboxCube();
	~VulkanSkyboxCube();

public:
	H2M::Ref<H2M::VertexBuffer> m_VertexBuffer;
	H2M::Ref<H2M::IndexBuffer> m_IndexBuffer;

	uint32_t m_IndexCount;

};
