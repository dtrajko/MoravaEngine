#pragma once

#include "H2M/Renderer/MeshH2M.h"


/**
 * This class shares the interface with HazelMesh, but uses tinyobjloader for loading meshes, instead of assimp
 */
class MoravaTinyMesh : public H2M::MeshH2M
{
public:
	MoravaTinyMesh(const std::string& filepath, H2M::RefH2M<MoravaShader> shader, H2M::RefH2M<H2M::HazelMaterial> material, bool isAnimated);

public:
	std::vector<H2M::VertexH2M> vertices{};
	std::vector<uint32_t> indices{};

};
