#pragma once

#include "Hazel/Renderer/HazelMesh.h"

#include <tiny_obj_loader.h>


/**
 * This class shares the interface with HazelMesh, but uses tinyobjloader for loading meshes, instead of assimp
 */
class MoravaTinyMesh : public Hazel::HazelMesh
{
public:
	MoravaTinyMesh(const std::string& filepath, Hazel::Ref<MoravaShader> shader, Hazel::Ref<Hazel::HazelMaterial> material, bool isAnimated);

public:
	std::vector<Hazel::Vertex> vertices{};
	std::vector<uint32_t> indices{};

};
