#pragma once

#include "Mesh/Mesh.h"

#include "H2M/Renderer/VertexBufferH2M.h"
#include "H2M/Renderer/IndexBufferH2M.h"


class VulkanSkyboxCube : public Mesh
{
public:
	VulkanSkyboxCube();
	~VulkanSkyboxCube();

public:
	H2M::RefH2M<H2M::VertexBufferH2M> m_VertexBuffer;
	H2M::RefH2M<H2M::IndexBufferH2M> m_IndexBuffer;

	uint32_t m_IndexCount;

};
