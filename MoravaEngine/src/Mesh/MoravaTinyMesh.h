#pragma once

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"


/**
 * This class shares the interface with HazelMesh, but uses tinyobjloader for loading meshes, instead of assimp
 */
class MoravaTinyMesh : public Hazel::MeshHazelLegacy
{
public:
	MoravaTinyMesh(const std::string& filepath, Hazel::Ref<MoravaShader> shader, Hazel::Ref<Hazel::HazelMaterial> material, bool isAnimated);

public:
	std::vector<Hazel::VertexHazelLegacy> vertices{};
	std::vector<uint32_t> indices{};

};
